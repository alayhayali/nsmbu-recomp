import org.nsmbrecomp.nsmbu.SetupPresentation;
public final class SetupPresentationTest {
    public static void main(String[] args) {
        if (!SetupPresentation.stageLabel("compile").equals("Compiling the game code")) throw new AssertionError();
        if (!SetupPresentation.stageLabel("setup").equals("setup")) throw new AssertionError("unknown stages pass through");
        for (String state : new String[]{"preparing", "running", "paused", "activating", "complete", "failed"}) {
            if (SetupPresentation.showUpdate(false, state)) throw new AssertionError("First install");
            if (SetupPresentation.showUpdate(true, state) != (!state.equals("complete") && !state.equals("failed")))
                throw new AssertionError(state);
        }
    }
}
