import net from "node:net";

export function canConnectSocket(path: string, timeoutMs = 500): Promise<boolean> {
    return new Promise((resolve) => {
        const socket = net.createConnection(path);
        const done = (ok: boolean) => {
            socket.removeAllListeners();
            try {
                socket.destroy();
            } catch {
                /* */
            }
            resolve(ok);
        };
        const timer = setTimeout(() => done(false), timeoutMs);
        socket.once("connect", () => {
            clearTimeout(timer);
            done(true);
        });
        socket.once("error", () => {
            clearTimeout(timer);
            done(false);
        });
    });
}

export function waitForSocket(path: string, timeoutMs: number): Promise<void> {
    const start = Date.now();
    return new Promise((resolve, reject) => {
        const tick = () => {
            canConnectSocket(path, 300).then((ok) => {
                if (ok) {
                    resolve();
                    return;
                }
                if (Date.now() - start >= timeoutMs) {
                    reject(new Error(`timed out waiting for agent socket (${timeoutMs}ms)`));
                    return;
                }
                setTimeout(tick, 200);
            });
        };
        tick();
    });
}
