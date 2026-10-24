import { cpSync, mkdirSync, rmSync, writeFileSync } from "node:fs";
import { join } from "node:path";
import { AgentClient } from "../agent/client.ts";
import { parseButtonBits } from "../agent/buttons.ts";
import { agentDir, agentPidPath, agentSocketPath } from "../agent/paths.ts";
import { isPidAlive, readAgentPid, removeAgentArtifacts } from "../agent/process.ts";
import { canConnectSocket, waitForSocket } from "../agent/socket.ts";
import { spawnPython } from "../run.ts";

export type PlayStatus = {
    frame?: number;
    step?: number;
    scene?: string;
    map?: boolean;
    course?: boolean;
    pack?: string;
    entering?: string;
    file?: number;
    cp?: number;
    node?: number;
    world?: number;
    player?: { x: number; y: number; z: number } | null;
};

export function formatStatus(s: PlayStatus): string {
    const parts = [`frame=${s.frame ?? "?"}`, `step=${s.step ?? "?"}`];
    if (s.scene) parts.push(`scene=${s.scene}`);
    if (s.map !== undefined) parts.push(`map=${s.map ? 1 : 0}`);
    if (s.course !== undefined) parts.push(`course=${s.course ? 1 : 0}`);
    if (s.pack) parts.push(`pack=${s.pack}`);
    if (s.entering) parts.push(`entering=${s.entering}`);
    if (s.file !== undefined) parts.push(`file=${s.file}`);
    if (s.cp !== undefined) parts.push(`cp=${s.cp}`);
    if (s.node !== undefined) parts.push(`node=${s.node}`);
    if (s.world !== undefined) parts.push(`world=${s.world}`);
    if (s.player) parts.push(`player=(${s.player.x}, ${s.player.y}, ${s.player.z})`);
    else parts.push("player=none");
    return parts.join(" ");
}

function withTimeout<T>(p: Promise<T>, ms: number, label: string): Promise<T> {
    return new Promise((resolve, reject) => {
        const t = setTimeout(() => reject(new Error(`${label} timed out after ${ms / 1000}s`)), ms);
        p.then(
            (v) => {
                clearTimeout(t);
                resolve(v);
            },
            (e) => {
                clearTimeout(t);
                reject(e instanceof Error ? e : new Error(String(e)));
            },
        );
    });
}

async function ensureIdle(repo: string): Promise<void> {
    const sock = agentSocketPath(repo);
    if (await canConnectSocket(sock)) {
        throw new Error("agent game already running (control socket is active)");
    }
    const pid = readAgentPid(repo);
    if (pid !== null && isPidAlive(pid)) {
        throw new Error(`agent game already running (pid ${pid})`);
    }
    removeAgentArtifacts(repo);
}

export class Session {
    readonly repo: string;
    readonly name: string;
    readonly saveDir: string;
    private client: AgentClient | null = null;
    private pid: number | null = null;
    last = "";

    constructor(repo: string, name: string) {
        this.repo = repo;
        this.name = name;
        this.saveDir = join(repo, ".tmp", "playtest", name, "save");
    }

    async start(opts: { save: "fresh" | "repo"; open?: string }): Promise<void> {
        await ensureIdle(this.repo);
        rmSync(join(this.repo, ".tmp", "playtest", this.name), { recursive: true, force: true });
        mkdirSync(this.saveDir, { recursive: true });
        if (opts.save === "repo") {
            cpSync(join(this.repo, "nsmbu", "save"), this.saveDir, { recursive: true });
        }
        mkdirSync(agentDir(this.repo), { recursive: true });
        const sock = agentSocketPath(this.repo);
        const args = ["--save", this.saveDir];
        if (opts.open) args.push(opts.open);
        const child = spawnPython(this.repo, "tools/run/launch.py", args, {
            detached: true,
            stdio: "ignore",
            extraEnv: {
                NSMBU_AGENT_SOCK: sock,
                NSMBU_HIDDEN_WINDOWS: "1",
                NSMBU_NO_HOST_INPUT: "1",
                NSMBU_NO_AUDIO: "1",
                NSMBU_VK_PRESENT_MODE: "immediate",
                NSMBU_RES_SCALE: "1",
            },
        });
        child.unref();
        if (child.pid === undefined) throw new Error("failed to spawn game");
        this.pid = child.pid;
        writeFileSync(agentPidPath(this.repo), String(child.pid));
        await waitForSocket(sock, 90_000);
        this.client = new AgentClient(this.repo);
    }

    private live(): AgentClient {
        if (!this.client) throw new Error("game is not running");
        return this.client;
    }

    async status(): Promise<PlayStatus> {
        const r = await withTimeout(this.live().status(), 20_000, "status");
        const s: PlayStatus = {
            frame: r.frame,
            step: r.step,
            scene: r.scene,
            map: r.map,
            course: r.course,
            pack: r.pack,
            entering: r.entering,
            file: r.file,
            cp: r.cp,
            node: r.node,
            world: r.world,
            player: r.player,
        };
        this.last = formatStatus(s);
        return s;
    }

    async eval(code: string, timeoutMs = 8000): Promise<string> {
        const r = await withTimeout(this.live().request({ cmd: "eval", code, timeout: timeoutMs }), timeoutMs + 5000, "eval");
        if (!r.ok) throw new Error(r.error ?? "eval failed");
        return (r.output ?? "").trim();
    }

    async press(button: string, frames = 20): Promise<void> {
        const r = await withTimeout(
            this.live().request({ cmd: "press", bits: parseButtonBits(button), frames }),
            Math.max(15000, frames * 50),
            "press",
        );
        if (!r.ok) throw new Error(r.error ?? "press failed");
    }

    async stick(x: number, y: number, frames = 60): Promise<void> {
        const r = await withTimeout(
            this.live().request({ cmd: "stick", x, y, frames }),
            Math.max(15000, frames * 50),
            "stick",
        );
        if (!r.ok) throw new Error(r.error ?? "stick failed");
    }

    async wait(pred: (s: PlayStatus) => boolean, timeoutSec: number): Promise<PlayStatus> {
        const start = Date.now();
        for (;;) {
            const s = await this.status();
            if (pred(s)) return s;
            if (Date.now() - start >= timeoutSec * 1000) {
                throw new Error(`wait timed out after ${timeoutSec}s: ${this.last}`);
            }
            await new Promise((r) => setTimeout(r, 100));
        }
    }

    async advance(steps: number, timeoutSec = 30): Promise<void> {
        const now = await this.status();
        const target = (now.step ?? 0) + steps;
        await this.wait((s) => (s.step ?? 0) >= target, timeoutSec);
    }

    async stop(): Promise<void> {
        const client = this.client;
        this.client = null;
        if (client) {
            try {
                await withTimeout(client.request({ cmd: "quit" }), 5000, "quit");
            } catch {
                /* process may already be gone */
            }
            client.close();
        }
        const pid = this.pid ?? readAgentPid(this.repo);
        const deadline = Date.now() + 5000;
        while (pid !== null && isPidAlive(pid) && Date.now() < deadline) {
            await new Promise((r) => setTimeout(r, 100));
        }
        if (pid !== null && isPidAlive(pid)) {
            try {
                process.kill(pid, "SIGTERM");
            } catch {
                /* already exited */
            }
        }
        removeAgentArtifacts(this.repo);
        this.pid = null;
    }
}
