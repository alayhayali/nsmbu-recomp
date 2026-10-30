package org.nsmbrecomp.nsmbu;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/** The chosen-folder check: the game folder, its parent, a folder inside it, several or no games. */
public final class SetupFolderCheckTest {
    /** A folder tree by path; a path ending in "/" is a folder. */
    private static final class Tree implements SetupFolderCheck.Lister {
        final Map<String, List<SetupFolderCheck.Entry>> children = new HashMap<>();
        int listed;
        Tree(String... paths) {
            children.put("", new ArrayList<>());
            for (String path : paths) add(path);
        }
        private void add(String path) {
            boolean directory = path.endsWith("/");
            String clean = directory ? path.substring(0, path.length() - 1) : path;
            int slash = clean.lastIndexOf('/');
            String parent = slash < 0 ? "" : clean.substring(0, slash);
            if (!children.containsKey(parent)) add(parent + "/");
            for (SetupFolderCheck.Entry entry : children.get(parent)) if (entry.id.equals(clean)) return;
            children.get(parent).add(new SetupFolderCheck.Entry(clean.substring(slash + 1), clean, directory));
            if (directory) children.putIfAbsent(clean, new ArrayList<>());
        }
        @Override public List<SetupFolderCheck.Entry> list(String id) {
            listed++;
            List<SetupFolderCheck.Entry> result = children.get(id);
            if (result == null) throw new AssertionError("Listed a file or missing folder: " + id);
            return result;
        }
    }

    private static final String[] GAME = {"code/red-pro2.rpx", "code/preload.txt", "content/Common/x.szs", "meta/meta.xml", "meta/iconTex.tga"};

    private static String[] game(String prefix) {
        String[] result = new String[GAME.length];
        for (int i = 0; i < GAME.length; i++) result[i] = prefix + GAME[i];
        return result;
    }

    private static String[] join(String[]... parts) {
        List<String> result = new ArrayList<>();
        for (String[] part : parts) for (String path : part) result.add(path);
        return result.toArray(new String[0]);
    }

    private static void expect(Object expected, Object actual, String what) {
        if (!java.util.Objects.equals(expected, actual)) throw new AssertionError(what + ": expected " + expected + ", got " + actual);
    }

    private static SetupFolderCheck.Result check(Tree tree, String root, String name) throws Exception {
        return SetupFolderCheck.check(tree, root, name);
    }

    public static void main(String[] args) throws Exception {
        SetupFolderCheck.Result result = check(new Tree(game("NewSuperMarioBrosU/")), "NewSuperMarioBrosU", "NewSuperMarioBrosU");
        expect(SetupFolderCheck.Kind.GAME, result.kind, "game folder");
        expect("NewSuperMarioBrosU", result.document, "game folder document");
        expect(true, result.usable(), "game folder usable");

        Tree parent = new Tree(join(game("Games/NewSuperMarioBrosU/"), new String[] {"Games/notes.txt", "Games/Other/readme.txt"}));
        result = check(parent, "Games", "Games");
        expect(SetupFolderCheck.Kind.SUBFOLDER, result.kind, "parent folder");
        expect("Games/NewSuperMarioBrosU", result.document, "parent folder uses the game subfolder");
        expect("NewSuperMarioBrosU", result.path, "parent folder names the subfolder");
        expect(true, result.usable(), "parent folder usable");
        if (!result.message().contains("NewSuperMarioBrosU")) throw new AssertionError("Subfolder message lacks its name");

        result = check(new Tree(game("Games/NSMBU/0005000010101D00/")), "Games", "Games");
        expect(SetupFolderCheck.Kind.SUBFOLDER, result.kind, "grandparent folder");
        expect("NSMBU/0005000010101D00", result.path, "grandparent path");
        expect("Games/NSMBU/0005000010101D00", result.document, "grandparent document");

        result = check(new Tree(join(game("G/A/"), game("G/B/C/"))), "G", "G");
        expect("A", result.path, "shallowest game");

        result = check(new Tree(game("G/a/b/c/")), "G", "G");
        expect(SetupFolderCheck.Kind.NO_GAME, result.kind, "too deep");

        Tree inside = new Tree(game("NewSuperMarioBrosU/"));
        result = check(inside, "NewSuperMarioBrosU/code", "code");
        expect(SetupFolderCheck.Kind.INSIDE_GAME, result.kind, "code folder");
        expect("code", result.path, "code folder name");
        expect(null, result.document, "refused choice has no document");
        expect(false, result.usable(), "code folder refused");
        expect("meta", check(inside, "NewSuperMarioBrosU/meta", "meta").path, "meta folder");
        expect("content", check(inside, "NewSuperMarioBrosU/content", "content").path, "content folder by name");
        expect(SetupFolderCheck.Kind.INSIDE_GAME, check(new Tree("x/red-pro2.rpx"), "x", "x").kind, "renamed code folder");
        if (!check(inside, "NewSuperMarioBrosU/code", "code").message().contains("one level up"))
            throw new AssertionError("Inside-game message does not say where to go");

        result = check(new Tree("Download/photo.jpg", "Download/Music/song.mp3"), "Download", "Download");
        expect(SetupFolderCheck.Kind.NO_GAME, result.kind, "no game");
        expect(false, result.usable(), "no game refused");
        if (!result.message().contains("code, content and meta")) throw new AssertionError("No-game message lacks the folder names");
        expect(SetupFolderCheck.Kind.NO_GAME, check(new Tree("Empty/"), "Empty", "Empty").kind, "empty folder");

        result = check(new Tree("Other/code/other.rpx", "Other/content/a", "Other/meta/meta.xml"), "Other", "Other");
        expect(SetupFolderCheck.Kind.NOT_THIS_GAME, result.kind, "another title");
        expect("", result.path, "another title here");
        result = check(new Tree("G/Half/code/red-pro2.rpx", "G/Half/content/a", "G/Half/meta/"), "G", "G");
        expect(SetupFolderCheck.Kind.NOT_THIS_GAME, result.kind, "incomplete game below");
        expect("Half", result.path, "incomplete game path");
        result = check(new Tree(join(game("G/Good/"), new String[] {"G/Half/code/", "G/Half/content/", "G/Half/meta/"})), "G", "G");
        expect(SetupFolderCheck.Kind.SUBFOLDER, result.kind, "complete next to incomplete");
        expect("Good", result.path, "complete game chosen");

        result = check(new Tree(join(game("G/USA/"), game("G/EUR/"))), "G", "G");
        expect(SetupFolderCheck.Kind.SEVERAL, result.kind, "several games");
        if (!result.path.contains("USA") || !result.path.contains("EUR")) throw new AssertionError("Several-games message lacks names");

        Tree hidden = new Tree(join(game("G/.trash/old/"), new String[] {"G/code/x.txt"}));
        expect(SetupFolderCheck.Kind.NO_GAME, check(hidden, "G", "G").kind, "hidden folder ignored");

        String[] many = new String[400];
        for (int i = 0; i < many.length; i++) many[i] = "Big/f" + i + "/file";
        Tree big = new Tree(many);
        expect(SetupFolderCheck.Kind.NO_GAME, check(big, "Big", "Big").kind, "big folder");
        if (big.listed > SetupFolderCheck.MAX_FOLDERS + 1) throw new AssertionError("Listed " + big.listed + " folders");
        System.out.println("SetupFolderCheckTest passed");
    }
}
