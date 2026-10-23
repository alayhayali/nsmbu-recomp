-- Lua Library inline imports
local function __TS__ArrayFilter(self, callbackfn, thisArg)
    local result = {}
    local len = 0
    for i = 1, #self do
        if callbackfn(thisArg, self[i], i - 1, self) then
            len = len + 1
            result[len] = self[i]
        end
    end
    return result
end

local function __TS__CountVarargs(...)
    return select("#", ...)
end

local function __TS__ArraySplice(self, ...)
    local args = {...}
    local len = #self
    local actualArgumentCount = __TS__CountVarargs(...)
    local start = args[1]
    local deleteCount = args[2]
    if start < 0 then
        start = len + start
        if start < 0 then
            start = 0
        end
    elseif start > len then
        start = len
    end
    local itemCount = actualArgumentCount - 2
    if itemCount < 0 then
        itemCount = 0
    end
    local actualDeleteCount
    if actualArgumentCount == 0 then
        actualDeleteCount = 0
    elseif actualArgumentCount == 1 then
        actualDeleteCount = len - start
    else
        actualDeleteCount = deleteCount or 0
        if actualDeleteCount < 0 then
            actualDeleteCount = 0
        end
        if actualDeleteCount > len - start then
            actualDeleteCount = len - start
        end
    end
    local out = {}
    for k = 1, actualDeleteCount do
        local from = start + k
        if self[from] ~= nil then
            out[k] = self[from]
        end
    end
    if itemCount < actualDeleteCount then
        for k = start + 1, len - actualDeleteCount do
            local from = k + actualDeleteCount
            local to = k + itemCount
            if self[from] then
                self[to] = self[from]
            else
                self[to] = nil
            end
        end
        for k = len - actualDeleteCount + itemCount + 1, len do
            self[k] = nil
        end
    elseif itemCount > actualDeleteCount then
        for k = len - actualDeleteCount, start + 1, -1 do
            local from = k + actualDeleteCount
            local to = k + itemCount
            if self[from] then
                self[to] = self[from]
            else
                self[to] = nil
            end
        end
    end
    local j = start + 1
    for i = 3, actualArgumentCount do
        self[j] = args[i]
        j = j + 1
    end
    for k = #self, len - actualDeleteCount + itemCount + 1, -1 do
        self[k] = nil
    end
    return out
end
-- End of Lua Library inline imports
local ____exports = {}
local PROFILE_MODE = {
    [591] = 1,
    [592] = 2,
    [593] = 3,
    [594] = 6,
    [595] = 4,
    [596] = 5,
    [598] = 7
}
local MODE_NAME = {
    [0] = "Small",
    [1] = "Super",
    [2] = "Fire",
    [3] = "Mini",
    [4] = "Propeller",
    [5] = "Penguin",
    [6] = "Ice",
    [7] = "Squirrel",
    [8] = "P-Squirrel"
}
local PROFILE_FOR_MODE = {
    [1] = 591,
    [2] = 592,
    [3] = 593,
    [4] = 595,
    [5] = 596,
    [6] = 594,
    [7] = 598,
    [8] = 598
}
local COLLECTABLE_SPAWN = 1
local SPAWN_OFFSET_Y = 48
local PICKUP_RANGE_X = 28
local PICKUP_RANGE_Y = 48
local SUMMON_TIMEOUT = 240
local reserve = {}
local prevMode = {}
local pending = {}
local tracked = {}
local handled = {}
local summonedItems = {}
local function clearSummonedFor(player)
    summonedItems = __TS__ArrayFilter(
        summonedItems,
        function(____, entry) return entry.player ~= player end
    )
end
local function isSummoned(id)
    for ____, entry in ipairs(summonedItems) do
        if entry.id == id then
            return true
        end
    end
    return false
end
local function takeSummoned(id)
    do
        local i = 0
        while i < #summonedItems do
            if summonedItems[i + 1].id == id then
                local player = summonedItems[i + 1].player
                __TS__ArraySplice(summonedItems, i, 1)
                return player
            end
            i = i + 1
        end
    end
    return -1
end
local function modeName(mode)
    return MODE_NAME[mode] or "Power-up"
end
local function near(item, x, y)
    local dx = item.x - x
    local dy = item.y - y
    local distance = dx * dx + dy * dy
    return distance <= 72 * 72 and distance or -1
end
local function wasHandled(id, step)
    for ____, entry in ipairs(handled) do
        if entry.id == id and step - entry.step < 180 then
            return true
        end
    end
    return false
end
local function markHandled(id, step)
    handled[#handled + 1] = {id = id, step = step}
end
local function handlePickup(index, itemMode, players, current)
    local before = prevMode[index] or 0
    if before <= 0 then
        return
    end
    reserve[index] = itemMode
    local currentMode = current[index]
    if currentMode ~= nil and currentMode ~= before then
        local p = players[index]
        if p then
            p:set_mode(before)
        end
        current[index] = before
    end
    nsmbu.log("reserve store " .. modeName(itemMode))
    nsmbu.status((("Player " .. tostring(index + 1)) .. " reserve: ") .. modeName(itemMode))
end
local function readPlayers(players, current)
    do
        local i = 0
        while i < 4 do
            do
                local __continue25
                repeat
                    local p = nsmbu.Player.get_player(i)
                    if not p then
                        players[i] = nil
                        current[i] = nil
                        reserve[i] = nil
                        prevMode[i] = nil
                        pending[i] = nil
                        clearSummonedFor(i)
                        __continue25 = true
                        break
                    end
                    local entered = p:entered()
                    if not entered then
                        players[i] = nil
                        current[i] = nil
                        reserve[i] = nil
                        prevMode[i] = nil
                        pending[i] = nil
                        clearSummonedFor(i)
                        __continue25 = true
                        break
                    end
                    players[i] = p
                    local mode = p:mode()
                    current[i] = mode
                    __continue25 = true
                until true
                if not __continue25 then
                    break
                end
            end
            i = i + 1
        end
    end
end
local function collectItems()
    local now = {}
    for ____, actor in ipairs(nsmbu.Level.actors()) do
        do
            local __continue29
            repeat
                local profile = actor:profile_id()
                if profile == nil then
                    __continue29 = true
                    break
                end
                local mode = PROFILE_MODE[profile]
                if mode == nil then
                    __continue29 = true
                    break
                end
                local id = actor:id()
                local pos = actor:pos()
                if id == nil or not pos then
                    __continue29 = true
                    break
                end
                local steps = 1
                for ____, old in ipairs(tracked) do
                    if old.id == id then
                        steps = old.steps + 1
                        break
                    end
                end
                now[#now + 1] = {
                    id = id,
                    mode = mode,
                    x = pos.x,
                    y = pos.y,
                    steps = steps
                }
                __continue29 = true
            until true
            if not __continue29 then
                break
            end
        end
    end
    return now
end
local function findNearestPlayer(players, item)
    local best = -1
    local bestDistance = 0
    do
        local i = 0
        while i < 4 do
            do
                local __continue39
                repeat
                    local p = players[i]
                    if not p then
                        __continue39 = true
                        break
                    end
                    local pos = p:pos()
                    if not pos then
                        __continue39 = true
                        break
                    end
                    local distance = near(item, pos.x, pos.y)
                    if distance < 0 then
                        __continue39 = true
                        break
                    end
                    if best < 0 or distance < bestDistance then
                        best = i
                        bestDistance = distance
                    end
                    __continue39 = true
                until true
                if not __continue39 then
                    break
                end
            end
            i = i + 1
        end
    end
    return best
end
local function detectUpgrades(players, current)
    do
        local i = 0
        while i < 4 do
            do
                local __continue46
                repeat
                    local before = prevMode[i] or 0
                    local now = current[i]
                    if now == nil then
                        __continue46 = true
                        break
                    end
                    if pending[i] then
                        if now > before then
                            pending[i] = nil
                            clearSummonedFor(i)
                            local p = players[i]
                            if p then
                                p:set_pickup_lock(now > 0)
                            end
                            nsmbu.log("used " .. modeName(now))
                            nsmbu.status((("Player " .. tostring(i + 1)) .. " used ") .. modeName(now))
                        end
                        __continue46 = true
                        break
                    end
                    if now <= before or before <= 0 then
                        __continue46 = true
                        break
                    end
                    local p = players[i]
                    if not p then
                        __continue46 = true
                        break
                    end
                    reserve[i] = now
                    p:set_mode(before)
                    current[i] = before
                    nsmbu.log("reserve upgrade " .. modeName(now))
                    nsmbu.status((("Player " .. tostring(i + 1)) .. " reserve: ") .. modeName(now))
                    __continue46 = true
                until true
                if not __continue46 then
                    break
                end
            end
            i = i + 1
        end
    end
end
local function deployStored(players, current, step)
    local button = nsmbu.Config.string("use_button", "Minus")
    local buttonName = button or "Minus"
    local pressed = nsmbu.Input.button.pressed(buttonName)
    if pressed ~= true then
        return
    end
    do
        local i = 0
        while i < 4 do
            do
                local __continue56
                repeat
                    local p = players[i]
                    if not p then
                        __continue56 = true
                        break
                    end
                    local stored = reserve[i]
                    if stored == nil or pending[i] then
                        __continue56 = true
                        break
                    end
                    local profile = PROFILE_FOR_MODE[stored]
                    if profile == nil then
                        __continue56 = true
                        break
                    end
                    local pos = p:pos()
                    if not pos then
                        __continue56 = true
                        break
                    end
                    local spawnY = pos.y + SPAWN_OFFSET_Y
                    local ok = nsmbu.Level.spawn(
                        profile,
                        pos.x,
                        spawnY,
                        pos.z,
                        {param0 = COLLECTABLE_SPAWN * 16777216}
                    )
                    if ok == true then
                        reserve[i] = nil
                        pending[i] = {mode = stored, x = pos.x, y = spawnY, step = step}
                        p:set_pickup_lock(false)
                        nsmbu.log("deploy " .. modeName(stored))
                        nsmbu.status((("Player " .. tostring(i + 1)) .. " deployed ") .. modeName(stored))
                    end
                    __continue56 = true
                until true
                if not __continue56 then
                    break
                end
            end
            i = i + 1
        end
    end
end
local function trackSummoned(players, current, now, step)
    do
        local i = 0
        while i < 4 do
            do
                local __continue64
                repeat
                    local summon = pending[i]
                    if not summon then
                        __continue64 = true
                        break
                    end
                    if step - summon.step > SUMMON_TIMEOUT then
                        pending[i] = nil
                        clearSummonedFor(i)
                        local p = players[i]
                        if p then
                            p:set_pickup_lock((current[i] or 0) > 0)
                        end
                        __continue64 = true
                        break
                    end
                    if step <= summon.step then
                        __continue64 = true
                        break
                    end
                    for ____, item in ipairs(now) do
                        do
                            local __continue69
                            repeat
                                if item.mode ~= summon.mode or isSummoned(item.id) then
                                    __continue69 = true
                                    break
                                end
                                if math.abs(item.x - summon.x) > 48 or math.abs(item.y - summon.y) > 128 then
                                    __continue69 = true
                                    break
                                end
                                summonedItems[#summonedItems + 1] = {id = item.id, player = i}
                                break
                            until true
                            if not __continue69 then
                                break
                            end
                        end
                    end
                    __continue64 = true
                until true
                if not __continue64 then
                    break
                end
            end
            i = i + 1
        end
    end
end
local function grantSummoned(players, current, now, step)
    do
        local i = 0
        while i < 4 do
            do
                local __continue75
                repeat
                    local summon = pending[i]
                    if not summon then
                        __continue75 = true
                        break
                    end
                    local p = players[i]
                    if not p then
                        __continue75 = true
                        break
                    end
                    local pos = p:pos()
                    if not pos then
                        __continue75 = true
                        break
                    end
                    for ____, item in ipairs(now) do
                        do
                            local __continue79
                            repeat
                                if item.mode ~= summon.mode or not isSummoned(item.id) then
                                    __continue79 = true
                                    break
                                end
                                if math.abs(item.x - pos.x) > PICKUP_RANGE_X or math.abs(item.y - pos.y) > PICKUP_RANGE_Y then
                                    __continue79 = true
                                    break
                                end
                                for ____, actor in ipairs(nsmbu.Level.actors()) do
                                    local id = actor:id()
                                    if id == item.id then
                                        actor:delete()
                                        break
                                    end
                                end
                                p:set_mode(item.mode)
                                current[i] = item.mode
                                markHandled(item.id, step)
                                takeSummoned(item.id)
                                pending[i] = nil
                                clearSummonedFor(i)
                                p:set_pickup_lock(item.mode > 0)
                                nsmbu.log("collected " .. modeName(item.mode))
                                nsmbu.status((("Player " .. tostring(i + 1)) .. " used ") .. modeName(item.mode))
                                break
                            until true
                            if not __continue79 then
                                break
                            end
                        end
                    end
                    __continue75 = true
                until true
                if not __continue75 then
                    break
                end
            end
            i = i + 1
        end
    end
end
local function iconMushroom(cx, cy, scale, cap)
    nsmbu.Gui.circle_filled(cx, cy - 3 * scale, 15 * scale, cap)
    nsmbu.Gui.rect_filled(
        cx - 8 * scale,
        cy + 4 * scale,
        16 * scale,
        12 * scale,
        4159162111
    )
    nsmbu.Gui.circle_filled(cx - 7 * scale, cy - 7 * scale, 3.2 * scale, 4294967295)
    nsmbu.Gui.circle_filled(cx + 6 * scale, cy - 2 * scale, 2.6 * scale, 4294967295)
    nsmbu.Gui.circle_filled(cx - 1 * scale, cy - 12 * scale, 2 * scale, 4294967295)
end
local function iconFlower(cx, cy, scale, petal, center)
    nsmbu.Gui.line(
        cx,
        cy + 4 * scale,
        cx,
        cy + 19 * scale,
        780876543,
        4 * scale
    )
    nsmbu.Gui.circle_filled(cx - 6 * scale, cy + 11 * scale, 4 * scale, 780876543)
    nsmbu.Gui.circle_filled(cx + 6 * scale, cy + 9 * scale, 4 * scale, 780876543)
    do
        local i = 0
        while i < 8 do
            local angle = math.pi * 2 * i / 8
            nsmbu.Gui.circle_filled(
                cx + math.cos(angle) * 9 * scale,
                cy - 2 * scale + math.sin(angle) * 9 * scale,
                5 * scale,
                petal
            )
            i = i + 1
        end
    end
    nsmbu.Gui.circle_filled(cx, cy - 2 * scale, 6 * scale, center)
end
local function iconPropeller(cx, cy, scale)
    iconMushroom(cx, cy + 4 * scale, scale, 4070587903)
    nsmbu.Gui.line(
        cx - 13 * scale,
        cy - 15 * scale,
        cx + 13 * scale,
        cy - 15 * scale,
        2324666623,
        3 * scale
    )
    nsmbu.Gui.circle_filled(cx - 13 * scale, cy - 15 * scale, 3 * scale, 3368867071)
    nsmbu.Gui.circle_filled(cx + 13 * scale, cy - 15 * scale, 3 * scale, 3368867071)
    nsmbu.Gui.circle_filled(cx, cy - 15 * scale, 2 * scale, 1802533119)
end
local function iconPenguin(cx, cy, scale)
    nsmbu.Gui.circle_filled(cx, cy + 1 * scale, 15 * scale, 726433535)
    nsmbu.Gui.circle_filled(cx, cy + 4 * scale, 10 * scale, 4294967295)
    nsmbu.Gui.circle_filled(cx - 5 * scale, cy - 6 * scale, 2.4 * scale, 4294967295)
    nsmbu.Gui.circle_filled(cx + 5 * scale, cy - 6 * scale, 2.4 * scale, 4294967295)
    nsmbu.Gui.circle_filled(cx - 5 * scale, cy - 6 * scale, 1 * scale, 255)
    nsmbu.Gui.circle_filled(cx + 5 * scale, cy - 6 * scale, 1 * scale, 255)
    nsmbu.Gui.triangle_filled(
        cx - 4 * scale,
        cy - 1 * scale,
        cx + 4 * scale,
        cy - 1 * scale,
        cx,
        cy + 4 * scale,
        4070587903
    )
end
local function iconAcorn(cx, cy, scale, letter)
    nsmbu.Gui.rect_filled(
        cx - 2 * scale,
        cy - 16 * scale,
        4 * scale,
        7 * scale,
        1547048959
    )
    nsmbu.Gui.circle_filled(cx, cy - 4 * scale, 13 * scale, 2051678719)
    nsmbu.Gui.circle_filled(cx, cy + 6 * scale, 11 * scale, 3381283839)
    nsmbu.Gui.rect_filled(
        cx - 12 * scale,
        cy - 2 * scale,
        24 * scale,
        4 * scale,
        1547048959
    )
    if letter then
        nsmbu.Gui.text(
            cx - 4 * scale,
            cy - 2 * scale,
            "P",
            4294967295,
            0.9
        )
    end
end
local function drawIcon(cx, cy, mode)
    repeat
        local ____switch95 = mode
        local ____cond95 = ____switch95 == 1
        if ____cond95 then
            iconMushroom(cx, cy, 1, 3795532799)
            break
        end
        ____cond95 = ____cond95 or ____switch95 == 2
        if ____cond95 then
            iconFlower(
                cx,
                cy,
                1,
                4117176319,
                4292038399
            )
            break
        end
        ____cond95 = ____cond95 or ____switch95 == 3
        if ____cond95 then
            iconMushroom(cx, cy, 0.62, 1250351359)
            break
        end
        ____cond95 = ____cond95 or ____switch95 == 4
        if ____cond95 then
            iconPropeller(cx, cy, 1)
            break
        end
        ____cond95 = ____cond95 or ____switch95 == 5
        if ____cond95 then
            iconPenguin(cx, cy, 1)
            break
        end
        ____cond95 = ____cond95 or ____switch95 == 6
        if ____cond95 then
            iconFlower(
                cx,
                cy,
                1,
                2414081535,
                4294967295
            )
            break
        end
        ____cond95 = ____cond95 or ____switch95 == 7
        if ____cond95 then
            iconAcorn(cx, cy, 1, false)
            break
        end
        ____cond95 = ____cond95 or ____switch95 == 8
        if ____cond95 then
            iconAcorn(cx, cy, 1, true)
            break
        end
        do
            break
        end
    until true
end
local function drawBox(x, y, size, mode)
    nsmbu.Gui.rect_filled(
        x + 3,
        y + 3,
        size,
        size,
        64,
        12
    )
    nsmbu.Gui.rect_filled(
        x,
        y,
        size,
        size,
        1031782399,
        12
    )
    nsmbu.Gui.rect_filled(
        x + 6,
        y + 6,
        size - 12,
        size - 12,
        4210219263,
        8
    )
    drawIcon(x + size / 2, y + size / 2, mode)
end
local function drawHud()
    nsmbu.Gui.clear()
    local show = nsmbu.Config.bool("show_hud", true)
    if show ~= true then
        return
    end
    local shown = {}
    do
        local i = 0
        while i < 4 do
            local mode = reserve[i]
            if mode ~= nil then
                shown[#shown + 1] = mode
            end
            i = i + 1
        end
    end
    if #shown == 0 then
        return
    end
    local size = 58
    local gap = 10
    local total = #shown * size + (#shown - 1) * gap
    local x = (1280 - total) / 2
    local y = 14
    for ____, mode in ipairs(shown) do
        drawBox(x, y, size, mode)
        x = x + (size + gap)
    end
end
nsmbu.Hooks.onLogicStep = function(step)
    if not nsmbu.Level.active() then
        nsmbu.Gui.clear()
        tracked = {}
        handled = {}
        summonedItems = {}
        do
            local i = 0
            while i < 4 do
                reserve[i] = nil
                prevMode[i] = nil
                pending[i] = nil
                nsmbu.Player.set_pickup_lock(i, false)
                i = i + 1
            end
        end
        return
    end
    local kept = {}
    for ____, entry in ipairs(handled) do
        if step - entry.step < 180 then
            kept[#kept + 1] = entry
        end
    end
    handled = kept
    local players = {}
    local current = {}
    readPlayers(players, current)
    do
        local i = 0
        while i < 4 do
            local lock = players[i] ~= nil and (current[i] or 0) > 0 and not pending[i]
            nsmbu.Player.set_pickup_lock(i, lock)
            i = i + 1
        end
    end
    deployStored(players, current, step)
    local now = collectItems()
    trackSummoned(players, current, now, step)
    local previous = tracked
    tracked = now
    for ____, old in ipairs(previous) do
        do
            local __continue114
            repeat
                local alive = false
                for ____, item in ipairs(now) do
                    if item.id == old.id then
                        alive = true
                        break
                    end
                end
                if not alive and old.steps >= 1 and not wasHandled(old.id, step) then
                    local index = findNearestPlayer(players, old)
                    if index >= 0 then
                        markHandled(old.id, step)
                        if isSummoned(old.id) then
                            takeSummoned(old.id)
                            pending[index] = nil
                            clearSummonedFor(index)
                            prevMode[index] = current[index]
                            local p = players[index]
                            if p then
                                p:set_pickup_lock((current[index] or 0) > 0)
                            end
                            nsmbu.log("collected " .. modeName(old.mode))
                            nsmbu.status((("Player " .. tostring(index + 1)) .. " used ") .. modeName(old.mode))
                            __continue114 = true
                            break
                        end
                        handlePickup(index, old.mode, players, current)
                    end
                end
                __continue114 = true
            until true
            if not __continue114 then
                break
            end
        end
    end
    detectUpgrades(players, current)
    grantSummoned(players, current, now, step)
    do
        local i = 0
        while i < 4 do
            prevMode[i] = current[i]
            i = i + 1
        end
    end
    drawHud()
end
nsmbu.Hooks.onUnload = function()
    do
        local i = 0
        while i < 4 do
            nsmbu.Player.set_pickup_lock(i, false)
            i = i + 1
        end
    end
end
return ____exports
