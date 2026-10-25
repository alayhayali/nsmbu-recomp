import { CASES } from "./cases.ts";
import { Session } from "./session.ts";

export function parseTestArgs(rest: string[]): { list: boolean; names: string[] } {
    const list = rest.includes("--list");
    const names: string[] = [];
    const bad: string[] = [];
    for (const a of rest) {
        if (a === "--list") continue;
        if (a.startsWith("-")) bad.push(a);
        else names.push(a);
    }
    if (bad.length > 0) {
        throw new Error(`unknown option: ${bad.join(", ")}\nusage: nsmbu test [--list] [name…]`);
    }
    return { list, names };
}

export async function cmdTest(repo: string, rest: string[]): Promise<void> {
    const { list, names } = parseTestArgs(rest);
    if (list && names.length === 0) {
        for (const c of CASES) console.log(`${c.name}\t${c.summary}`);
        return;
    }
    const unknown = names.filter((n) => !CASES.some((c) => c.name === n));
    if (unknown.length > 0) {
        throw new Error(`unknown playtest: ${unknown.join(", ")}\nknown: ${CASES.map((c) => c.name).join(", ")}`);
    }
    const selected = names.length === 0 ? CASES : CASES.filter((c) => names.includes(c.name));
    let failed = 0;
    for (const c of selected) {
        const session = new Session(repo, c.name);
        const t0 = Date.now();
        process.stdout.write(`playtest ${c.name}\n`);
        try {
            await c.run(session);
            const sec = ((Date.now() - t0) / 1000).toFixed(1);
            process.stdout.write(`  PASS ${sec}s\n`);
        } catch (e) {
            failed++;
            const msg = e instanceof Error ? e.message : String(e);
            const where = session.last ? ` (${session.last})` : "";
            process.stdout.write(`  FAIL ${msg}${where}\n`);
        } finally {
            await session.stop();
        }
    }
    if (failed > 0) {
        throw new Error(`${failed} playtest${failed === 1 ? "" : "s"} failed`);
    }
}
