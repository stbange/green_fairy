-- Preset-Inspired, Music-Reactive 3D World Surroundings Engine
-- Dynamically transforms the 3D surroundings (non-living structures, living plants,
-- living animals, sky gradient, fog, and architecture harmony) for every preset name
-- and animates them continuously in response to keystrokes, volume, spectrum & pitch.

local PresetWorlds = {
    theme = "emerald_cavern",
    preset_name = "Emerald Sanctum Choir",
    items = {},
    room_centers = {},
    anchor_x = 0.0,
    anchor_z = 0.0,
    time = 0.0,
    base_rgb = {0.16, 0.95, 0.58},
    accent_rgb = {0.42, 1.0, 0.84},
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
        "petal", "garden", "wisp", "wormwood", "canopy", "leaf", "forest"
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

local PALETTE_OVERRIDES = {
    {words = {"emerald", "jade", "verdant", "wormwood", "moss"},
     pri = {0.10, 0.96, 0.56}, acc = {0.36, 1.00, 0.80}},
    {words = {"amethyst", "violet", "purple", "twilight", "nebula"},
     pri = {0.66, 0.34, 1.00}, acc = {0.92, 0.54, 1.00}},
    {words = {"sapphire", "azure", "cobalt", "ice", "frost", "glacier", "arctic"},
     pri = {0.24, 0.78, 1.00}, acc = {0.68, 0.94, 1.00}},
    {words = {"ruby", "crimson", "ember", "flame", "fire", "phoenix", "magma"},
     pri = {1.00, 0.34, 0.22}, acc = {1.00, 0.74, 0.32}},
    {words = {"gold", "amber", "sun", "solar", "dawn", "bronze", "citrine", "arrakis"},
     pri = {1.00, 0.76, 0.24}, acc = {1.00, 0.92, 0.54}},
    {words = {"rose", "blossom", "lotus", "coral", "sakura", "pink"},
     pri = {1.00, 0.44, 0.74}, acc = {1.00, 0.78, 0.90}},
    {words = {"silver", "moon", "pearl", "quartz", "glass", "opal"},
     pri = {0.72, 0.92, 1.00}, acc = {0.92, 0.98, 1.00}},
    {words = {"obsidian", "void", "shadow", "eclipse", "midnight"},
     pri = {0.42, 0.28, 0.92}, acc = {0.72, 0.46, 1.00}},
}

local BIOME_ATMOSPHERE = {
    emerald_cavern = {
        zen = {0.012, 0.055, 0.048}, hor = {0.035, 0.210, 0.145},
        fog = {0.022, 0.135, 0.098}, dens = 0.021,
        pri = {0.10, 0.96, 0.56},    acc = {0.40, 1.00, 0.80},
    },
    starfall_cosmos = {
        zen = {0.008, 0.012, 0.055}, hor = {0.068, 0.042, 0.165},
        fog = {0.028, 0.022, 0.092}, dens = 0.007,
        pri = {0.42, 0.78, 1.00},    acc = {0.92, 0.62, 1.00},
    },
    biolum_ocean = {
        zen = {0.008, 0.032, 0.095}, hor = {0.024, 0.165, 0.255},
        fog = {0.015, 0.105, 0.175}, dens = 0.024,
        pri = {0.12, 0.88, 0.98},    acc = {0.36, 1.00, 0.82},
    },
    enchanted_forest = {
        zen = {0.026, 0.072, 0.115}, hor = {0.072, 0.215, 0.155},
        fog = {0.045, 0.140, 0.112}, dens = 0.020,
        pri = {0.24, 0.88, 0.46},    acc = {0.88, 0.96, 0.48},
    },
    winged_sky_sanctum = {
        zen = {0.075, 0.055, 0.175}, hor = {0.240, 0.155, 0.275},
        fog = {0.145, 0.105, 0.215}, dens = 0.014,
        pri = {0.96, 0.82, 0.54},    acc = {0.72, 0.88, 1.00},
    },
    ancient_desert_temple = {
        zen = {0.135, 0.058, 0.040}, hor = {0.345, 0.185, 0.075},
        fog = {0.220, 0.120, 0.060}, dens = 0.018,
        pri = {0.96, 0.72, 0.30},    acc = {1.00, 0.46, 0.24},
    },
    cyber_clockwork = {
        zen = {0.018, 0.025, 0.095}, hor = {0.145, 0.045, 0.220},
        fog = {0.072, 0.035, 0.148}, dens = 0.016,
        pri = {0.22, 0.94, 0.92},    acc = {1.00, 0.32, 0.78},
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
    local acc = {atm.acc[1], atm.acc[2], atm.acc[3]}
    for _, ov in ipairs(PALETTE_OVERRIDES) do
        if match_any(lower, ov.words) then
            pri = {ov.pri[1], ov.pri[2], ov.pri[3]}
            acc = {ov.acc[1], ov.acc[2], ov.acc[3]}
            break
        end
    end

    -- Subtle deterministic per-preset hue nuance so all 320+ presets feel distinct
    local seed = hash_string(name or "Sanctum")
    local n1 = ((seed % 100) / 100.0 - 0.5) * 0.16
    local n2 = (((math.floor(seed / 100)) % 100) / 100.0 - 0.5) * 0.16
    pri[1] = clamp(pri[1] + n1, 0.08, 1.0)
    pri[2] = clamp(pri[2] + n2 * 0.7, 0.08, 1.0)
    pri[3] = clamp(pri[3] - n1 * 0.7, 0.08, 1.0)

    local zen = {
        clamp(atm.zen[1] * 0.75 + pri[1] * 0.06, 0.005, 0.35),
        clamp(atm.zen[2] * 0.75 + pri[2] * 0.06, 0.005, 0.35),
        clamp(atm.zen[3] * 0.75 + pri[3] * 0.06, 0.005, 0.35),
    }
    local hor = {
        clamp(atm.hor[1] * 0.65 + pri[1] * 0.16, 0.015, 0.55),
        clamp(atm.hor[2] * 0.65 + pri[2] * 0.16, 0.015, 0.55),
        clamp(atm.hor[3] * 0.65 + pri[3] * 0.16, 0.015, 0.55),
    }
    local fog = {
        clamp(atm.fog[1] * 0.65 + pri[1] * 0.10, 0.010, 0.45),
        clamp(atm.fog[2] * 0.65 + pri[2] * 0.10, 0.010, 0.45),
        clamp(atm.fog[3] * 0.65 + pri[3] * 0.10, 0.010, 0.45),
    }
    return {
        theme = theme,
        seed = seed,
        pri = pri,
        acc = acc,
        zen = zen,
        hor = hor,
        fog = fog,
        dens = atm.dens,
        wants_whale = match_any(lower, {"whale", "leviathan", "siren"}),
        wants_jelly = match_any(lower, {"jelly", "medusa", "squid"}),
        wants_bird  = match_any(lower, {"bird", "wing", "phoenix", "raven", "owl", "seraph", "choir"}),
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
        gf.scene.set_sprite_anim(id, cfg.sp_w or 0.9, cfg.sp_h or 0.9, 4, cfg.frame or 0)
    end
    local col = cfg.col or {0.4, 0.95, 0.75}
    gf.scene.set_tint(id, col[1], col[2], col[3], cfg.alpha or 1.0)

    local it = {
        id = id,
        kind = kind,
        behavior = cfg.behavior or "static",
        ox = cfg.x or 0.0,
        oy = cfg.y or 0.0,
        oz = cfg.z or 0.0,
        rx = cfg.rx or 0.0,
        ry = cfg.ry or 0.0,
        rz = cfg.rz or 0.0,
        sx = cfg.sx or 1.0,
        sy = cfg.sy or 1.0,
        sz = cfg.sz or 1.0,
        radius = cfg.radius or 12.0,
        angle = cfg.angle or 0.0,
        speed = cfg.speed or 0.4,
        phase = cfg.phase or 0.0,
        amp = cfg.amp or 0.5,
        band = cfg.band or 1,
        sp_w = cfg.sp_w or 0.9,
        sp_h = cfg.sp_h or 0.9,
        col = {col[1], col[2], col[3]},
        alpha = cfg.alpha or 1.0,
        emissive = cfg.emissive == true,
    }
    PresetWorlds.items[#PresetWorlds.items + 1] = it
    return it
end

local function cache_room_centers()
    if #PresetWorlds.room_centers > 0 then return end
    local rms = gf.spatial.rooms()
    for _, r in ipairs(rms) do
        PresetWorlds.room_centers[r.id] = {x = r.x, z = r.z}
    end
end

function PresetWorlds.apply(name, realm, floorplan)
    cache_room_centers()
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
        floorplan.apply_world_tint(spec.pri[1], spec.pri[2], spec.pri[3])
    end

    local rng = make_rng(spec.seed)
    local pri, acc = spec.pri, spec.acc
    local dark_pri = {pri[1] * 0.52, pri[2] * 0.52, pri[3] * 0.52}
    local th = spec.theme

    if th == "starfall_cosmos" then
        -- 32 Flying Warp Star Streaks rushing past the sanctum (framed through the vaulted arches!)
        for i = 1, 32 do
            local front_band = (i <= 14)
            local ang = rng(0, 6.283)
            local rad = rng(7.6, 22.0)
            local sx_p = front_band and rng(-11.5, 11.5) or (math.cos(ang) * rad)
            local sy_p = front_band and rng(1.6, 5.6) or rng(0.8, 10.5)
            local sc = rng(0.95, 2.2)
            local col = (i % 3 == 0) and acc or ((i % 2 == 0) and {0.92, 0.97, 1.0} or pri)
            spawn_item("mesh", "models/env_star_streak.obj", {
                behavior = "star_fly", emissive = true,
                x = sx_p, y = sy_p, z = rng(-34.0, 10.0),
                sx = sc * 0.75, sy = sc * 0.75, sz = sc * 1.6,
                speed = rng(7.0, 18.0), phase = rng(0, 6.28), band = (i % 8) + 1, col = col,
            })
        end
        -- 16 Twinkling Animated Star-Spark Sprites flying by right through the open arches
        for i = 1, 16 do
            local front_band = (i <= 10)
            local ang = rng(0, 6.283)
            local rad = rng(8.0, 22.0)
            local sz = rng(0.75, 1.55)
            spawn_item("sprite", "sprites/env_star_spark.tga", {
                behavior = "star_fly",
                x = front_band and rng(-10.0, 10.0) or (math.cos(ang) * rad),
                y = front_band and rng(1.8, 5.2) or rng(1.2, 9.5),
                z = rng(-30.0, 8.0),
                sp_w = sz, sp_h = sz, speed = rng(5.0, 13.0), phase = rng(0, 6.28),
                band = (i % 8) + 1, col = (i % 2 == 0) and acc or {1.0, 0.97, 0.86}, alpha = 0.96,
            })
        end
        -- 3 Majestic Ringed Planets framing the sky ahead and overhead
        local planet_pos = {{-8.5, 5.6, -18.5, 2.3}, {9.2, 4.8, -16.5, 1.85}, {0.2, 10.2, -25.0, 2.9}}
        for i, pp in ipairs(planet_pos) do
            spawn_item("mesh", "models/env_planet_ringed.obj", {
                behavior = "planet_spin", emissive = (i <= 2),
                x = pp[1], y = pp[2], z = pp[3],
                rx = rng(-0.35, 0.35), rz = rng(-0.3, 0.3),
                sx = pp[4], sy = pp[4], sz = pp[4],
                speed = rng(0.14, 0.30), phase = i * 1.7, band = i,
                col = (i == 2) and acc or pri,
            })
        end
        -- 4 Living Cosmic Whales gliding peacefully through the nebula arches
        for i = 1, 4 do
            spawn_item("mesh", "models/env_creature_whale.obj", {
                behavior = "orbit_swim", emissive = true,
                radius = 10.5 + i * 2.4, angle = -1.57 + (i - 2) * 0.75,
                y = 2.8 + i * 0.75, sx = 1.45, sy = 1.45, sz = 1.45,
                speed = 0.18 + i * 0.03, phase = i * 1.4, amp = 0.75, band = i, col = acc,
            })
        end
        -- 6 Glowing Asteroid Crystals orbiting in the background
        for i = 1, 6 do
            local ang = (i / 6.0) * 6.283 + 0.3
            spawn_item("mesh", "models/env_crystal_cluster.obj", {
                behavior = "crystal_pulse", emissive = true,
                x = math.cos(ang) * 15.5, y = rng(1.8, 5.5), z = math.sin(ang) * 15.5,
                rx = rng(-0.5, 0.5), ry = ang, sx = 1.3, sy = 1.5, sz = 1.3,
                phase = i * 1.1, band = (i % 8) + 1, col = pri,
            })
        end

    elseif th == "emerald_cavern" then
        -- 18 Giant Glowing Emerald Crystal Walls surrounding the sanctum like a cavern
        for i = 1, 18 do
            local ang = ((i - 1) / 18.0) * 6.283 + rng(-0.08, 0.08)
            local rad = (i % 2 == 0) and rng(8.2, 10.6) or rng(11.8, 16.2)
            local sc_h = rng(2.4, 4.4)
            local sc_w = rng(1.8, 3.2)
            spawn_item("mesh", "models/env_crystal_cluster.obj", {
                behavior = "crystal_pulse", emissive = true,
                x = math.cos(ang) * rad, y = -0.3, z = math.sin(ang) * rad,
                rx = rng(-0.18, 0.18), ry = ang + rng(-0.5, 0.5), rz = rng(-0.16, 0.16),
                sx = sc_w, sy = sc_h, sz = sc_w,
                phase = rng(0, 6.28), band = (i % 8) + 1,
                col = (i % 3 == 0) and acc or pri,
            })
        end
        -- 16 Hanging Cavern Stalactites forming the crystal cave ceiling & arches
        for i = 1, 16 do
            local ang = ((i - 1) / 16.0) * 6.283 + 0.18
            local rad = rng(6.8, 14.5)
            local sc = rng(1.8, 3.4)
            spawn_item("mesh", "models/env_cave_stalactite.obj", {
                behavior = "crystal_pulse", emissive = (i % 2 == 0),
                x = math.cos(ang) * rad, y = rng(6.4, 9.8), z = math.sin(ang) * rad,
                ry = rng(0, 6.28), sx = sc * 0.9, sy = sc * 1.25, sz = sc * 0.9,
                phase = rng(0, 6.28), band = (i % 8) + 1, col = dark_pri,
            })
        end
        -- 6 Giant Voxel Emerald Cave Archways framing the outer perimeter
        for i = 1, 6 do
            local ang = ((i - 1) / 6.0) * 6.283 + 0.52
            spawn_item("voxel", "voxels/env_emerald_arch.gfvox", {
                behavior = "crystal_pulse",
                x = math.cos(ang) * 11.4, y = 0.0, z = math.sin(ang) * 11.4,
                ry = -ang + 1.57, sx = 2.6, sy = 2.9, sz = 2.2,
                phase = i * 1.0, band = i, col = pri,
            })
        end
        -- 8 Glowing Cavern Blossoms sprouting along the crystal floor
        for i = 1, 8 do
            local ang = ((i - 1) / 8.0) * 6.283 + 0.25
            local rad = rng(7.5, 11.0)
            spawn_item("mesh", "models/env_flora_blossom.obj", {
                behavior = "flora_bloom", emissive = true,
                x = math.cos(ang) * rad, y = 0.0, z = math.sin(ang) * rad,
                ry = rng(0, 6.28), sx = 1.4, sy = 1.4, sz = 1.4,
                phase = i * 0.8, band = (i % 8) + 1, col = acc,
            })
        end
        -- 4 Luminous Crystal Choir Seraph Birds gliding through the emerald vault
        for i = 1, 4 do
            spawn_item("mesh", "models/env_creature_bird.obj", {
                behavior = "orbit_fly", emissive = true,
                radius = 7.8 + i * 1.4, angle = -1.57 + (i - 2) * 0.7,
                y = 3.0 + i * 0.5, sx = 1.15, sy = 1.15, sz = 1.15,
                speed = 0.42 + i * 0.06, phase = i * 1.5, amp = 0.55, band = i, col = acc,
            })
        end
        -- 14 Animated Luminous Cavern Wisps rising and shimmering with notes
        for i = 1, 14 do
            local ang = rng(0, 6.283)
            local rad = rng(7.0, 14.0)
            spawn_item("sprite", "sprites/env_butterfly_wisp.tga", {
                behavior = "wisp_float",
                x = math.cos(ang) * rad, y = rng(1.5, 5.2), z = math.sin(ang) * rad,
                sp_w = 0.78, sp_h = 0.78, speed = rng(0.6, 1.4), phase = rng(0, 6.28),
                amp = 0.8, band = (i % 8) + 1, col = acc, alpha = 0.92,
            })
        end

    elseif th == "biolum_ocean" then
        -- 18 Swaying Giant Kelp Fronds rising around the abyssal sanctum
        for i = 1, 18 do
            local ang = ((i - 1) / 18.0) * 6.283 + rng(-0.07, 0.07)
            local rad = rng(7.8, 15.5)
            spawn_item("mesh", "models/env_kelp_frond.obj", {
                behavior = "plant_sway",
                x = math.cos(ang) * rad, y = -0.2, z = math.sin(ang) * rad,
                ry = ang, sx = rng(1.5, 2.4), sy = rng(2.0, 3.6), sz = rng(1.5, 2.4),
                speed = rng(0.7, 1.3), phase = rng(0, 6.28), band = (i % 8) + 1,
                col = {pri[1] * 0.65, clamp(pri[2] * 0.95, 0.2, 1.0), clamp(pri[3] * 0.85, 0.2, 1.0)},
            })
        end
        -- 10 Bioluminescent Pulsing Jellyfish (framed directly through the front & side arches!)
        for i = 1, 10 do
            local ang = ((i - 1) / 10.0) * 6.283 - 1.57 + rng(-0.14, 0.14)
            local rad = (i <= 4) and rng(8.2, 11.8) or rng(10.5, 15.5)
            local sc = rng(1.15, 1.95)
            spawn_item("mesh", "models/env_creature_jelly.obj", {
                behavior = "jelly_pulse", emissive = true,
                x = math.cos(ang) * rad, y = rng(2.4, 4.8), z = math.sin(ang) * rad,
                sx = sc, sy = sc, sz = sc, speed = rng(1.1, 1.9), phase = rng(0, 6.28),
                amp = 0.65, band = (i % 8) + 1, col = (i % 2 == 0) and acc or pri,
            })
        end
        -- 5 Majestic Swimming Abyssal Whales
        for i = 1, 5 do
            spawn_item("mesh", "models/env_creature_whale.obj", {
                behavior = "orbit_swim", emissive = true,
                radius = 9.5 + i * 2.2, angle = -1.57 + (i - 2.5) * 0.55,
                y = 2.5 + i * 0.65, sx = 1.65, sy = 1.65, sz = 1.65,
                speed = 0.20 + i * 0.03, phase = i * 1.3, amp = 0.65, band = i, col = pri,
            })
        end
        -- 8 Glowing Sea Anemone Coral Blossoms
        for i = 1, 8 do
            local ang = ((i - 1) / 8.0) * 6.283 + 0.35
            local rad = rng(8.0, 12.2)
            spawn_item("mesh", "models/env_flora_blossom.obj", {
                behavior = "flora_bloom", emissive = true,
                x = math.cos(ang) * rad, y = 0.0, z = math.sin(ang) * rad,
                ry = rng(0, 6.28), sx = 1.5, sy = 1.5, sz = 1.5,
                phase = i * 0.9, band = (i % 8) + 1, col = acc,
            })
        end
        -- 18 Rising Bioluminescent Ocean Bubbles
        for i = 1, 18 do
            local ang = rng(0, 6.283)
            local rad = rng(7.2, 15.5)
            spawn_item("sprite", "sprites/env_bubble_spore.tga", {
                behavior = "bubble_rise",
                x = math.cos(ang) * rad, y = rng(0.4, 8.0), z = math.sin(ang) * rad,
                sp_w = rng(0.50, 1.00), sp_h = rng(0.50, 1.00),
                speed = rng(0.9, 2.2), phase = rng(0, 6.28), band = (i % 8) + 1, col = acc, alpha = 0.90,
            })
        end

    elseif th == "enchanted_forest" then
        -- 16 Ancient Swaying Enchanted Trees ringing the forest glade
        for i = 1, 16 do
            local ang = ((i - 1) / 16.0) * 6.283 + rng(-0.08, 0.08)
            local rad = (i % 2 == 0) and rng(8.5, 11.5) or rng(12.8, 18.0)
            local sc = rng(1.8, 3.1)
            spawn_item("mesh", "models/env_tree_canopy.obj", {
                behavior = "plant_sway",
                x = math.cos(ang) * rad, y = -0.1, z = math.sin(ang) * rad,
                ry = rng(0, 6.28), sx = sc, sy = sc * rng(0.9, 1.35), sz = sc,
                speed = rng(0.5, 1.0), phase = rng(0, 6.28), band = (i % 8) + 1, col = pri,
            })
        end
        -- 12 Glowing Faerie Blossoms blooming with chords
        for i = 1, 12 do
            local ang = ((i - 1) / 12.0) * 6.283 + 0.14
            local rad = rng(7.6, 12.5)
            spawn_item("mesh", "models/env_flora_blossom.obj", {
                behavior = "flora_bloom", emissive = true,
                x = math.cos(ang) * rad, y = 0.0, z = math.sin(ang) * rad,
                ry = rng(0, 6.28), sx = rng(1.2, 2.0), sy = rng(1.2, 2.0), sz = rng(1.2, 2.0),
                phase = i * 0.7, band = (i % 8) + 1, col = acc,
            })
        end
        -- 6 Soaring Woodland Birds gliding through the trees
        for i = 1, 6 do
            spawn_item("mesh", "models/env_creature_bird.obj", {
                behavior = "orbit_fly", emissive = (i % 2 == 0),
                radius = 8.8 + i * 1.4, angle = i * 1.05,
                y = 4.0 + i * 0.7, sx = 1.15, sy = 1.15, sz = 1.15,
                speed = 0.48 + i * 0.06, phase = i * 1.2, amp = 0.7, band = i, col = acc,
            })
        end
        -- 6 Mossy Crystal Runestones
        for i = 1, 6 do
            local ang = ((i - 1) / 6.0) * 6.283 + 0.45
            spawn_item("mesh", "models/env_crystal_cluster.obj", {
                behavior = "crystal_pulse", emissive = true,
                x = math.cos(ang) * 10.2, y = -0.1, z = math.sin(ang) * 10.2,
                ry = ang, sx = 1.25, sy = 1.65, sz = 1.25,
                phase = i * 1.0, band = i, col = acc,
            })
        end
        -- 18 Animated Fluttering Butterflies & Firefly Wisps
        for i = 1, 18 do
            local ang = rng(0, 6.283)
            local rad = rng(7.0, 15.0)
            spawn_item("sprite", "sprites/env_butterfly_wisp.tga", {
                behavior = "wisp_float",
                x = math.cos(ang) * rad, y = rng(1.0, 6.2), z = math.sin(ang) * rad,
                sp_w = 0.80, sp_h = 0.80, speed = rng(0.8, 1.7), phase = rng(0, 6.28),
                amp = 1.1, band = (i % 8) + 1, col = (i % 2 == 0) and acc or pri, alpha = 0.95,
            })
        end

    elseif th == "winged_sky_sanctum" then
        -- 12 Soaring Celestial Birds / Seraphs in harmonic flight
        for i = 1, 12 do
            spawn_item("mesh", "models/env_creature_bird.obj", {
                behavior = "orbit_fly", emissive = true,
                radius = 8.4 + (i % 4) * 2.8, angle = i * 0.52,
                y = 3.6 + (i % 5) * 1.25, sx = rng(1.1, 1.75), sy = rng(1.1, 1.75), sz = rng(1.1, 1.75),
                speed = 0.42 + (i % 4) * 0.10, phase = i * 0.9, amp = 0.95, band = (i % 8) + 1,
                col = (i % 2 == 0) and pri or acc,
            })
        end
        -- 10 Floating Sky Cathedral Pillars
        for i = 1, 10 do
            local ang = ((i - 1) / 10.0) * 6.283
            local rad = rng(9.5, 16.5)
            spawn_item("mesh", "models/env_ruin_pillar.obj", {
                behavior = "crystal_pulse",
                x = math.cos(ang) * rad, y = rng(-0.5, 2.2), z = math.sin(ang) * rad,
                ry = ang, sx = 1.6, sy = rng(2.2, 3.4), sz = 1.6,
                phase = i * 0.6, band = (i % 8) + 1, col = acc,
            })
        end
        -- 8 Radiant Halo Crystals
        for i = 1, 8 do
            local ang = ((i - 1) / 8.0) * 6.283 + 0.35
            spawn_item("mesh", "models/env_crystal_cluster.obj", {
                behavior = "crystal_pulse", emissive = true,
                x = math.cos(ang) * 12.2, y = 4.2 + (i % 2) * 2.2, z = math.sin(ang) * 12.2,
                ry = ang, sx = 1.5, sy = 2.2, sz = 1.5,
                phase = i * 1.1, band = i, col = pri,
            })
        end
        -- 10 Sky Blossoms & 16 Shimmering Star/Feather Motes
        for i = 1, 10 do
            local ang = ((i - 1) / 10.0) * 6.283 + 0.2
            spawn_item("mesh", "models/env_flora_blossom.obj", {
                behavior = "flora_bloom", emissive = true,
                x = math.cos(ang) * 8.6, y = 0.1, z = math.sin(ang) * 8.6,
                sx = 1.4, sy = 1.4, sz = 1.4, phase = i * 0.8, band = (i % 8) + 1, col = pri,
            })
        end
        for i = 1, 16 do
            local ang = rng(0, 6.283)
            local rad = rng(7.5, 18.0)
            spawn_item("sprite", (i % 2 == 0) and "sprites/env_star_spark.tga" or "sprites/env_butterfly_wisp.tga", {
                behavior = "wisp_float",
                x = math.cos(ang) * rad, y = rng(1.8, 9.5), z = math.sin(ang) * rad,
                sp_w = 0.85, sp_h = 0.85, speed = rng(0.6, 1.4), phase = rng(0, 6.28),
                amp = 1.2, band = (i % 8) + 1, col = pri, alpha = 0.94,
            })
        end

    elseif th == "ancient_desert_temple" then
        -- 16 Ancient Temple Pillars & Monoliths
        for i = 1, 16 do
            local ang = ((i - 1) / 16.0) * 6.283
            local rad = (i % 2 == 0) and rng(8.6, 11.0) or rng(13.2, 18.0)
            spawn_item("mesh", "models/env_ruin_pillar.obj", {
                behavior = "static",
                x = math.cos(ang) * rad, y = -0.2, z = math.sin(ang) * rad,
                ry = ang, sx = rng(1.6, 2.4), sy = rng(2.2, 3.8), sz = rng(1.6, 2.4),
                phase = i, band = (i % 8) + 1, col = pri,
            })
        end
        -- 8 Glowing Solar/Ember Crystal Braziers
        for i = 1, 8 do
            local ang = ((i - 1) / 8.0) * 6.283 + 0.25
            spawn_item("mesh", "models/env_crystal_cluster.obj", {
                behavior = "crystal_pulse", emissive = true,
                x = math.cos(ang) * 9.8, y = 0.0, z = math.sin(ang) * 9.8,
                ry = ang, sx = 1.5, sy = 2.0, sz = 1.5,
                phase = i * 0.9, band = i, col = acc,
            })
        end
        -- 6 Oasis Desert Trees
        for i = 1, 6 do
            local ang = ((i - 1) / 6.0) * 6.283 + 0.5
            spawn_item("mesh", "models/env_tree_canopy.obj", {
                behavior = "plant_sway",
                x = math.cos(ang) * 14.2, y = -0.1, z = math.sin(ang) * 14.2,
                ry = rng(0, 6.28), sx = 2.0, sy = 2.5, sz = 2.0,
                speed = 0.65, phase = i * 1.1, band = i, col = {0.45, 0.72, 0.34},
            })
        end
        -- 6 Soaring Desert Hawks
        for i = 1, 6 do
            spawn_item("mesh", "models/env_creature_bird.obj", {
                behavior = "orbit_fly", emissive = false,
                radius = 10.5 + i * 1.8, angle = i * 1.04,
                y = 5.4 + i * 0.85, sx = 1.35, sy = 1.35, sz = 1.35,
                speed = 0.38 + i * 0.05, phase = i * 1.3, amp = 0.8, band = i, col = acc,
            })
        end
        -- 4 Giant Solar Astrolabe Rings in the sky
        for i = 1, 4 do
            local ang = ((i - 1) / 4.0) * 6.283 + 0.78
            spawn_item("mesh", "models/env_gear_astrolabe.obj", {
                behavior = "gear_spin", emissive = true,
                x = math.cos(ang) * 16.0, y = 7.5 + (i % 2) * 2.5, z = math.sin(ang) * 16.0,
                ry = -ang + 1.57, sx = 2.0, sy = 2.0, sz = 2.0,
                speed = (i % 2 == 0) and 0.35 or -0.35, phase = i, band = i, col = pri,
            })
        end
        -- 14 Rising Desert Ember Sparks
        for i = 1, 14 do
            local ang = rng(0, 6.283)
            local rad = rng(7.5, 16.0)
            spawn_item("sprite", "sprites/env_star_spark.tga", {
                behavior = "bubble_rise",
                x = math.cos(ang) * rad, y = rng(0.5, 9.5), z = math.sin(ang) * rad,
                sp_w = 0.72, sp_h = 0.72, speed = rng(1.0, 2.4), phase = rng(0, 6.28),
                band = (i % 8) + 1, col = acc, alpha = 0.92,
            })
        end

    else -- cyber_clockwork
        -- 12 Giant Rotating Clockwork Astrolabe Gears
        for i = 1, 12 do
            local ang = ((i - 1) / 12.0) * 6.283
            local rad = (i % 2 == 0) and 11.2 or 16.5
            local sc = rng(1.5, 2.7)
            spawn_item("mesh", "models/env_gear_astrolabe.obj", {
                behavior = "gear_spin", emissive = true,
                x = math.cos(ang) * rad, y = rng(3.2, 9.2), z = math.sin(ang) * rad,
                ry = -ang + 1.57, sx = sc, sy = sc, sz = sc,
                speed = ((i % 2 == 0) and 0.55 or -0.55) * rng(0.7, 1.4),
                phase = i * 0.7, band = (i % 8) + 1, col = (i % 2 == 0) and pri or acc,
            })
        end
        -- 10 Voxel Neon Cyber Monolith Towers
        for i = 1, 10 do
            local ang = ((i - 1) / 10.0) * 6.283 + 0.31
            local rad = rng(9.5, 16.0)
            spawn_item("voxel", "voxels/env_cyber_tower.gfvox", {
                behavior = "crystal_pulse",
                x = math.cos(ang) * rad, y = 0.0, z = math.sin(ang) * rad,
                ry = -ang, sx = 2.2, sy = rng(2.2, 3.8), sz = 2.2,
                phase = i * 0.8, band = (i % 8) + 1, col = pri,
            })
        end
        -- 6 Orbiting Cyber-Bird Drones
        for i = 1, 6 do
            spawn_item("mesh", "models/env_creature_bird.obj", {
                behavior = "orbit_fly", emissive = true,
                radius = 9.0 + i * 1.5, angle = i * 1.04,
                y = 4.4 + i * 0.65, sx = 1.15, sy = 1.15, sz = 1.15,
                speed = 0.65 + i * 0.08, phase = i * 1.1, amp = 0.6, band = i, col = acc,
            })
        end
        -- 16 Neon Data-Stream Streaks rushing past
        for i = 1, 16 do
            local ang = rng(0, 6.283)
            local rad = rng(8.0, 22.0)
            spawn_item("mesh", "models/env_star_streak.obj", {
                behavior = "star_fly", emissive = true,
                x = math.cos(ang) * rad, y = rng(1.0, 12.0), z = rng(-32.0, 12.0),
                sx = 0.9, sy = 0.9, sz = 2.0,
                speed = rng(9.0, 18.0), phase = rng(0, 6.28), band = (i % 8) + 1,
                col = (i % 2 == 0) and acc or pri,
            })
        end
        -- 8 Neon Crystal Conduits
        for i = 1, 8 do
            local ang = ((i - 1) / 8.0) * 6.283 + 0.15
            spawn_item("mesh", "models/env_crystal_cluster.obj", {
                behavior = "crystal_pulse", emissive = true,
                x = math.cos(ang) * 8.6, y = 0.0, z = math.sin(ang) * 8.6,
                ry = ang, sx = 1.2, sy = 1.9, sz = 1.2,
                phase = i, band = i, col = acc,
            })
        end
    end

    -- Compound Living Creature / Plant Overlays when preset name explicitly calls for them
    if spec.wants_whale and th ~= "biolum_ocean" and th ~= "starfall_cosmos" then
        for i = 1, 3 do
            spawn_item("mesh", "models/env_creature_whale.obj", {
                behavior = "orbit_swim", emissive = true,
                radius = 12.0 + i * 2.5, angle = i * 2.1,
                y = 5.0 + i * 1.2, sx = 1.5, sy = 1.5, sz = 1.5,
                speed = 0.20, phase = i * 1.5, amp = 0.8, band = i, col = acc,
            })
        end
    end
    if spec.wants_jelly and th ~= "biolum_ocean" then
        for i = 1, 4 do
            local ang = i * 1.57 + 0.4
            spawn_item("mesh", "models/env_creature_jelly.obj", {
                behavior = "jelly_pulse", emissive = true,
                x = math.cos(ang) * 10.5, y = 3.8 + i * 0.7, z = math.sin(ang) * 10.5,
                sx = 1.35, sy = 1.35, sz = 1.35, speed = 1.5, phase = i, amp = 0.7, band = i, col = acc,
            })
        end
    end
    if spec.wants_flora and th ~= "enchanted_forest" and th ~= "emerald_cavern" and th ~= "biolum_ocean" then
        for i = 1, 6 do
            local ang = ((i - 1) / 6.0) * 6.283 + 0.3
            spawn_item("mesh", "models/env_flora_blossom.obj", {
                behavior = "flora_bloom", emissive = true,
                x = math.cos(ang) * 8.4, y = 0.0, z = math.sin(ang) * 8.4,
                sx = 1.4, sy = 1.4, sz = 1.4, phase = i, band = i, col = acc,
            })
        end
    end

    PresetWorlds.update(0.016, nil, {energy = 0.0, pitch_norm = 0.5, strike = 0.45})
    return spec.theme
end

function PresetWorlds.update(dt, tel, music)
    PresetWorlds.time = PresetWorlds.time + (dt or 0.016)
    local t = PresetWorlds.time
    local cam = gf.spatial.camera_state()
    local rc = PresetWorlds.room_centers[cam.room] or {x = 0.0, z = 0.0}
    -- Smoothly follow the active room center so every room sits inside the preset's world
    local blend = clamp((dt or 0.016) * 5.0, 0.0, 1.0)
    PresetWorlds.anchor_x = PresetWorlds.anchor_x + (rc.x - PresetWorlds.anchor_x) * blend
    PresetWorlds.anchor_z = PresetWorlds.anchor_z + (rc.z - PresetWorlds.anchor_z) * blend
    local ax, az = PresetWorlds.anchor_x, PresetWorlds.anchor_z

    local rms = tel and ((tel.rms_l or 0) + (tel.rms_r or 0)) * 0.5 or 0.0
    local energy = clamp((music and music.energy or 0.0) + rms * 2.2, 0.0, 1.0)
    local pitch  = clamp(music and music.pitch_norm or 0.5, 0.0, 1.0)
    local strike = clamp(music and music.strike or 0.0, 0.0, 1.0)
    local bands  = tel and tel.bands or nil

    gf.scene.set_music_state(energy, pitch, strike)

    -- Pitch-reactive subtle tint shift (warmer low notes, luminous cyan-gold high notes)
    local p_shift = (pitch - 0.5) * 0.32
    local glow = 0.82 + energy * 0.32 + strike * 0.48
    local sp_frame = math.floor(t * (7.0 + energy * 8.0 + strike * 6.0)) % 4

    for _, it in ipairs(PresetWorlds.items) do
        local b_val = (bands and bands[it.band]) and clamp(bands[it.band] * 2.8, 0.0, 1.0) or energy
        local beh = it.behavior
        local px, py, pz = ax + it.ox, it.oy, az + it.oz
        local rx, ry, rz = it.rx, it.ry, it.rz
        local sx, sy, sz = it.sx, it.sy, it.sz

        if beh == "star_fly" then
            local warp = 1.0 + energy * 2.4 + strike * 3.8 + pitch * 0.9
            it.oz = it.oz + it.speed * warp * (dt or 0.016)
            if it.oz > 18.0 then
                it.oz = -38.0 - (it.phase * 1.5)
            end
            pz = az + it.oz
            py = it.oy + math.sin(t * 0.8 + it.phase) * 0.4 + (pitch - 0.5) * 2.2
            sz = it.sz * (1.0 + energy * 1.4 + strike * 2.6)
        elseif beh == "crystal_pulse" then
            local pulse = 1.0 + b_val * 0.24 + strike * 0.20 + math.sin(t * 1.6 + it.phase) * 0.04
            sx = it.sx * (1.0 + strike * 0.10)
            sy = it.sy * pulse
            sz = it.sz * (1.0 + strike * 0.10)
        elseif beh == "planet_spin" then
            ry = it.ry + t * (it.speed + energy * 0.25)
            py = it.oy + math.sin(t * 0.4 + it.phase) * 0.45 + (pitch - 0.5) * 1.5
            local ps = 1.0 + strike * 0.08 + energy * 0.06
            sx, sy, sz = it.sx * ps, it.sy * ps, it.sz * ps
        elseif beh == "orbit_swim" then
            local ang = it.angle + t * (it.speed + energy * 0.18 + strike * 0.12)
            px = ax + math.cos(ang) * it.radius
            pz = az + math.sin(ang) * it.radius
            py = it.oy + math.sin(t * 1.1 + it.phase) * it.amp + (pitch - 0.5) * 2.6 + strike * 0.55
            ry = -ang
            rx = math.cos(t * 1.1 + it.phase) * 0.14
            rz = math.sin(t * 2.2 + it.phase) * (0.08 + energy * 0.16)
        elseif beh == "orbit_fly" then
            local ang = it.angle + t * (it.speed + energy * 0.35 + strike * 0.28)
            px = ax + math.cos(ang) * it.radius
            pz = az + math.sin(ang) * it.radius
            local flap = math.sin(t * (7.5 + energy * 6.0 + strike * 8.0) + it.phase)
            py = it.oy + math.sin(t * 1.5 + it.phase) * it.amp + flap * 0.16 + (pitch - 0.5) * 2.8 + strike * 0.75
            ry = -ang
            rz = flap * (0.22 + strike * 0.25)
            sy = it.sy * (0.85 + math.abs(flap) * 0.35)
        elseif beh == "jelly_pulse" then
            local beat = math.sin(t * (it.speed + energy * 1.4) + it.phase)
            local contract = clamp(beat * 0.5 + 0.5 + strike * 0.55, 0.0, 1.3)
            py = it.oy + math.sin(t * 0.9 + it.phase) * it.amp + (pitch - 0.5) * 2.4 + strike * 0.65
            sx = it.sx * (1.12 - contract * 0.24)
            sy = it.sy * (0.86 + contract * 0.32)
            sz = it.sz * (1.12 - contract * 0.24)
            ry = t * 0.35 + it.phase
        elseif beh == "plant_sway" then
            local sway = math.sin(t * (it.speed + energy * 0.9) + it.phase) * (0.07 + energy * 0.14 + strike * 0.12)
            rx = sway
            rz = math.cos(t * it.speed * 0.8 + it.phase) * (0.06 + energy * 0.10)
            sy = it.sy * (1.0 + b_val * 0.12 + strike * 0.08)
        elseif beh == "flora_bloom" then
            local bl = 1.0 + b_val * 0.36 + strike * 0.32 + math.sin(t * 2.0 + it.phase) * 0.08
            sx, sy, sz = it.sx * bl, it.sy * (0.95 + bl * 0.15), it.sz * bl
            ry = it.ry + t * (0.25 + energy * 0.5)
        elseif beh == "gear_spin" then
            rz = it.rz + t * (it.speed * (1.0 + energy * 1.8 + strike * 1.6))
            local gs = 1.0 + strike * 0.10
            sx, sy, sz = it.sx * gs, it.sy * gs, it.sz * gs
        elseif beh == "wisp_float" then
            px = ax + it.ox + math.cos(t * it.speed + it.phase) * 1.25
            pz = az + it.oz + math.sin(t * it.speed * 0.8 + it.phase) * 1.25
            py = it.oy + math.sin(t * (it.speed * 1.3) + it.phase) * it.amp + (pitch - 0.5) * 2.2 + strike * 0.85
            local ws = 1.0 + energy * 0.55 + strike * 0.55
            sx, sy = ws, ws
        elseif beh == "bubble_rise" then
            it.oy = it.oy + it.speed * (1.0 + energy * 1.8 + strike * 2.2) * (dt or 0.016)
            if it.oy > 11.5 then it.oy = 0.2 end
            py = it.oy
            px = ax + it.ox + math.sin(t * 1.8 + it.phase) * 0.55
            local bs = 1.0 + energy * 0.45 + strike * 0.35
            sx, sy = bs, bs
        end

        gf.scene.set_transform(it.id, px, py, pz, rx, ry, rz, sx, sy, sz)
        if it.kind == "sprite" then
            gf.scene.set_sprite_anim(it.id, it.sp_w, it.sp_h, 4, (sp_frame + math.floor(it.phase)) % 4)
        end
        if it.emissive or it.kind == "sprite" then
            local cr = clamp(it.col[1] * glow - p_shift * 0.35, 0.05, 1.0)
            local cg = clamp(it.col[2] * glow + b_val * 0.15, 0.05, 1.0)
            local cb = clamp(it.col[3] * glow + p_shift * 0.45, 0.05, 1.0)
            gf.scene.set_tint(it.id, cr, cg, cb, it.alpha)
        end
    end
end

return PresetWorlds
