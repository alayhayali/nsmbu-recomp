import * as p from "@clack/prompts";

function requireTty(): void {
    if (!process.stdin.isTTY || !process.stdout.isTTY) {
        throw new Error(
            "interactive prompts need a TTY; use nsmbu game <id> [--extract] [--recomp] or nsmbu game --list",
        );
    }
}

export async function selectOption(
    title: string,
    options: { value: string; label: string; hint?: string }[],
    initialValue?: string,
): Promise<string> {
    if (options.length === 0) {
        throw new Error("nothing to choose");
    }
    requireTty();

    const value = await p.select({
        message: title,
        options,
        initialValue,
    });
    if (p.isCancel(value)) {
        p.cancel("Cancelled.");
        process.exit(0);
    }
    return value;
}

export async function confirm(message: string, initialValue = true): Promise<boolean> {
    requireTty();
    const value = await p.confirm({ message, initialValue });
    if (p.isCancel(value)) {
        p.cancel("Cancelled.");
        process.exit(0);
    }
    return value;
}

export async function textInput(message: string, placeholder?: string): Promise<string> {
    requireTty();
    const value = await p.text({
        message,
        placeholder,
        validate: (v) => {
            const t = (v ?? "").trim();
            if (!t) {
                return "Enter a value";
            }
            if (!/^[A-Za-z0-9][A-Za-z0-9._-]*$/.test(t)) {
                return "Use letters, digits, ., _, or -";
            }
            return undefined;
        },
    });
    if (p.isCancel(value)) {
        p.cancel("Cancelled.");
        process.exit(0);
    }
    return value.trim();
}

export { p as clack };
