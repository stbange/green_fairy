-- Preset-Inspired, Music-Reactive Distant 3D World Surroundings Engine
-- Places monumental non-living structures, living plants, and living animals at a clean
-- physical distance strictly outside the 7-room sanctum perimeter (R >= 35m around origin),
-- styled with curated, theme-fitting palettes and atmospheric perspective separation.

local PresetWorlds = {
    theme = "emerald_cavern",
    preset_name = "Emerald Sanctum Choir",
    min_clearance = 35.0, -- 7-room sanctum spans R <= 25.6m; world begins cleanly at >= 35m
    items = {},
    time = 0.0,
    base_rgb = {0.14, 0.56, 0.48},
    accent_rgb = {0.92, 0.86, 0.66},
}

local function clamp(v, lo, hi)
    if v < lo then return lo end
    if v > hi then return hi end
    return v
end

local function hash_string(s)
    local h = 2166136261
    for i = 1, #s do
        h = (h * 16777619 + s:byte(i)) % 4294967296
    end
    return h
end

local function make_rng(seed)
    local st = (seed % 2147483647)
    if st <= 0 then st = 1337 end
    return function(lo, hi)
        st = (st * 48271) % 2147483647
        local u = st / 2147483647.0
        if lo and hi then return lo + u * (hi - lo) end
        return u
    end
end

local function match_any(str, words)
    for _, w in ipairs(words) do
        if str:find(w, 1, true) then return true end
    end
    return false
end

local function enforce_clearance(x, z, min_r)
    local d2 = x * x + z * z
    if d2 < min_r * min_r then
        local d = math.sqrt(d2)
        if d < 1e-4 then return 0.0, -min_r end
        local s = min_r / d
        return x * s, z * s
    end
    return x, z
end

local ARCHETYPE_RULES = {
    {"starfall_cosmos", {
        "starfall", "starlight", "star", "nebula", "galaxy", "cosmic", "cosmos",
        "pulsar", "quasar", "orbit", "horizon", "supernova", "comet", "eclipse",
        "meteor", "nova", "andromeda", "voyager", "planet", "saturn", "jupiter",
        "neptune", "lunar", "moon", "void", "space", "astral", "zenith", "black hole"
    }},
    {"emerald_cavern", {
        "emerald", "cave", "cavern", "crystal", "quartz", "prism", "gem", "jade",
        "diamond", "amethyst", "sapphire", "ruby", "garnet", "opal", "shard",
        "geode", "glacier", "frost", "ice", "citrine", "beryl", "obsidian",
        "tourmaline", "glass", "spire", "mineral", "crystalline"
    }},
    {"biolum_ocean", {
        "whale", "jelly", "medusa", "ocean", "abyss", "mariana", "trench", "coral",
        "kelp", "siren", "leviathan", "tide", "aqua", "sea", "hydro", "lagoon",
        "nautilus", "reef", "brook", "water", "sunken", "deluge", "submerged", "biolum"
    }},
    {"enchanted_forest", {
        "forest", "sylvan", "oak", "tree", "wood", "grove", "briar", "verdant",
        "fern", "moss", "meadow", "glade", "fae", "fairy", "pixie", "elven",
        "lothlorien", "rivendell", "avalon", "willow", "thorn", "blossom",
        "petal", "garden", "wisp", "wormwood", "canopy", "leaf"
    }},
    {"winged_sky_sanctum", {
        "seraph", "angel", "choir", "wing", "cloud", "sky", "heaven", "aether",
        "halo", "cathedral", "hymn", "anthem", "wind", "zephyr", "aurora",
        "phoenix", "bird", "raven", "owl", "falcon", "swan", "gossamer", "celest"
    }},
    {"ancient_desert_temple", {
        "oracle", "delphi", "temple", "pyramid", "dune", "desert", "arrakis",
        "sand", "pharaoh", "myth", "olympus", "sphinx", "ruin", "cairn",
        "monolith", "obelisk", "colossus", "babylon", "titan", "bronze",
        "solar", "sun", "ember", "volcano", "magma", "canyon", "mesa"
    }},
    {"cyber_clockwork", {
        "cyber", "neon", "dyson", "mainframe", "clockwork", "astrolabe", "gear",
        "chrono", "pulse", "matrix", "circuit", "grid", "mech", "automaton",
        "vector", "quantum", "reactor", "server", "tachyon", "pendulum",
        "odyssey", "machine", "chrome", "laser", "synth"
    }},
}

local REALM_FALLBACK = {
    deep_space           = "starfall_cosmos",
    crystalline_sanctum  = "emerald_cavern",
    bioluminescent_abyss = "biolum_ocean",
    fantasy_realms       = "enchanted_forest",
    astral_choirs        = "winged_sky_sanctum",
    ancient_mythology    = "ancient_desert_temple",
    cybernetic_ruins     = "cyber_clockwork",
    rhythmic_odysseys    = "cyber_clockwork",
}

-- Curated, editorial split-complementary palettes (stylish, never raw garish primaries)
local PALETTE_OVERRIDES = {
    {words = {"emerald", "jade", "verdant", "wormwood", "moss"},
     pri = {0.10, 0.56, 0.44}, sec = {0.26, 0.78, 0.62}, acc = {0.93, 0.87, 0.64},
     hor = {0.07, 0.21, 0.21}},
    {words = {"amethyst", "violet", "purple", "twilight", "nebula"},
     pri = {0.42, 0.28, 0.66}, sec = {0.68, 0.50, 0.86}, acc = {0.94, 0.80, 0.66},
     hor = {0.16, 0.11, 0.26}},
    {words = {"sapphire", "azure", "cobalt", "ice", "frost", "glacier", "arctic"},
     pri = {0.18, 0.48, 0.72}, sec = {0.52, 0.78, 0.92}, acc = {0.94, 0.92, 0.82},
     hor = {0.09, 0.20, 0.30}},
    {words = {"ruby", "crimson", "ember", "flame", "fire", "phoenix", "magma"},
     pri = {0.68, 0.24, 0.24}, sec = {0.86, 0.48, 0.32}, acc = {0.96, 0.84, 0.58},
     hor = {0.25, 0.11, 0.13}},
    {words = {"gold", "amber", "sun", "solar", "dawn", "bronze", "citrine", "arrakis"},
     pri = {0.74, 0.54, 0.30}, sec = {0.90, 0.72, 0.42}, acc = {0.36, 0.72, 0.78},
     hor = {0.28, 0.17, 0.12}},
    {words = {"rose", "blossom", "lotus", "coral", "sakura", "pink"},
     pri = {0.74, 0.38, 0.54}, sec = {0.90, 0.64, 0.74}, acc = {0.94, 0.88, 0.70},
     hor = {0.24, 0.14, 0.21}},
    {words = {"silver", "moon", "pearl", "quartz", "glass", "opal"},
     pri = {0.52, 0.64, 0.76}, sec = {0.76, 0.86, 0.94}, acc = {0.94, 0.86, 0.72},
     hor = {0.14, 0.19, 0.26}},
    {words = {"obsidian", "void", "shadow", "eclipse", "midnight"},
     pri = {0.24, 0.22, 0.46}, sec = {0.50, 0.42, 0.74}, acc = {0.90, 0.76, 0.56},
     hor = {0.10, 0.09, 0.19}},
}

local BIOME_ATMOSPHERE = {
    emerald_cavern = {
        zen = {0.024, 0.068, 0.086}, hor = {0.075, 0.215, 0.210},
        fog = {0.052, 0.165, 0.168}, dens = 0.013,
        pri = {0.10, 0.56, 0.44},    sec = {0.28, 0.80, 0.64}, acc = {0.93, 0.87, 0.64},
    },
    starfall_cosmos = {
        zen = {0.022, 0.028, 0.092}, hor = {0.135, 0.092, 0.235},
        fog = {0.082, 0.062, 0.165}, dens = 0.0045,
        pri = {0.52, 0.46, 0.82},    sec = {0.46, 0.74, 0.92}, acc = {0.96, 0.90, 0.76},
    },
    biolum_ocean = {
        zen = {0.018, 0.052, 0.125}, hor = {0.065, 0.215, 0.305},
        fog = {0.045, 0.160, 0.240}, dens = 0.014,
        pri = {0.12, 0.42, 0.48},    sec = {0.36, 0.78, 0.84}, acc = {0.94, 0.52, 0.66},
    },
    enchanted_forest = {
        zen = {0.042, 0.078, 0.135}, hor = {0.125, 0.225, 0.210},
        fog = {0.088, 0.175, 0.175}, dens = 0.012,
        pri = {0.16, 0.42, 0.34},    sec = {0.82, 0.58, 0.68}, acc = {0.95, 0.84, 0.54},
    },
    winged_sky_sanctum = {
        zen = {0.095, 0.078, 0.195}, hor = {0.310, 0.190, 0.275},
        fog = {0.225, 0.145, 0.235}, dens = 0.0085,
        pri = {0.78, 0.74, 0.84},    sec = {0.92, 0.72, 0.62}, acc = {0.96, 0.85, 0.58},
    },
    ancient_desert_temple = {
        zen = {0.110, 0.082, 0.160}, hor = {0.365, 0.200, 0.135},
        fog = {0.275, 0.160, 0.120}, dens = 0.0105,
        pri = {0.72, 0.52, 0.34},    sec = {0.30, 0.66, 0.72}, acc = {0.95, 0.76, 0.44},
    },
    cyber_clockwork = {
        zen = {0.042, 0.042, 0.118}, hor = {0.175, 0.095, 0.245},
        fog = {0.115, 0.070, 0.185}, dens = 0.0095,
        pri = {0.78, 0.60, 0.34},    sec = {0.32, 0.76, 0.86}, acc = {0.86, 0.42, 0.68},
    },
}

function PresetWorlds.classify(name, realm)
    local lower = (name or ""):lower()
    local theme = nil
    for _, rule in ipairs(ARCHETYPE_RULES) do
        if match_any(lower, rule[2]) then
            theme = rule[1]
            break
        end
    end
    if not theme then
        theme = REALM_FALLBACK[realm or ""] or "enchanted_forest"
    end

    local atm = BIOME_ATMOSPHERE[theme] or BIOME_ATMOSPHERE.emerald_cavern
    local pri = {atm.pri[1], atm.pri[2], atm.pri[3]}
    local sec = {atm.sec[1], atm.sec[2], atm.sec[3]}
    local acc = {atm.acc[1], atm.acc[2], atm.acc[3]}
    local hor = {atm.hor[1], atm.hor[2], atm.hor[3]}
    for _, ov in ipairs(PALETTE_OVERRIDES) do
        if match_any(lower, ov.words) then
            pri = {ov.pri[1], ov.pri[2], ov.pri[3]}
            sec = {ov.sec[1], ov.sec[2], ov.sec[3]}
            acc = {ov.acc[1], ov.acc[2], ov.acc[3]}
            hor = {
                atm.hor[1] * 0.45 + ov.hor[1] * 0.55,
                atm.hor[2] * 0.45 + ov.hor[2] * 0.55,
                atm.hor[3] * 0.45 + ov.hor[3] * 0.55,
            }
            break
        end
    end

    -- Subtle per-preset tonal nuance (keeps palettes cohesive & stylish across all 320+ presets)
    local seed = hash_string(name or "Sanctum")
    local n1 = ((seed % 100) / 100.0 - 0.5) * 0.09
    local n2 = (((math.floor(seed / 100)) % 100) / 100.0 - 0.5) * 0.09
    pri[1] = clamp(pri[1] + n1, 0.08, 0.92)
    pri[2] = clamp(pri[2] + n2 * 0.8, 0.08, 0.92)
    pri[3] = clamp(pri[3] - n1 * 0.8, 0.08, 0.92)

    local zen = {
        clamp(atm.zen[1] * 0.82 + pri[1] * 0.04, 0.015, 0.25),
        clamp(atm.zen[2] * 0.82 + pri[2] * 0.04, 0.015, 0.25),
        clamp(atm.zen[3] * 0.82 + pri[3] * 0.04, 0.015, 0.28),
    }
    local fog = {
        clamp(hor[1] * 0.78 + atm.fog[1] * 0.22, 0.025, 0.42),
        clamp(hor[2] * 0.78 + atm.fog[2] * 0.22, 0.025, 0.42),
        clamp(hor[3] * 0.78 + atm.fog[3] * 0.22, 0.030, 0.45),
    }
    return {
        theme = theme,
        seed = seed,
        pri = pri,
        sec = sec,
        acc = acc,
        zen = zen,
        hor = hor,
        fog = fog,
        dens = atm.dens,
        wants_whale = match_any(lower, {"whale", "leviathan", "siren"}),
        wants_jelly = match_any(lower, {"jelly", "medusa", "squid"}),
        wants_flora = match_any(lower, {"flower", "blossom", "petal", "lotus", "briar", "garden", "coral"}),
    }
end

function PresetWorlds.clear()
    for _, it in ipairs(PresetWorlds.items) do
        if it.id and it.id > 0 then
            gf.scene.remove(it.id)
        end
    end
    PresetWorlds.items = {}
end

local function spawn_item(kind, asset, cfg)
    local id = -1
    if kind == "mesh" then
        id = gf.scene.add_mesh(asset, 0)
    elseif kind == "sprite" then
        id = gf.scene.add_sprite(asset, 0)
    elseif kind == "voxel" then
        id = gf.scene.add_voxel(asset, 0)
    end
    if id <= 0 then return nil end

    if cfg.emissive then
        gf.scene.set_emissive(id, true)
    end
    if kind == "sprite" then
        gf.scene.set_sprite_anim(id, cfg.sp_w or 2.4, cfg.sp_h or 2.4, 4, cfg.frame or 0)
    end
    local col = cfg.col or {0.4, 0.75, 0.72}
    gf.scene.set_tint(id, col[1], col[2], col[3], cfg.alpha or 1.0)

    local ox, oz = cfg.x or 0.0, cfg.z or -48.0
    if cfg.behavior ~= "orbit_swim" and cfg.behavior ~= "orbit_fly" then
        ox, oz = enforce_clearance(ox, oz, PresetWorlds.min_clearance)
    end
    local rad = math.max(PresetWorlds.min_clearance + 3.0, cfg.radius or 46.0)

    local it = {
        id = id,
        kind = kind,
        behavior = cfg.behavior or "static",
        ox = ox,
        oy = cfg.y or 0.0,
        oz = oz,
        rx = cfg.rx or 0.0,
        ry = cfg.ry or 0.0,
        rz = cfg.rz or 0.0,
        sx = cfg.sx or 1.0,
        sy = cfg.sy or 1.0,
        sz = cfg.sz or 1.0,
        radius = rad,
        angle = cfg.angle or 0.0,
        speed = cfg.speed or 0.3,
        phase = cfg.phase or 0.0,
        amp = cfg.amp or 1.2,
        band = cfg.band or 1,
        z_min = cfg.z_min or -145.0,
        z_max = cfg.z_max or -38.0,
        sp_w = cfg.sp_w or 2.4,
        sp_h = cfg.sp_h or 2.4,
        col = {col[1], col[2], col[3]},
        alpha = cfg.alpha or 1.0,
        emissive = cfg.emissive == true,
        wx = ox,
        wz = oz,
    }
    PresetWorlds.items[#PresetWorlds.items + 1] = it
    return it
end

function PresetWorlds.apply(name, realm, floorplan)
    PresetWorlds.clear()

    local spec = PresetWorlds.classify(name, realm)
    PresetWorlds.theme = spec.theme
    PresetWorlds.preset_name = name or "Emerald Sanctum Choir"
    PresetWorlds.base_rgb = spec.pri
    PresetWorlds.accent_rgb = spec.acc

    gf.scene.set_world_theme(spec.theme)
    gf.scene.set_atmosphere(
        spec.zen[1], spec.zen[2], spec.zen[3],
        spec.hor[1], spec.hor[2], spec.hor[3],
        spec.fog[1], spec.fog[2], spec.fog[3],
        spec.dens)
    if floorplan and floorplan.apply_world_tint then
        -- Pass subtle horizon hue for tiny rim reflection while sanctum stays crisp dark slate
        floorplan.apply_world_tint(spec.hor[1] * 1.4, spec.hor[2] * 1.4, spec.hor[3] * 1.4)
    end

    local rng = make_rng(spec.seed)
    local pri, sec, acc = spec.pri, spec.sec, spec.acc
    local deep_pri = {pri[1] * 0.58, pri[2] * 0.58, pri[3] * 0.64}
    local th = spec.theme

    if th == "starfall_cosmos" then
        -- 32 Distant Flying Star Streaks streaming through outer deep space (never inside R < 35m!)
        for i = 1, 32 do
            local is_front = (i <= 16)
            local sx_p, z_min, z_max, sy_p
            if is_front then
                sx_p = rng(-54.0, 54.0)
                sy_p = (i <= 10) and rng(4.5, 16.5) or rng(14.0, 34.0)
                z_min, z_max = -155.0, -40.0
            else
                local side = (i % 2 == 0) and 1.0 or -1.0
                sx_p = side * rng(38.0, 92.0)
                sy_p = rng(4.0, 36.0)
                z_min, z_max = -140.0, 90.0
            end
            local sc = rng(1.8, 3.8)
            local col = (i % 3 == 0) and acc or ((i % 2 == 0) and sec or pri)
            spawn_item("mesh", "models/env_star_streak.obj", {
                behavior = "star_fly", emissive = true,
                x = sx_p, y = sy_p, z = rng(z_min, z_max),
                z_min = z_min, z_max = z_max,
                sx = sc * 0.85, sy = sc * 0.85, sz = sc * 2.4,
                speed = rng(16.0, 34.0), phase = rng(0, 6.28), band = (i % 8) + 1, col = col,
            })
        end
        -- 16 Distant Twinkling Star-Spark Sprites sweeping past outside the sanctum
        for i = 1, 16 do
            local is_front = (i <= 10)
            local sx_p = is_front and rng(-48.0, 48.0) or (((i % 2 == 0) and 1 or -1) * rng(40.0, 86.0))
            local z_min, z_max = -145.0, is_front and -40.0 or 85.0
            local sz = rng(2.2, 4.4)
            spawn_item("sprite", "sprites/env_star_spark.tga", {
                behavior = "star_fly",
                x = sx_p, y = is_front and rng(5.5, 18.0) or rng(6.0, 32.0), z = rng(z_min, z_max),
                z_min = z_min, z_max = z_max,
                sp_w = sz, sp_h = sz, speed = rng(10.0, 22.0), phase = rng(0, 6.28),
                band = (i % 8) + 1, col = (i % 2 == 0) and acc or sec, alpha = 0.94,
            })
        end
        -- 3 Colossal Ringed Planets floating majestically across the distant cosmic skyline
        local planet_pos = {
            {-44.0, 22.0, -92.0, 10.5, sec},
            { 46.0, 18.0, -82.0,  8.5, pri},
            {  2.0, 36.0, -118.0, 12.5, acc},
        }
        for i, pp in ipairs(planet_pos) do
            spawn_item("mesh", "models/env_planet_ringed.obj", {
                behavior = "planet_spin", emissive = true,
                x = pp[1], y = pp[2], z = pp[3],
                rx = rng(-0.32, 0.32), rz = rng(-0.28, 0.28),
                sx = pp[4], sy = pp[4], sz = pp[4],
                speed = rng(0.08, 0.20), phase = i * 1.7, band = i, col = pp[5],
            })
        end
        -- 4 Living Cosmic Whales gliding along an outer orbit well outside the sanctum
        for i = 1, 4 do
            spawn_item("mesh", "models/env_creature_whale.obj", {
                behavior = "orbit_swim", emissive = true,
                radius = 44.0 + i * 8.0, angle = -1.57 + (i - 2.5) * 0.55,
                y = 11.0 + i * 3.0, sx = 4.6, sy = 4.6, sz = 4.6,
                speed = 0.11 + i * 0.02, phase = i * 1.4, amp = 2.0, band = i, col = sec,
            })
        end
        -- 8 Floating Nebula Crystal Asteroids on the outer ring
        for i = 1, 8 do
            local ang = ((i - 1) / 8.0) * 6.283 + 0.38
            local rad = rng(50.0, 82.0)
            spawn_item("mesh", "models/env_crystal_cluster.obj", {
                behavior = "crystal_pulse", emissive = true,
                x = math.cos(ang) * rad, y = rng(6.0, 22.0), z = math.sin(ang) * rad,
                rx = rng(-0.5, 0.5), ry = ang, sx = 4.5, sy = 5.5, sz = 4.5,
                phase = i * 1.1, band = (i % 8) + 1, col = (i % 2 == 0) and pri or acc,
            })
        end

    elseif th == "emerald_cavern" then
        -- 20 Colossal Faceted Emerald Crystal Cliffs ringing the distant cavern chasm (R = 40m..84m)
        for i = 1, 20 do
            local ang = ((i - 1) / 20.0) * 6.283 + rng(-0.06, 0.06)
            local inner = (i % 2 == 0)
            local rad = inner and rng(40.0, 52.0) or rng(62.0, 86.0)
            local sc_h = inner and rng(8.5, 13.5) or rng(14.0, 21.0)
            local sc_w = inner and rng(5.5, 8.5) or rng(8.5, 13.0)
            spawn_item("mesh", "models/env_crystal_cluster.obj", {
                behavior = "crystal_pulse", emissive = true,
                x = math.cos(ang) * rad, y = -4.5, z = math.sin(ang) * rad,
                rx = rng(-0.14, 0.14), ry = ang + rng(-0.4, 0.4), rz = rng(-0.14, 0.14),
                sx = sc_w, sy = sc_h, sz = sc_w,
                phase = rng(0, 6.28), band = (i % 8) + 1,
                col = (i % 3 == 0) and sec or pri,
            })
        end
        -- 16 Monumental Hanging Cavern Stalactites crowning the outer cave dome
        for i = 1, 16 do
            local ang = ((i - 1) / 16.0) * 6.283 + 0.18
            local rad = rng(42.0, 78.0)
            local sc = rng(6.5, 11.5)
            spawn_item("mesh", "models/env_cave_stalactite.obj", {
                behavior = "crystal_pulse", emissive = (i % 2 == 0),
                x = math.cos(ang) * rad, y = rng(28.0, 42.0), z = math.sin(ang) * rad,
                ry = rng(0, 6.28), sx = sc * 0.85, sy = sc * 1.25, sz = sc * 0.85,
                phase = rng(0, 6.28), band = (i % 8) + 1, col = deep_pri,
            })
        end
        -- 6 Colossal Voxel Emerald Archways framing the outer cavern perimeter
        for i = 1, 6 do
            local ang = ((i - 1) / 6.0) * 6.283 + 0.52
            spawn_item("voxel", "voxels/env_emerald_arch.gfvox", {
                behavior = "crystal_pulse",
                x = math.cos(ang) * 52.0, y = -2.0, z = math.sin(ang) * 52.0,
                ry = -ang + 1.57, sx = 8.5, sy = 10.5, sz = 6.5,
                phase = i * 1.0, band = i, col = pri,
            })
        end
        -- 10 Champagne-Gold Luminous Cave Blossoms crowning the distant cliff ledges
        for i = 1, 10 do
            local ang = ((i - 1) / 10.0) * 6.283 + 0.28
            local rad = rng(38.0, 50.0)
            spawn_item("mesh", "models/env_flora_blossom.obj", {
                behavior = "flora_bloom", emissive = true,
                x = math.cos(ang) * rad, y = -0.8, z = math.sin(ang) * rad,
                ry = rng(0, 6.28), sx = 4.5, sy = 4.5, sz = 4.5,
                phase = i * 0.8, band = (i % 8) + 1, col = acc,
            })
        end
        -- 6 Warm Champagne-Gold Choir Seraph Birds soaring high across the outer cavern
        for i = 1, 6 do
            spawn_item("mesh", "models/env_creature_bird.obj", {
                behavior = "orbit_fly", emissive = true,
                radius = 40.0 + i * 5.0, angle = -1.57 + (i - 3.5) * 0.45,
                y = 10.5 + i * 2.0, sx = 3.2, sy = 3.2, sz = 3.2,
                speed = 0.24 + i * 0.03, phase = i * 1.5, amp = 1.6, band = i, col = acc,
            })
        end
        -- 14 Distant Shimmering Sanctuary Wisps drifting in the cavern abyss
        for i = 1, 14 do
            local ang = rng(0, 6.283)
            local rad = rng(38.0, 68.0)
            spawn_item("sprite", "sprites/env_butterfly_wisp.tga", {
                behavior = "wisp_float",
                x = math.cos(ang) * rad, y = rng(6.0, 22.0), z = math.sin(ang) * rad,
                sp_w = 2.6, sp_h = 2.6, speed = rng(0.5, 1.1), phase = rng(0, 6.28),
                amp = 2.2, band = (i % 8) + 1, col = acc, alpha = 0.90,
            })
        end

    elseif th == "biolum_ocean" then
        -- 20 Towering Deep-Sea Kelp Forests swaying far beyond the sanctum (R = 40m..78m)
        for i = 1, 20 do
            local ang = ((i - 1) / 20.0) * 6.283 + rng(-0.06, 0.06)
            local rad = rng(40.0, 78.0)
            spawn_item("mesh", "models/env_kelp_frond.obj", {
                behavior = "plant_sway",
                x = math.cos(ang) * rad, y = -6.0, z = math.sin(ang) * rad,
                ry = ang, sx = rng(4.5, 7.5), sy = rng(7.0, 12.5), sz = rng(4.5, 7.5),
                speed = rng(0.55, 1.05), phase = rng(0, 6.28), band = (i % 8) + 1, col = pri,
            })
        end
        -- 10 Sculpted Coral-Rose & Aquamarine Bioluminescent Jellyfish drifting in the outer sea
        for i = 1, 10 do
            local ang = ((i - 1) / 10.0) * 6.283 - 1.57 + rng(-0.12, 0.12)
            local rad = (i <= 5) and rng(40.0, 54.0) or rng(58.0, 76.0)
            local sc = rng(3.8, 6.2)
            spawn_item("mesh", "models/env_creature_jelly.obj", {
                behavior = "jelly_pulse", emissive = true,
                x = math.cos(ang) * rad, y = rng(10.0, 22.0), z = math.sin(ang) * rad,
                sx = sc, sy = sc, sz = sc, speed = rng(0.9, 1.6), phase = rng(0, 6.28),
                amp = 1.8, band = (i % 8) + 1, col = (i % 2 == 0) and acc or sec,
            })
        end
        -- 5 Colossal Abyssal Whales cruising around the sanctum perimeter
        for i = 1, 5 do
            spawn_item("mesh", "models/env_creature_whale.obj", {
                behavior = "orbit_swim", emissive = true,
                radius = 46.0 + i * 7.5, angle = -1.57 + (i - 2.5) * 0.50,
                y = 11.0 + i * 2.8, sx = 5.5, sy = 5.5, sz = 5.5,
                speed = 0.12 + i * 0.02, phase = i * 1.3, amp = 1.8, band = i, col = sec,
            })
        end
        -- 10 Glowing Deep-Sea Coral Blossoms along the distant shelf
        for i = 1, 10 do
            local ang = ((i - 1) / 10.0) * 6.283 + 0.35
            local rad = rng(39.0, 56.0)
            spawn_item("mesh", "models/env_flora_blossom.obj", {
                behavior = "flora_bloom", emissive = true,
                x = math.cos(ang) * rad, y = -2.0, z = math.sin(ang) * rad,
                ry = rng(0, 6.28), sx = 5.0, sy = 5.0, sz = 5.0,
                phase = i * 0.9, band = (i % 8) + 1, col = acc,
            })
        end
        -- 18 Rising Bioluminescent Ocean Bubble Columns outside the sanctum
        for i = 1, 18 do
            local ang = rng(0, 6.283)
            local rad = rng(38.0, 74.0)
            spawn_item("sprite", "sprites/env_bubble_spore.tga", {
                behavior = "bubble_rise",
                x = math.cos(ang) * rad, y = rng(2.0, 32.0), z = math.sin(ang) * rad,
                sp_w = rng(1.8, 3.6), sp_h = rng(1.8, 3.6),
                speed = rng(2.2, 4.8), phase = rng(0, 6.28), band = (i % 8) + 1, col = sec, alpha = 0.86,
            })
        end

    elseif th == "enchanted_forest" then
        -- 18 Towering Ancient Enchanted Trees framing a vast twilight forest beyond the sanctum
        for i = 1, 18 do
            local ang = ((i - 1) / 18.0) * 6.283 + rng(-0.07, 0.07)
            local inner = (i % 2 == 0)
            local rad = inner and rng(40.0, 54.0) or rng(62.0, 86.0)
            local sc = inner and rng(6.5, 9.5) or rng(9.5, 13.5)
            spawn_item("mesh", "models/env_tree_canopy.obj", {
                behavior = "plant_sway",
                x = math.cos(ang) * rad, y = -4.0, z = math.sin(ang) * rad,
                ry = rng(0, 6.28), sx = sc, sy = sc * rng(0.95, 1.3), sz = sc,
                speed = rng(0.4, 0.85), phase = rng(0, 6.28), band = (i % 8) + 1, col = pri,
            })
        end
        -- 12 Soft Rose & Honey-Amber Woodland Blossoms along the outer meadow
        for i = 1, 12 do
            local ang = ((i - 1) / 12.0) * 6.283 + 0.18
            local rad = rng(38.0, 54.0)
            spawn_item("mesh", "models/env_flora_blossom.obj", {
                behavior = "flora_bloom", emissive = true,
                x = math.cos(ang) * rad, y = -1.2, z = math.sin(ang) * rad,
                ry = rng(0, 6.28), sx = rng(4.2, 6.2), sy = rng(4.2, 6.2), sz = rng(4.2, 6.2),
                phase = i * 0.7, band = (i % 8) + 1, col = (i % 2 == 0) and sec or acc,
            })
        end
        -- 6 Soaring Woodland Birds gliding above the distant tree canopy
        for i = 1, 6 do
            spawn_item("mesh", "models/env_creature_bird.obj", {
                behavior = "orbit_fly", emissive = true,
                radius = 42.0 + i * 5.0, angle = -1.57 + (i - 3.5) * 0.48,
                y = 12.0 + i * 2.2, sx = 3.4, sy = 3.4, sz = 3.4,
                speed = 0.26 + i * 0.04, phase = i * 1.2, amp = 1.8, band = i, col = acc,
            })
        end
        -- 8 Distant Mossy Crystal Standing Stones
        for i = 1, 8 do
            local ang = ((i - 1) / 8.0) * 6.283 + 0.42
            spawn_item("mesh", "models/env_crystal_cluster.obj", {
                behavior = "crystal_pulse", emissive = true,
                x = math.cos(ang) * 50.0, y = -2.5, z = math.sin(ang) * 50.0,
                ry = ang, sx = 4.5, sy = 6.5, sz = 4.5,
                phase = i * 1.0, band = i, col = sec,
            })
        end
        -- 18 Golden Firefly & Butterfly Wisps dancing in the distant forest clearing
        for i = 1, 18 do
            local ang = rng(0, 6.283)
            local rad = rng(38.0, 72.0)
            spawn_item("sprite", "sprites/env_butterfly_wisp.tga", {
                behavior = "wisp_float",
                x = math.cos(ang) * rad, y = rng(5.0, 22.0), z = math.sin(ang) * rad,
                sp_w = 2.6, sp_h = 2.6, speed = rng(0.6, 1.2), phase = rng(0, 6.28),
                amp = 2.5, band = (i % 8) + 1, col = acc, alpha = 0.92,
            })
        end

    elseif th == "winged_sky_sanctum" then
        -- 12 Soaring Burnished-Gold & Pearl Seraphs in grand aerial formations
        for i = 1, 12 do
            spawn_item("mesh", "models/env_creature_bird.obj", {
                behavior = "orbit_fly", emissive = true,
                radius = 40.0 + (i % 4) * 9.0, angle = -1.57 + (i - 6.5) * 0.35,
                y = 10.5 + (i % 5) * 3.5, sx = rng(3.4, 5.2), sy = rng(3.4, 5.2), sz = rng(3.4, 5.2),
                speed = 0.22 + (i % 4) * 0.05, phase = i * 0.9, amp = 2.2, band = (i % 8) + 1,
                col = (i % 2 == 0) and acc or pri,
            })
        end
        -- 12 Monumental Floating Alabaster Sky Cathedral Pillars
        for i = 1, 12 do
            local ang = ((i - 1) / 12.0) * 6.283
            local rad = rng(44.0, 78.0)
            spawn_item("mesh", "models/env_ruin_pillar.obj", {
                behavior = "crystal_pulse",
                x = math.cos(ang) * rad, y = rng(-3.0, 6.0), z = math.sin(ang) * rad,
                ry = ang, sx = 5.2, sy = rng(7.5, 11.5), sz = 5.2,
                phase = i * 0.6, band = (i % 8) + 1, col = pri,
            })
        end
        -- 8 Radiant Apricot-Gold Halo Crystal Spires
        for i = 1, 8 do
            local ang = ((i - 1) / 8.0) * 6.283 + 0.35
            spawn_item("mesh", "models/env_crystal_cluster.obj", {
                behavior = "crystal_pulse", emissive = true,
                x = math.cos(ang) * 58.0, y = 8.0 + (i % 2) * 6.0, z = math.sin(ang) * 58.0,
                ry = ang, sx = 4.8, sy = 7.5, sz = 4.8,
                phase = i * 1.1, band = i, col = sec,
            })
        end
        -- 10 Cloud-Terrace Blossoms & 16 Starlight Motes
        for i = 1, 10 do
            local ang = ((i - 1) / 10.0) * 6.283 + 0.2
            spawn_item("mesh", "models/env_flora_blossom.obj", {
                behavior = "flora_bloom", emissive = true,
                x = math.cos(ang) * 41.0, y = -1.0, z = math.sin(ang) * 41.0,
                sx = 4.6, sy = 4.6, sz = 4.6, phase = i * 0.8, band = (i % 8) + 1, col = sec,
            })
        end
        for i = 1, 16 do
            local ang = rng(0, 6.283)
            local rad = rng(38.0, 78.0)
            spawn_item("sprite", (i % 2 == 0) and "sprites/env_star_spark.tga" or "sprites/env_butterfly_wisp.tga", {
                behavior = "wisp_float",
                x = math.cos(ang) * rad, y = rng(7.0, 30.0), z = math.sin(ang) * rad,
                sp_w = 2.8, sp_h = 2.8, speed = rng(0.5, 1.1), phase = rng(0, 6.28),
                amp = 2.6, band = (i % 8) + 1, col = acc, alpha = 0.92,
            })
        end

    elseif th == "ancient_desert_temple" then
        -- 16 Colossal Sandstone Temple Monoliths ringing the desert horizon
        for i = 1, 16 do
            local ang = ((i - 1) / 16.0) * 6.283
            local inner = (i % 2 == 0)
            local rad = inner and rng(42.0, 54.0) or rng(64.0, 88.0)
            spawn_item("mesh", "models/env_ruin_pillar.obj", {
                behavior = "static",
                x = math.cos(ang) * rad, y = -4.0, z = math.sin(ang) * rad,
                ry = ang, sx = rng(5.0, 7.5), sy = rng(7.5, 13.0), sz = rng(5.0, 7.5),
                phase = i, band = (i % 8) + 1, col = pri,
            })
        end
        -- 8 Lapis-Turquoise & Amber Sacred Crystal Braziers (classic desert-temple contrast)
        for i = 1, 8 do
            local ang = ((i - 1) / 8.0) * 6.283 + 0.25
            spawn_item("mesh", "models/env_crystal_cluster.obj", {
                behavior = "crystal_pulse", emissive = true,
                x = math.cos(ang) * 46.0, y = -1.5, z = math.sin(ang) * 46.0,
                ry = ang, sx = 4.8, sy = 6.8, sz = 4.8,
                phase = i * 0.9, band = i, col = (i % 2 == 0) and sec or acc,
            })
        end
        -- 8 Distant Oasis Palms & Canyon Trees
        for i = 1, 8 do
            local ang = ((i - 1) / 8.0) * 6.283 + 0.5
            spawn_item("mesh", "models/env_tree_canopy.obj", {
                behavior = "plant_sway",
                x = math.cos(ang) * 56.0, y = -3.0, z = math.sin(ang) * 56.0,
                ry = rng(0, 6.28), sx = 6.5, sy = 8.2, sz = 6.5,
                speed = 0.55, phase = i * 1.1, band = i, col = {0.26, 0.46, 0.34},
            })
        end
        -- 6 Soaring Desert Hawks circling over the monuments
        for i = 1, 6 do
            spawn_item("mesh", "models/env_creature_bird.obj", {
                behavior = "orbit_fly", emissive = true,
                radius = 44.0 + i * 6.0, angle = -1.57 + (i - 3.5) * 0.45,
                y = 14.0 + i * 2.4, sx = 3.8, sy = 3.8, sz = 3.8,
                speed = 0.22 + i * 0.03, phase = i * 1.3, amp = 2.0, band = i, col = acc,
            })
        end
        -- 4 Giant Solar Astrolabe Rings suspended over the distant dunes
        for i = 1, 4 do
            local ang = ((i - 1) / 4.0) * 6.283 - 0.78
            spawn_item("mesh", "models/env_gear_astrolabe.obj", {
                behavior = "gear_spin", emissive = true,
                x = math.cos(ang) * 72.0, y = 22.0 + (i % 2) * 7.0, z = math.sin(ang) * 72.0,
                ry = -ang + 1.57, sx = 7.5, sy = 7.5, sz = 7.5,
                speed = (i % 2 == 0) and 0.25 or -0.25, phase = i, band = i, col = acc,
            })
        end
        -- 14 Rising Desert Ember Sparks
        for i = 1, 14 do
            local ang = rng(0, 6.283)
            local rad = rng(38.0, 74.0)
            spawn_item("sprite", "sprites/env_star_spark.tga", {
                behavior = "bubble_rise",
                x = math.cos(ang) * rad, y = rng(2.0, 28.0), z = math.sin(ang) * rad,
                sp_w = 2.4, sp_h = 2.4, speed = rng(2.4, 5.2), phase = rng(0, 6.28),
                band = (i % 8) + 1, col = acc, alpha = 0.90,
            })
        end

    else -- cyber_clockwork
        -- 12 Colossal Rotating Brushed-Gold & Cyan Clockwork Astrolabe Gears
        for i = 1, 12 do
            local ang = ((i - 1) / 12.0) * 6.283
            local rad = (i % 2 == 0) and 48.0 or 72.0
            local sc = rng(5.5, 9.5)
            spawn_item("mesh", "models/env_gear_astrolabe.obj", {
                behavior = "gear_spin", emissive = true,
                x = math.cos(ang) * rad, y = rng(10.0, 26.0), z = math.sin(ang) * rad,
                ry = -ang + 1.57, sx = sc, sy = sc, sz = sc,
                speed = ((i % 2 == 0) and 0.38 or -0.38) * rng(0.7, 1.3),
                phase = i * 0.7, band = (i % 8) + 1, col = (i % 2 == 0) and pri or sec,
            })
        end
        -- 10 Monumental Voxel Cyber Monolith Towers on the distant skyline
        for i = 1, 10 do
            local ang = ((i - 1) / 10.0) * 6.283 + 0.31
            local rad = rng(44.0, 76.0)
            spawn_item("voxel", "voxels/env_cyber_tower.gfvox", {
                behavior = "crystal_pulse",
                x = math.cos(ang) * rad, y = -4.0, z = math.sin(ang) * rad,
                ry = -ang, sx = 7.2, sy = rng(8.0, 14.0), sz = 7.2,
                phase = i * 0.8, band = (i % 8) + 1, col = acc,
            })
        end
        -- 6 Orbiting Cyber-Bird Drones sweeping outside the sanctum
        for i = 1, 6 do
            spawn_item("mesh", "models/env_creature_bird.obj", {
                behavior = "orbit_fly", emissive = true,
                radius = 42.0 + i * 5.5, angle = -1.57 + (i - 3.5) * 0.45,
                y = 12.0 + i * 2.0, sx = 3.4, sy = 3.4, sz = 3.4,
                speed = 0.36 + i * 0.05, phase = i * 1.1, amp = 1.8, band = i, col = sec,
            })
        end
        -- 16 Distant Data-Stream Light Streaks rushing past outside R >= 38m
        for i = 1, 16 do
            local side = (i % 2 == 0) and 1.0 or -1.0
            local z_min, z_max = -140.0, 85.0
            spawn_item("mesh", "models/env_star_streak.obj", {
                behavior = "star_fly", emissive = true,
                x = side * rng(39.0, 84.0), y = rng(6.0, 30.0), z = rng(z_min, z_max),
                z_min = z_min, z_max = z_max,
                sx = 2.2, sy = 2.2, sz = 5.5,
                speed = rng(20.0, 38.0), phase = rng(0, 6.28), band = (i % 8) + 1,
                col = (i % 2 == 0) and sec or acc,
            })
        end
        -- 8 Holographic Crystal Conduits along the outer perimeter
        for i = 1, 8 do
            local ang = ((i - 1) / 8.0) * 6.283 + 0.15
            spawn_item("mesh", "models/env_crystal_cluster.obj", {
                behavior = "crystal_pulse", emissive = true,
                x = math.cos(ang) * 42.0, y = -2.0, z = math.sin(ang) * 42.0,
                ry = ang, sx = 4.2, sy = 6.8, sz = 4.2,
                phase = i, band = i, col = sec,
            })
        end
    end

    -- Compound Living Creature / Plant Overlays when preset name explicitly calls for them
    if spec.wants_whale and th ~= "biolum_ocean" and th ~= "starfall_cosmos" then
        for i = 1, 3 do
            spawn_item("mesh", "models/env_creature_whale.obj", {
                behavior = "orbit_swim", emissive = true,
                radius = 48.0 + i * 8.0, angle = -1.57 + (i - 2) * 0.8,
                y = 13.0 + i * 3.0, sx = 5.0, sy = 5.0, sz = 5.0,
                speed = 0.12, phase = i * 1.5, amp = 2.0, band = i, col = sec,
            })
        end
    end
    if spec.wants_jelly and th ~= "biolum_ocean" then
        for i = 1, 4 do
            local ang = -1.57 + (i - 2.5) * 0.65
            spawn_item("mesh", "models/env_creature_jelly.obj", {
                behavior = "jelly_pulse", emissive = true,
                x = math.cos(ang) * 46.0, y = 12.0 + i * 2.2, z = math.sin(ang) * 46.0,
                sx = 4.5, sy = 4.5, sz = 4.5, speed = 1.2, phase = i, amp = 1.6, band = i, col = acc,
            })
        end
    end
    if spec.wants_flora and th ~= "enchanted_forest" and th ~= "emerald_cavern" and th ~= "biolum_ocean" then
        for i = 1, 6 do
            local ang = ((i - 1) / 6.0) * 6.283 + 0.3
            spawn_item("mesh", "models/env_flora_blossom.obj", {
                behavior = "flora_bloom", emissive = true,
                x = math.cos(ang) * 41.0, y = -1.0, z = math.sin(ang) * 41.0,
                sx = 4.5, sy = 4.5, sz = 4.5, phase = i, band = i, col = acc,
            })
        end
    end

    PresetWorlds.update(0.016, nil, {energy = 0.0, pitch_norm = 0.5, strike = 0.35})
    return spec.theme
end

function PresetWorlds.update(dt, tel, music)
    PresetWorlds.time = PresetWorlds.time + (dt or 0.016)
    local t = PresetWorlds.time

    local rms = tel and ((tel.rms_l or 0) + (tel.rms_r or 0)) * 0.5 or 0.0
    local energy = clamp((music and music.energy or 0.0) + rms * 2.2, 0.0, 1.0)
    local pitch  = clamp(music and music.pitch_norm or 0.5, 0.0, 1.0)
    local strike = clamp(music and music.strike or 0.0, 0.0, 1.0)
    local bands  = tel and tel.bands or nil

    gf.scene.set_music_state(energy, pitch, strike)

    -- Tasteful pitch-reactive warmth/coolness shimmer & keystroke luminance bloom
    local p_shift = (pitch - 0.5) * 0.18
    local glow = 0.88 + energy * 0.22 + strike * 0.30
    local sp_frame = math.floor(t * (7.0 + energy * 7.0 + strike * 5.0)) % 4
    local min_r = PresetWorlds.min_clearance

    for _, it in ipairs(PresetWorlds.items) do
        local b_val = (bands and bands[it.band]) and clamp(bands[it.band] * 2.8, 0.0, 1.0) or energy
        local beh = it.behavior
        local px, py, pz = it.ox, it.oy, it.oz
        local rx, ry, rz = it.rx, it.ry, it.rz
        local sx, sy, sz = it.sx, it.sy, it.sz

        if beh == "star_fly" then
            local warp = 1.0 + energy * 2.2 + strike * 3.4 + pitch * 0.8
            it.oz = it.oz + it.speed * warp * (dt or 0.016)
            if it.oz > it.z_max then
                it.oz = it.z_min - (it.phase * 2.5)
            end
            pz = it.oz
            py = it.oy + math.sin(t * 0.7 + it.phase) * 0.9 + (pitch - 0.5) * 4.5
            sz = it.sz * (1.0 + energy * 1.2 + strike * 2.2)
        elseif beh == "crystal_pulse" then
            local pulse = 1.0 + b_val * 0.16 + strike * 0.14 + math.sin(t * 1.4 + it.phase) * 0.03
            sx = it.sx * (1.0 + strike * 0.06)
            sy = it.sy * pulse
            sz = it.sz * (1.0 + strike * 0.06)
        elseif beh == "planet_spin" then
            ry = it.ry + t * (it.speed + energy * 0.20)
            py = it.oy + math.sin(t * 0.35 + it.phase) * 1.1 + (pitch - 0.5) * 3.0
            local ps = 1.0 + strike * 0.05 + energy * 0.04
            sx, sy, sz = it.sx * ps, it.sy * ps, it.sz * ps
        elseif beh == "orbit_swim" then
            local ang = it.angle + t * (it.speed + energy * 0.14 + strike * 0.10)
            px = math.cos(ang) * it.radius
            pz = math.sin(ang) * it.radius
            py = it.oy + math.sin(t * 0.9 + it.phase) * it.amp + (pitch - 0.5) * 5.0 + strike * 1.2
            ry = -ang
            rx = math.cos(t * 0.9 + it.phase) * 0.12
            rz = math.sin(t * 1.9 + it.phase) * (0.06 + energy * 0.14)
        elseif beh == "orbit_fly" then
            local ang = it.angle + t * (it.speed + energy * 0.24 + strike * 0.20)
            px = math.cos(ang) * it.radius
            pz = math.sin(ang) * it.radius
            local flap = math.sin(t * (7.0 + energy * 5.5 + strike * 7.0) + it.phase)
            py = it.oy + math.sin(t * 1.2 + it.phase) * it.amp + flap * 0.35 + (pitch - 0.5) * 5.2 + strike * 1.6
            ry = -ang
            rz = flap * (0.20 + strike * 0.22)
            sy = it.sy * (0.86 + math.abs(flap) * 0.32)
        elseif beh == "jelly_pulse" then
            local beat = math.sin(t * (it.speed + energy * 1.3) + it.phase)
            local contract = clamp(beat * 0.5 + 0.5 + strike * 0.50, 0.0, 1.25)
            py = it.oy + math.sin(t * 0.8 + it.phase) * it.amp + (pitch - 0.5) * 4.5 + strike * 1.4
            sx = it.sx * (1.12 - contract * 0.22)
            sy = it.sy * (0.86 + contract * 0.30)
            sz = it.sz * (1.12 - contract * 0.22)
            ry = t * 0.3 + it.phase
        elseif beh == "plant_sway" then
            local sway = math.sin(t * (it.speed + energy * 0.8) + it.phase) * (0.06 + energy * 0.11 + strike * 0.09)
            rx = sway
            rz = math.cos(t * it.speed * 0.8 + it.phase) * (0.05 + energy * 0.08)
            sy = it.sy * (1.0 + b_val * 0.09 + strike * 0.06)
        elseif beh == "flora_bloom" then
            local bl = 1.0 + b_val * 0.28 + strike * 0.24 + math.sin(t * 1.8 + it.phase) * 0.06
            sx, sy, sz = it.sx * bl, it.sy * (0.95 + bl * 0.12), it.sz * bl
            ry = it.ry + t * (0.20 + energy * 0.4)
        elseif beh == "gear_spin" then
            rz = it.rz + t * (it.speed * (1.0 + energy * 1.6 + strike * 1.4))
            local gs = 1.0 + strike * 0.07
            sx, sy, sz = it.sx * gs, it.sy * gs, it.sz * gs
        elseif beh == "wisp_float" then
            px = it.ox + math.cos(t * it.speed + it.phase) * 2.2
            pz = it.oz + math.sin(t * it.speed * 0.8 + it.phase) * 2.2
            py = it.oy + math.sin(t * (it.speed * 1.2) + it.phase) * it.amp + (pitch - 0.5) * 4.2 + strike * 1.6
            local ws = 1.0 + energy * 0.40 + strike * 0.40
            sx, sy = ws, ws
        elseif beh == "bubble_rise" then
            it.oy = it.oy + it.speed * (1.0 + energy * 1.6 + strike * 2.0) * (dt or 0.016)
            if it.oy > 34.0 then it.oy = 1.5 end
            py = it.oy
            px = it.ox + math.sin(t * 1.5 + it.phase) * 1.2
            local bs = 1.0 + energy * 0.35 + strike * 0.30
            sx, sy = bs, bs
        end

        px, pz = enforce_clearance(px, pz, min_r)
        it.wx, it.wz = px, pz
        gf.scene.set_transform(it.id, px, py, pz, rx, ry, rz, sx, sy, sz)
        if it.kind == "sprite" then
            gf.scene.set_sprite_anim(it.id, it.sp_w, it.sp_h, 4, (sp_frame + math.floor(it.phase)) % 4)
        end
        if it.emissive or it.kind == "sprite" then
            local cr = clamp(it.col[1] * glow - p_shift * 0.25, 0.05, 1.0)
            local cg = clamp(it.col[2] * glow + b_val * 0.10, 0.05, 1.0)
            local cb = clamp(it.col[3] * glow + p_shift * 0.32, 0.05, 1.0)
            gf.scene.set_tint(it.id, cr, cg, cb, it.alpha)
        end
    end
end

return PresetWorlds
