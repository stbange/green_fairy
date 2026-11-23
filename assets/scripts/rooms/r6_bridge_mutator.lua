-- Room 6: Performance Bridge, 3D Keyboard, XYZ Morph Orb & Mutator Lab (0, 0, 0)

local RB = require("rooms.rack_builder")

local R6 = {
    console_rack = -1,
    macro_rack = -1,
    mutator_rack = -1,
    key_widgets = {},
    orb_widget = -1,
    arp_switch = -1,
}

function R6.build()
    R6.key_widgets = {}

    -- 1. Floating 3D Holographic XYZ Morph Orb hovering above central Morph Pedestal (0, 1.18, -1.35)
    R6.orb_widget = gf.spatial.add_widget({
        room = 6, rack = -1, kind = "xyz_orb",
        pos = {0.0, 1.18, -1.35}, size = {0.26, 0.26, 0},
        min = 0.0, max = 1.0, default = 0.5,
        color = {0.42, 1.0, 0.84, 1.0},
        label = "XYZ MORPH ORB",
        action = "xyz:orb",
    })

    -- 2. Grand Tilted 61-Key (5-Octave) 3D Concert Synthesizer Console in front of player
    R6.console_rack = RB.add_rack(6, "61-KEY 3D CONCERT KEYBOARD & TUNING", 0.0, 1.04, 0.85, 0.0, 4.25, 1.38,
                                  {0.20, 0.42, 0.44, 1.0}, -0.72)

    -- 61 Playable 3D Chromatic Synthesizer Keys (C2 = 36 .. C7 = 96: 36 white + 25 raised black)
    local white_step = 0.112
    local x_start = -((36 - 1) * white_step) * 0.5
    local w_idx = 0
    for note = 36, 96 do
        local pc = note % 12
        local is_black = (pc == 1 or pc == 3 or pc == 6 or pc == 8 or pc == 10)
        local lx, ly, lz, kw, kh, col
        if is_black then
            lx = x_start + (w_idx - 0.5) * white_step
            ly = 0.02
            lz = 0.044
            kw, kh = 0.064, 0.30
            col = {0.14, 0.34, 0.34, 1.0}
        else
            w_idx = w_idx + 1
            lx = x_start + (w_idx - 1) * white_step
            ly = -0.10
            lz = 0.015
            kw, kh = 0.104, 0.50
            col = (pc == 0) and {0.78, 0.98, 0.90, 1.0} or {0.88, 0.98, 0.95, 1.0}
        end
        R6.key_widgets[note] = gf.spatial.add_widget({
            room = 6, rack = R6.console_rack, kind = "piano_key",
            pos = {lx, ly, lz},
            size = {kw, kh, 0},
            aux = note, min = 0.0, max = 1.0, default = 0.0,
            color = col,
            action = "key:3d",
        })
    end

    local c_ids = RB.add_controls(6, R6.console_rack, {
        {kind="switch",    x=-1.60, y=0.44, w=0.10,           min=0, max=1,   param="voice.drone_latch", label="DRONE",   color=RB.COLOR_SWITCH},
        {kind="switch",    x=-0.96, y=0.44, w=0.10,           min=0, max=1,   default=0,                 label="ARP ON",  action="arp:toggle", color=RB.COLOR_SWITCH},
        {kind="step_dial", x=-0.32, y=0.44, w=0.10, steps=8,  min=0, max=7,   param="tuning.scale",      label="SCALE",   color=RB.COLOR_DIAL},
        {kind="knob",      x= 0.32, y=0.44, w=0.10,           min=415,max=466,param="tuning.a4",         label="A4 HZ"},
        {kind="knob",      x= 0.96, y=0.44, w=0.10,           min=0, max=0.6, param="voice.glide",       label="GLIDE"},
        {kind="switch",    x= 1.60, y=0.44, w=0.10,           min=0, max=1,   param="xy.enabled",        label="XYZ ON",  color=RB.COLOR_SWITCH},
    })
    R6.arp_switch = c_ids[2]

    RB.add_jack_strip(6, R6.console_rack, -0.54, {
        {"midi.pitch_cv",      true, "KEY CV",  "cv"},
        {"midi.gate_out",      true, "GATE",    "trig"},
        {"midi.vel_out",       true, "VEL CV",  "cv"},
        {"midi.aftertouch_out",true, "AT CV",   "cv"},
        {"xyz.x_out",          true, "XYZ X",   "cv"},
        {"xyz.y_out",          true, "XYZ Y",   "cv"},
        {"xyz.z_out",          true, "XYZ Z",   "cv"},
    })

    -- 3. 8-Macro Performance & CV Output Rack (-3.85, 1.50, -2.55)
    local mx, mz = -3.85, -2.55
    R6.macro_rack = RB.add_rack(6, "8X PERFORMANCE MACROS & CV", mx, 1.50, mz, math.atan(-mx, -mz), 2.30, 1.72,
                                {0.32, 0.92, 0.82, 1.0})
    local m_knobs, m_jacks = {}, {}
    for m = 1, 8 do
        local col = (m - 1) % 4
        local row = math.floor((m - 1) / 4)
        m_knobs[m] = {
            kind = "knob", x = -0.75 + col * 0.50, y = 0.42 - row * 0.36, w = 0.115,
            min = 0.0, max = 1.0, param = "macro" .. m, label = "MACRO " .. m,
        }
        m_jacks[m] = {"macro" .. m .. ".out", true, "M" .. m .. " OUT", "cv"}
    end
    RB.add_controls(6, R6.macro_rack, m_knobs)
    RB.add_jack_strip(6, R6.macro_rack, -0.66, m_jacks)

    -- 4. Absynth Sound-Breeding Mutator Lab Rack (3.85, 1.50, -2.55)
    local bx, bz = 3.85, -2.55
    R6.mutator_rack = RB.add_rack(6, "ABSYNTH SOUND MUTATOR LAB", bx, 1.50, bz, math.atan(-bx, -bz), 2.30, 1.72,
                                  {0.48, 1.0, 0.72, 1.0})
    RB.add_controls(6, R6.mutator_rack, {
        {kind="knob",   x=-0.50, y= 0.40, w=0.13, min=0.05,max=1.0, default=0.28, label="MUT DEPTH", action="mutator:depth"},
        {kind="knob",   x= 0.50, y= 0.40, w=0.13, min=0.05,max=1.0, default=0.35, label="RANDOM",    action="mutator:rand"},
        {kind="button", x=-0.52, y=-0.02, w=0.78, h=0.16, label="BREED MUTATION", action="mutator:mutate",    color={0.30, 1.0, 0.72, 1.0}},
        {kind="button", x= 0.52, y=-0.02, w=0.78, h=0.16, label="WILD EVOLVE",    action="mutator:wild",      color=RB.COLOR_DIAL},
        {kind="button", x=-0.52, y=-0.34, w=0.78, h=0.14, label="UNDO MUTATION",  action="mutator:undo",      color=RB.COLOR_SWITCH},
        {kind="button", x= 0.52, y=-0.34, w=0.78, h=0.14, label="VOXELS TOGGLE",  action="scene:voxels",      color=RB.COLOR_BUTTON},
        {kind="button", x= 0.00, y=-0.62, w=0.95, h=0.13, label="INIT DEFAULT",   action="mutator:reset_init",color={0.9, 0.55, 0.45, 1.0}},
    })

    -- 5. Cinematic Phrase Looper & Hans Zimmer Score Vault Rack (-4.35, 1.50, 0.30)
    local lx, lz = -4.35, 0.30
    R6.looper_rack = RB.add_rack(6, "CINEMATIC PHRASE LOOPER & ZIMMER VAULT", lx, 1.50, lz,
                                 math.atan(0.0 - lx, 1.8 - lz), 2.45, 1.82, {0.96, 0.78, 0.36, 1.0})
    RB.add_controls(6, R6.looper_rack, {
        {kind="knob",      x=-0.82, y= 0.50, w=0.11,           min=0.4, max=2.0, default=1.0, label="SPEED",     action="looper:speed"},
        {kind="step_dial", x=-0.28, y= 0.50, w=0.11, steps=25, min=-12, max=12,  default=0.0, label="TRANSPOSE", action="looper:transpose", color=RB.COLOR_DIAL},
        {kind="knob",      x= 0.28, y= 0.50, w=0.11,           min=0.3, max=1.4, default=1.0, label="DYNAMICS",  action="looper:vel"},
        {kind="switch",    x= 0.82, y= 0.50, w=0.10,           min=0,   max=1,   default=0.0, label="OVERDUB",   action="looper:overdub",   color=RB.COLOR_SWITCH},

        {kind="button",    x=-0.72, y=-0.22, w=0.62, h=0.14, label="REC PHRASE",  action="looper:record",     color={0.98, 0.40, 0.36, 1.0}},
        {kind="button",    x= 0.00, y=-0.22, w=0.62, h=0.14, label="PLAY / STOP", action="looper:play_toggle",color={0.32, 1.00, 0.76, 1.0}},
        {kind="button",    x= 0.72, y=-0.22, w=0.62, h=0.14, label="CLEAR LOOP",  action="looper:clear",      color=RB.COLOR_SWITCH},

        {kind="button",    x=-0.82, y=-0.46, w=0.46, h=0.13, label="< MELODY",    action="looper:step:-1",    color=RB.COLOR_DIAL},
        {kind="button",    x=-0.28, y=-0.46, w=0.46, h=0.13, label="MELODY >",    action="looper:step:1",     color=RB.COLOR_DIAL},
        {kind="button",    x= 0.28, y=-0.46, w=0.46, h=0.13, label="< SUITE",     action="looper:suite:-1",   color=RB.COLOR_BUTTON},
        {kind="button",    x= 0.82, y=-0.46, w=0.46, h=0.13, label="SUITE >",     action="looper:suite:1",    color=RB.COLOR_BUTTON},

        {kind="button",    x= 0.00, y=-0.68, w=1.25, h=0.12, label="LOAD PAIRED PATCH", action="looper:pair_patch", color={0.55, 0.86, 1.0, 1.0}},
    })
end

function R6.refresh_looper_curve(looper)
    if R6.looper_rack and R6.looper_rack >= 0 and looper then
        local ys = looper:build_curve(40)
        local is_rec = (looper.state == "recording" or looper.state == "rec_armed")
        gf.spatial.set_curve(9, R6.looper_rack, 0.0, 0.14, 2.05, 0.30, nil, ys, true,
                             is_rec and 1.0 or 0.96, is_rec and 0.38 or 0.82, is_rec and 0.35 or 0.34, 0.95)
    end
end

function R6.set_key_visual(note, pressed)
    local wid = R6.key_widgets[note]
    if wid then
        gf.spatial.set_widget_state(wid, pressed and 1.0 or 0.0)
    end
end

return R6
