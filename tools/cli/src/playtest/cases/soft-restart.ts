import type { PlayCase } from "../helpers.ts";

export const softRestart: PlayCase = {
    name: "soft-restart",
    summary: "In-window soft guest restart returns to a living boot (language/mod restart path).",
    run: async (session) => {
        await session.start({ save: "repo" });
        await session.wait((s) => s.scene !== undefined && s.scene !== "Boot", 180);
        const before = await session.status();
        if ((before.frame ?? 0) < 1) throw new Error(`no frames before soft restart: ${session.last}`);

        const ok = await session.eval('return nsmbu.Scene.soft_restart("playtest soft restart")');
        if (ok !== "true" && ok !== "1") throw new Error(`soft_restart refused: ${ok}`);

        await session.wait((s) => (s.frame ?? 0) < (before.frame ?? 0) || s.scene === "Boot", 120);
        await session.wait((s) => s.scene !== undefined && s.scene !== "Boot", 180);
        await session.advance(60, 30);
        const after = await session.status();
        if ((after.frame ?? 0) < 1) throw new Error(`no frames after soft restart: ${session.last}`);
    },
};
