import { mkdirSync, readFileSync } from "node:fs";
import { join } from "node:path";
import type { PlayStatus, Session } from "./session.ts";

export type PlayCase = {
    name: string;
    summary: string;
    run: (session: Session) => Promise<void>;
};

const MAP_VIEW = `
local G = nsmbu.Guest
local function u32(a) return G.read_u32(a) end
local function s32(a) return G.read_s32(a) end
local function u8(a) return G.read_u8(a) end
local cs = u32(0x101CC96C)
if cs == 0 then return "pending" end
local lock = u8(cs + 0xDC9)
local game = u32(0x101E7D7C)
if game == 0 then return "pending" end
local world = s32(game + 0x19C)
local section = s32(game + 0x1A0)
local node = s32(game + 0x1A4)
local mgr = u32(0x101CBE14)
if mgr == 0 or node < 0 then return string.format("lock=%d world=%d node=%d title=-1 name=", lock, world, node) end
local path = u32(mgr + 0x60 + (world * 2 + section) * 4)
if path == 0 then return string.format("lock=%d world=%d node=%d title=-1 name=", lock, world, node) end
local base = path + 0x10 + node * 0x68
local title = s32(base + 0x50)
local name = ""
local p = u32(base)
if p ~= 0 then
    local raw = G.read(p, 8)
    name = raw:match("^[%w_]+") or ""
end
local players = nsmbu.Level.count(854)
local px, py = 0, 0
local ok, list = pcall(nsmbu.Level.actors)
if ok and type(list) == "table" then
    for i = 1, #list do
        local a = list[i]
        local idok, id = pcall(function() return a:profile_id() end)
        if idok and id == 854 then
            local aok, addr = pcall(function() return a:address() end)
            if aok and type(addr) == "number" and addr ~= 0 then
                px = G.read_f32(addr + 0x50)
                py = G.read_f32(addr + 0x54)
            end
        end
    end
end
return string.format("lock=%d players=%s world=%d node=%d title=%d name=%s px=%.2f py=%.2f", lock, tostring(players), world, node, title, name, px, py)
`;

export type MapView = {
    lock: number;
    players: number;
    world: number;
    node: number;
    title: number;
    name: string;
    px: number;
    py: number;
    raw: string;
};

export function parseMap(raw: string): MapView | null {
    const m = raw.match(
        /^lock=(-?\d+) players=(-?\d+) world=(-?\d+) node=(-?\d+) title=(-?\d+) name=(\S*) px=(-?[\d.]+) py=(-?[\d.]+)/,
    );
    if (!m) return null;
    return {
        lock: Number(m[1]),
        players: Number(m[2]),
        world: Number(m[3]),
        node: Number(m[4]),
        title: Number(m[5]),
        name: m[6] ?? "",
        px: Number(m[7]),
        py: Number(m[8]),
        raw,
    };
}

function isCourseNode(name: string): boolean {
    return /^W[1-9]\d{2}$/.test(name);
}

function moved(a: MapView, b: MapView): boolean {
    if (a.node !== b.node || a.name !== b.name) return true;
    return Math.abs(a.px - b.px) > 1 || Math.abs(a.py - b.py) > 1;
}

async function mapView(session: Session): Promise<MapView | null> {
    const raw = await session.eval(MAP_VIEW);
    return parseMap(raw);
}

async function waitMap(session: Session, timeoutSec: number, needInput: boolean): Promise<MapView> {
    const start = Date.now();
    let last = "";
    for (;;) {
        let view: MapView | null = null;
        try {
            view = await mapView(session);
        } catch (e) {
            last = e instanceof Error ? e.message : String(e);
        }
        if (!view && last === "") await session.status();
        if (view) last = view.raw;
        if (view && view.players >= 1 && view.title >= 0 && view.name !== "" && (!needInput || view.lock === 0)) {
            return view;
        }
        if (Date.now() - start >= timeoutSec * 1000) {
            throw new Error(`map did not become ready: ${last} ${session.last}`);
        }
        await session.advance(20, 15);
    }
}

async function hold(session: Session, frames: number, run: () => Promise<void>): Promise<void> {
    const now = await session.status();
    const mark = now.frame ?? 0;
    await run();
    await session.wait((s) => (s.frame ?? 0) >= mark + frames, 30);
}

async function walk(session: Session, x: number, y: number): Promise<MapView | null> {
    const before = await mapView(session);
    await hold(session, 180, () => session.stick(x, y, 180));
    const after = await mapView(session);
    if (!before || !after || !moved(before, after)) return null;
    await hold(session, 240, () => session.stick(x, y, 240));
    return (await mapView(session)) ?? after;
}

async function waitUntilMoves(session: Session, timeoutSec: number): Promise<MapView> {
    const start = Date.now();
    let last = "";
    while (Date.now() - start < timeoutSec * 1000) {
        const before = await mapView(session);
        if (!before || before.lock !== 0 || before.players < 1) {
            await hold(session, 20, () => session.press("A", 12));
            continue;
        }
        last = before.raw;
        await session.advance(60, 15);
        const settled = await mapView(session);
        if (!settled || moved(before, settled)) continue;
        await hold(session, 20, () => session.press("A", 12));
        for (const [x, y] of [
            [1, 0],
            [0, 1],
            [-1, 0],
            [0, -1],
        ] as const) {
            const stepped = await walk(session, x, y);
            if (!stepped) continue;
            last = stepped.raw;
            if (isCourseNode(stepped.name) && stepped.title >= 0 && stepped.players >= 1) return stepped;
        }
    }
    throw new Error(`map did not take control on a course node: ${last} ${session.last}`);
}

async function waitCourse(session: Session, pack: string, timeoutSec: number): Promise<void> {
    await session.wait((s) => s.course === true && s.map !== true && s.pack === pack && s.player != null, timeoutSec);
}

async function reachMap(session: Session, timeoutSec: number): Promise<void> {
    await session.wait((s) => s.scene !== undefined && s.scene !== "Boot", 180);
    const start = Date.now();
    let last = "";
    while (Date.now() - start < timeoutSec * 1000) {
        const s = await session.status();
        if (s.map === true && (s.scene === "Map" || s.scene === "CourseSelect")) return;
        last = await session.eval("return nsmbu.Scene.open_first_save(0)");
        if (!last.startsWith("true")) {
            await session.eval("return nsmbu.Scene.open_title()");
            await session.press("A", 15);
        }
        await session.advance(40, 20);
    }
    throw new Error(`opening never reached the map: ${last} ${session.last}`);
}

function onMap(s: PlayStatus): boolean {
    return s.map === true;
}

function onWorldMap(s: PlayStatus): boolean {
    return s.map === true && s.scene === "Map";
}

function inCourse(s: PlayStatus): boolean {
    return s.course === true && s.map !== true && s.player != null && !!s.pack;
}

async function reachWorldMap(session: Session, timeoutSec: number): Promise<PlayStatus> {
    const start = Date.now();
    while (Date.now() - start < timeoutSec * 1000) {
        const s = await session.status();
        if (onMap(s)) return s;
        await session.press("A", 20);
        await session.advance(50, 20);
    }
    throw new Error(`never reached the world map: ${session.last}`);
}

async function advanceUntilFrame(session: Session, frame: number, timeoutSec: number): Promise<PlayStatus> {
    const start = Date.now();
    while (Date.now() - start < timeoutSec * 1000) {
        const s = await session.status();
        if ((s.frame ?? 0) >= frame) return s;
        await session.advance(30, 20);
    }
    throw new Error(`frame ${frame} not reached: ${session.last}`);
}

function assertPortablePlace(repo: string, states: string, slot: number, place: "map" | "course"): void {
    const path = join(states, `slot${slot}.nsstate`);
    let text = "";
    try {
        text = readFileSync(path, "utf8");
    } catch {
        text = "";
    }
    if (!text.includes(`place = ${place}\n`)) {
        const why = playtestLog(repo)
            .split("\n")
            .filter((line) => line.includes("[savestate]") && (line.includes(`slot ${slot}:`) || line.includes(`Slot ${slot}:`)))
            .slice(-3)
            .join(" | ");
        throw new Error(`slot ${slot} is not a ${place} state (${why || "no savestate log"})`);
    }
}

async function enterCourse(session: Session): Promise<string> {
    await session.advance(90, 20);
    const start = Date.now();
    while (Date.now() - start < 90_000) {
        const s = await session.status();
        if (inCourse(s)) return s.pack ?? "";
        if (!s.entering) await session.press("A", 25);
        await session.advance(60, 20);
    }
    throw new Error(`course did not start: ${session.last}`);
}

async function leaveCourse(session: Session): Promise<void> {
    await session.advance(180, 30);
    const start = Date.now();
    while (Date.now() - start < 60_000) {
        if (onWorldMap(await session.status())) return;
        await session.press("B", 15);
        await session.advance(20, 15);
        await session.press("Plus", 20);
        await session.advance(40, 15);
        await session.press("Down", 15);
        await session.advance(25, 15);
        await session.press("A", 20);
        await session.advance(40, 15);
        await session.press("Right", 15);
        await session.advance(25, 15);
        await session.press("A", 20);
        const until = Date.now() + 8_000;
        while (Date.now() < until) {
            if (onWorldMap(await session.status())) return;
            await session.advance(30, 15);
        }
    }
    throw new Error(`did not return to the map: ${session.last}`);
}

async function walkToOtherNode(session: Session, from: number | undefined): Promise<PlayStatus> {
    const start = Date.now();
    while (Date.now() - start < 60_000) {
        await session.stick(1, 0, 180);
        await session.advance(200, 20);
        const s = await session.status();
        if (onMap(s) && s.node !== undefined && s.node !== from) return s;
    }
    throw new Error(`map node did not change: ${session.last}`);
}

function hostQuickNode(saveDir: string, file = 0): { exists: number; node: number } | null {
    let buf: Buffer;
    try {
        buf = readFileSync(join(saveDir, "common", "rp_savedata.dat"));
    } catch {
        return null;
    }
    const off = 0xc28 + file * 0x204;
    if (buf.length < off + 7) return null;
    return { exists: buf[off], node: buf[off + 6] };
}

type HostRecord = {
    exists: number;
    lives: number;
    complete: number;
    flags: number;
    world: number;
    section: number;
    node: number;
};

function hostRecord(saveDir: string, base: number, file = 0): HostRecord | null {
    let buf: Buffer;
    try {
        buf = readFileSync(join(saveDir, "common", "rp_savedata.dat"));
    } catch {
        return null;
    }
    const off = base + file * 0x204;
    if (buf.length < off + 0x6b + 123) return null;
    let flags = 0;
    for (let i = 0; i < 123; i++) if (buf[off + 0x6b + i]) flags++;
    return {
        exists: buf[off],
        lives: buf[off + 0x0c],
        complete: buf[off + 0x1e],
        flags,
        world: buf[off + 4],
        section: buf[off + 5],
        node: buf[off + 6],
    };
}

type SlotLog = HostRecord & { uses: "quick" | "main" };

function playtestLog(repo: string): string {
    try {
        return readFileSync(join(repo, "nsmbu", "logs", "nsmbu.log"), "utf8");
    } catch {
        return "";
    }
}

function lastFileSelectSlot(repo: string, file: number): SlotLog | null {
    const re =
        /\[save\] file-select slot file (\d+) uses (quick|main) exists=(\d+) lives=(\d+) complete=(\d+) flags=(-?\d+) map=(\d+)\.(\d+)\.(\d+)/g;
    let found: SlotLog | null = null;
    for (const m of playtestLog(repo).matchAll(re)) {
        if (Number(m[1]) !== file) continue;
        found = {
            uses: m[2] === "quick" ? "quick" : "main",
            exists: Number(m[3]),
            lives: Number(m[4]),
            complete: Number(m[5]),
            flags: Number(m[6]),
            world: Number(m[7]),
            section: Number(m[8]),
            node: Number(m[9]),
        };
    }
    return found;
}

function lastFileSelectComplete(repo: string, file: number): number | null {
    const re = /\[save\] file-select complete file (\d+) byte=(\d+)/g;
    let byte: number | null = null;
    for (const m of playtestLog(repo).matchAll(re)) {
        if (Number(m[1]) === file) byte = Number(m[2]);
    }
    return byte;
}

function sameProgress(a: HostRecord, b: HostRecord): boolean {
    return a.lives === b.lives && a.flags === b.flags && a.world === b.world && a.section === b.section && a.node === b.node;
}

function assertFileSelectQuick(session: Session, saved: HostRecord, file = 0): void {
    const logged = lastFileSelectSlot(session.repo, file);
    if (!logged) throw new Error("file-select slot did not log which record it used");
    if (logged.uses !== "quick") throw new Error(`file-select slot file ${file} uses ${logged.uses}, want quick`);
    if (!sameProgress(logged, saved) || logged.exists !== saved.exists || logged.complete !== saved.complete) {
        throw new Error(
            `file-select slot file ${file} logged exists=${logged.exists} lives=${logged.lives} complete=${logged.complete} flags=${logged.flags} map=${logged.world}.${logged.section}.${logged.node}, quick save exists=${saved.exists} lives=${saved.lives} complete=${saved.complete} flags=${saved.flags} map=${saved.world}.${saved.section}.${saved.node}`,
        );
    }
    const byte = lastFileSelectComplete(session.repo, file);
    if (byte === null) throw new Error("file-select slot did not log the completion byte");
    if (byte !== saved.complete) {
        throw new Error(`file-select completion byte is ${byte}, quick save complete=${saved.complete}`);
    }
    const main = hostRecord(session.saveDir, 0x10, file);
    if (!main || main.exists === 0 || !sameProgress(main, saved)) {
        throw new Error(
            `story record after continue is exists=${main?.exists} lives=${main?.lives} flags=${main?.flags} map=${main?.world}.${main?.section}.${main?.node}, quick save lives=${saved.lives} flags=${saved.flags} map=${saved.world}.${saved.section}.${saved.node}`,
        );
    }
}

async function quickSaveHere(session: Session): Promise<number> {
    await session.advance(120, 30);
    const here = await session.status();
    if (!onMap(here) || here.node === undefined) throw new Error(`not on the map: ${session.last}`);
    const node = here.node;
    await session.press("Plus", 20);
    await session.advance(45, 15);
    await session.press("Down", 15);
    await session.advance(30, 15);
    await session.press("A", 20);
    await session.advance(60, 15);
    await session.press("Right", 15);
    await session.advance(30, 15);
    await session.press("A", 20);
    const start = Date.now();
    let host = hostQuickNode(session.saveDir);
    while (Date.now() - start < 30_000) {
        host = hostQuickNode(session.saveDir);
        if (host && host.exists !== 0 && host.node === node) return node;
        await session.advance(20, 15);
    }
    throw new Error(`quick save did not store node ${node} (host exists=${host?.exists} node=${host?.node}) ${session.last}`);
}

async function loadPack(session: Session, pack: string): Promise<void> {
    const start = Date.now();
    let last = "";
    for (;;) {
        last = await session.eval(`return nsmbu.Scene.load_pack("${pack}")`);
        if (last.startsWith("true")) return;
        if (Date.now() - start >= 90_000) throw new Error(`load_pack ${pack} failed: ${last} ${session.last}`);
        await session.advance(30, 20);
    }
}

const GLYPHS = `
local task = nsmbu.Guest.read_u32(0x101D15F0)
if task == 0 then return "none" end
local a = nsmbu.Guest.read_u8(task + 0x134)
local b = nsmbu.Guest.read_u8(task + 0x135)
local c = nsmbu.Guest.read_u8(task + 0x136)
return string.format("%d %d %d", a, b, c)
`;

export const CASES: PlayCase[] = [
    {
        name: "startup",
        summary: "Regression for #6: the game stays up past Boot with a prepared save.",
        run: async (session) => {
            await session.start({ save: "repo" });
            await session.wait((s) => s.scene !== undefined && s.scene !== "Boot", 180);
            const s = await session.status();
            if ((s.frame ?? 0) < 1) throw new Error(`no frames after boot: ${session.last}`);
            await session.advance(60, 30);
            const again = await session.status();
            if ((again.frame ?? 0) <= (s.frame ?? 0)) {
                throw new Error(`frame counter stuck after boot: ${session.last}`);
            }
        },
    },
    {
        name: "opened-course",
        summary:
            "0.6.1 course-open regressions: loading card stays put, a second pack loads, leave returns to W102.",
        run: async (session) => {
            await session.start({ save: "repo", open: "1-5" });
            await waitCourse(session, "1-5", 180);
            const glyphs = await session.eval(GLYPHS);
            if (glyphs === "4 255 255") {
                throw new Error(`direct open rewrote the loading-card digits for 1-5: ${glyphs}`);
            }
            await loadPack(session, "1-2");
            await waitCourse(session, "1-2", 180);
            const left = await session.eval("return nsmbu.Scene.leave_course()");
            if (!left.startsWith("true")) throw new Error(`leave_course failed: ${left} ${session.last}`);
            await session.wait((s) => s.map === true && (s.scene === "Map" || s.scene === "CourseSelect"), 120);
            const view = await waitMap(session, 90, false);
            if (view.name !== "W102") throw new Error(`map node is ${view.name}, want W102 (${view.raw})`);
            if (view.world !== 0) throw new Error(`map world is ${view.world}, want 0`);
            const status = await session.status();
            if (status.cp === undefined || status.cp < 0) throw new Error(`map checkpoint missing: ${session.last}`);
        },
    },
    {
        name: "opening-cinematic",
        summary: "0.6.1 / #14 map regress: after Story Mode the map takes input and shows a course number.",
        run: async (session) => {
            await session.start({ save: "fresh" });
            await reachMap(session, 180);
            const movedTo = await waitUntilMoves(session, 240);
            if (movedTo.title < 0 || !isCourseNode(movedTo.name)) {
                throw new Error(`course number hidden after input: ${movedTo.raw}`);
            }
        },
    },
    {
        name: "save-roundtrip",
        summary: "Regression for #14: the map Quick Save survives a new process and the file-select slot uses the quick record.",
        run: async (session) => {
            await session.start({ save: "repo" });
            const start = await reachWorldMap(session, 180);
            await walkToOtherNode(session, start.node);
            const node = await quickSaveHere(session);
            const saved = hostRecord(session.saveDir, 0xc28);
            if (!saved || saved.exists === 0 || saved.node !== node) {
                throw new Error(`quick save file is exists=${saved?.exists} node=${saved?.node}, want node ${node}`);
            }
            await session.restart();
            const kept = hostRecord(session.saveDir, 0xc28);
            if (!kept || !sameProgress(kept, saved) || kept.exists === 0) {
                throw new Error(
                    `quick save did not survive restart (exists=${kept?.exists} lives=${kept?.lives} flags=${kept?.flags} map=${kept?.world}.${kept?.section}.${kept?.node})`,
                );
            }
            const back = await reachWorldMap(session, 180);
            assertFileSelectQuick(session, saved);
            if (back.node !== node) {
                const host = hostQuickNode(session.saveDir);
                throw new Error(`reloaded node is ${back.node}, want ${node} (host exists=${host?.exists} node=${host?.node})`);
            }
        },
    },
    {
        name: "save-state",
        summary: "Regression for #14: a portable save state writes on the world map and inside a course.",
        run: async (session) => {
            await session.start({ save: "repo" });
            const map = await reachWorldMap(session, 180);
            const mapFrame = map.frame ?? 0;
            if (mapFrame < 1) throw new Error(`map frame missing: ${session.last}`);
            await enterCourse(session);
            const course = await session.status();
            const courseFrame = course.frame ?? 0;
            if (courseFrame <= mapFrame) throw new Error(`course frame ${courseFrame} is not after the map`);
            await session.stop();

            const mapAt = mapFrame + 2400;
            const courseAt = courseFrame + 4200;
            const states = join(session.repo, ".tmp", "playtest", "save-state", "states");
            mkdirSync(states, { recursive: true });
            await session.start({
                save: "keep",
                env: {
                    NSMBU_STATE_DIR: states,
                    NSMBU_PORTABLE_SAVE_AT: `${mapAt}:1,${courseAt}:2`,
                },
            });
            const back = await reachWorldMap(session, 180);
            if ((back.frame ?? 0) >= mapAt) {
                throw new Error(`map save frame ${mapAt} passed before the map was open: ${session.last}`);
            }
            const heldMap = await advanceUntilFrame(session, mapAt, 90);
            if (!onMap(heldMap)) throw new Error(`left the map before the save: ${session.last}`);
            await session.advance(45, 20);
            assertPortablePlace(session.repo, states, 1, "map");

            await enterCourse(session);
            const playing = await session.status();
            if ((playing.frame ?? 0) >= courseAt) {
                throw new Error(`course save frame ${courseAt} passed before the course was playing: ${session.last}`);
            }
            const heldCourse = await advanceUntilFrame(session, courseAt, 120);
            if (!inCourse(heldCourse)) throw new Error(`left the course before the save: ${session.last}`);
            await session.advance(45, 20);
            assertPortablePlace(session.repo, states, 2, "course");
        },
    },
    {
        name: "course-enter",
        summary: "Regression for #14 at 165 fps: enter one course, leave, enter another without freezing.",
        run: async (session) => {
            await session.start({
                save: "repo",
                env: { NSMBU_INTERP: "1", NSMBU_INTERP_FPS: "165" },
            });
            await reachWorldMap(session, 180);
            const first = await enterCourse(session);
            await leaveCourse(session);
            const back = await session.status();
            await walkToOtherNode(session, back.node);
            const second = await enterCourse(session);
            if (second === first) throw new Error(`second course is still ${second}`);
        },
    },
    {
        name: "course-enter-120",
        summary:
            "Regression for #15 at 120 fps: enter one course from the map, leave, enter another (ZoomHio / freed camera).",
        run: async (session) => {
            await session.start({
                save: "repo",
                env: { NSMBU_INTERP: "1", NSMBU_INTERP_FPS: "120" },
            });
            await reachWorldMap(session, 180);
            const first = await enterCourse(session);
            await leaveCourse(session);
            const back = await session.status();
            await walkToOtherNode(session, back.node);
            const second = await enterCourse(session);
            if (second === first) throw new Error(`second course is still ${second}`);
        },
    },
];
