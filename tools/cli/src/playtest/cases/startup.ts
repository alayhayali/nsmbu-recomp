import type { PlayCase } from "../helpers.ts";

export const startup: PlayCase = {
    name: "startup",
    summary: "Regression for #6: the game stays up past Boot with a prepared save.",
    run: async (session) => {
        await session.start({ save: "repo" });
        await session.wait((s) => s.scene !== undefined && s.scene !== "Boot", 180);
        const s = await session.status();
        if ((s.frame ?? 0) < 1) throw new Error(`no frames after boot: ${session.last}`);
        await session.advance(60, 30);
        const again = await session.status();
        if ((again.frame ?? 0) <= (s.frame ?? 0)) {
            throw new Error(`frame counter stuck after boot: ${session.last}`);
        }
    },
};
