-- Room 4: Clockwork Modulation, 16-Step Sequencer & Central Trunk Patchbay (0, 0, 18)

local RB = require("rooms.rack_builder")

local R4 = {
    lfo_rack = -1,
    seq_rack = -1,
    trunk_rack = -1,
    seq_bars = {},
}

function R4.build()
    R4.seq_bars = {}
    local cx, cz = 0.0, 18.0

    -- Rack 1: Triple LFO & Quad Modular VCA Attenuverter Rack
    local lx, lz = cx - 3.65, cz + 3.45
    R4.lfo_rack = RB.add_rack(4, "TRIPLE LFO & QUAD VCA MATRIX", lx, 1.52, lz, math.atan(cx - lx, cz - lz), 2.35, 1.82,
                              {0.95, 0.85, 0.32, 1.0})

    local lfo_items = {}
    for l = 1, 3 do
        local y = 0.48 - (l - 1) * 0.28
        local pfx = "lfo" .. l
        lfo_items[#lfo_items + 1] = {kind="step_dial", x=-0.78, y=y, w=0.10, steps=5, min=0, max=4,    param=pfx..".wave",   label="L"..l.." WAV", color=RB.COLOR_DIAL}
        lfo_items[#lfo_items + 1] = {kind="step_dial", x=-0.26, y=y, w=0.10, steps=6, min=0, max=5,    param=pfx..".target", label="L"..l.." TGT", color=RB.COLOR_DIAL}
        lfo_items[#lfo_items + 1] = {kind="knob",      x= 0.26, y=y, w=0.10,          min=0.02,max=25, param=pfx..".rate",   label="L"..l.." HZ"}
        lfo_items[#lfo_items + 1] = {kind="knob",      x= 0.78, y=y, w=0.10,          min=0.0, max=2.0,param=pfx..".depth",  label="L"..l.." DEP"}
    end
    RB.add_controls(4, R4.lfo_rack, lfo_items)

    RB.add_jack_strip(4, R4.lfo_rack, -0.42, {
        {"lfo1.out", true, "L1 OUT", "cv"}, {"lfo2.out", true, "L2 OUT", "cv"}, {"lfo3.out", true, "L3 OUT", "cv"},
        {"lfo1.rate_cv", false, "L1 RT", "cv"}, {"lfo2.rate_cv", false, "L2 RT", "cv"}, {"lfo3.rate_cv", false, "L3 RT", "cv"},
    })
    RB.add_jack_strip(4, R4.lfo_rack, -0.70, {
        {"vca1.in", false, "V1 IN", "cv"}, {"vca1.cv", false, "V1 CV", "cv"}, {"vca1.out", true, "V1 OUT", "cv"},
        {"vca2.in", false, "V2 IN", "cv"}, {"vca2.cv", false, "V2 CV", "cv"}, {"vca2.out", true, "V2 OUT", "cv"},
        {"vca3.in", false, "V3 IN", "cv"}, {"vca3.cv", false, "V3 CV", "cv"}, {"vca3.out", true, "V3 OUT", "cv"},
    })

    -- Rack 2: 16-Step Polyrhythmic Modulator Sequencer
    local sx, sz = cx, cz + 4.65
    R4.seq_rack = RB.add_rack(4, "16-STEP POLYRHYTHMIC SEQUENCER", sx, 1.52, sz, math.atan(cx - sx, cz - sz), 2.55, 1.82,
                              {0.98, 0.88, 0.36, 1.0})

    RB.add_controls(4, R4.seq_rack, {
        {kind="switch",    x=-0.92, y=0.50, w=0.105,          min=0, max=1,   param="seq.enabled", label="SEQ ON", color=RB.COLOR_SWITCH},
        {kind="knob",      x=-0.46, y=0.50, w=0.105,          min=40,max=220, param="seq.bpm",     label="BPM"},
        {kind="step_dial", x= 0.00, y=0.50, w=0.105, steps=16,min=1, max=16,  param="seq.steps",   label="STEPS",  color=RB.COLOR_DIAL},
        {kind="knob",      x= 0.46, y=0.50, w=0.105,          min=0, max=0.45,param="seq.swing",   label="SWING"},
        {kind="knob",      x= 0.92, y=0.50, w=0.105,          min=0, max=0.95,param="seq.slew",    label="SLEW"},
    })

    for s = 1, 16 do
        local st = gf.synth.get_seq_step(s) or {pitch = 0, mod = 0.5}
        local lx = -1.08 + (s - 0.5) * (2.16 / 16.0)
        R4.seq_bars[s] = gf.spatial.add_widget({
            room = 4, rack = R4.seq_rack, kind = "harmonic_bar",
            pos = {lx, -0.32, 0.016}, size = {0.085, 0.085, 0},
            track_len = 0.50, aux = s, min = -12.0, max = 12.0,
            default = st.pitch,
            color = {0.98, 0.82, 0.28, 1.0},
            action = "seq:step",
        })
    end

    RB.add_jack_strip(4, R4.seq_rack, -0.70, {
        {"seq.pitch_cv", true, "PITCH CV", "cv"},
        {"seq.gate_out", true, "GATE OUT", "trig"},
        {"seq.mod_cv",   true, "MOD CV",   "cv"},
        {"vca4.in",      false,"V4 IN",    "cv"},
        {"vca4.cv",      false,"V4 CV",    "cv"},
        {"vca4.out",     true, "V4 OUT",   "cv"},
    })

    -- Rack 3: Central Cross-Room Satellite Trunk Patchbay (mirrors key cross-room jacks locally!)
    local tx, tz = cx + 3.65, cz + 3.45
    R4.trunk_rack = RB.add_rack(4, "CENTRAL SANCTUM TRUNK PATCHBAY", tx, 1.52, tz, math.atan(cx - tx, cz - tz), 2.40, 1.82,
                                {0.42, 0.94, 0.86, 1.0})

    RB.add_jack_strip(4, R4.trunk_rack, 0.42, {
        {"env1.out", true, "ENV 1", "cv"}, {"env2.out", true, "ENV 2", "cv"},
        {"env3.out", true, "ENV 3", "cv"}, {"env4.out", true, "ENV 4", "cv"},
        {"xyz.x_out", true, "XYZ X", "cv"}, {"xyz.y_out", true, "XYZ Y", "cv"}, {"xyz.z_out", true, "XYZ Z", "cv"},
    })
    RB.add_jack_strip(4, R4.trunk_rack, 0.08, {
        {"osc1.audio_out", true, "OSC 1", "audio"}, {"osc2.audio_out", true, "OSC 2", "audio"}, {"osc3.audio_out", true, "OSC 3", "audio"},
        {"osc1.pitch_cv", false, "O1 PIT", "cv"}, {"osc2.fm_in", false, "O2 FM", "audio"}, {"osc1.morph_cv", false, "O1 MRP", "cv"},
    })
    RB.add_jack_strip(4, R4.trunk_rack, -0.28, {
        {"flt1.cutoff_cv", false, "F1 CUT", "cv"}, {"flt2.cutoff_cv", false, "F2 CUT", "cv"}, {"flt3.cutoff_cv", false, "F3 CUT", "cv"},
        {"flt1.morph_cv",  false, "F1 VOW", "cv"}, {"mflt.cutoff_cv", false, "MCUT",   "cv"},
    })
    RB.add_jack_strip(4, R4.trunk_rack, -0.64, {
        {"pipe.freq_cv",     false, "PIPE CV", "cv"}, {"multicomb.tune_cv", false, "COMB CV", "cv"},
        {"aether.pitch_cv",  false, "AETH PT", "cv"}, {"aether.mix_cv",     false, "AETH MX", "cv"},
        {"reverb.decay_cv",  false, "REV DEC", "cv"},
    })
end

function R4.on_seq_drag(step_idx, norm_val)
    if step_idx >= 1 and step_idx <= 16 then
        local semi = math.floor((norm_val * 24.0 - 12.0) + 0.5)
        gf.synth.set_seq_step(step_idx, semi, norm_val, true)
    end
end

return R4
