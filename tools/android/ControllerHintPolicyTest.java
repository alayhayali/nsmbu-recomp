package org.nsmbrecomp.nsmbu;

public final class ControllerHintPolicyTest {
    private static void check(boolean ok, String what) { if (!ok) throw new AssertionError(what); }

    public static void main(String[] args) {
        ControllerHintPolicy fresh = new ControllerHintPolicy(false);
        check(!fresh.claim(false), "no controller: no hint");
        check(fresh.claim(true), "the first controller shows the hint");
        check(!fresh.claim(true), "only once");
        check(!new ControllerHintPolicy(true).claim(true), "a hint shown before stays hidden");
    }
}
