import { courseEnterRoundtrip, type PlayCase } from "../helpers.ts";

export const courseEnter120: PlayCase = {
    name: "course-enter-120",
    summary:
        "Regression for #15 at 120 fps: enter a course from the map, leave, walk, enter again (ZoomHio / freed camera).",
    run: async (session) => {
        await courseEnterRoundtrip(session, "120");
    },
};
