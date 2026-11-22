-- Declarative 3D Synth Rack & Modular Hardware Control Builder

local RB = {}

RB.COLOR_AUDIO   = {0.22, 0.98, 0.66, 1.0}
RB.COLOR_CV_OUT  = {0.32, 0.84, 1.00, 1.0}
RB.COLOR_CV_IN   = {0.98, 0.74, 0.26, 1.0}
RB.COLOR_TRIG    = {1.00, 0.52, 0.82, 1.0}
RB.COLOR_KNOB    = {0.36, 0.92, 0.78, 1.0}
RB.COLOR_DIAL    = {0.95, 0.78, 0.34, 1.0}
RB.COLOR_SWITCH  = {0.48, 0.86, 1.00, 1.0}
RB.COLOR_BUTTON  = {0.32, 0.95, 0.75, 1.0}

function RB.add_rack(room, title, x, y, z, rot_y, width, height, color, tilt_x)
    return gf.spatial.add_rack({
        room = room,
        title = title,
        pos = {x, y, z},
        rot_y = rot_y or 0.0,
        tilt_x = tilt_x or 0.0,
        width = width,
        height = height,
        color = color or {0.28, 0.85, 0.72, 1.0},
    })
end

function RB.add_controls(room, rack_id, items)
    local ids = {}
    for i, it in ipairs(items) do
        local cur_def = it.default
        if cur_def == nil and it.param then
            cur_def = gf.synth.get_param(it.param)
        end
        ids[i] = gf.spatial.add_widget({
            room = room,
            rack = rack_id,
            kind = it.kind or "knob",
            pos = {it.x, it.y, it.z or 0.015},
            size = {it.w or 0.115, it.h or (it.w or 0.115), 0},
            track_len = it.track_len or 0.28,
            param = it.param,
            port = it.port,
            aux = it.aux or 0,
            steps = it.steps or 2,
            min = it.min or 0.0,
            max = it.max or 1.0,
            default = cur_def or (it.min or 0.0),
            color = it.color or RB.COLOR_KNOB,
            label = it.label or "",
            action = it.action or "",
        })
    end
    return ids
end

function RB.add_jack_strip(room, rack_id, y_row, ports)
    local n = #ports
    if n == 0 then return {} end
    local span = math.min(2.2, n * 0.23)
    local start_x = -span * 0.5 + (span / (2.0 * n))
    local step_x = (n > 1) and ((span - span / n) / (n - 1)) or 0.0
    local ids = {}
    for i, p in ipairs(ports) do
        local is_out = p[2]
        local sig = p[4] or "cv"
        local col = (sig == "audio") and RB.COLOR_AUDIO
                 or (sig == "trig")  and RB.COLOR_TRIG
                 or (is_out and RB.COLOR_CV_OUT or RB.COLOR_CV_IN)
        ids[i] = gf.spatial.add_widget({
            room = room,
            rack = rack_id,
            kind = is_out and "jack_out" or "jack_in",
            pos = {start_x + (i - 1) * step_x, y_row, 0.016},
            size = {0.092, 0.092, 0},
            port = p[1],
            min = 0.0,
            max = 1.0,
            default = 0.0,
            color = col,
            label = p[3] or p[1],
        })
    end
    return ids
end

return RB
