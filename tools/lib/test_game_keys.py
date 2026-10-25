import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import game_keys
import game_versions as gv

KEY_A = "00112233445566778899aabbccddeeff"
KEY_B = "ffeeddccbbaa99887766554433221100"


class GameKeys(unittest.TestCase):
    def test_list_loose_images(self):
        with tempfile.TemporaryDirectory() as tmp:
            open(os.path.join(tmp, "My Game.wux"), "wb").close()
            open(os.path.join(tmp, "other.wud"), "wb").close()
            os.makedirs(os.path.join(tmp, "usa-base"))
            open(os.path.join(tmp, "usa-base", "game.wux"), "wb").close()
            names = [os.path.basename(p) for p in game_keys.list_loose_images(tmp)]
            self.assertEqual(names, ["My Game.wux", "other.wud"])

    def test_keys_from_game_root(self):
        with tempfile.TemporaryDirectory() as tmp:
            folder = os.path.join(tmp, "usa-base")
            os.makedirs(folder)
            image = os.path.join(folder, "game.wux")
            open(image, "wb").close()
            with open(os.path.join(tmp, "common.key"), "w") as f:
                f.write(KEY_A + "\n")
            with open(os.path.join(tmp, "usa-base.key"), "w") as f:
                f.write(KEY_B + "\n")
            disc, disc_src = game_keys.find_disc_key(image, tmp)
            common, common_src = game_keys.find_common_key(image, tmp)
            self.assertEqual(disc.hex(), KEY_B)
            self.assertTrue(disc_src.endswith("usa-base.key"))
            self.assertEqual(common.hex(), KEY_A)
            self.assertTrue(common_src.endswith("common.key"))
            self.assertTrue(game_keys.keys_ready(image, tmp))

    def test_cemu_keys_txt(self):
        with tempfile.TemporaryDirectory() as tmp:
            folder = os.path.join(tmp, "eu-base")
            os.makedirs(folder)
            image = os.path.join(folder, "game.wux")
            open(image, "wb").close()
            with open(os.path.join(tmp, "keys.txt"), "w") as f:
                f.write("common = %s\ndisc = %s\n" % (KEY_A, KEY_B))
            disc, _ = game_keys.find_disc_key(image, tmp)
            common, src = game_keys.find_common_key(image, tmp)
            self.assertEqual(disc.hex(), KEY_B)
            self.assertEqual(common.hex(), KEY_A)
            self.assertTrue(src.endswith("keys.txt"))

    def test_import_moves_and_renames(self):
        with tempfile.TemporaryDirectory() as tmp:
            image = os.path.join(tmp, "New SUPER MARIO.wux")
            side = os.path.join(tmp, "New SUPER MARIO.key")
            open(image, "wb").close()
            with open(side, "w") as f:
                f.write(KEY_B + "\n")
            dest = game_keys.import_loose_image(tmp, image, "usa-base")
            self.assertEqual(dest, os.path.join(tmp, "usa-base", "game.wux"))
            self.assertTrue(os.path.isfile(dest))
            self.assertFalse(os.path.lexists(image))
            self.assertFalse(os.path.lexists(side))
            self.assertTrue(os.path.isfile(os.path.join(tmp, "usa-base.key")))
            disc, src = game_keys.find_disc_key(dest, tmp)
            self.assertEqual(disc.hex(), KEY_B)
            self.assertTrue(src.endswith("usa-base.key"))
            self.assertTrue(os.path.isdir(os.path.join(tmp, "usa-base", "code")))
            with open(os.path.join(tmp, "common.key"), "w") as f:
                f.write(KEY_A + "\n")
            self.assertTrue(game_keys.keys_ready(dest, tmp))

    def test_legacy_sidecar_in_version_folder(self):
        with tempfile.TemporaryDirectory() as tmp:
            folder = os.path.join(tmp, "usa")
            os.makedirs(folder)
            image = os.path.join(folder, "game.wux")
            open(image, "wb").close()
            with open(os.path.join(folder, "game.key"), "w") as f:
                f.write(KEY_B + "\n")
            with open(os.path.join(tmp, "common.key"), "w") as f:
                f.write(KEY_A + "\n")
            disc, src = game_keys.find_disc_key(image, tmp)
            self.assertEqual(disc.hex(), KEY_B)
            self.assertTrue(src.endswith("game.key"))
            info = gv.describe_version(tmp, "usa")
            self.assertTrue(info["has_wux"])
            self.assertTrue(info["has_key"])


if __name__ == "__main__":
    unittest.main()
