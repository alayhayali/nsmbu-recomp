#!/usr/bin/env python3
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools", "recomp"))
import builds
import game_dump


def check_update(game):
    update = game_dump.update_root(game)
    known = ", ".join("%s %s (v%d)" % (b.name, b.title_id, b.title_version) for b in builds.all_builds())
    print("game: %s" % game)
    print("known builds: %s" % known)
    print("update folder: %s/ (optional for a known base-disc RPX)" % game_dump.UPDATE_DIRNAME)
    print("")
    print("== update (1.3.0) ==")
    errors, rpx = game_dump.folder_layout_errors(update, game_dump.UPDATE_DIRNAME)
    if not os.path.isdir(update):
        print("no update folder; will use the base disc if its RPX is a known build")
        return False
    if rpx:
        e, _ = game_dump.check_rpx(
            rpx,
            "%s/code/red-pro2.rpx" % game_dump.UPDATE_DIRNAME,
            require_update_title=True,
        )
        errors.extend(e)
    if errors:
        print("")
        print("check failed:")
        for e in errors:
            print("  %s" % e)
        sys.exit(
            "fix game/%s/ (code/, content/, meta/) or remove it to use a known base disc alone"
            % game_dump.UPDATE_DIRNAME
        )
    print("")
    print("update check ok")
    return True


def main():
    game = game_dump.game_root()
    image = os.path.join(game, "game.wux")
    key = os.path.join(game, "game.key")
    if not os.path.isfile(image):
        sys.exit("put your dump as game/game.wux")
    if not os.path.isfile(key):
        sys.exit("put the title key as game/game.key (next to game.wux)")

    have_update = check_update(game)

    print("")
    print("extracting base disc into game/...")
    subprocess.run(
        [
            sys.executable,
            os.path.join(ROOT, "tools", "disc", "wudextract.py"),
            image,
            "extract",
            game,
        ],
        check=True,
        cwd=ROOT,
    )

    if have_update:
        print("applying game/%s onto game/..." % game_dump.UPDATE_DIRNAME)
        game_dump.apply_update(game)

    rpx = os.path.join(game, "code", "red-pro2.rpx")
    digest = builds.file_sha256(rpx)
    build = builds.by_sha256(digest)
    if not build:
        known = ", ".join("%s %s..." % (b.name, b.sha256[:16]) for b in builds.all_builds())
        sys.exit(
            "code/red-pro2.rpx is %s... (need one of: %s)%s"
            % (
                digest[:16],
                known,
                "" if have_update else "; put the matching 1.3.0 update under game/%s/" % game_dump.UPDATE_DIRNAME,
            )
        )
    if have_update:
        print("extract ok: base disc + %s -> %s (title version %d)" % (
            game_dump.UPDATE_DIRNAME, build.name, build.title_version))
    else:
        print("extract ok: base disc -> %s (title version %d)" % (build.name, build.title_version))
    if not build.mods:
        print("note: mods are not supported on this build; install the 1.3.0 update for mod support")


if __name__ == "__main__":
    main()
