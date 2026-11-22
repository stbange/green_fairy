-- Room 3: 68-Stage Envelope Observatory (16, 0, 10)
-- Interactive 3D Breakpoint Rail + 12 Assignable Envelopes + Env-LFO & Scrubbing

local RB = require("rooms.rack_builder")

local R3 = {
    rail_rack = -1,
    ctrl_rack = -1,
    active_env = 1,
    node_widgets = {},
    max_handles = 12,
    span_sec = 3.0,
}

function R3.build()
    R3.node_widgets = {}
    local cx, cz = 16.0, 10.0

    -- Grand 3D 68-Stage Holographic Envelope Rail Rack
    local rx = cx + math.sin(2.05) * 4.45
    local rz = cz - math.cos(2.05) * 4.45
    local ryaw = math.atan(cx - rx, cz - rz)
    R3.rail_rack = RB.add_rack(3, "68-STAGE HOLOGRAPHIC ENVELOPE RAIL", rx, 1.54, rz, ryaw, 2.75, 1.88,
                               {0.30, 0.85, 1.0, 1.0})

    -- Create 12 draggable 3D breakpoint octahedron handles along the rail
    for i = 1, R3.max_handles do
        R3.node_widgets[i] = gf.spatial.add_widget({
            room = 3, rack = R3.rail_rack, kind = "env_node",
            pos = {-1.1 + (i - 1) * 0.18, 0.0, 0.028},
            size = {0.082, 0.082, 0},
            aux = i, min = 0.0, max = 1.0, default = 0.5,
            color = {0.42, 0.96, 1.0, 1.0},
            action = "env:node",
        })
    end

    -- Envelope Selector Buttons (ENV 1 .. ENV 12) + Add Stage / Rhythmic Preset Buttons
    local btns = {}
    for e = 1, 12 do
        local col = (e - 1) % 6
        local row = math.floor((e - 1) / 6)
        btns[#btns + 1] = {
            kind = "button",
            x = -1.05 + col * 0.34,
            y = -0.34 - row * 0.15,
            w = 0.28, h = 0.105,
            label = "ENV " .. e,
            action = "env:sel:" .. e,
            color = RB.COLOR_SWITCH,
        }
    end
    btns[#btns + 1] = {kind="button", x=0.98, y=-0.34, w=0.44, h=0.11, label="+ STAGE", action="env:add_pt", color=RB.COLOR_BUTTON}
    btns[#btns + 1] = {kind="button", x=0.98, y=-0.49, w=0.44, h=0.11, label="16-PULSE", action="env:pulse",  color=RB.COLOR_DIAL}
    RB.add_controls(3, R3.rail_rack, btns)

    RB.add_jack_strip(3, R3.rail_rack, -0.74, {
        {"env1.out", true, "E1 OUT", "cv"}, {"env2.out", true, "E2 OUT", "cv"},
        {"env3.out", true, "E3 OUT", "cv"}, {"env4.out", true, "E4 OUT", "cv"},
        {"env5.out", true, "E5 OUT", "cv"}, {"env6.out", true, "E6 OUT", "cv"},
        {"env7.out", true, "E7 OUT", "cv"}, {"env8.out", true, "E8 OUT", "cv"},
    })

    -- Companion Envelope Mode, Time Scale, Embedded Env-LFO & Scrub Patchbay Rack
    local kx = cx + math.sin(2.90) * 4.45
    local kz = cz - math.cos(2.90) * 4.45
    local kyaw = math.atan(cx - kx, cz - kz)
    R3.ctrl_rack = RB.add_rack(3, "ENV MODES, EMBEDDED LFO & SCRUB", kx, 1.50, kz, kyaw, 2.35, 1.78,
                               {0.28, 0.80, 0.98, 1.0})

    local m_items = {}
    for e = 1, 4 do
        local y = 0.50 - (e - 1) * 0.24
        local pfx = "env" .. e
        m_items[#m_items + 1] = {kind="step_dial", x=-0.78, y=y, w=0.095, steps=5, min=0, max=4,    param=pfx..".mode",      label="E"..e.." MOD", color=RB.COLOR_DIAL}
        m_items[#m_items + 1] = {kind="knob",      x=-0.26, y=y, w=0.095,          min=0.1,max=6.0, param=pfx..".scale",     label="E"..e.." SPD"}
        m_items[#m_items + 1] = {kind="knob",      x= 0.26, y=y, w=0.095,          min=0.2,max=25.0,param=pfx..".lfo_rate",  label="E"..e.." LFO"}
        m_items[#m_items + 1] = {kind="knob",      x= 0.78, y=y, w=0.095,          min=0.0,max=0.8, param=pfx..".lfo_depth", label="E"..e.." DEP"}
    end
    RB.add_controls(3, R3.ctrl_rack, m_items)

    RB.add_jack_strip(3, R3.ctrl_rack, -0.48, {
        {"env9.out",  true, "E9 OUT",  "cv"}, {"env10.out", true, "E10 OUT", "cv"},
        {"env11.out", true, "E11 OUT", "cv"}, {"env12.out", true, "E12 OUT", "cv"},
        {"env1.rate_cv", false, "E1 SPD", "cv"}, {"env2.rate_cv", false, "E2 SPD", "cv"},
    })
    RB.add_jack_strip(3, R3.ctrl_rack, -0.72, {
        {"env3.rate_cv",  false, "E3 SPD", "cv"}, {"env4.rate_cv",  false, "E4 SPD", "cv"},
        {"env1.scrub_in", false, "E1 SCRB","cv"}, {"env2.scrub_in", false, "E2 SCRB","cv"},
        {"env3.scrub_in", false, "E3 SCRB","cv"}, {"env4.scrub_in", false, "E4 SCRB","cv"},
    })

    R3.sync_rail()
end

function R3.sync_rail()
    local ed = gf.synth.get_envelope(R3.active_env)
    if not ed or not ed.points or #ed.points < 2 then return end
    local pts = ed.points
    local total_t = math.max(0.25, pts[#pts][1])
    R3.span_sec = total_t

    local xs, ys = {}, {}
    local n_curve = math.min(68, #pts)
    for i = 1, n_curve do
        xs[i] = math.min(1.0, math.max(0.0, pts[i][1] / total_t))
        ys[i] = math.min(1.0, math.max(0.0, pts[i][2]))
    end
    gf.spatial.set_curve(7, R3.rail_rack, 0.0, 0.16, 2.38, 0.72, xs, ys, false, 0.35, 0.92, 1.0, 1.0)

    for i = 1, R3.max_handles do
        local wid = R3.node_widgets[i]
        if i <= #pts then
            local nx = xs[i]
            local ny = ys[i]
            local lx = -1.12 + nx * 2.24
            local ly = -0.16 + ny * 0.64
            gf.spatial.set_widget_local_pos(wid, lx, ly, 0.028)
            gf.spatial.set_widget_state(wid, ny, "S" .. i, true)
        else
            gf.spatial.set_widget_state(wid, 0.0, "", false)
        end
    end
end

function R3.on_node_drag(pt_idx, rack_lx, rack_ly)
    local ed = gf.synth.get_envelope(R3.active_env)
    if not ed or not ed.points or pt_idx < 1 or pt_idx > #ed.points then return end
    local pts = ed.points
    local nx = math.min(1.0, math.max(0.0, (rack_lx + 1.12) / 2.24))
    local ny = math.min(1.0, math.max(0.0, (rack_ly + 0.16) / 0.64))
    if pt_idx == 1 then
        pts[1][1] = 0.0
        pts[1][2] = ny
    else
        local min_t = pts[pt_idx - 1][1] + 0.01
        local max_t = (pt_idx < #pts) and math.max(min_t, pts[pt_idx + 1][1] - 0.01) or (R3.span_sec * 1.35)
        pts[pt_idx][1] = math.min(max_t, math.max(min_t, nx * R3.span_sec))
        pts[pt_idx][2] = ny
    end
    gf.synth.set_envelope(R3.active_env, pts, ed.sustain, ed.loop_start, ed.loop_end)
    R3.sync_rail()
end

function R3.on_action(act)
    local sel = act:match("^env:sel:(%d+)$")
    if sel then
        R3.active_env = tonumber(sel)
        R3.sync_rail()
        return true
    end
    if act == "env:add_pt" then
        local ed = gf.synth.get_envelope(R3.active_env)
        if ed and #ed.points < 68 then
            local pts = ed.points
            local last = pts[#pts]
            table.insert(pts, #pts, {last[1] * 0.78, 0.65, 0.0})
            last[1] = last[1] + 0.35
            gf.synth.set_envelope(R3.active_env, pts, ed.sustain, ed.loop_start, ed.loop_end)
            R3.sync_rail()
        end
        return true
    end
    if act == "env:pulse" then
        local pts = {{0.0, 0.0, 0.0}}
        for s = 1, 16 do
            local t = s * 0.15
            pts[#pts + 1] = {t - 0.07, (s % 2 == 1) and 0.95 or 0.35, 0.2}
            pts[#pts + 1] = {t, 0.08, -0.2}
        end
        gf.synth.set_envelope(R3.active_env, pts, #pts - 1, 2, #pts - 1)
        gf.synth.set_param("env" .. R3.active_env .. ".mode", 2)
        R3.sync_rail()
        return true
    end
    return false
end

return R3
