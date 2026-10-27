import { readFileSync } from "node:fs";
import { join } from "node:path";
import { advanceUntilFrame, inCourse, waitCourse, type PlayCase } from "../helpers.ts";
import type { Session } from "../session.ts";
import { readPng } from "./widescreen-map-ui.ts";

const kSwitch = 1000;
const kBase = 16 / 9;
const kWide = 64 / 27;

function near(value: number, target: number, slop: number): boolean {
    return Math.abs(value - target) <= slop;
}

async function capture(session: Session, name: string): Promise<{ w: number; h: number; rgb: Uint8Array }> {
    const path = join(session.workDir, name);
    await session.screenshot(path);
    const ready = Date.now() + 5000;
    for (;;) {
        try {
            return readPng(path);
        } catch (err) {
            if (Date.now() > ready) throw err;
            await new Promise((r) => setTimeout(r, 50));
        }
    }
}

function marioFraction(img: { w: number; h: number; rgb: Uint8Array }): number {
    const y0 = Math.floor(img.h * 0.45);
    const y1 = Math.floor(img.h * 0.95);
    const hits: number[] = [];
    for (let y = y0; y < y1; y += 2) {
        const row = y * img.w * 3;
        for (let x = 0; x < img.w; x += 2) {
            const i = row + x * 3;
            const r = img.rgb[i];
            const g = img.rgb[i + 1];
            const b = img.rgb[i + 2];
            if (r > 180 && g < 70 && b < 80 && r > g + 100) hits.push(x);
        }
    }
    if (hits.length < 40) throw new Error(`Mario not found (${hits.length} red pixels)`);
    hits.sort((a, b) => a - b);
    const groups: number[][] = [];
    let cur = [hits[0]];
    for (const x of hits.slice(1)) {
        if (x - cur[cur.length - 1] <= 12) cur.push(x);
        else {
            groups.push(cur);
            cur = [x];
        }
    }
    groups.push(cur);
    groups.sort((a, b) => b.length - a.length);
    const body = groups[0];
    if (body.length < 30) throw new Error(`Mario cluster too small (${body.length})`);
    const mid = (body[0] + body[body.length - 1]) / 2;
    return mid / img.w;
}

function aspectLogged(session: Session): boolean {
    const log = readFileSync(session.logPath, "utf8");
    return log.includes("[aspect] game aspect 2.3704");
}

export const widescreenMario: PlayCase = {
    name: "widescreen-mario",
    summary: "Mario stays at the same horizontal fraction in 1-1 when the picture switches from 16:9 to 21:9.",
    run: async (session) => {
        await session.start({
            save: "repo",
            open: "1-1",
            env: { NSMBU_ASPECT_AT: `${kSwitch}:2.37037037037` },
        });
        await waitCourse(session, "1-1", 180);
        await session.advance(60, 20);
        await session.stick(1, 0, 25);
        await session.advance(90, 30);
        const before = await session.status();
        const frame = before.frame ?? 0;
        if (frame > kSwitch - 60) {
            throw new Error(`16:9 capture missed the aspect switch at frame ${frame}`);
        }
        const narrow = await capture(session, "mario-16x9.png");
        const narrowAspect = narrow.w / narrow.h;
        if (!near(narrowAspect, kBase, 0.05)) {
            throw new Error(`16:9 picture is ${narrowAspect.toFixed(3)} (${narrow.w}x${narrow.h})`);
        }
        const narrowFrac = marioFraction(narrow);
        if (narrowFrac >= 0.48) {
            throw new Error(`Mario is already at ${narrowFrac.toFixed(3)} of the 16:9 picture`);
        }
        await advanceUntilFrame(session, kSwitch + 45, 40);
        const ready = Date.now() + 15000;
        while (!aspectLogged(session)) {
            if (Date.now() > ready) throw new Error("21:9 aspect did not apply");
            await session.advance(20, 15);
        }
        await session.advance(30, 15);
        const playing = await session.status();
        if (!inCourse(playing)) throw new Error(`left 1-1 before the 21:9 shot: ${session.last}`);
        const wide = await capture(session, "mario-21x9.png");
        const wideAspect = wide.w / wide.h;
        if (!near(wideAspect, kWide, 0.05)) {
            throw new Error(`21:9 picture is ${wideAspect.toFixed(3)} (${wide.w}x${wide.h})`);
        }
        const wideFrac = marioFraction(wide);
        if (Math.abs(wideFrac - narrowFrac) > 0.03) {
            throw new Error(
                `Mario shifted from ${narrowFrac.toFixed(3)} at 16:9 to ${wideFrac.toFixed(3)} at 21:9`,
            );
        }
        const posed = await session.status();
        if (!posed.player) throw new Error(`player missing before the turn: ${session.last}`);
        const x0 = posed.player.x;
        const y0 = posed.player.y;
        await session.stick(-1, 0, 20);
        await session.press("A", 12);
        await session.advance(90, 30);
        const after = await session.status();
        if (!inCourse(after) || !after.player) throw new Error(`left the course after the turn: ${session.last}`);
        const dx = Math.abs(after.player.x - x0);
        const dy = Math.abs(after.player.y - y0);
        if (dx > 700 || dy > 400) {
            throw new Error(`camera drift flung Mario by ${dx.toFixed(0)}, ${dy.toFixed(0)}`);
        }
    },
};
