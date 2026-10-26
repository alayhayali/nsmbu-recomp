import { isCourseNode, reachMap, waitUntilMoves, type PlayCase } from "../helpers.ts";

export const openingCinematic: PlayCase = {
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
};
