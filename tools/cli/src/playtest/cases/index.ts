import type { PlayCase } from "../helpers.ts";
import { courseEnter } from "./course-enter.ts";
import { courseEnter120 } from "./course-enter-120.ts";
import { mapShadow120 } from "./map-shadow-120.ts";
import { openedCourse } from "./opened-course.ts";
import { openingCinematic } from "./opening-cinematic.ts";
import { saveRoundtrip } from "./save-roundtrip.ts";
import { saveState } from "./save-state.ts";
import { softRestart } from "./soft-restart.ts";
import { startup } from "./startup.ts";

export type { PlayCase } from "../helpers.ts";

export const CASES: PlayCase[] = [
    startup,
    softRestart,
    openedCourse,
    openingCinematic,
    saveRoundtrip,
    saveState,
    courseEnter,
    courseEnter120,
    mapShadow120,
];
