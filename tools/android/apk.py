#!/usr/bin/env python3
"""Build a local Android APK and optionally install it with adb."""
import argparse
import glob
import json
import os
import re
import shlex
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "lib"))
import game_versions as gv

NDK_VERSION = "30.0.16248370"
PACKAGE = "org.nsmbrecomp.nsmbu"
APK = os.path.join(ROOT, "android", "app", "build", "outputs", "apk", "release", "app-release.apk")
DEVICE_GAME = "/sdcard/Android/data/%s/files/game" % PACKAGE
VALID_ABI = frozenset({"arm64-v8a", "x86_64"})
GRADLE_JAVA_MAX = 21


class ApkError(Exception):
    pass


def default_sdk():
    if os.environ.get("ANDROID_SDK"):
        return os.environ["ANDROID_SDK"]
    if os.environ.get("ANDROID_HOME"):
        return os.environ["ANDROID_HOME"]
    if os.environ.get("ANDROID_SDK_ROOT"):
        return os.environ["ANDROID_SDK_ROOT"]
    return os.path.expanduser("~/Android/Sdk")


def java_major_from_version_text(text):
    m = re.search(r'version "1\.(\d+)\.', text)
    if m:
        return int(m.group(1))
    m = re.search(r'version "(\d+)\.', text)
    if m:
        return int(m.group(1))
    return None


def java_major(java_bin):
    r = run([java_bin, "-version"], capture=True, check=False)
    text = ((r.stdout or b"") + (r.stderr or b"")).decode(errors="replace")
    return java_major_from_version_text(text)


def jdk_candidates():
    seen = set()
    roots = []
    if os.environ.get("JAVA_HOME"):
        roots.append(os.environ["JAVA_HOME"])
    for pattern in ("/usr/lib/jvm/*", "/Library/Java/JavaVirtualMachines/*/Contents/Home"):
        roots.extend(glob.glob(pattern))
    for root in roots:
        root = os.path.abspath(root)
        if root in seen:
            continue
        seen.add(root)
        java = os.path.join(root, "bin", "java")
        if not os.path.isfile(java):
            continue
        major = java_major(java)
        if major is None or major > GRADLE_JAVA_MAX:
            continue
        yield root, major


def pick_gradle_java_home():
    picks = list(jdk_candidates())
    if picks:
        picks.sort(key=lambda item: (0 if item[1] == 17 else 1, item[1]))
        return picks[0][0]
    default = java_major("java")
    raise ApkError(
        "Gradle needs JDK 17 or 21 (default java is %s). "
        "Install openjdk-21-jdk or point JAVA_HOME at a supported JDK."
        % (default if default is not None else "unknown")
    )


def gradle_env(base):
    env = base.copy()
    home = pick_gradle_java_home()
    env["JAVA_HOME"] = home
    env["PATH"] = os.path.join(home, "bin") + os.pathsep + env.get("PATH", "")
    return env


def adb_bin():
    path = os.environ.get("ADB") or "adb"
    if path != "adb" and not os.path.isfile(path):
        raise ApkError("ADB=%s is not a file" % path)
    return path


def run(cmd, *, check=True, capture=False, input_bytes=None, env=None):
    kwargs = {
        "cwd": ROOT,
        "env": env if env is not None else os.environ,
    }
    if capture:
        kwargs.update({"stdout": subprocess.PIPE, "stderr": subprocess.PIPE})
    else:
        kwargs["stdout"] = None
        kwargs["stderr"] = None
    try:
        r = subprocess.run(cmd, input=input_bytes, **kwargs)
    except FileNotFoundError as e:
        raise ApkError(str(e)) from e
    if check and r.returncode != 0:
        err = (r.stderr or b"").decode(errors="replace").strip() if capture else ""
        msg = "command failed (%s): %s" % (r.returncode, " ".join(shlex.quote(c) for c in cmd))
        if err:
            msg += "\n" + err
        raise ApkError(msg)
    return r


def adb_cmd(serial, *args):
    cmd = [adb_bin()]
    if serial:
        cmd += ["-s", serial]
    cmd += list(args)
    return cmd


def list_devices():
    r = run(adb_cmd(None, "devices", "-l"), capture=True)
    out = (r.stdout or b"").decode(errors="replace")
    devices = []
    for line in out.splitlines():
        line = line.strip()
        if not line or line.startswith("List of devices"):
            continue
        parts = line.split()
        if len(parts) < 2 or parts[1] != "device":
            continue
        serial = parts[0]
        model = None
        for token in parts[2:]:
            if token.startswith("model:"):
                model = token.split(":", 1)[1]
                break
        devices.append({"serial": serial, "model": model})
    return devices


def device_abi(serial):
    r = run(adb_cmd(serial, "shell", "getprop", "ro.product.cpu.abi"), capture=True)
    text = (r.stdout or b"").decode(errors="replace").strip()
    if text in VALID_ABI:
        return text
    if text == "arm64-v8a" or text.startswith("arm64"):
        return "arm64-v8a"
    return text or None


def gen_has_code(gen_dir):
    return bool(glob.glob(os.path.join(gen_dir, "code_*.c")))


def resolve_gen(stub):
    if stub:
        return "stub"
    gen = os.path.join(ROOT, "build", "gen")
    if gen_has_code(gen):
        return gen
    raise ApkError(
        "build/gen has no recompiled game code; run nsmbu game --recomp or pass --stub for a placeholder APK"
    )


def active_game_dir():
    try:
        dump = gv.active_dir()
    except gv.GameVersionsError as e:
        raise ApkError(str(e)) from e
    if not dump or not os.path.isdir(dump):
        raise ApkError("no active game dump; run nsmbu game")
    if not os.path.isfile(gv.rpx_path(dump)):
        raise ApkError("active dump is not extracted; run nsmbu game --extract")
    return dump


def ensure_luajit():
    luajit = os.path.join(ROOT, "vendor", "luajit", "Makefile")
    if os.path.isfile(luajit):
        return
    run(["git", "submodule", "update", "--init", "vendor/luajit"])


def build_apk(*, abi, gen_dir, jobs):
    sdk = default_sdk()
    if not os.path.isdir(sdk):
        raise ApkError("Android SDK not found at %s (set ANDROID_HOME)" % sdk)
    ndk = os.path.join(sdk, "ndk", NDK_VERSION)
    if not os.path.isfile(os.path.join(ndk, "build", "cmake", "android.toolchain.cmake")):
        raise ApkError(
            'install NDK %s: sdkmanager --install "ndk;%s"' % (NDK_VERSION, NDK_VERSION)
        )
    ensure_luajit()
    env = os.environ.copy()
    env["ANDROID_SDK"] = sdk
    env["ABI"] = abi
    env["JOBS"] = str(jobs)
    env["GEN_DIR"] = gen_dir
    print("== native (ABI %s)" % abi)
    run(["bash", os.path.join(ROOT, "android", "build_native.sh")], env=env)
    gradle = os.path.join(ROOT, "android", "gradlew")
    props = ["-PnsmbuAbi=%s" % abi]
    print("== APK (assembleRelease)")
    run(
        [gradle, "-p", "android", "--no-daemon", "assembleRelease", *props],
        env=gradle_env(env),
    )
    if not os.path.isfile(APK):
        raise ApkError("Gradle did not produce %s" % APK)
    print("built %s" % APK)


def install_apk(serial):
    print("== install on %s" % (serial or "default device"))
    r = run(adb_cmd(serial, "install", "-r", APK), check=False, capture=True)
    out = ((r.stdout or b"") + (r.stderr or b"")).decode(errors="replace")
    if r.returncode == 0:
        print(out.strip() or "Success")
        return
    if "INSTALL_FAILED_UPDATE_INCOMPATIBLE" in out or "signatures do not match" in out.lower():
        raise ApkError(
            "install failed: the device has NSMBU signed with a different key.\n"
            "Uninstall the old app, then run nsmbu apk again.\n" + out.strip()
        )
    raise ApkError("adb install failed:\n" + out.strip())


def push_game(serial, dump_dir):
    remote = DEVICE_GAME.rstrip("/") + "/"
    rel = os.path.relpath(dump_dir, ROOT)
    print("== game assets")
    print("   from  %s/  (code, content, meta)" % rel)
    print("   to    %s/" % remote)
    print("   (app data: Android/data/%s/files/game/)" % PACKAGE)
    run(adb_cmd(serial, "shell", "mkdir", "-p", remote))
    src = dump_dir.rstrip("/") + "/."
    run(adb_cmd(serial, "push", src, remote))


def parse_args(argv):
    p = argparse.ArgumentParser(description="Build and install a local NSMBU Android APK.")
    p.add_argument("--list-devices", action="store_true", help=argparse.SUPPRESS)
    p.add_argument("--abi", choices=sorted(VALID_ABI))
    p.add_argument("--device", help="adb serial (default: only connected device)")
    p.add_argument("--stub", action="store_true", help="Placeholder guest code (CI-style)")
    p.add_argument("--build-only", action="store_true", help="Skip adb install and push")
    p.add_argument("--push-only", action="store_true", help="Copy the active dump to the device only")
    p.add_argument("--no-push", action="store_true", help="Do not copy the active dump after install")
    p.add_argument("--push-game", action="store_true", help="Copy the active dump after install")
    p.add_argument("--jobs", type=int, default=max(1, os.cpu_count() or 4))
    return p.parse_args(argv)


def main(argv=None):
    args = parse_args(argv if argv is not None else sys.argv[1:])
    if args.list_devices:
        print(json.dumps(list_devices()))
        return 0
    if args.push_only:
        devices = list_devices()
        if not devices:
            raise ApkError("no adb device in 'device' state; plug one in and enable USB debugging")
        serial = args.device
        if serial is None:
            if len(devices) != 1:
                raise ApkError("multiple devices; pass --device SERIAL")
            serial = devices[0]["serial"]
        push_game(serial, active_game_dir())
        return 0
    if args.install_only:
        if not os.path.isfile(APK):
            raise ApkError("no APK at %s; build first" % APK)
        devices = list_devices()
        if not devices:
            raise ApkError("no adb device in 'device' state; plug one in and enable USB debugging")
        serial = args.device
        if serial is None:
            if len(devices) != 1:
                raise ApkError("multiple devices; pass --device SERIAL")
            serial = devices[0]["serial"]
        install_apk(serial)
        return 0
    gen_dir = resolve_gen(args.stub)
    abi = args.abi
    serial = args.device
    devices = []
    if not args.build_only:
        devices = list_devices()
        if not devices:
            raise ApkError("no adb device in 'device' state; plug one in and enable USB debugging")
        if serial is None and len(devices) == 1:
            serial = devices[0]["serial"]
        if serial and abi is None:
            detected = device_abi(serial)
            if detected in VALID_ABI:
                abi = detected
    if abi is None:
        abi = "arm64-v8a"
    build_apk(abi=abi, gen_dir=gen_dir, jobs=args.jobs)
    if args.build_only:
        return 0
    if serial is None:
        raise ApkError("multiple adb devices; pass --device SERIAL")
    install_apk(serial)
    if args.no_push:
        return 0
    if args.stub and not args.push_game:
        return 0
    push_game(serial, active_game_dir())
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ApkError as e:
        print(e, file=sys.stderr)
        raise SystemExit(1)
