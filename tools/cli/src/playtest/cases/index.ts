import type { PlayCase } from "../helpers.ts";
import { courseEnter } from "./course-enter.ts";
import { courseEnter120 } from "./course-enter-120.ts";
import { mapShadow120 } from "./map-shadow-120.ts";
import { openedCourse } from "./opened-course.ts";
import { openingCinematic } from "./opening-cinematic.ts";
import { saveRoundtrip } from "./save-roundtrip.ts";
import { saveState } from "./save-state.ts";
import { startup } from "./startup.ts";
import { widescreenMapUi } from "./widescreen-map-ui.ts";
import { widescreenMario } from "./widescreen-mario.ts";
import { widescreenTitle } from "./widescreen-title.ts";

export type { PlayCase } from "../helpers.ts";

export const CASES: PlayCase[] = [
    startup,
    openedCourse,
    openingCinematic,
    saveRoundtrip,
    saveState,
    courseEnter,
    courseEnter120,
    mapShadow120,
    widescreenMapUi,
    widescreenMario,
    widescreenTitle,
];
