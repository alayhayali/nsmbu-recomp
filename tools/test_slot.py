import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import slot


class TakeArgs(unittest.TestCase):
    def test_slot_only(self):
        s, p, rest = slot.take_args(["1", "--trace"])
        self.assertEqual(s, 1)
        self.assertIsNone(p)
        self.assertEqual(rest, ["--trace"])

    def test_pack_only(self):
        s, p, rest = slot.take_args(["2-3"])
        self.assertIsNone(s)
        self.assertEqual(p, "2-3")
        self.assertEqual(rest, [])

    def test_take_slot_strips_pack(self):
        s, rest = slot.take_slot(["1-1", "extra"])
        self.assertIsNone(s)
        self.assertEqual(rest, ["extra"])

    def test_rejects_both(self):
        with self.assertRaises(SystemExit):
            slot.take_args(["1", "1-1"])

    def test_rejects_two_slots(self):
        with self.assertRaises(SystemExit):
            slot.take_args(["1", "2"])

    def test_course_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "1-27.szs")
            with open(path, "wb") as handle:
                handle.write(b"")
            s, p, rest = slot.take_args([path, "--trace"])
            self.assertIsNone(s)
            self.assertEqual(p, "1-27")
            self.assertEqual(rest, ["--trace"])

    def test_course_file_basename(self):
        with tempfile.TemporaryDirectory() as tmp:
            pack_dir = os.path.join(tmp, "game", "content", "Common", "course_res_pack")
            os.makedirs(pack_dir)
            with open(os.path.join(pack_dir, "1-27.szs"), "wb") as handle:
                handle.write(b"")
            old = os.getcwd()
            os.chdir(tmp)
            try:
                s, p, rest = slot.take_args(["1-27.szs"])
            finally:
                os.chdir(old)
            self.assertIsNone(s)
            self.assertEqual(p, "1-27")
            self.assertEqual(rest, [])

    def test_missing_course_file(self):
        with self.assertRaises(SystemExit):
            slot.take_args(["game/nope/1-27.szs"])

    def test_bad_course_file_name(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "nope.szs")
            with open(path, "wb") as handle:
                handle.write(b"")
            with self.assertRaises(SystemExit):
                slot.take_args([path])


class ApplyCoursePack(unittest.TestCase):
    def test_updates_mods_json(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "mods.json")
            with open(path, "w", encoding="utf-8") as handle:
                handle.write(
                    '{"format_version":1,"enabled":{},"config":{"example.intro-skip":{"save_file":"2"}}}\n'
                )
            slot.apply_course_pack(tmp, "3-4")
            with open(path, encoding="utf-8") as handle:
                data = __import__("json").load(handle)
            self.assertTrue(data["enabled"]["example.intro-skip"])
            self.assertEqual(data["config"]["example.intro-skip"]["course_pack"], "3-4")
            self.assertEqual(data["config"]["example.intro-skip"]["save_file"], "2")


if __name__ == "__main__":
    unittest.main(verbosity=2)
