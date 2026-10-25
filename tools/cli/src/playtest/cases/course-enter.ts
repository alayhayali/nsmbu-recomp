import { courseEnterRoundtrip, type PlayCase } from "../helpers.ts";

export const courseEnter: PlayCase = {
    name: "course-enter",
    summary: "Regression for #14 at 165 fps: enter one course, leave, enter another without freezing.",
    run: async (session) => {
        await courseEnterRoundtrip(session, "165");
    },
};
