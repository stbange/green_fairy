-- Room 5: Aetherium & Acoustics Chamber (-16, 0, 10)
-- Aetherizer Pro, Waveguide Pipe, 6-Voice Chordal Multi-Comb, 6-Tap Delay & 8x8 FDN Space Reverb

local RB = require("rooms.rack_builder")

local R5 = {
    aether_rack = -1,
    pipe_rack = -1,
    reverb_rack = -1,
}

function R5.build()
    local cx, cz = -16.0, 10.0

    -- Rack 1: Aetherizer Pro 24-Grain Crystal Cloud & 6-Tap Stereo Delay
    local ax, az = cx - 4.45, cz + 1.2
    R5.aether_rack = RB.add_rack(5, "AETHERIZER PRO & MULTITAP", ax, 1.52, az, math.atan(cx - ax, cz - az), 2.40, 1.82,
                                 {0.78, 0.44, 1.0, 1.0})
    RB.add_controls(5, R5.aether_rack, {
        {kind="knob", x=-0.90, y= 0.46, min=0.05,max=0.95, param="aether.time",     label="GRAIN TM"},
        {kind="knob", x=-0.45, y= 0.46, min=0.0, max=0.92, param="aether.feedback", label="FEEDBACK"},
        {kind="knob", x= 0.00, y= 0.46, min=0.0, max=1.0,  param="aether.mod",      label="MODUL"},
        {kind="knob", x= 0.45, y= 0.46, min=0.0, max=0.95, param="aether.damping",  label="DAMPING"},
        {kind="knob", x= 0.90, y= 0.46, min=-24, max=24.0, param="aether.pitch",    label="CRYST PT"},
        {kind="knob", x=-0.90, y= 0.12, min=0.0, max=1.0,  param="aether.reverse",  label="REV PRB"},
        {kind="knob", x=-0.45, y= 0.12, min=0.0, max=1.0,  param="aether.mix",      label="AETH MIX"},
        {kind="knob", x= 0.05, y= 0.12, min=0.04,max=0.90, param="multitap.time",   label="TAP TIME"},
        {kind="knob", x= 0.48, y= 0.12, min=0.0, max=0.88, param="multitap.feedback",label="TAP FBK"},
        {kind="knob", x= 0.90, y= 0.12, min=0.0, max=1.0,  param="multitap.mix",    label="TAP MIX"},
        {kind="knob", x= 0.48, y=-0.22, min=0.0, max=1.0,  param="multitap.spread", label="TAP SPRD"},
    })
    RB.add_jack_strip(5, R5.aether_rack, -0.68, {
        {"aether.in",       false, "AETH IN",  "audio"}, {"aether.out",      true,  "AETH OUT", "audio"},
        {"aether.pitch_cv", false, "PITCH CV", "cv"},    {"aether.mix_cv",   false, "MIX CV",   "cv"},
        {"multitap.in",     false, "TAP IN",   "audio"}, {"multitap.out",    true,  "TAP OUT",  "audio"},
        {"multitap.time_cv",false, "TIME CV",  "cv"},
    })

    -- Rack 2: Bidirectional Waveguide Acoustic Pipe & 6-Voice Chordal Multi-Comb
    local px, pz = cx - 2.65, cz + 3.95
    R5.pipe_rack = RB.add_rack(5, "WAVEGUIDE PIPE & CHORDAL COMB", px, 1.52, pz, math.atan(cx - px, cz - pz), 2.35, 1.82,
                               {0.68, 0.52, 1.0, 1.0})
    RB.add_controls(5, R5.pipe_rack, {
        {kind="knob",      x=-0.88, y= 0.46,          min=35, max=1200, param="pipe.freq",          label="PIPE HZ"},
        {kind="knob",      x=-0.44, y= 0.46,          min=0,  max=0.97, param="pipe.feedback",      label="PIPE FBK"},
        {kind="knob",      x= 0.00, y= 0.46,          min=0,  max=0.95, param="pipe.damping",       label="PIPE DMP"},
        {kind="knob",      x= 0.44, y= 0.46,          min=0.05,max=0.95,param="pipe.pickup",        label="PICKUP"},
        {kind="knob",      x= 0.88, y= 0.46,          min=0,  max=1.0,  param="pipe.mix",           label="PIPE MIX"},
        {kind="knob",      x=-0.88, y= 0.10,          min=40, max=880,  param="multicomb.freq",     label="COMB HZ"},
        {kind="step_dial", x=-0.44, y= 0.10, steps=6, min=0,  max=5,    param="multicomb.chord",    label="CHORD",   color=RB.COLOR_DIAL},
        {kind="knob",      x= 0.00, y= 0.10,          min=0,  max=0.96, param="multicomb.feedback", label="COMB FBK"},
        {kind="knob",      x= 0.44, y= 0.10,          min=0,  max=0.95, param="multicomb.damping",  label="COMB DMP"},
        {kind="knob",      x= 0.88, y= 0.10,          min=0,  max=1.0,  param="multicomb.mix",      label="COMB MIX"},
    })
    RB.add_jack_strip(5, R5.pipe_rack, -0.68, {
        {"pipe.in",          false, "PIPE IN",  "audio"}, {"pipe.out",          true,  "PIPE OUT", "audio"},
        {"pipe.freq_cv",     false, "TENS CV",  "cv"},    {"multicomb.in",      false, "COMB IN",  "audio"},
        {"multicomb.out",    true,  "COMB OUT", "audio"}, {"multicomb.tune_cv", false, "TUNE CV",  "cv"},
    })

    -- Rack 3: 8x8 Hadamard FDN Space Cathedral Reverb & Stereo Master Output Bus
    local vx, vz = cx - 4.15, cz - 1.95
    R5.reverb_rack = RB.add_rack(5, "8X8 HADAMARD CATHEDRAL REVERB", vx, 1.52, vz, math.atan(cx - vx, cz - vz), 2.45, 1.85,
                                 {0.85, 0.50, 1.0, 1.0})
    RB.add_controls(5, R5.reverb_rack, {
        {kind="knob",   x=-0.92, y= 0.48, min=0.1, max=0.99, param="reverb.decay",   label="DECAY"},
        {kind="knob",   x=-0.46, y= 0.48, min=0.0, max=0.95, param="reverb.damping", label="DAMPING"},
        {kind="knob",   x= 0.00, y= 0.48, min=0.0, max=1.0,  param="reverb.shimmer", label="SHIMMER"},
        {kind="knob",   x= 0.46, y= 0.48, min=0.25,max=1.5,  param="reverb.size",    label="HALL SZ"},
        {kind="knob",   x= 0.92, y= 0.48, min=0.0, max=1.0,  param="reverb.mix",     label="REV MIX"},
        {kind="switch", x=-0.46, y= 0.14, min=0.0, max=1.0,  param="reverb.freeze",  label="FREEZE",  color=RB.COLOR_SWITCH},
        {kind="fader",  x= 0.46, y= 0.08, min=0.0, max=1.0,  param="master.gain",    label="MASTER",  track_len=0.30},
    })

    RB.add_jack_strip(5, R5.reverb_rack, -0.42, {
        {"voice.bus_l", false, "VBUS L", "audio"}, {"voice.bus_r", false, "VBUS R", "audio"},
        {"voice.out_l", true,  "VOUT L", "audio"}, {"voice.out_r", true,  "VOUT R", "audio"},
        {"voice.amp_cv",false, "V AMP",  "cv"},    {"voice.pan_cv",false, "V PAN",  "cv"},
    })
    RB.add_jack_strip(5, R5.reverb_rack, -0.72, {
        {"reverb.in",          false, "REV IN",  "audio"}, {"reverb.out",      true,  "REV OUT", "audio"},
        {"reverb.decay_cv",    false, "DEC CV",  "cv"},    {"reverb.freeze_gate",false,"FRZ GT", "trig"},
        {"feedback.bus_out",   true,  "FBK OUT", "audio"},
        {"master.in_l",        false, "MST L",   "audio"}, {"master.in_r",     false, "MST R",   "audio"},
    })
end

function R5.update_spectrum(tel)
    if tel and tel.bands then
        gf.spatial.set_curve(8, R5.reverb_rack, -0.15, -0.14, 1.15, 0.24, nil, tel.bands, true, 0.85, 0.52, 1.0, 0.92)
    end
end

return R5
