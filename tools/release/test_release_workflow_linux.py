import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
WORKFLOW = (ROOT / ".github/workflows/release.yml").read_text()


def job(name):
    match = re.search(r"^  %s:\n(.*?)(?=^  [a-z][a-z0-9_-]*:\n|\Z)" % re.escape(name), WORKFLOW, re.S | re.M)
    if not match:
        raise AssertionError("no %s job in release.yml" % name)
    return match.group(1)


class ReleaseWorkflowLinux(unittest.TestCase):
    def test_linux_package_uses_sdl3_license_beside_cached_install(self):
        linux = job("linux")
        self.assertIn('SDL3 license for release packaging', linux)
        self.assertIn('"$HOME/sdl3/LICENSE.txt"', linux)
        self.assertNotIn("$PWD/SDL3-3.4.18/LICENSE.txt", linux)


if __name__ == "__main__":
    unittest.main()
