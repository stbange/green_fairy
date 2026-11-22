-- Absynth-Inspired Sound-Breeding Mutator with 16-Slot Undo Stack

local Mutator = {
    history = {},
    max_history = 16,
    depth = 0.28,
    randomness = 0.35,
    rng_state = 0x5A17C9E3,
}

local function next_rand()
    Mutator.rng_state = (Mutator.rng_state * 1664525 + 1013904223) % 4294967296
    return Mutator.rng_state / 4294967296.0
end

local function rand_sym()
    return next_rand() * 2.0 - 1.0
end

local function clamp(v, lo, hi)
    if v < lo then return lo end
    if v > hi then return hi end
    return v
end

-- Musically safe continuous parameter ranges for controlled breeding
local MUTABLE_RANGES = {
    {"ch1.morph", 0.0, 1.0}, {"ch2.morph", 0.0, 1.0}, {"ch3.morph", 0.0, 1.0},
    {"ch1.fine", -18.0, 18.0}, {"ch2.fine", -18.0, 18.0}, {"ch3.fine", -18.0, 18.0},
    {"ch1.pan", -0.75, 0.75}, {"ch2.pan", -0.75, 0.75}, {"ch3.pan", -0.75, 0.75},
    {"ch1.fm_index", 0.0, 6.5}, {"ch2.fm_index", 0.0, 6.5}, {"ch3.fm_index", 0.0, 5.0},
    {"ch1.fm_feedback", 0.0, 0.72}, {"ch2.fm_feedback", 0.0, 0.72},
    {"ch1.fractal_depth", 0.0, 0.85}, {"ch2.fractal_depth", 0.0, 0.85},
    {"ch1.shift_hz", -180.0, 180.0}, {"ch2.shift_hz", -180.0, 180.0},
    {"ch1.grain_pos", 0.05, 0.95}, {"ch1.grain_spray", 0.01, 0.35},
    {"ch1.unison_detune", 0.05, 0.65}, {"ch3.unison_detune", 0.05, 0.65},
    {"ch1.filter.cutoff", 220.0, 12000.0}, {"ch2.filter.cutoff", 260.0, 12000.0}, {"ch3.filter.cutoff", 160.0, 8000.0},
    {"ch1.filter.res", 0.08, 0.78}, {"ch2.filter.res", 0.08, 0.78}, {"ch3.filter.res", 0.05, 0.68},
    {"ch1.filter.morph", 0.0, 1.0}, {"ch2.filter.morph", 0.0, 1.0},
    {"master.filter.cutoff", 1800.0, 15500.0}, {"master.filter.res", 0.05, 0.55},
    {"pipe.freq", 65.0, 660.0}, {"pipe.mix", 0.0, 0.42},
    {"multicomb.freq", 80.0, 440.0}, {"multicomb.mix", 0.0, 0.42},
    {"multitap.time", 0.12, 0.68}, {"multitap.mix", 0.05, 0.45},
    {"aether.mix", 0.08, 0.62}, {"aether.mod", 0.1, 0.85},
    {"reverb.decay", 0.45, 0.95}, {"reverb.shimmer", 0.0, 0.55}, {"reverb.mix", 0.15, 0.65},
    {"lfo1.rate", 0.08, 6.5}, {"lfo2.rate", 0.05, 5.0}, {"lfo3.rate", 0.05, 4.0},
    {"xy.x", 0.0, 1.0}, {"xy.y", 0.0, 1.0}, {"xy.z", 0.0, 1.0},
}

function Mutator.push_undo()
    local snap = gf.mutator.snapshot()
    table.insert(Mutator.history, snap)
    if #Mutator.history > Mutator.max_history then
        table.remove(Mutator.history, 1)
    end
end

function Mutator.clear_history()
    Mutator.history = {}
end

function Mutator.mutate(depth, randomness)
    local d = clamp(depth or Mutator.depth, 0.02, 1.0)
    local r = clamp(randomness or Mutator.randomness, 0.05, 1.0)
    Mutator.push_undo()

    for _, spec in ipairs(MUTABLE_RANGES) do
        if next_rand() <= (0.35 + 0.65 * r) then
            local name, lo, hi = spec[1], spec[2], spec[3]
            local cur = gf.synth.get_param(name) or ((lo + hi) * 0.5)
            local span = hi - lo
            local delta = rand_sym() * span * d * 0.45
            gf.synth.set_param(name, clamp(cur + delta, lo, hi))
        end
    end

    -- At higher randomness, occasionally evolve oscillator/filter modes & harmonic ratios
    if r > 0.45 and next_rand() < r * 0.6 then
        local ratios = {0.5, 1.0, 1.5, 2.0, 3.0, 4.0, 5.0, 7.0}
        gf.synth.set_param("ch2.mod_ratio", ratios[math.floor(next_rand() * #ratios) + 1])
        gf.synth.set_param("ch1.mode", math.floor(1 + next_rand() * 8.99))
    end

    -- Gently perturb existing CV modular cable gains (preserving audio cable integrity)
    local cables = gf.patchbay.list_cables()
    for _, c in ipairs(cables) do
        if c.dst:find("_cv") and next_rand() < r * 0.5 then
            local ng = clamp(c.gain + rand_sym() * d * 0.6, -1.5, 1.5)
            gf.patchbay.set_gain(c.id, ng)
        end
    end

    -- Mutate envelope 1..3 intermediate breakpoint times/levels slightly
    for eslot = 1, 3 do
        local ed = gf.synth.get_envelope(eslot)
        if ed and ed.points and #ed.points >= 3 then
            local pts = ed.points
            for i = 2, #pts - 1 do
                pts[i][1] = math.max(pts[i - 1][1] + 0.015, pts[i][1] * (1.0 + rand_sym() * d * 0.30))
                pts[i][2] = clamp(pts[i][2] + rand_sym() * d * 0.25, 0.05, 1.0)
            end
            pts[#pts][1] = math.max(pts[#pts - 1][1] + 0.05, pts[#pts][1] * (1.0 + rand_sym() * d * 0.25))
            gf.synth.set_envelope(eslot, pts, ed.sustain, ed.loop_start, ed.loop_end)
        end
    end

    gf.spatial.sync_widgets()
    return true
end

function Mutator.undo()
    if #Mutator.history == 0 then
        return false
    end
    local snap = table.remove(Mutator.history)
    gf.mutator.restore(snap)
    return true
end

return Mutator
