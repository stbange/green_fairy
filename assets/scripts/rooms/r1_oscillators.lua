-- Room 1: Oscillator Conservatory (0, 0, -18)
-- 3 Full 9-Mode Absynth Oscillator Racks + Interactive 32-Bar Harmonic Sculptor

local RB = require("rooms.rack_builder")

local R1 = {
    osc_racks = {},
    harm_rack = -1,
    harm_bars = {},
    harmonics = {},
    active_harm_ch = 1,
}

for i = 1, 32 do
    R1.harmonics[i] = (i == 1) and 1.0 or (i % 2 == 1 and (0.55 / i) or 0.0)
end

local OSC_SPECS = {
    -- Row 1 (y = +0.52): Mode (9-step), Level, Pan, Coarse, Fine, Morph
    {kind="step_dial", x=-1.00, y=0.52, steps=10, min=0, max=9,   s="mode",          label="MODE",    color=RB.COLOR_DIAL},
    {kind="knob",      x=-0.60, y=0.52,           min=0, max=1,   s="level",         label="LEVEL"},
    {kind="knob",      x=-0.20, y=0.52,           min=-1,max=1,   s="pan",           label="PAN"},
    {kind="knob",      x= 0.20, y=0.52,           min=-24,max=24, s="detune",        label="SEMI"},
    {kind="knob",      x= 0.60, y=0.52,           min=-100,max=100,s="fine",         label="FINE"},
    {kind="knob",      x= 1.00, y=0.52,           min=0, max=1,   s="morph",         label="MORPH"},
    -- Row 2 (y = +0.20): FM Ratio, FM Index, FM Fbk, Fractal Depth, Fractal Ratio, Hilbert Hz
    {kind="knob",      x=-1.00, y=0.20,           min=0.25,max=12,s="mod_ratio",     label="RATIO"},
    {kind="knob",      x=-0.60, y=0.20,           min=0, max=10,  s="fm_index",      label="FM IDX"},
    {kind="knob",      x=-0.20, y=0.20,           min=0, max=0.92,s="fm_feedback",   label="FM FBK"},
    {kind="knob",      x= 0.20, y=0.20,           min=0, max=1,   s="fractal_depth", label="FRACTAL"},
    {kind="knob",      x= 0.60, y=0.20,           min=1.5,max=4.0,s="fractal_ratio", label="FRAC RT"},
    {kind="knob",      x= 1.00, y=0.20,           min=-800,max=800,s="shift_hz",     label="HILBERT"},
    -- Row 3 (y = -0.12): Grain Size, Density, Pos, Spray, Reverse + Sample Slot
    {kind="knob",      x=-1.00, y=-0.12,          min=0.015,max=0.32,s="grain_size", label="GR SIZE"},
    {kind="knob",      x=-0.60, y=-0.12,          min=2, max=55,  s="grain_density", label="GR DENS"},
    {kind="knob",      x=-0.20, y=-0.12,          min=0, max=1,   s="grain_pos",     label="GR POS"},
    {kind="knob",      x= 0.20, y=-0.12,          min=0, max=0.5, s="grain_spray",   label="SPRAY"},
    {kind="knob",      x= 0.60, y=-0.12,          min=0, max=1,   s="grain_reverse", label="REVERSE"},
    {kind="step_dial", x= 1.00, y=-0.12, steps=16,min=0, max=15,  s="sample",        label="SAMPLE",  color=RB.COLOR_DIAL},
    -- Row 4 (y = -0.42, right side): Super-Unison Voices, Detune, Stereo Spread
    {kind="step_dial", x= 0.35, y=-0.42, steps=8, min=1, max=8,   s="unison_voices", label="UNI V",   color=RB.COLOR_DIAL},
    {kind="knob",      x= 0.70, y=-0.42,          min=0, max=1,   s="unison_detune", label="UNI DET"},
    {kind="knob",      x= 1.03, y=-0.42,          min=0, max=1,   s="unison_spread", label="SPREAD"},
}

function R1.build()
    R1.osc_racks = {}
    R1.harm_bars = {}
    local cx, cz = 0.0, -18.0
    local angles = {-0.62, 0.0, 0.62}

    for ch = 1, 3 do
        local ang = angles[ch]
        local rx = cx + math.sin(ang) * 4.55
        local rz = cz - math.cos(ang) * 4.55
        local yaw = math.atan(cx - rx, cz - rz)
        local rk = RB.add_rack(1, string.format("OSC %d CONSERVATORY", ch), rx, 1.54, rz, yaw, 2.55, 1.88,
                               {0.22, 0.96, 0.72, 1.0})
        R1.osc_racks[ch] = rk

        local items = {}
        for i, sp in ipairs(OSC_SPECS) do
            items[i] = {
                kind = sp.kind, x = sp.x, y = sp.y, w = 0.105,
                steps = sp.steps, min = sp.min, max = sp.max,
                param = string.format("ch%d.%s", ch, sp.s),
                label = sp.label, color = sp.color,
            }
        end
        RB.add_controls(1, rk, items)

        local pfx = "osc" .. ch
        RB.add_jack_strip(1, rk, -0.74, {
            {pfx .. ".audio_out",    true,  "OUT",   "audio"},
            {pfx .. ".pitch_cv",     false, "PITCH", "cv"},
            {pfx .. ".fm_in",        false, "FM IN", "audio"},
            {pfx .. ".morph_cv",     false, "MORPH", "cv"},
            {pfx .. ".fractal_cv",   false, "FRAC",  "cv"},
            {pfx .. ".grain_pos_cv", false, "GRAIN", "cv"},
        })
    end

    -- 32-Partial Harmonic Wavetable Sculptor Rack on West Wall
    local hx, hz = cx - 4.75, cz + 0.6
    local hyaw = math.atan(cx - hx, cz - hz)
    R1.harm_rack = RB.add_rack(1, "WAVETABLE HARMONIC SCULPTOR", hx, 1.48, hz, hyaw, 2.35, 1.60,
                               {0.35, 1.0, 0.85, 1.0})

    for i = 1, 32 do
        local lx = -1.04 + (i - 0.5) * (2.08 / 32.0)
        R1.harm_bars[i] = gf.spatial.add_widget({
            room = 1, rack = R1.harm_rack, kind = "harmonic_bar",
            pos = {lx, -0.22, 0.016}, size = {0.048, 0.048, 0},
            track_len = 0.58, aux = i, min = 0.0, max = 1.0,
            default = R1.harmonics[i],
            color = {0.28, 1.0, 0.78, 1.0},
            action = "harm:bar",
        })
    end

    RB.add_controls(1, R1.harm_rack, {
        {kind="button", x=-0.75, y=-0.56, w=0.38, h=0.11, label="TARGET CH1", action="harm:ch:1", color=RB.COLOR_BUTTON},
        {kind="button", x= 0.00, y=-0.56, w=0.38, h=0.11, label="TARGET CH2", action="harm:ch:2", color=RB.COLOR_BUTTON},
        {kind="button", x= 0.75, y=-0.56, w=0.38, h=0.11, label="TARGET CH3", action="harm:ch:3", color=RB.COLOR_BUTTON},
    })

    R1.refresh_curves()
end

function R1.refresh_curves()
    for ch = 1, 3 do
        local wv = gf.synth.get_wavetable_preview(ch, false, 48)
        gf.spatial.set_curve(ch, R1.osc_racks[ch], -0.45, -0.42, 1.25, 0.28, nil, wv, false, 0.25, 1.0, 0.75, 0.95)
    end
end

function R1.on_harmonic_drag(bar_idx, norm_val)
    if bar_idx >= 1 and bar_idx <= 32 then
        R1.harmonics[bar_idx] = norm_val
        gf.synth.set_harmonics(R1.active_harm_ch, false, R1.harmonics)
        R1.refresh_curves()
    end
end

function R1.on_action(act)
    local ch = act:match("^harm:ch:(%d+)$")
    if ch then
        R1.active_harm_ch = tonumber(ch)
        gf.synth.set_harmonics(R1.active_harm_ch, false, R1.harmonics)
        R1.refresh_curves()
        return true
    end
    return false
end

return R1
