#!/usr/bin/env python3
import io
import json
import os
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "android"))
import apk


class JavaPick(unittest.TestCase):
    def test_java_major_from_version_text(self):
        self.assertEqual(apk.java_major_from_version_text('openjdk version "25.0.1"'), 25)
        self.assertEqual(apk.java_major_from_version_text('openjdk version "1.8.0_412"'), 8)
        self.assertEqual(apk.java_major_from_version_text('openjdk version "21.0.6"'), 21)

    def test_pick_gradle_java_home_prefers_21_over_default_25(self):
        with patch.dict(os.environ, {}, clear=True):
            with patch.object(apk, "jdk_candidates", return_value=[("/usr/lib/jvm/java-21", 21)]):
                self.assertEqual(apk.pick_gradle_java_home(), "/usr/lib/jvm/java-21")


class ListDevices(unittest.TestCase):
    def test_parse_devices(self):
        stdout = b"List of devices attached\nemulator-5554\tdevice product:sdk model:sdk_gphone64_arm64\n"
        with patch.object(apk, "run", return_value=type("R", (), {"stdout": stdout, "returncode": 0})()):
            out = apk.list_devices()
        self.assertEqual(out, [{"serial": "emulator-5554", "model": "sdk_gphone64_arm64"}])


class ResolveGen(unittest.TestCase):
    def test_stub(self):
        self.assertEqual(apk.resolve_gen(True), "stub")

    def test_missing_code(self):
        with tempfile.TemporaryDirectory() as tmp:
            with patch.object(apk, "ROOT", tmp):
                os.makedirs(os.path.join(tmp, "build", "gen"), exist_ok=True)
                with self.assertRaises(apk.ApkError):
                    apk.resolve_gen(False)

    def test_has_code(self):
        with tempfile.TemporaryDirectory() as tmp:
            gen = os.path.join(tmp, "build", "gen")
            os.makedirs(gen)
            open(os.path.join(gen, "code_00000000.c"), "w").close()
            with patch.object(apk, "ROOT", tmp):
                self.assertEqual(apk.resolve_gen(False), gen)


class MainFlow(unittest.TestCase):
    def test_list_devices_json(self):
        buf = io.StringIO()
        with patch.object(apk, "list_devices", return_value=[{"serial": "abc", "model": None}]):
            with patch("sys.stdout", buf):
                self.assertEqual(apk.main(["--list-devices"]), 0)
        self.assertEqual(json.loads(buf.getvalue()), [{"serial": "abc", "model": None}])


class PushSync(unittest.TestCase):
    def test_local_sync_payload(self):
        with tempfile.TemporaryDirectory() as tmp:
            dump = os.path.join(tmp, "usa-1.3.0")
            os.makedirs(os.path.join(dump, "code"))
            rpx = os.path.join(dump, "code", "red-pro2.rpx")
            with open(rpx, "wb") as f:
                f.write(b"x" * 42)
            self.assertEqual(
                apk.local_sync_payload(dump),
                {"id": "usa-1.3.0", "rpx_size": 42},
            )

    def test_game_push_up_to_date_marker(self):
        payload = {"id": "usa-1.3.0", "rpx_size": 42}
        with patch.object(apk, "read_remote_sync", return_value=payload):
            with patch.object(apk, "local_sync_payload", return_value=payload):
                self.assertTrue(apk.game_push_up_to_date("dev", "/game/usa-1.3.0"))

    def test_game_push_up_to_date_legacy_rpx(self):
        payload = {"id": "usa-1.3.0", "rpx_size": 42}
        with patch.object(apk, "read_remote_sync", return_value=None):
            with patch.object(apk, "local_sync_payload", return_value=payload):
                with patch.object(apk, "remote_rpx_size", return_value=42):
                    with patch.object(apk, "write_remote_sync") as write:
                        self.assertTrue(apk.game_push_up_to_date("dev", "/game/usa-1.3.0"))
                        write.assert_called_once()

    def test_push_game_skips_when_up_to_date(self):
        with patch.object(apk, "game_push_up_to_date", return_value=True):
            with patch.object(apk, "run") as run:
                self.assertFalse(apk.push_game("dev", "/game/usa-1.3.0"))
                run.assert_not_called()

    def test_push_game_force_runs(self):
        with patch.object(apk, "game_push_up_to_date", return_value=True):
            with patch.object(apk, "write_remote_sync"):
                with patch.object(apk, "run") as run:
                    self.assertTrue(apk.push_game("dev", "/game/usa-1.3.0", force=True))
                    self.assertTrue(run.called)


if __name__ == "__main__":
    unittest.main()
