import type { Session } from "./session.ts";

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
        name: "opened-course",
        summary: "Direct open loads the pack, a second pack does not crash, and quitting returns to that course's map node.",
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
        summary: "After the opening cinematic the world map takes input and shows a course number.",
        run: async (session) => {
            await session.start({ save: "fresh" });
            await session.wait((s) => s.scene !== undefined && s.scene !== "Boot", 180);
            const start = Date.now();
            let last = "";
            while (Date.now() - start < 180_000) {
                const s = await session.status();
                if (s.map === true && (s.scene === "Map" || s.scene === "CourseSelect")) break;
                last = await session.eval("return nsmbu.Scene.open_first_save(0)");
                if (!last.startsWith("true")) {
                    await session.eval("return nsmbu.Scene.open_title()");
                    await session.press("A", 15);
                }
                await session.advance(40, 20);
            }
            const arrived = await session.status();
            if (!(arrived.map === true && (arrived.scene === "Map" || arrived.scene === "CourseSelect"))) {
                throw new Error(`opening never reached the map: ${last} ${session.last}`);
            }
            const movedTo = await waitUntilMoves(session, 240);
            if (movedTo.title < 0 || !isCourseNode(movedTo.name)) {
                throw new Error(`course number hidden after input: ${movedTo.raw}`);
            }
        },
    },
];
