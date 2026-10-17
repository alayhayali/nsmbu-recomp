const BUTTON_BITS: Record<string, number> = {
    A: 0x8000,
    B: 0x4000,
    X: 0x2000,
    Y: 0x1000,
    L: 0x0020,
    R: 0x0010,
    ZL: 0x0080,
    ZR: 0x0040,
    Plus: 0x0008,
    Minus: 0x0004,
    Home: 0x0002,
    Up: 0x0200,
    Down: 0x0100,
    Left: 0x0800,
    Right: 0x0400,
};

export function parseButtonBits(token: string): number {
    const named = BUTTON_BITS[token];
    if (named !== undefined) {
        return named;
    }
    const t = token.startsWith("0x") ? token.slice(2) : token;
    const n = Number.parseInt(t, 16);
    if (!Number.isFinite(n) || n < 0) {
        throw new Error(`unknown button: ${token}`);
    }
    return n;
}
