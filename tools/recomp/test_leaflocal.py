#!/usr/bin/env python3
"""Unit tests for non-leaf register locals: python3 test_leaflocal.py
"""
import os
import re
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import leaflocal

_SWITCHES = (
    "NSMBU_RECOMP_NONLEAF_LIVE",
    "NSMBU_RECOMP_NONLEAF_ABI",
    "NSMBU_RECOMP_PLAIN",
    "NSMBU_RECOMP_LEAF_POISON",
)


def gpr_reloads(text):
    return {int(m.group(1)) for m in re.finditer(r"(?<![\w])r(\d+) = c->r\[\1\];", text)}


def fpr_reloads(text):
    return {int(m.group(1)) for m in re.finditer(
        r"(?<![\w])f(\d+)_0 = c->f\[\1\]\.ps0; f\1_1 = c->f\[\1\]\.ps1;", text)}


class Nonleaf(unittest.TestCase):
    def setUp(self):
        self._env = dict(os.environ)
        for key in _SWITCHES:
            os.environ.pop(key, None)

    def tearDown(self):
        os.environ.clear()
        os.environ.update(self._env)

    def nonleaf(self, stmts, addrs=None):
        if addrs is None:
            addrs = [0x02000000 + 4 * i for i in range(len(stmts))]
        done = leaflocal.transform_nonleaf(stmts, addrs)
        self.assertIsNotNone(done)
        return done

    def test_straight_line_skips_overwritten_registers(self):
        _, out, _ = self.nonleaf([
            "c->r[3] = 1;",
            "c->r[4] = 2;",
            "f_02000000(c);",
            "c->r[3] = 3;",
            "c->r[6] = c->r[3];",
        ])
        self.assertEqual(gpr_reloads(out[2]), set())

    def test_use_after_call_in_the_same_statement(self):
        _, out, _ = self.nonleaf(["site_02000000(c); c->r[4] = c->r[3];"])
        self.assertEqual(gpr_reloads(out[0]), {3})

    def test_loop_reloads_the_branch_register(self):
        _, out, _ = self.nonleaf([
            "c->r[3] = 1;",
            "f_02000000(c);",
            "if (c->r[4]) goto L_02000000;",
        ], [0x02000000, 0x02000004, 0x02000008])
        self.assertEqual(gpr_reloads(out[1]), {4})

    def test_two_calls_fallthrough(self):
        _, out, _ = self.nonleaf([
            "f_02000000(c); goto L_0200000C;",
            "c->r[6] = c->r[5];",
            "f_02000004(c);",
            "c->r[6] = c->r[7];",
        ], [0x02000000, 0x02000004, 0x02000008, 0x0200000C])
        self.assertEqual(gpr_reloads(out[0]), {5, 7})
        self.assertEqual(gpr_reloads(out[2]), {7})

    def test_branch_reloads_the_register_read_after_the_call(self):
        _, out, _ = self.nonleaf([
            "if (c->r[3]) goto L_0200000C;",
            "f_02000000(c);",
            "c->r[6] = c->r[5];",
            "return;",
        ], [0x02000000, 0x02000004, 0x02000008, 0x0200000C])
        self.assertEqual(gpr_reloads(out[1]), {5})

    def test_abi_skips_r0_write_back_at_the_call(self):
        prologue, out, tail = self.nonleaf(
            ["c->r[0] = 1; c->r[3] = 2; f_02000000(c); return;"])
        text = "\n".join(prologue + out + [tail])
        self.assertIn("c->r[3] = r3;", text)
        self.assertNotIn("c->r[0] = r0", text)

    def test_live_r0_stays_in_its_local(self):
        prologue, out, tail = self.nonleaf(
            ["c->r[0] = c->r[0] + 1; f_02000000(c); c->r[4] = c->r[0];"])
        body = "\n".join(out)
        self.assertNotIn("r0 = c->r[0];", body)
        self.assertNotIn("c->r[0] = r0", "\n".join(prologue + out + [tail]))

    def test_poison_fills_a_dead_local_after_the_call(self):
        os.environ["NSMBU_RECOMP_LEAF_POISON"] = "1"
        _, out, _ = self.nonleaf([
            "c->r[0] = 1; c->r[3] = 2; c->r[4] = 3; f_02000000(c); c->r[5] = c->r[4] + c->r[0];",
        ])
        text = out[0]
        self.assertIn("r3 = 0xDEADBEEFu;", text)
        self.assertNotIn("r4 = 0xDEADBEEFu;", text)
        self.assertNotIn("r0 = 0xDEADBEEFu;", text)
        self.assertIn("r4 = c->r[4];", text)
        self.assertNotIn("r0 = c->r[0];", text)

    def test_live_switch_and_plain(self):
        stmts = ["c->r[3] = 1; f_02000000(c); c->r[3] = 2;"]
        os.environ["NSMBU_RECOMP_NONLEAF_LIVE"] = "0"
        _, out, _ = self.nonleaf(stmts)
        self.assertEqual(gpr_reloads(out[0]), {3})
        os.environ.pop("NSMBU_RECOMP_NONLEAF_LIVE")
        os.environ["NSMBU_RECOMP_PLAIN"] = "1"
        _, out, _ = self.nonleaf(stmts)
        self.assertEqual(gpr_reloads(out[0]), {3})
        os.environ["NSMBU_RECOMP_NONLEAF_LIVE"] = "1"
        _, out, _ = self.nonleaf(stmts)
        self.assertEqual(gpr_reloads(out[0]), set())

    def test_fpr_read_reloads_both_halves(self):
        _, out, _ = self.nonleaf(["f_02000000(c); c->r[3] = c->f[1].ps0;"])
        self.assertEqual(fpr_reloads(out[0]), {1})
        self.assertNotIn("r3 = c->r[3];", out[0])
