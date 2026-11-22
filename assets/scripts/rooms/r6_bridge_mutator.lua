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

    -- 2. Tilted 3D Synthesizer Keyboard & Performance Console in front of player (0, 1.02, 0.75)
    R6.console_rack = RB.add_rack(6, "3D PERFORMANCE KEYBOARD & TUNING", 0.0, 1.04, 0.85, 0.0, 2.75, 1.15,
                                  {0.32, 0.96, 0.78, 1.0}, -0.72)

    -- 25 Playable 3D Synthesizer Keys (C3 = 48 .. C5 = 72)
    for k = 0, 24 do
        local note = 48 + k
        local pc = note % 12
        local is_black = (pc == 1 or pc == 3 or pc == 6 or pc == 8 or pc == 10)
        local lx = -1.20 + k * 0.10
        local ly = is_black and 0.04 or -0.14
        local lz = is_black and 0.038 or 0.014
        R6.key_widgets[note] = gf.spatial.add_widget({
            room = 6, rack = R6.console_rack, kind = "piano_key",
            pos = {lx, ly, lz},
            size = {is_black and 0.068 or 0.088, is_black and 0.24 or 0.34, 0},
            aux = note, min = 0.0, max = 1.0, default = 0.0,
            color = is_black and {0.22, 0.55, 0.48, 1.0} or {0.86, 0.98, 0.94, 1.0},
            action = "key:3d",
        })
    end

    local c_ids = RB.add_controls(6, R6.console_rack, {
        {kind="switch",    x=-1.05, y=0.33, w=0.095,          min=0, max=1,   param="voice.drone_latch", label="DRONE",   color=RB.COLOR_SWITCH},
        {kind="switch",    x=-0.65, y=0.33, w=0.095,          min=0, max=1,   default=0,                 label="ARP ON",  action="arp:toggle", color=RB.COLOR_SWITCH},
        {kind="step_dial", x=-0.20, y=0.33, w=0.095, steps=8, min=0, max=7,   param="tuning.scale",      label="SCALE",   color=RB.COLOR_DIAL},
        {kind="knob",      x= 0.25, y=0.33, w=0.095,          min=415,max=466,param="tuning.a4",         label="A4 HZ"},
        {kind="knob",      x= 0.68, y=0.33, w=0.095,          min=0, max=0.6, param="voice.glide",       label="GLIDE"},
        {kind="switch",    x= 1.05, y=0.33, w=0.095,          min=0, max=1,   param="xy.enabled",        label="XYZ ON",  color=RB.COLOR_SWITCH},
    })
    R6.arp_switch = c_ids[2]

    RB.add_jack_strip(6, R6.console_rack, -0.44, {
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
end

function R6.set_key_visual(note, pressed)
    local wid = R6.key_widgets[note]
    if wid then
        gf.spatial.set_widget_state(wid, pressed and 1.0 or 0.0)
    end
end

return R6
