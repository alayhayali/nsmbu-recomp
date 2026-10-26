import { mkdirSync } from "node:fs";
import { join } from "node:path";
import {
    advanceUntilFrame,
    assertPortablePlace,
    enterCourse,
    inCourse,
    onMap,
    reachWorldMap,
    type PlayCase,
} from "../helpers.ts";

export const saveState: PlayCase = {
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
        assertPortablePlace(session, states, 1, "map");

        await enterCourse(session);
        const playing = await session.status();
        if ((playing.frame ?? 0) >= courseAt) {
            throw new Error(`course save frame ${courseAt} passed before the course was playing: ${session.last}`);
        }
        const heldCourse = await advanceUntilFrame(session, courseAt, 120);
        if (!inCourse(heldCourse)) throw new Error(`left the course before the save: ${session.last}`);
        await session.advance(45, 20);
        assertPortablePlace(session, states, 2, "course");
    },
};
