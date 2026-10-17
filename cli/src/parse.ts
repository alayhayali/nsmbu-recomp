export type ParsedArgv = {
    flags: Map<string, string | true>;
    positional: string[];
};

export function parseArgv(argv: string[], flagNames: string[]): ParsedArgv {
    const flags = new Map<string, string | true>();
    const positional: string[] = [];
    const known = new Set(flagNames);
    for (let i = 0; i < argv.length; i++) {
        const a = argv[i];
        if (a === "--") {
            positional.push(...argv.slice(i + 1));
            break;
        }
        if (a.startsWith("--")) {
            const eq = a.indexOf("=");
            if (eq !== -1) {
                const name = a.slice(2, eq);
                if (!known.has(name)) {
                    positional.push(a);
                    continue;
                }
                flags.set(name, a.slice(eq + 1));
                continue;
            }
            const name = a.slice(2);
            if (!known.has(name)) {
                positional.push(a);
                continue;
            }
            const next = argv[i + 1];
            if (next !== undefined && !next.startsWith("-")) {
                flags.set(name, next);
                i++;
            } else {
                flags.set(name, true);
            }
            continue;
        }
        positional.push(a);
    }
    return { flags, positional };
}

export function hasFlag(flags: Map<string, string | true>, name: string): boolean {
    return flags.has(name);
}

export function flagString(flags: Map<string, string | true>, name: string, fallback?: string): string | undefined {
    const v = flags.get(name);
    if (v === undefined) {
        return fallback;
    }
    if (v === true) {
        return fallback;
    }
    return v;
}

export function flagNumber(flags: Map<string, string | true>, name: string, fallback: number): number {
    const s = flagString(flags, name);
    if (s === undefined) {
        return fallback;
    }
    const n = Number(s);
    if (!Number.isFinite(n)) {
        throw new Error(`invalid number for --${name}: ${s}`);
    }
    return n;
}
