"""Compile and run the plain-Java checks of Android app classes that use no Android types."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
APP = ROOT / "android/app/src/main/java/org/nsmbrecomp/nsmbu"
TESTS = ROOT / "tools/android"
SUITES = {
    "org.nsmbrecomp.nsmbu.ResourceVerifierTest": ["ResourceVerifier.java"],
    "org.nsmbrecomp.nsmbu.ResourceCacheCleanupTest": ["ResourceVerifier.java", "ResourceCacheCleanup.java"],
}


class JvmTests(unittest.TestCase):
    def run_suite(self, main, sources):
        if not shutil.which("javac") or not shutil.which("java"):
            self.skipTest("JDK required")
        test = TESTS / (main.rsplit(".", 1)[-1] + ".java")
        with tempfile.TemporaryDirectory() as classes:
            subprocess.run(["javac", "-d", classes] + [str(APP / name) for name in sources] + [str(test)],
                           check=True, capture_output=True, text=True)
            result = subprocess.run(["java", "-cp", classes, main], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_suites(self):
        for main, sources in SUITES.items():
            with self.subTest(main=main):
                self.run_suite(main, sources)


if __name__ == "__main__":
    unittest.main()
