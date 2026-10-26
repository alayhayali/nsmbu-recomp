import {
    assertFileSelectQuick,
    hostQuickNode,
    hostRecord,
    quickSaveHere,
    reachWorldMap,
    sameProgress,
    walkToOtherNode,
    type PlayCase,
} from "../helpers.ts";

export const saveRoundtrip: PlayCase = {
    name: "save-roundtrip",
    summary: "Regression for #14: the map Quick Save survives a new process and the file-select slot uses the quick record.",
    run: async (session) => {
        await session.start({ save: "repo" });
        const start = await reachWorldMap(session, 180);
        await walkToOtherNode(session, start.node);
        const node = await quickSaveHere(session);
        const saved = hostRecord(session.saveDir, 0xc28);
        if (!saved || saved.exists === 0 || saved.node !== node) {
            throw new Error(`quick save file is exists=${saved?.exists} node=${saved?.node}, want node ${node}`);
        }
        await session.restart();
        const kept = hostRecord(session.saveDir, 0xc28);
        if (!kept || !sameProgress(kept, saved) || kept.exists === 0) {
            throw new Error(
                `quick save did not survive restart (exists=${kept?.exists} lives=${kept?.lives} flags=${kept?.flags} map=${kept?.world}.${kept?.section}.${kept?.node})`,
            );
        }
        const back = await reachWorldMap(session, 180);
        assertFileSelectQuick(session, saved);
        if (back.node !== node) {
            const host = hostQuickNode(session.saveDir);
            throw new Error(`reloaded node is ${back.node}, want ${node} (host exists=${host?.exists} node=${host?.node})`);
        }
    },
};
