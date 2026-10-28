import { onMap, reachWorldMap, type PlayCase } from "../helpers.ts";
import type { Session } from "../session.ts";

const caps = [120, 144, 165, 240];

async function measure(session: Session, cap: number, first: boolean): Promise<void> {
    const env = { NSMBU_INTERP: "1", NSMBU_INTERP_FPS: String(cap) };
    if (first) {
        await session.start({ save: "repo", env });
    } else {
        await session.restart(env);
    }
    await reachWorldMap(session, 180);
    const a = await session.status();
    if (!onMap(a)) throw new Error(`not on map at ${cap}: ${session.last}`);
    const t0 = Date.now();
    const step0 = a.step ?? 0;
    const frame0 = a.frame ?? 0;
    await session.advance(180, 20);
    const b = await session.status();
    const elapsed = (Date.now() - t0) / 1000;
    if (elapsed < 0.5) throw new Error(`window too short at ${cap}: ${elapsed}s`);
    const steps = (b.step ?? 0) - step0;
    const frames = (b.frame ?? 0) - frame0;
    const stepsPerSec = steps / elapsed;
    const framesPerSec = frames / elapsed;
    if (stepsPerSec < 50 || stepsPerSec > 70) {
        throw new Error(`logic rate ${stepsPerSec.toFixed(1)}/s out of band at ${cap} (want ~60); ${session.last}`);
    }
    const presentFloor = cap * 0.75;
    if (framesPerSec < presentFloor) {
        throw new Error(
            `present rate ${framesPerSec.toFixed(1)}/s too low for ${cap} cap; ${session.last}`,
        );
    }
    if (frames < steps * 1.3) {
        throw new Error(`presents ${frames} not clearly above logic steps ${steps} at ${cap}; ${session.last}`);
    }
}

export const logicRate120: PlayCase = {
    name: "logic-rate-120",
    summary: "At present caps 120, 144, 165, and 240, logic steps stay near 60/s while presents track the cap.",
    run: async (session) => {
        for (let i = 0; i < caps.length; i++) await measure(session, caps[i], i === 0);
    },
};
