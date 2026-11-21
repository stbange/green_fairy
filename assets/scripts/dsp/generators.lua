-- Green Fairy: Procedural Wavetable & Multi-Stage Envelope Generators
local M = {}

function M.gen_harmonic_table(harmonics, size)
    local n = size or 256
    local out = {}
    local tau = math.pi * 2.0
    for i = 1, n do
        local ph = (i - 1) / n
        local s = 0.0
        for h, amp in pairs(harmonics) do
            s = s + amp * math.sin(tau * h * ph)
        end
        out[i] = s
    end
    return out
end

function M.gen_formant_table(size)
    return M.gen_harmonic_table({
        [1] = 0.60, [2] = 0.25, [3] = 0.42, [5] = 0.28,
        [7] = 0.16, [9] = 0.10, [11] = 0.06
    }, size or 256)
end

function M.gen_crystal_table(size)
    return M.gen_harmonic_table({
        [1] = 0.55, [3] = 0.30, [4] = 0.18, [7] = 0.22, [13] = 0.09
    }, size or 256)
end

function M.build_aether_envelope()
    -- Multi-stage Absynth-style breathing envelope with sustain and loop
    return {
        {0.00, 0.00,  0.0},
        {0.03, 0.95, -0.3},
        {0.18, 0.62,  0.2},
        {0.42, 0.88, -0.2},
        {0.70, 0.68,  0.1},
        {1.30, 0.00,  0.4},
    }, 5, 2, 4
end

function M.build_pluck_envelope()
    return {
        {0.00, 0.00,  0.0},
        {0.006, 1.00, -0.5},
        {0.14, 0.32,  0.4},
        {0.45, 0.00,  0.3},
    }, 3, -1, -1
end

function M.apply_patch(patch)
    if type(patch) ~= "table" then
        return false
    end
    if patch.samples then
        for slot, rel in pairs(patch.samples) do
            gf.synth.load_sample(slot, rel)
        end
    end
    if patch.wavetables then
        for ch, wt in pairs(patch.wavetables) do
            if wt.a then gf.synth.set_wavetable(ch, false, wt.a) end
            if wt.b then gf.synth.set_wavetable(ch, true, wt.b) end
        end
    end
    if patch.envelopes then
        for slot, env in pairs(patch.envelopes) do
            gf.synth.set_envelope(slot, env.points, env.sustain or -1, env.loop_start or -1, env.loop_end or -1)
        end
    end
    if patch.params then
        for k, v in pairs(patch.params) do
            gf.synth.set_param(k, v)
        end
    end
    if patch.xy then
        gf.synth.set_xy(patch.xy[1] or 0.5, patch.xy[2] or 0.5)
    end
    return true
end

return M
