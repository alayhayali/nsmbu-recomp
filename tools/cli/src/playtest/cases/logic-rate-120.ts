import { onMap, reachWorldMap, type PlayCase } from "../helpers.ts";

export const logicRate120: PlayCase = {
    name: "logic-rate-120",
    summary:
        "At a 120 present cap, logic steps stay near 60/s while presents track the cap.",
    run: async (session) => {
        await session.start({
            save: "repo",
            env: { NSMBU_INTERP: "1", NSMBU_INTERP_FPS: "120" },
        });
        await reachWorldMap(session, 180);
        const a = await session.status();
        if (!onMap(a)) throw new Error(`not on map: ${session.last}`);
        const t0 = Date.now();
        const step0 = a.step ?? 0;
        const frame0 = a.frame ?? 0;
        await session.advance(180, 20);
        const b = await session.status();
        const elapsed = (Date.now() - t0) / 1000;
        if (elapsed < 0.5) throw new Error(`window too short: ${elapsed}s`);
        const steps = (b.step ?? 0) - step0;
        const frames = (b.frame ?? 0) - frame0;
        const stepsPerSec = steps / elapsed;
        const framesPerSec = frames / elapsed;
        if (stepsPerSec < 50 || stepsPerSec > 70) {
            throw new Error(`logic rate ${stepsPerSec.toFixed(1)}/s out of band (want ~60); ${session.last}`);
        }
        if (framesPerSec < 90) {
            throw new Error(`present rate ${framesPerSec.toFixed(1)}/s too low for 120 cap; ${session.last}`);
        }
        if (frames < steps * 1.3) {
            throw new Error(`presents ${frames} not clearly above logic steps ${steps}; ${session.last}`);
        }
    },
};
