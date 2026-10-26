import org.nsmbrecomp.nsmbu.SetupKeyRecoveryPolicy;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;

public final class SetupKeyRecoveryPolicyTest {
    private static void check(boolean value, String message) {
        if (!value) throw new AssertionError(message);
    }
    private static void keys(String[] actual, String... expected) {
        check(Arrays.equals(actual, expected), "Wrong affected key roles");
    }
    private static final class Descriptor implements AutoCloseable {
        boolean closed;
        @Override public void close() { closed = true; }
    }
    private static boolean multi(String text) throws IOException {
        return SetupKeyRecoveryPolicy.isMultiKeyDocument(new ByteArrayInputStream(text.getBytes(StandardCharsets.UTF_8)));
    }
    public static void main(String[] args) throws Exception {
        for (String key : new String[] {"disc", "common"}) {
            String other = SetupKeyRecoveryPolicy.other(key);
            keys(SetupKeyRecoveryPolicy.lostKeys(key, "shared-document", "shared-document"), key, other);
            keys(SetupKeyRecoveryPolicy.lostKeys(key, "disc-document", "common-document"), key);
            keys(SetupKeyRecoveryPolicy.lostKeys(key, "", ""), key);
            keys(SetupKeyRecoveryPolicy.lostKeys(key, null, null), key);
            keys(SetupKeyRecoveryPolicy.replacementKeys(key, true, "lost", "lost", true, false), key, other);
            keys(SetupKeyRecoveryPolicy.replacementKeys(key, true, "lost", "other-lost", true, true), key, other);
            keys(SetupKeyRecoveryPolicy.replacementKeys(key, true, "healthy", "other-lost", false, true), key, other);
            keys(SetupKeyRecoveryPolicy.replacementKeys(key, true, "lost", "healthy", true, false), key);
            keys(SetupKeyRecoveryPolicy.replacementKeys(key, true, "healthy", "healthy", false, false), key);
            keys(SetupKeyRecoveryPolicy.replacementKeys(key, false, "lost", "lost", true, true), key);
            for (String absent : new String[] {null, ""}) {
                keys(SetupKeyRecoveryPolicy.replacementKeys(key, false, "", absent, true, true), key);
                keys(SetupKeyRecoveryPolicy.replacementKeys(key, false, "first-selection", absent, false, true), key);
                keys(SetupKeyRecoveryPolicy.replacementKeys(key, true, "", absent, true, true), key, other);
            }
        }
        List<String> attempts = new ArrayList<>();
        IOException discFailure = new IOException("authored inaccessible disc document");
        try {
            SetupKeyRecoveryPolicy.<Descriptor>openBoth(key -> {
                attempts.add(key);
                throw key.equals("disc") ? discFailure : new IOException("authored inaccessible common document");
            });
            throw new AssertionError("Access failure was ignored");
        } catch (IOException failure) {
            check(failure == discFailure, "First actionable error was lost");
        }
        check(attempts.equals(Arrays.asList("disc", "common")), "Stopped at the first key failure");
        for (String failed : new String[] {"disc", "common"}) {
            Descriptor successful = new Descriptor();
            attempts.clear();
            try {
                SetupKeyRecoveryPolicy.openBoth(key -> {
                    attempts.add(key);
                    if (key.equals(failed)) throw new IOException("authored unavailable document");
                    return successful;
                });
                throw new AssertionError("Partial success was accepted");
            } catch (IOException expected) { }
            check(attempts.size() == 2 && successful.closed, "Failed key check leaked its other descriptor");
        }
        Map<String, Descriptor> opened = SetupKeyRecoveryPolicy.openBoth(key -> new Descriptor());
        check(opened.size() == 2, "Successful key pair was lost");
        for (Descriptor descriptor : opened.values()) {
            check(!descriptor.closed, "Successful descriptor closed before use");
            descriptor.close();
        }
        String one = "01010101010101010101010101010101", two = "02020202020202020202020202020202";
        check(multi("\uFEFF# Cemu\r\njunk\r\ncommon = " + one + "\r\ndisc = " + two + "\r\n"), "Cemu keys.txt not recognized");
        check(multi("\"Wii U common key\": \"" + one + "\"\nDisc_Key=0x" + two), "Labels and quotes not recognized");
        check(!multi(one + "\n" + two), "Unlabelled keys replaced both roles");
        check(!multi("common = " + one + "\ncommon key = " + two), "One role replaced both roles");
        check(!multi("disc = " + one + " # comment\ncommon = " + two), "Trailing comment counted as a key line");
        for (String key : new String[] {"disc", "common"}) {
            keys(SetupKeyRecoveryPolicy.replacementKeys(key, multi("disc = " + one), "", "", true, true), key);
            keys(SetupKeyRecoveryPolicy.replacementKeys(key, multi("disc = " + one + "\ncommon = " + two), "", "", true, true),
                key, SetupKeyRecoveryPolicy.other(key));
            keys(SetupKeyRecoveryPolicy.replacementKeys(key,
                SetupKeyRecoveryPolicy.isMultiKeyDocument(new ByteArrayInputStream(new byte[16])),
                "", "", true, true), key);
        }
        check(!multi("disc = " + one + "0\ncommon = " + two), "Malformed entry counted as a key");
        check(!multi(""), "Empty file counted as multi-key");
        check(!SetupKeyRecoveryPolicy.isMultiKeyDocument(new ByteArrayInputStream(new byte[16])), "Raw key counted as multi-key");
        byte[] oversized = ("disc = " + one + "\ncommon = " + two + "\n" + " ".repeat(4096)).getBytes(StandardCharsets.UTF_8);
        check(!SetupKeyRecoveryPolicy.isMultiKeyDocument(new ByteArrayInputStream(oversized)), "Unbounded key file accepted");
        System.out.println("Shared key access, descriptor cleanup and multi-key reselection checks passed");
    }
}
