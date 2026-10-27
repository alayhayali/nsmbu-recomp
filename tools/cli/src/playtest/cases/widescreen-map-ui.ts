import { inflateSync } from "node:zlib";
import { readFileSync } from "node:fs";
import { join } from "node:path";
import { reachWorldMap, type PlayCase } from "../helpers.ts";

export function readPng(path: string): { w: number; h: number; rgb: Uint8Array } {
    const file = readFileSync(path);
    if (file.length < 8 || file[0] !== 137) throw new Error(`not a png: ${path}`);
    let w = 0;
    let h = 0;
    let channels = 3;
    let off = 8;
    const idat: Buffer[] = [];
    while (off + 12 <= file.length) {
        const len = file.readUInt32BE(off);
        const type = file.toString("ascii", off + 4, off + 8);
        const data = file.subarray(off + 8, off + 8 + len);
        if (type === "IHDR") {
            w = data.readUInt32BE(0);
            h = data.readUInt32BE(4);
            const depth = data[8];
            const color = data[9];
            if (depth !== 8 || (color !== 2 && color !== 6) || data[12] !== 0) {
                throw new Error(`unsupported png depth ${depth} color ${color}`);
            }
            channels = color === 6 ? 4 : 3;
        } else if (type === "IDAT") {
            idat.push(Buffer.from(data));
        } else if (type === "IEND") {
            break;
        }
        off += 12 + len;
    }
    if (!w || !h || idat.length === 0) throw new Error(`empty png: ${path}`);
    const raw = inflateSync(Buffer.concat(idat));
    const srcStride = w * channels;
    const stride = w * 3;
    const rgb = new Uint8Array(stride * h);
    let prev = new Uint8Array(srcStride);
    const src = new Uint8Array(srcStride);
    let p = 0;
    for (let y = 0; y < h; y++) {
        const filter = raw[p++];
        for (let i = 0; i < srcStride; i++) {
            const v = raw[p++];
            const a = i >= channels ? src[i - channels] : 0;
            const b = prev[i];
            const c = i >= channels ? prev[i - channels] : 0;
            let pr = 0;
            if (filter === 1) pr = a;
            else if (filter === 2) pr = b;
            else if (filter === 3) pr = (a + b) >> 1;
            else if (filter === 4) {
                const guess = a + b - c;
                const pa = Math.abs(guess - a);
                const pb = Math.abs(guess - b);
                const pc = Math.abs(guess - c);
                pr = pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
            } else if (filter !== 0) {
                throw new Error(`png filter ${filter}`);
            }
            src[i] = (v + pr) & 255;
        }
        const row = rgb.subarray(y * stride, (y + 1) * stride);
        for (let x = 0; x < w; x++) {
            row[x * 3] = src[x * channels];
            row[x * 3 + 1] = src[x * channels + 1];
            row[x * 3 + 2] = src[x * channels + 2];
        }
        prev = src.slice();
    }
    return { w, h, rgb };
}

function blue(rgb: Uint8Array, stride: number, x: number, y: number): boolean {
    const i = y * stride + x * 3;
    const r = rgb[i];
    const g = rgb[i + 1];
    const b = rgb[i + 2];
    return b > 150 && r < 90 && g > 70 && g < 170 && b > r + 60 && b > g + 20;
}

function buttonEdges(img: { w: number; h: number; rgb: Uint8Array }): { left: number; right: number } | null {
    const stride = img.w * 3;
    const y0 = Math.floor(img.h * 0.7);
    const y1 = Math.floor(img.h * 0.98);
    let left = img.w;
    let right = 0;
    for (let y = y0; y < y1; y += 2) {
        let x = 0;
        while (x < img.w) {
            if (!blue(img.rgb, stride, x, y)) {
                x += 2;
                continue;
            }
            const x0 = x;
            while (x < img.w && blue(img.rgb, stride, x, y)) x += 2;
            if (x - x0 < 36) continue;
            if (x0 < img.w / 2) left = Math.min(left, x0);
            if (x > img.w / 2) right = Math.max(right, x);
        }
    }
    if (left === img.w || right === 0) return null;
    return { left, right };
}

export const widescreenMapUi: PlayCase = {
    name: "widescreen-map-ui",
    summary: "21:9 world map keeps Map View and Items on the screen edges.",
    run: async (session) => {
        await session.start({ save: "repo", env: { NSMBU_ASPECT: "21:9" } });
        await reachWorldMap(session, 180);
        const path = join(session.workDir, "map.png");
        let edges: { left: number; right: number } | null = null;
        let img = { w: 0, h: 0, rgb: new Uint8Array() };
        for (let i = 0; i < 12; i++) {
            await session.advance(60, 20);
            await session.screenshot(path);
            const ready = Date.now() + 5000;
            for (;;) {
                try {
                    img = readPng(path);
                    break;
                } catch (err) {
                    if (Date.now() > ready) throw err;
                    await new Promise((r) => setTimeout(r, 50));
                }
            }
            edges = buttonEdges(img);
            if (edges) break;
        }
        if (!edges) throw new Error(`map buttons not found in ${path}`);
        const limit = img.w * 0.12;
        if (edges.left > limit || img.w - edges.right > limit) {
            throw new Error(
                `map buttons inset at 21:9: left ${edges.left}px right ${img.w - edges.right}px of ${img.w} (limit ${Math.round(limit)})`,
            );
        }
    },
};
