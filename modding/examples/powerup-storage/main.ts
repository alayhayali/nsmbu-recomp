const PROFILE_MODE: Record<number, number> = {
    591: 1,
    592: 2,
    593: 3,
    594: 6,
    595: 4,
    596: 5,
    598: 7,
};

const MODE_NAME: Record<number, string> = {
    0: "Small",
    1: "Super",
    2: "Fire",
    3: "Mini",
    4: "Propeller",
    5: "Penguin",
    6: "Ice",
    7: "Squirrel",
    8: "P-Squirrel",
};

const PROFILE_FOR_MODE: Record<number, number | undefined> = {
    1: 591,
    2: 592,
    3: 593,
    4: 595,
    5: 596,
    6: 594,
    7: 598,
    8: 598,
};

const COLLECTABLE_SPAWN = 1;
const SPAWN_OFFSET_Y = 48;
const PICKUP_RANGE_X = 28;
const PICKUP_RANGE_Y = 48;
const SUMMON_TIMEOUT = 240;

interface TrackedItem {
    id: number;
    mode: number;
    x: number;
    y: number;
    steps: number;
}

interface HandledItem {
    id: number;
    step: number;
}

interface PendingSummon {
    mode: number;
    x: number;
    y: number;
    step: number;
}

interface SummonedItem {
    id: number;
    player: number;
}

const reserve: Record<number, number | undefined> = {};
const prevMode: Record<number, number | undefined> = {};
const pending: Record<number, PendingSummon | undefined> = {};
let tracked: TrackedItem[] = [];
let handled: HandledItem[] = [];
let summonedItems: SummonedItem[] = [];

function clearSummonedFor(player: number): void {
    summonedItems = summonedItems.filter((entry) => entry.player !== player);
}

function isSummoned(id: number): boolean {
    for (const entry of summonedItems) {
        if (entry.id === id) {
            return true;
        }
    }
    return false;
}

function takeSummoned(id: number): number {
    for (let i = 0; i < summonedItems.length; i++) {
        if (summonedItems[i].id === id) {
            const player = summonedItems[i].player;
            summonedItems.splice(i, 1);
            return player;
        }
    }
    return -1;
}

function modeName(mode: number): string {
    return MODE_NAME[mode] ?? "Power-up";
}

function near(item: TrackedItem, x: number, y: number): number {
    const dx = item.x - x;
    const dy = item.y - y;
    const distance = dx * dx + dy * dy;
    return distance <= 72 * 72 ? distance : -1;
}

function wasHandled(id: number, step: number): boolean {
    for (const entry of handled) {
        if (entry.id === id && step - entry.step < 180) {
            return true;
        }
    }
    return false;
}

function markHandled(id: number, step: number): void {
    handled.push({ id, step });
}

function handlePickup(
    index: number,
    itemMode: number,
    players: Record<number, Player | undefined>,
    current: Record<number, number | undefined>,
): void {
    const before = prevMode[index] ?? 0;
    if (before <= 0) {
        return;
    }
    reserve[index] = itemMode;
    const currentMode = current[index];
    if (currentMode !== undefined && currentMode !== before) {
        const p = players[index];
        if (p) {
            p.set_mode(before);
        }
        current[index] = before;
    }
    nsmbu.log(`reserve store ${modeName(itemMode)}`);
    nsmbu.status(`Player ${index + 1} reserve: ${modeName(itemMode)}`);
}

function readPlayers(
    players: Record<number, Player | undefined>,
    current: Record<number, number | undefined>,
): void {
    for (let i = 0; i < 4; i++) {
        const [p] = nsmbu.Player.get_player(i);
        if (!p) {
            players[i] = undefined;
            current[i] = undefined;
            reserve[i] = undefined;
            prevMode[i] = undefined;
            pending[i] = undefined;
            clearSummonedFor(i);
            continue;
        }
        const [entered] = p.entered();
        if (!entered) {
            players[i] = undefined;
            current[i] = undefined;
            reserve[i] = undefined;
            prevMode[i] = undefined;
            pending[i] = undefined;
            clearSummonedFor(i);
            continue;
        }
        players[i] = p;
        const [mode] = p.mode();
        current[i] = mode;
    }
}

function collectItems(): TrackedItem[] {
    const now: TrackedItem[] = [];
    for (const actor of nsmbu.Level.actors()) {
        const [profile] = actor.profile_id();
        if (profile === undefined) {
            continue;
        }
        const mode = PROFILE_MODE[profile];
        if (mode === undefined) {
            continue;
        }
        const [id] = actor.id();
        const [pos] = actor.pos();
        if (id === undefined || !pos) {
            continue;
        }
        let steps = 1;
        for (const old of tracked) {
            if (old.id === id) {
                steps = old.steps + 1;
                break;
            }
        }
        now.push({ id, mode, x: pos.x, y: pos.y, steps });
    }
    return now;
}

function findNearestPlayer(
    players: Record<number, Player | undefined>,
    item: TrackedItem,
): number {
    let best = -1;
    let bestDistance = 0;
    for (let i = 0; i < 4; i++) {
        const p = players[i];
        if (!p) {
            continue;
        }
        const [pos] = p.pos();
        if (!pos) {
            continue;
        }
        const distance = near(item, pos.x, pos.y);
        if (distance < 0) {
            continue;
        }
        if (best < 0 || distance < bestDistance) {
            best = i;
            bestDistance = distance;
        }
    }
    return best;
}

function detectUpgrades(
    players: Record<number, Player | undefined>,
    current: Record<number, number | undefined>,
): void {
    for (let i = 0; i < 4; i++) {
        const before = prevMode[i] ?? 0;
        const now = current[i];
        if (now === undefined) {
            continue;
        }
        if (pending[i]) {
            if (now > before) {
                pending[i] = undefined;
                clearSummonedFor(i);
                const p = players[i];
                if (p) {
                    p.set_pickup_lock(now > 0);
                }
                nsmbu.log(`used ${modeName(now)}`);
                nsmbu.status(`Player ${i + 1} used ${modeName(now)}`);
            }
            continue;
        }
        if (now <= before || before <= 0) {
            continue;
        }
        const p = players[i];
        if (!p) {
            continue;
        }
        reserve[i] = now;
        p.set_mode(before);
        current[i] = before;
        nsmbu.log(`reserve upgrade ${modeName(now)}`);
        nsmbu.status(`Player ${i + 1} reserve: ${modeName(now)}`);
    }
}

function deployStored(
    players: Record<number, Player | undefined>,
    current: Record<number, number | undefined>,
    step: number,
): void {
    const [button] = nsmbu.Config.string("use_button", "Minus");
    const buttonName = (button ?? "Minus") as ButtonName;
    const [pressed] = nsmbu.Input.button.pressed(buttonName);
    if (pressed !== true) {
        return;
    }
    for (let i = 0; i < 4; i++) {
        const p = players[i];
        if (!p) {
            continue;
        }
        const stored = reserve[i];
        if (stored === undefined || pending[i]) {
            continue;
        }
        const profile = PROFILE_FOR_MODE[stored];
        if (profile === undefined) {
            continue;
        }
        const [pos] = p.pos();
        if (!pos) {
            continue;
        }
        const spawnY = pos.y + SPAWN_OFFSET_Y;
        const [ok] = nsmbu.Level.spawn(profile, pos.x, spawnY, pos.z, {
            param0: COLLECTABLE_SPAWN * 16777216,
        });
        if (ok === true) {
            reserve[i] = undefined;
            pending[i] = { mode: stored, x: pos.x, y: spawnY, step };
            p.set_pickup_lock(false);
            nsmbu.log(`deploy ${modeName(stored)}`);
            nsmbu.status(`Player ${i + 1} deployed ${modeName(stored)}`);
        }
    }
}

function trackSummoned(
    players: Record<number, Player | undefined>,
    current: Record<number, number | undefined>,
    now: TrackedItem[],
    step: number,
): void {
    for (let i = 0; i < 4; i++) {
        const summon = pending[i];
        if (!summon) {
            continue;
        }
        if (step - summon.step > SUMMON_TIMEOUT) {
            pending[i] = undefined;
            clearSummonedFor(i);
            const p = players[i];
            if (p) {
                p.set_pickup_lock((current[i] ?? 0) > 0);
            }
            continue;
        }
        if (step <= summon.step) {
            continue;
        }
        for (const item of now) {
            if (item.mode !== summon.mode || isSummoned(item.id)) {
                continue;
            }
            if (Math.abs(item.x - summon.x) > 48 || Math.abs(item.y - summon.y) > 128) {
                continue;
            }
            summonedItems.push({ id: item.id, player: i });
            break;
        }
    }
}

function grantSummoned(
    players: Record<number, Player | undefined>,
    current: Record<number, number | undefined>,
    now: TrackedItem[],
    step: number,
): void {
    for (let i = 0; i < 4; i++) {
        const summon = pending[i];
        if (!summon) {
            continue;
        }
        const p = players[i];
        if (!p) {
            continue;
        }
        const [pos] = p.pos();
        if (!pos) {
            continue;
        }
        for (const item of now) {
            if (item.mode !== summon.mode || !isSummoned(item.id)) {
                continue;
            }
            if (Math.abs(item.x - pos.x) > PICKUP_RANGE_X || Math.abs(item.y - pos.y) > PICKUP_RANGE_Y) {
                continue;
            }
            for (const actor of nsmbu.Level.actors()) {
                const [id] = actor.id();
                if (id === item.id) {
                    actor.delete();
                    break;
                }
            }
            p.set_mode(item.mode);
            current[i] = item.mode;
            markHandled(item.id, step);
            takeSummoned(item.id);
            pending[i] = undefined;
            clearSummonedFor(i);
            p.set_pickup_lock(item.mode > 0);
            nsmbu.log(`collected ${modeName(item.mode)}`);
            nsmbu.status(`Player ${i + 1} used ${modeName(item.mode)}`);
            break;
        }
    }
}

function iconMushroom(cx: number, cy: number, scale: number, cap: number): void {
    nsmbu.Gui.circle_filled(cx, cy - 3 * scale, 15 * scale, cap);
    nsmbu.Gui.rect_filled(cx - 8 * scale, cy + 4 * scale, 16 * scale, 12 * scale, 0xf7e7c6ff);
    nsmbu.Gui.circle_filled(cx - 7 * scale, cy - 7 * scale, 3.2 * scale, 0xffffffff);
    nsmbu.Gui.circle_filled(cx + 6 * scale, cy - 2 * scale, 2.6 * scale, 0xffffffff);
    nsmbu.Gui.circle_filled(cx - 1 * scale, cy - 12 * scale, 2 * scale, 0xffffffff);
}

function iconFlower(cx: number, cy: number, scale: number, petal: number, center: number): void {
    nsmbu.Gui.line(cx, cy + 4 * scale, cx, cy + 19 * scale, 0x2e8b3aff, 4 * scale);
    nsmbu.Gui.circle_filled(cx - 6 * scale, cy + 11 * scale, 4 * scale, 0x2e8b3aff);
    nsmbu.Gui.circle_filled(cx + 6 * scale, cy + 9 * scale, 4 * scale, 0x2e8b3aff);
    for (let i = 0; i < 8; i++) {
        const angle = (Math.PI * 2 * i) / 8;
        nsmbu.Gui.circle_filled(cx + Math.cos(angle) * 9 * scale, cy - 2 * scale + Math.sin(angle) * 9 * scale, 5 * scale, petal);
    }
    nsmbu.Gui.circle_filled(cx, cy - 2 * scale, 6 * scale, center);
}

function iconPropeller(cx: number, cy: number, scale: number): void {
    iconMushroom(cx, cy + 4 * scale, scale, 0xf2a03dff);
    nsmbu.Gui.line(cx - 13 * scale, cy - 15 * scale, cx + 13 * scale, cy - 15 * scale, 0x8a8f98ff, 3 * scale);
    nsmbu.Gui.circle_filled(cx - 13 * scale, cy - 15 * scale, 3 * scale, 0xc8ccd4ff);
    nsmbu.Gui.circle_filled(cx + 13 * scale, cy - 15 * scale, 3 * scale, 0xc8ccd4ff);
    nsmbu.Gui.circle_filled(cx, cy - 15 * scale, 2 * scale, 0x6b7078ff);
}

function iconPenguin(cx: number, cy: number, scale: number): void {
    nsmbu.Gui.circle_filled(cx, cy + 1 * scale, 15 * scale, 0x2b4c7eff);
    nsmbu.Gui.circle_filled(cx, cy + 4 * scale, 10 * scale, 0xffffffff);
    nsmbu.Gui.circle_filled(cx - 5 * scale, cy - 6 * scale, 2.4 * scale, 0xffffffff);
    nsmbu.Gui.circle_filled(cx + 5 * scale, cy - 6 * scale, 2.4 * scale, 0xffffffff);
    nsmbu.Gui.circle_filled(cx - 5 * scale, cy - 6 * scale, 1 * scale, 0x000000ff);
    nsmbu.Gui.circle_filled(cx + 5 * scale, cy - 6 * scale, 1 * scale, 0x000000ff);
    nsmbu.Gui.triangle_filled(
        cx - 4 * scale,
        cy - 1 * scale,
        cx + 4 * scale,
        cy - 1 * scale,
        cx,
        cy + 4 * scale,
        0xf2a03dff,
    );
}

function iconAcorn(cx: number, cy: number, scale: number, letter: boolean): void {
    nsmbu.Gui.rect_filled(cx - 2 * scale, cy - 16 * scale, 4 * scale, 7 * scale, 0x5c3617ff);
    nsmbu.Gui.circle_filled(cx, cy - 4 * scale, 13 * scale, 0x7a4a21ff);
    nsmbu.Gui.circle_filled(cx, cy + 6 * scale, 11 * scale, 0xc98a4bff);
    nsmbu.Gui.rect_filled(cx - 12 * scale, cy - 2 * scale, 24 * scale, 4 * scale, 0x5c3617ff);
    if (letter) {
        nsmbu.Gui.text(cx - 4 * scale, cy - 2 * scale, "P", 0xffffffff, 0.9);
    }
}

function drawIcon(cx: number, cy: number, mode: number): void {
    switch (mode) {
        case 1:
            iconMushroom(cx, cy, 1, 0xe23b3bff);
            break;
        case 2:
            iconFlower(cx, cy, 1, 0xf5671fff, 0xffd34eff);
            break;
        case 3:
            iconMushroom(cx, cy, 0.62, 0x4a86d8ff);
            break;
        case 4:
            iconPropeller(cx, cy, 1);
            break;
        case 5:
            iconPenguin(cx, cy, 1);
            break;
        case 6:
            iconFlower(cx, cy, 1, 0x8fe3f5ff, 0xffffffff);
            break;
        case 7:
            iconAcorn(cx, cy, 1, false);
            break;
        case 8:
            iconAcorn(cx, cy, 1, true);
            break;
        default:
            break;
    }
}

function drawBox(x: number, y: number, size: number, mode: number): void {
    nsmbu.Gui.rect_filled(x + 3, y + 3, size, size, 0x00000040, 12);
    nsmbu.Gui.rect_filled(x, y, size, size, 0x3d7fbfff, 12);
    nsmbu.Gui.rect_filled(x + 6, y + 6, size - 12, size - 12, 0xfaf2d8ff, 8);
    drawIcon(x + size / 2, y + size / 2, mode);
}

function drawHud(): void {
    nsmbu.Gui.clear();
    const [show] = nsmbu.Config.bool("show_hud", true);
    if (show !== true) {
        return;
    }
    const shown: number[] = [];
    for (let i = 0; i < 4; i++) {
        const mode = reserve[i];
        if (mode !== undefined) {
            shown.push(mode);
        }
    }
    if (shown.length === 0) {
        return;
    }
    const size = 58;
    const gap = 10;
    const total = shown.length * size + (shown.length - 1) * gap;
    let x = (1280 - total) / 2;
    const y = 14;
    for (const mode of shown) {
        drawBox(x, y, size, mode);
        x += size + gap;
    }
}

nsmbu.Hooks.onLogicStep = (step) => {
    if (!nsmbu.Level.active()) {
        nsmbu.Gui.clear();
        tracked = [];
        handled = [];
        summonedItems = [];
        for (let i = 0; i < 4; i++) {
            reserve[i] = undefined;
            prevMode[i] = undefined;
            pending[i] = undefined;
            nsmbu.Player.set_pickup_lock(i, false);
        }
        return;
    }
    const kept: HandledItem[] = [];
    for (const entry of handled) {
        if (step - entry.step < 180) {
            kept.push(entry);
        }
    }
    handled = kept;

    const players: Record<number, Player | undefined> = {};
    const current: Record<number, number | undefined> = {};
    readPlayers(players, current);
    for (let i = 0; i < 4; i++) {
        const lock = players[i] !== undefined && (current[i] ?? 0) > 0 && !pending[i];
        nsmbu.Player.set_pickup_lock(i, lock);
    }
    deployStored(players, current, step);

    const now = collectItems();
    trackSummoned(players, current, now, step);
    const previous = tracked;
    tracked = now;
    for (const old of previous) {
        let alive = false;
        for (const item of now) {
            if (item.id === old.id) {
                alive = true;
                break;
            }
        }
        if (!alive && old.steps >= 1 && !wasHandled(old.id, step)) {
            const index = findNearestPlayer(players, old);
            if (index >= 0) {
                markHandled(old.id, step);
                if (isSummoned(old.id)) {
                    takeSummoned(old.id);
                    pending[index] = undefined;
                    clearSummonedFor(index);
                    prevMode[index] = current[index];
                    const p = players[index];
                    if (p) {
                        p.set_pickup_lock((current[index] ?? 0) > 0);
                    }
                    nsmbu.log(`collected ${modeName(old.mode)}`);
                    nsmbu.status(`Player ${index + 1} used ${modeName(old.mode)}`);
                    continue;
                }
                handlePickup(index, old.mode, players, current);
            }
        }
    }
    detectUpgrades(players, current);
    grantSummoned(players, current, now, step);

    for (let i = 0; i < 4; i++) {
        prevMode[i] = current[i];
    }
    drawHud();
};

nsmbu.Hooks.onUnload = () => {
    for (let i = 0; i < 4; i++) {
        nsmbu.Player.set_pickup_lock(i, false);
    }
};

export {};
