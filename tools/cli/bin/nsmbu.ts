#!/usr/bin/env node
import { findRepoRoot } from "../src/repo.ts";
import { HELP } from "../src/help.ts";
import { flagString, hasFlag, parseArgv } from "../src/parse.ts";
import {
    cmdBuild,
    cmdCrash,
    cmdExtract,
    cmdInstaller,
    cmdLaunch,
    cmdLint,
    cmdPerf,
    cmdProfile,
    cmdRecomp,
    cmdRecord,
    cmdReplay,
    cmdRun,
    cmdScripted,
    cmdWine,
} from "../src/dev.ts";
import {
    cmdOpen,
    cmdPress,
    cmdQuit,
    cmdScreenshot,
    cmdStatus,
    cmdStick,
    cmdWait,
} from "../src/agent/commands.ts";
import { cmdCompletion } from "../src/completion.ts";

async function main(): Promise<void> {
    const argv = process.argv.slice(2);
    if (argv.length === 0 || argv[0] === "--help" || argv[0] === "-h") {
        console.log(HELP.trimEnd());
        return;
    }
    const [cmd, ...rest] = argv;
    if (cmd === "completion") {
        try {
            cmdCompletion(rest);
        } catch (e) {
            const msg = e instanceof Error ? e.message : String(e);
            console.error(msg);
            process.exit(1);
        }
        return;
    }
    const repo = findRepoRoot();

    try {
        switch (cmd) {
            case "build": {
                const { flags, positional } = parseArgv(rest, ["release", "sanitizer"], []);
                cmdBuild(repo, hasFlag(flags, "release"), hasFlag(flags, "sanitizer"), positional);
                break;
            }
            case "launch": {
                const { flags, positional } = parseArgv(rest, ["no-debug", "trace", "gdb", "lldb"], []);
                cmdLaunch(
                    repo,
                    {
                        noDebug: hasFlag(flags, "no-debug"),
                        trace: hasFlag(flags, "trace"),
                        gdb: hasFlag(flags, "gdb"),
                        lldb: hasFlag(flags, "lldb"),
                    },
                    positional,
                );
                break;
            }
            case "crash":
                cmdCrash(repo);
                break;
            case "run": {
                const { flags, positional } = parseArgv(rest, ["release"], []);
                cmdRun(repo, hasFlag(flags, "release"), positional);
                break;
            }
            case "wine": {
                const { flags, positional } = parseArgv(rest, ["no-debug", "trace"], []);
                cmdWine(
                    repo,
                    {
                        noDebug: hasFlag(flags, "no-debug"),
                        trace: hasFlag(flags, "trace"),
                    },
                    positional,
                );
                break;
            }
            case "extract":
                cmdExtract(repo, rest);
                break;
            case "record": {
                const [course, ...launchRest] = rest;
                if (!course) {
                    throw new Error("usage: nsmbu record <course> […]");
                }
                cmdRecord(repo, course, launchRest);
                break;
            }
            case "replay": {
                const { flags, positional } = parseArgv(rest, ["release", "uncapped", "exit-at", "log"], ["exit-at", "log"]);
                const [course, ...launchRest] = positional;
                if (!course) {
                    throw new Error(
                        "usage: nsmbu replay [--release] [--uncapped] [--exit-at N] [--log PATH] <course> […]",
                    );
                }
                const exitAt = flagString(flags, "exit-at");
                if (hasFlag(flags, "exit-at") && (exitAt === undefined || !/^\d+$/.test(exitAt))) {
                    throw new Error("usage: nsmbu replay --exit-at N");
                }
                const log = flagString(flags, "log");
                if (hasFlag(flags, "log") && !log) {
                    throw new Error("usage: nsmbu replay --log PATH");
                }
                cmdReplay(
                    repo,
                    course,
                    {
                        release: hasFlag(flags, "release"),
                        uncapped: hasFlag(flags, "uncapped"),
                        exitAt,
                        log,
                    },
                    launchRest,
                );
                break;
            }
            case "profile":
                cmdProfile(repo, rest);
                break;
            case "perf":
                cmdPerf(repo, rest);
                break;
            case "installer":
                cmdInstaller(repo, rest);
                break;
            case "lint":
                cmdLint(repo);
                break;
            case "recomp":
                cmdRecomp(repo);
                break;
            case "scripted": {
                const [name, ...scriptRest] = rest;
                if (!name) {
                    throw new Error("usage: nsmbu scripted <name> […]");
                }
                cmdScripted(repo, name, scriptRest);
                break;
            }
            case "open":
                await cmdOpen(repo, rest);
                break;
            case "status":
                await cmdStatus(repo);
                break;
            case "press":
                await cmdPress(repo, rest);
                break;
            case "stick":
                await cmdStick(repo, rest, false);
                break;
            case "rstick":
                await cmdStick(repo, rest, true);
                break;
            case "screenshot":
                await cmdScreenshot(repo, rest);
                break;
            case "wait":
                await cmdWait(repo, rest);
                break;
            case "quit":
            case "close":
                await cmdQuit(repo);
                break;
            default:
                console.error(`unknown command: ${cmd}\n`);
                console.log(HELP.trimEnd());
                process.exit(1);
        }
    } catch (e) {
        const msg = e instanceof Error ? e.message : String(e);
        console.error(msg);
        process.exit(1);
    }
}

await main();
