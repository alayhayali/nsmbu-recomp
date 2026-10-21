import { existsSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const pkgRoot = dirname(dirname(fileURLToPath(import.meta.url)));

function walkUp(start: string): string | null {
    let dir = start;
    for (;;) {
        if (existsSync(join(dir, "tools", "build", "build.py")) || existsSync(join(dir, ".git"))) {
            return dir;
        }
        const parent = dirname(dir);
        if (parent === dir) {
            return null;
        }
        dir = parent;
    }
}

export function findRepoRoot(): string {
    for (const start of [process.cwd(), pkgRoot]) {
        const root = walkUp(start);
        if (root) {
            return root;
        }
    }
    throw new Error("NSMBU repo root not found (looked for tools/build/build.py or .git)");
}

export function nsmbuUserDir(repo: string): string {
    return join(repo, "nsmbu");
}
