"""compare.py on synthetic run folders, no game files."""
import io
import os
import tempfile
import unittest
import unittest.mock
from contextlib import redirect_stdout

import compare


def status(step, scene="CourseTask", x=1.0, y=2.0, z=0.0, pack="1-1"):
    return "%d %s 0 1 %s %.3f %.3f %.3f" % (step, scene, pack, x, y, z)


def make_run(root, name, lines, marker=None):
    run = os.path.join(root, name)
    os.makedirs(run)
    with open(os.path.join(run, "status.txt"), "w") as f:
        f.write("\n".join(lines) + "\n")
    with open(os.path.join(run, "log"), "w") as f:
        f.write("[boot] start\n")
        if marker is not None:
            f.write("[test] align at logic step %d scene=CourseTask\n" % marker)
    return run


def course(first, count, offset=0.0):
    return [status(first + i, x=1.0 + 0.5 * i + offset) for i in range(count)]


class CompareTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = self.tmp.name

    def tearDown(self):
        self.tmp.cleanup()

    def test_same_trace_with_shifted_boot_is_identical(self):
        boot_a = [status(s, scene="Title", pack="-") for s in range(1, 40)]
        boot_b = [status(s, scene="Title", pack="-") for s in range(1, 90)]
        a = make_run(self.root, "a", boot_a + course(40, 30), marker=40)
        b = make_run(self.root, "b", boot_b + course(90, 30), marker=90)
        code, report = compare.compare(a, b, "CourseTask")
        self.assertEqual(code, 0, report)
        self.assertIn("30 steps, identical", report[1])

    def test_first_differing_step_is_rebased(self):
        a_lines, b_lines = course(40, 30), course(90, 30)
        b_lines[7] = status(97, x=99.0)
        a = make_run(self.root, "a", a_lines, marker=40)
        b = make_run(self.root, "b", b_lines, marker=90)
        code, report = compare.compare(a, b, "CourseTask")
        self.assertEqual(code, 1)
        self.assertIn("first at step 7", report[1])
        self.assertIn("differs in: x", "\n".join(report))

    def test_marker_missing_falls_back_to_status_lines(self):
        a = make_run(self.root, "a", [status(3, scene="Title", pack="-")] + course(10, 5))
        b = make_run(self.root, "b", [status(3, scene="Title", pack="-")] + course(20, 5))
        self.assertEqual(compare.align_step(a, "coursetask"), 10)
        self.assertEqual(compare.align_step(b, "CourseTask"), 20)
        self.assertEqual(compare.compare(a, b, "CourseTask")[0], 0)

    def test_missing_scene_is_exit_two(self):
        a = make_run(self.root, "a", course(1, 5), marker=1)
        b = make_run(self.root, "b", [status(s, scene="Title", pack="-") for s in range(1, 5)])
        code, report = compare.compare(a, b, "CourseTask")
        self.assertEqual(code, 2)
        self.assertIn("never reached", report[-1])

    def test_only_the_common_range_counts(self):
        a = make_run(self.root, "a", course(10, 50), marker=10)
        b = make_run(self.root, "b", course(30, 20), marker=30)
        self.assertEqual(compare.compare(a, b, "CourseTask")[0], 0)

    def test_steps_limit_ignores_later_difference(self):
        a_lines, b_lines = course(10, 40), course(10, 40)
        b_lines[30] = status(40, x=-5.0)
        a = make_run(self.root, "a", a_lines, marker=10)
        b = make_run(self.root, "b", b_lines, marker=10)
        self.assertEqual(compare.compare(a, b, "CourseTask")[0], 1)
        self.assertEqual(compare.compare(a, b, "CourseTask", steps=20)[0], 0)

    def test_scene_change_is_a_difference(self):
        a_lines, b_lines = course(10, 10), course(10, 10)
        b_lines[4] = status(14, scene="Map")
        a = make_run(self.root, "a", a_lines, marker=10)
        b = make_run(self.root, "b", b_lines, marker=10)
        code, report = compare.compare(a, b, "CourseTask")
        self.assertEqual(code, 1)
        self.assertIn("first at step 4", report[1])
        self.assertIn("scene", report[-1])

    def test_main_exit_status_and_environment_scene(self):
        a = make_run(self.root, "a", course(10, 5), marker=10)
        b = make_run(self.root, "b", course(10, 5), marker=10)
        with unittest.mock.patch.dict(os.environ, {"ALIGN": "CourseTask"}), redirect_stdout(io.StringIO()):
            self.assertEqual(compare.main([a, b]), 0)

    def test_partial_trailing_line_is_ignored(self):
        lines = course(10, 5) + ["15 CourseTask 0 1"]
        a = make_run(self.root, "a", lines, marker=10)
        self.assertEqual(len(compare.rebased(a, 10)), 5)


if __name__ == "__main__":
    unittest.main()
