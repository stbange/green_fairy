-- Room 7: Astral Preset Archive (-16, 0, -10)
-- 8 Curated Fantasy & Space Realms (320+ External .lua Presets)

local RB = require("rooms.rack_builder")

local R7 = {
    realm_rack = -1,
    vault_rack = -1,
    slot_buttons = {},
    realms = {
        {slug = "fantasy_realms",       title = "1 FANTASY REALMS"},
        {slug = "deep_space",           title = "2 DEEP SPACE"},
        {slug = "bioluminescent_abyss", title = "3 BIOLUM ABYSS"},
        {slug = "crystalline_sanctum",  title = "4 CRYSTAL SANCTUM"},
        {slug = "ancient_mythology",    title = "5 ANCIENT MYTH"},
        {slug = "cybernetic_ruins",     title = "6 CYBER RUINS"},
        {slug = "astral_choirs",        title = "7 ASTRAL CHOIRS"},
        {slug = "rhythmic_odysseys",    title = "8 RHYTHMIC ODYSSEY"},
    },
    by_realm = {},
    all_presets = {},
    active_realm_idx = 1,
    page = 0,
    active_global_idx = 1,
    active_name = "Emerald Aether",
}

function R7.load_catalog()
    R7.by_realm = {}
    R7.all_presets = {}
    for _, rm in ipairs(R7.realms) do
        R7.by_realm[rm.slug] = {}
    end

    -- 1. Parse patches/catalog.json if present (fast single-file boot on Web & Desktop)
    local raw = gf.assets.read_text("patches/catalog.json")
    if raw then
        for obj in raw:gmatch("%{(.-)%}") do
            local path  = obj:match('"path"%s*:%s*"([^"]+)"')
            local name  = obj:match('"name"%s*:%s*"([^"]+)"')
            local realm = obj:match('"realm"%s*:%s*"([^"]+)"')
            if path and name and realm then
                local entry = {path = path, name = name, realm = realm}
                R7.all_presets[#R7.all_presets + 1] = entry
                if not R7.by_realm[realm] then R7.by_realm[realm] = {} end
                local rl = R7.by_realm[realm]
                rl[#rl + 1] = entry
            end
        end
    end

    -- 2. Fallback scan if catalog.json is not yet generated
    if #R7.all_presets == 0 then
        local root_files = gf.assets.list("patches")
        for _, f in ipairs(root_files) do
            if f:match("%.lua$") then
                local entry = {
                    path = "patches/" .. f,
                    name = f:gsub("%.lua$", ""):gsub("_", " "),
                    realm = "fantasy_realms",
                }
                R7.all_presets[#R7.all_presets + 1] = entry
                table.insert(R7.by_realm["fantasy_realms"], entry)
            end
        end
    end
end

function R7.build()
    R7.load_catalog()
    R7.slot_buttons = {}
    local cx, cz = -16.0, -10.0

    -- Rack 1: 8-Realm Cosmic Selector
    local rx, rz = cx - 4.35, cz - 1.4
    R7.realm_rack = RB.add_rack(7, "8 REALMS OF FANTASY & SPACE", rx, 1.52, rz, math.atan(cx - rx, cz - rz), 2.25, 1.80,
                                {0.48, 0.68, 1.0, 1.0})

    local r_btns = {}
    for i, rm in ipairs(R7.realms) do
        local col = (i - 1) % 2
        local row = math.floor((i - 1) / 2)
        r_btns[i] = {
            kind = "button",
            x = -0.52 + col * 1.04,
            y = 0.42 - row * 0.25,
            w = 0.94, h = 0.14,
            label = rm.title,
            action = "vault:realm:" .. i,
            color = RB.COLOR_SWITCH,
        }
    end
    r_btns[#r_btns + 1] = {kind="button", x=0.0, y=-0.62, w=1.45, h=0.14, label="AUDITION CHORD", action="vault:audition", color=RB.COLOR_BUTTON}
    RB.add_controls(7, R7.realm_rack, r_btns)

    -- Rack 2: 10-Slot Pageable Preset Vault Matrix (4 pages x 10 = 40 presets per realm)
    local vx, vz = cx - 2.35, cz - 4.10
    R7.vault_rack = RB.add_rack(7, "ASTRAL PRESET VAULT MATRIX", vx, 1.52, vz, math.atan(cx - vx, cz - vz), 2.55, 1.85,
                                {0.42, 0.78, 1.0, 1.0})

    for s = 1, 10 do
        local col = (s - 1) % 2
        local row = math.floor((s - 1) / 2)
        R7.slot_buttons[s] = gf.spatial.add_widget({
            room = 7, rack = R7.vault_rack, kind = "button",
            pos = {-0.58 + col * 1.16, 0.44 - row * 0.20, 0.016},
            size = {1.06, 0.13, 0},
            aux = s, min = 0.0, max = 1.0, default = 0.0,
            color = {0.35, 0.85, 0.95, 1.0},
            label = "SLOT " .. s,
            action = "vault:slot:" .. s,
        })
    end

    RB.add_controls(7, R7.vault_rack, {
        {kind="button", x=-0.82, y=-0.64, w=0.48, h=0.13, label="< PREV PG",  action="vault:page:-1", color=RB.COLOR_DIAL},
        {kind="button", x=-0.27, y=-0.64, w=0.48, h=0.13, label="NEXT PG >",  action="vault:page:1",  color=RB.COLOR_DIAL},
        {kind="button", x= 0.28, y=-0.64, w=0.48, h=0.13, label="< PATCH",    action="vault:step:-1", color=RB.COLOR_BUTTON},
        {kind="button", x= 0.83, y=-0.64, w=0.48, h=0.13, label="PATCH >",    action="vault:step:1",  color=RB.COLOR_BUTTON},
    })

    R7.refresh_labels()
end

function R7.refresh_labels()
    local rm = R7.realms[R7.active_realm_idx]
    local list = (rm and R7.by_realm[rm.slug]) or R7.all_presets
    local base = R7.page * 10
    for s = 1, 10 do
        local entry = list[base + s]
        local wid = R7.slot_buttons[s]
        if wid then
            if entry then
                local short = entry.name:sub(1, 20):upper()
                gf.spatial.set_widget_state(wid, (entry.name == R7.active_name) and 1.0 or 0.0, short, true)
            else
                gf.spatial.set_widget_state(wid, 0.0, "---", true)
            end
        end
    end
end

function R7.apply_entry(entry)
    if not entry then return false end
    local meta = gf.presets.apply_file(entry.path, true)
    if not meta then return false end
    R7.active_name = meta.name or entry.name
    gf.presets.set_active_meta(R7.active_name, meta.realm or entry.realm)
    R7.refresh_labels()
    return true
end

function R7.select_global(idx)
    if #R7.all_presets == 0 then return false end
    local wrapped = ((idx - 1) % #R7.all_presets) + 1
    R7.active_global_idx = wrapped
    return R7.apply_entry(R7.all_presets[wrapped])
end

function R7.on_action(act)
    local r_idx = act:match("^vault:realm:(%d+)$")
    if r_idx then
        R7.active_realm_idx = tonumber(r_idx)
        R7.page = 0
        R7.refresh_labels()
        local list = R7.by_realm[R7.realms[R7.active_realm_idx].slug] or {}
        if list[1] then R7.apply_entry(list[1]) end
        return true
    end
    local s_idx = act:match("^vault:slot:(%d+)$")
    if s_idx then
        local rm = R7.realms[R7.active_realm_idx]
        local list = (rm and R7.by_realm[rm.slug]) or R7.all_presets
        local entry = list[R7.page * 10 + tonumber(s_idx)]
        if entry then R7.apply_entry(entry) end
        return true
    end
    local pg_d = act:match("^vault:page:([%-]?%d+)$")
    if pg_d then
        local rm = R7.realms[R7.active_realm_idx]
        local list = (rm and R7.by_realm[rm.slug]) or R7.all_presets
        local max_pg = math.max(0, math.floor((math.max(1, #list) - 1) / 10))
        R7.page = (R7.page + tonumber(pg_d)) % (max_pg + 1)
        R7.refresh_labels()
        return true
    end
    local st_d = act:match("^vault:step:([%-]?%d+)$")
    if st_d then
        return R7.select_global(R7.active_global_idx + tonumber(st_d))
    end
    if act == "vault:audition" then
        gf.synth.set_param("voice.drone_latch", 1.0)
        gf.synth.note_on(48, 0.80)
        gf.synth.note_on(55, 0.78)
        gf.synth.note_on(62, 0.75)
        gf.synth.note_on(67, 0.72)
        gf.spatial.sync_widgets()
        return true
    end
    return false
end

return R7
