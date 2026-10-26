import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
APP = ROOT / "android/app/src/main"
JAVA = APP / "java/org/nsmbrecomp/nsmbu"


def text(path):
    return (ROOT / path).read_text()


class PhoneWiring(unittest.TestCase):
    def test_heat_guard_jni_matches_the_java_class(self):
        native = text("runtime/src/platform/perf_hint.cpp")
        self.assertIn("Java_org_nsmbrecomp_nsmbu_HeatGuardDevice_nativeSetDeviceClass", native)
        self.assertIn("private static native void nativeSetDeviceClass(int deviceClass);", text(JAVA / "HeatGuardDevice.java"))
        self.assertIn('ENV(HEAT_GUARD, "NSMBU_HEAT_GUARD")', text("runtime/src/core/env.h"))

    def test_shared_logs_are_the_ones_the_runtime_writes(self):
        main = text("runtime/src/main.cpp")
        for name in ("nsmbu.log", "nsmbu-previous.log"):
            self.assertIn(name, main)
            self.assertIn('"%s"' % name, text(JAVA / "LogShare.java"))
        self.assertIn('new File(AndroidUserDirectories.config(root), "logs")', text(JAVA / "LogShare.java"))
        self.assertIn('return config_dir() + "/logs";', text("runtime/src/platform/host.h"))
        self.assertNotRegex(main, r"#ifdef __ANDROID__\s*\n\s*if \(!e \|\| !\*e\) return;")

    def test_back_opens_settings(self):
        self.assertIn("SDL_SCANCODE_AC_BACK", text("runtime/src/platform/input_sdl.cpp"))
        self.assertIn("SDL_HINT_ANDROID_TRAP_BACK_BUTTON", text("runtime/src/gfx/vulkan/backend.cpp"))

    def test_files_provider_is_system_only_and_protects_the_game(self):
        manifest = text(APP / "AndroidManifest.xml")
        provider = re.search(r'<provider android:name="\.GameFilesProvider".*?</provider>', manifest, re.S)
        self.assertIsNotNone(provider)
        self.assertIn('android:permission="android.permission.MANAGE_DOCUMENTS"', provider.group(0))
        self.assertIn('{"game", "config/nsmbu/shadercache"}', text(JAVA / "GameFilesProvider.java"))
        self.assertIn('@xml/game_mode_config', manifest)
        config = text(APP / "res/xml/game_mode_config.xml")
        self.assertIn('android:allowGameDownscaling="false"', config)
        self.assertIn('android:allowGameFpsOverride="false"', config)


if __name__ == "__main__":
    unittest.main()
