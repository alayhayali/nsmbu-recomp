import { spawn, spawnSync } from "node:child_process";
import type { SpawnOptions } from "node:child_process";
import { join } from "node:path";
import { nsmbuUserDir } from "./repo.ts";

export function baseEnv(repo: string, extra: Record<string, string> = {}): NodeJS.ProcessEnv {
    return {
        ...process.env,
        NSMBU_USER_DIR: nsmbuUserDir(repo),
        ...extra,
    };
}

export function runPython(
    repo: string,
    script: string,
    args: string[],
    extraEnv: Record<string, string> = {},
): void {
    const r = spawnSync("python3", [join(repo, script), ...args], {
        cwd: repo,
        env: baseEnv(repo, extraEnv),
        stdio: "inherit",
    });
    if (r.error) {
        console.error(r.error.message);
        process.exit(1);
    }
    process.exit(r.status ?? 1);
}

export function spawnPython(
    repo: string,
    script: string,
    args: string[],
    options: SpawnOptions & { extraEnv?: Record<string, string> } = {},
): ReturnType<typeof spawn> {
    const { extraEnv = {}, ...spawnOpts } = options;
    return spawn("python3", [join(repo, script), ...args], {
        cwd: repo,
        env: baseEnv(repo, extraEnv),
        ...spawnOpts,
    });
}

export function runShell(repo: string, command: string, extraEnv: Record<string, string> = {}): void {
    const r = spawnSync(command, {
        cwd: repo,
        env: baseEnv(repo, extraEnv),
        stdio: "inherit",
        shell: true,
    });
    if (r.error) {
        console.error(r.error.message);
        process.exit(1);
    }
    process.exit(r.status ?? 1);
}
