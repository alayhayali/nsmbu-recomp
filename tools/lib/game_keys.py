"""Disc and common keys for dumps under game/.

Keys live in game/ (Cemu keys.txt, common.key, per-dump <id>.key, or a sidecar next to a
dropped .wux). Version folders do not need their own key files.
"""
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "installer"))
import setup

IMAGE_SUFFIXES = (".wux", ".wud")


def is_image_name(name):
    lower = name.lower()
    return any(lower.endswith(s) for s in IMAGE_SUFFIXES)


def game_root_of(image, game=None):
    """game/ directory that owns this image (version folder parent, or the image's directory)."""
    if game:
        return os.path.abspath(game)
    image = os.path.abspath(image)
    parent = os.path.dirname(image)
    if os.path.basename(image).lower() == "game.wux" or os.path.basename(image).lower() == "game.wud":
        return os.path.dirname(parent)
    return parent


def version_id_of(image, game=None):
    """Version folder id when image is game/<id>/game.wux; else None."""
    image = os.path.abspath(image)
    base = os.path.basename(image).lower()
    if base not in ("game.wux", "game.wud"):
        return None
    game = game_root_of(image, game)
    parent = os.path.dirname(image)
    if os.path.dirname(parent) != game:
        return None
    return os.path.basename(parent)


def list_loose_images(game):
    """Disc images sitting directly in game/ (not inside a version folder)."""
    if not os.path.isdir(game):
        return []
    found = []
    for name in sorted(os.listdir(game)):
        path = os.path.join(game, name)
        if os.path.isfile(path) and is_image_name(name):
            found.append(path)
    return found


def sidecar_key_path(image):
    return os.path.splitext(image)[0] + ".key"


def dump_key_path(game, version_id):
    return os.path.join(game, version_id + ".key")


def _read_disc(path):
    if path and os.path.isfile(path):
        return setup.read_key_file(path, "disc"), path
    return None, None


def _read_common(path):
    if path and os.path.isfile(path):
        return setup.read_key_file(path, "common"), path
    return None, None


def find_disc_key(image, game=None):
    """(key_bytes, source_path) for this disc image, or (None, None)."""
    image = os.path.abspath(image)
    game = game_root_of(image, game)

    key, src = _read_disc(sidecar_key_path(image))
    if key:
        return key, src

    version_id = version_id_of(image, game)
    if version_id:
        key, src = _read_disc(dump_key_path(game, version_id))
        if key:
            return key, src
        key, src = _read_disc(os.path.join(os.path.dirname(image), "game.key"))
        if key:
            return key, src

    key, src = _read_disc(os.path.join(game, "keys.txt"))
    if key:
        return key, src

    key, src = _read_disc(os.path.join(game, "game.key"))
    if key:
        return key, src

    return None, None


def find_common_key(image=None, game=None):
    """(key_bytes, source_label) for the Wii U common key, or (None, None)."""
    if image:
        game = game_root_of(image, game)
    elif game:
        game = os.path.abspath(game)
    else:
        game = None

    env = os.environ.get("WIIU_COMMON_KEY")
    if env:
        key = setup.parse_key(env)
        if key:
            return key, "WIIU_COMMON_KEY"

    search = []
    if game:
        search.extend([os.path.join(game, "common.key"), os.path.join(game, "keys.txt")])
    if image:
        search.append(os.path.join(os.path.dirname(os.path.abspath(image)), "common.key"))
        if game and os.path.dirname(os.path.abspath(image)) != game:
            pass
    search.append(os.path.join(os.getcwd(), "common.key"))

    seen = set()
    for path in search:
        path = os.path.abspath(path)
        if path in seen:
            continue
        seen.add(path)
        key, src = _read_common(path)
        if key:
            return key, src

    if game:
        key, src = _read_common(os.path.join(game, "keys.txt"))
        if key:
            return key, src

    return None, None


def keys_ready(image, game=None):
    """True when both disc and common keys resolve for this image."""
    disc, _ = find_disc_key(image, game)
    common, _ = find_common_key(image, game)
    return bool(disc and common)


def import_loose_image(game, image_path, version_id):
    """Move a dropped game/*.wux into game/<id>/game.wux and its sidecar key to game/<id>.key.

    Creates the version folder. Returns the new image path.
    """
    import game_versions as gv

    image_path = os.path.abspath(image_path)
    game = os.path.abspath(game)
    if not os.path.isfile(image_path):
        raise gv.GameVersionsError("missing image: %s" % image_path)
    if os.path.dirname(image_path) != game:
        raise gv.GameVersionsError("image must sit directly in game/: %s" % image_path)
    if not is_image_name(image_path):
        raise gv.GameVersionsError("not a .wux/.wud image: %s" % image_path)

    folder = gv.create_dump(game, version_id)
    ext = os.path.splitext(image_path)[1].lower()
    dest_image = os.path.join(folder, "game" + ext)

    side_src = sidecar_key_path(image_path)
    side_dst = dump_key_path(game, version_id)
    os.rename(image_path, dest_image)
    if os.path.isfile(side_src):
        if os.path.lexists(side_dst):
            os.remove(side_dst)
        os.rename(side_src, side_dst)

    return dest_image
