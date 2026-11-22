-- Green Fairy v2.0: Seven-Room Retro-3D Modular Sanctum Entry Orchestrator

local Arp       = require("dsp.arp")
local Mutator   = require("dsp.mutator")
local Floorplan = require("rooms.sanctum_floorplan")
local R1        = require("rooms.r1_oscillators")
local R2        = require("rooms.r2_filters")
local R3        = require("rooms.r3_envelopes")
local R4        = require("rooms.r4_modulation_patchbay")
local R5        = require("rooms.r5_aetherium_fx")
local R6        = require("rooms.r6_bridge_mutator")
local R7        = require("rooms.r7_preset_archive")
local Compass   = require("ui.compass_minimap")

local State = {
    mode = "desktop",
    preset_name = "Emerald Aether",
    voices = 0,
    arp = Arp.new(),
    pressed_notes = {},
    walk_keys = {w = false, s = false, a = false, d = false},
    drag = nil, -- Active 3D pointer/XR drag interaction (bound to pointer id)
    scene_touches = {}, -- pointer_id -> {x = ..., y = ...} for 1-finger look & 2-finger pinch/walk
    pinch_prev = nil,
}

function State.note_on(note, vel)
    State.pressed_notes[note] = true
    R6.set_key_visual(note, true)
    State.arp:note_on(note, vel or 0.85)
end

function State.note_off(note)
    State.pressed_notes[note] = nil
    R6.set_key_visual(note, false)
    State.arp:note_off(note)
end

local function clamp01(v)
    if v < 0.0 then return 0.0 end
    if v > 1.0 then return 1.0 end
    return v
end

local function apply_widget_norm(w_info, norm)
    norm = clamp01(norm)
    gf.spatial.set_widget_state(w_info.id, norm)
    local real_v = w_info.min + norm * (w_info.max - w_info.min)
    if w_info.kind == "step_dial" and w_info.steps and w_info.steps > 1 then
        local st = math.floor(norm * (w_info.steps - 1) + 0.5)
        real_v = w_info.min + (st / (w_info.steps - 1)) * (w_info.max - w_info.min)
    end
    if w_info.param then
        gf.synth.set_param(w_info.param, real_v)
        if w_info.param:find("filter") then R2.refresh_curves() end
    elseif w_info.action == "mutator:depth" then
        Mutator.depth = real_v
    elseif w_info.action == "mutator:rand" then
        Mutator.randomness = real_v
    elseif w_info.action == "harm:bar" then
        R1.on_harmonic_drag(w_info.aux, norm)
    elseif w_info.action == "seq:step" then
        R4.on_seq_drag(w_info.aux, norm)
    end
    gf.spatial.set_tooltip(true, string.format("%s: %.2f", w_info.label ~= "" and w_info.label or "VAL", real_v), w_info.id)
end

local function dispatch_button_action(act)
    if not act or act == "" then return end
    if R1.on_action(act) or R3.on_action(act) or R7.on_action(act) then
        State.preset_name = R7.active_name
        return
    end
    if act == "mutator:mutate" then
        Mutator.mutate(Mutator.depth, Mutator.randomness)
        R1.refresh_curves(); R2.refresh_curves(); R3.sync_rail()
    elseif act == "mutator:wild" then
        Mutator.mutate(math.min(1.0, Mutator.depth * 1.85), math.min(1.0, Mutator.randomness * 1.75))
        R1.refresh_curves(); R2.refresh_curves(); R3.sync_rail()
    elseif act == "mutator:undo" then
        Mutator.undo()
        R1.refresh_curves(); R2.refresh_curves(); R3.sync_rail()
    elseif act == "mutator:reset_init" then
        Mutator.push_undo()
        gf.synth.reset_default()
        gf.spatial.sync_widgets()
        R1.refresh_curves(); R2.refresh_curves(); R3.sync_rail()
    elseif act == "scene:voxels" then
        gf.scene.set_voxels_enabled(not gf.scene.get_voxels_enabled())
    end
end

local function begin_3d_pick(pk, x, y, stack_mod)
    if pk.hit == "ring" then
        gf.spatial.set_ring_hover(pk.cable_id, pk.cable_id)
        State.drag = {kind = "ring", cable_id = pk.cable_id, start_y = y, start_gain = pk.gain or 0.75}
        return true
    end
    if pk.hit ~= "widget" then return false end

    gf.spatial.set_hover(pk.id, pk.id)
    local k = pk.kind
    if k == "rack_header" then
        gf.spatial.focus_rack(pk.rack)
        return true
    elseif k == "jack_out" or k == "jack_in" then
        gf.spatial.cable_begin(pk.port_id, stack_mod == true)
        State.drag = {kind = "cable", dist = pk.dist or 2.0}
        return true
    elseif k == "switch" then
        local nxt = (pk.norm < 0.5) and 1.0 or 0.0
        gf.spatial.set_widget_state(pk.id, nxt)
        if pk.param then
            gf.synth.set_param(pk.param, nxt >= 0.5 and pk.max or pk.min)
            if pk.param == "voice.drone_latch" and nxt < 0.5 then gf.synth.all_notes_off() end
        elseif pk.action == "arp:toggle" then
            State.arp:set_enabled(nxt >= 0.5)
        end
        return true
    elseif k == "button" then
        dispatch_button_action(pk.action)
        return true
    elseif k == "piano_key" then
        State.note_on(pk.aux, 0.88)
        State.drag = {kind = "piano_key", note = pk.aux}
        return true
    elseif k == "env_node" then
        State.drag = {kind = "env_node", rack = pk.rack, aux = pk.aux}
        return true
    elseif k == "xyz_orb" then
        State.drag = {
            kind = "xyz_orb", start_x = x, start_y = y,
            ox = gf.synth.get_param("xy.x") or 0.5,
            oy = gf.synth.get_param("xy.y") or 0.5,
            oz = gf.synth.get_param("xy.z") or 0.5,
        }
        return true
    else
        -- Rotary knob, fader, step_dial, harmonic_bar
        State.drag = {kind = "value", info = pk, start_y = y, start_norm = pk.norm or 0.5}
        return true
    end
end

function gf_init()
    State.mode = gf.ui.input_mode()
    gf.scene.clear()
    gf.spatial.clear()

    Floorplan.build_architecture()
    R1.build()
    R2.build()
    R3.build()
    R4.build()
    R5.build()
    R6.build()
    R7.build()

    if #R7.all_presets > 0 then
        R7.select_global(1)
        State.preset_name = R7.active_name
    end
    gf.spatial.teleport_room(6)
    gf.spatial.sync_widgets()
    Mutator.clear_history()
    gf.log(string.format("Green Fairy 7-Room Sanctum initialized (%d presets, mode=%s)", #R7.all_presets, State.mode))
end

function gf_on_preset_path(rel_path)
    local meta = gf.presets.apply_file(rel_path, true)
    if not meta then return false end
    R7.active_name = meta.name or rel_path
    State.preset_name = R7.active_name
    R1.refresh_curves()
    R2.refresh_curves()
    R3.sync_rail()
    R7.refresh_labels()
    Mutator.clear_history()
    return true
end

function gf_on_mutator(action, amount, randomness)
    if action == "undo" then
        Mutator.undo()
    else
        Mutator.mutate(amount, randomness)
    end
    R1.refresh_curves(); R2.refresh_curves(); R3.sync_rail()
end

function gf_update(dt, tel)
    State.voices = tel and tel.active_voices or 0
    local fwd = (State.walk_keys.w and 1 or 0) - (State.walk_keys.s and 1 or 0)
    local rgt = (State.walk_keys.d and 1 or 0) - (State.walk_keys.a and 1 or 0)
    if fwd ~= 0 or rgt ~= 0 then
        gf.spatial.walk(fwd, rgt, dt)
    end
    Compass.step_touch_walk(dt)
    Floorplan.update(dt, tel)
    R5.update_spectrum(tel)
    State.arp:update(dt)
end

function gf_render_ui(vp_w, vp_h)
    Compass.render(vp_w, vp_h, State.mode, State)
end

function gf_on_mode_change(new_mode)
    State.mode = new_mode
    gf.log("Input mode changed to: " .. new_mode)
end

function gf_on_input(ev)
    local vw, vh = gf.ui.viewport()
    if ev.type == "note_on" then
        State.note_on(ev.id, ev.extra > 0 and ev.extra or 0.85)
        return
    elseif ev.type == "note_off" then
        State.note_off(ev.id)
        return
    elseif ev.type == "nav_key" then
        local down = (ev.extra > 0.5)
        local kc = ev.id
        if kc == 119 then State.walk_keys.w = down
        elseif kc == 115 then State.walk_keys.s = down
        elseif kc == 97  then State.walk_keys.a = down
        elseif kc == 100 then State.walk_keys.d = down
        elseif down and kc >= 49 and kc <= 55 then
            gf.spatial.teleport_room(kc - 48)
        elseif down and kc == 27 then
            gf.spatial.exit_focus()
        end
        return
    elseif ev.type == "mouse_look" then
        gf.spatial.look(ev.x * 0.0045, -ev.y * 0.0038)
        return
    elseif ev.type == "xr_trigger" then
        if ev.extra > 0.5 then State.note_on(60 + (ev.id % 4) * 4, ev.extra)
        else State.note_off(60 + (ev.id % 4) * 4) end
        return
    end

    -- 1. Check minimal Compass / Mobile Touch Strip HUD first
    if Compass.handle_hud_pointer(ev, vw, vh, State.mode, State) then
        return
    end

    -- 2. 3D World-Space Raycast Interaction & Multi-Finger Navigation across all 7 rooms
    local pid = ev.id or 0

    local function scene_touch_pair()
        local a, b, cnt = nil, nil, 0
        for _, pt in pairs(State.scene_touches) do
            cnt = cnt + 1
            if cnt == 1 then a = pt elseif cnt == 2 then b = pt end
        end
        return cnt, a, b
    end

    local function sync_pinch_anchor()
        local cnt, a, b = scene_touch_pair()
        if cnt >= 2 and a and b then
            local dx, dy = b.x - a.x, b.y - a.y
            State.pinch_prev = {
                mx = (a.x + b.x) * 0.5,
                my = (a.y + b.y) * 0.5,
                dist = math.sqrt(dx * dx + dy * dy),
            }
        else
            State.pinch_prev = nil
        end
    end

    if ev.type == "pointer_down" then
        if ev.extra == 2 then
            State.scene_touches[pid] = {x = ev.x, y = ev.y}
            sync_pinch_anchor()
            return
        end
        if not State.drag then
            local pk = gf.spatial.pick_screen(ev.x, ev.y)
            if begin_3d_pick(pk, ev.x, ev.y, ev.extra == 1) then
                if State.drag then State.drag.id = pid end
                return
            end
        end
        -- Open 3D space touch/drag: 1 finger smoothly orbits look; 2 fingers pinch/glide to walk
        State.scene_touches[pid] = {x = ev.x, y = ev.y}
        sync_pinch_anchor()
    elseif ev.type == "pointer_move" then
        if State.drag and (State.drag.id == nil or State.drag.id == pid) then
            local pk = gf.spatial.pick_screen(ev.x, ev.y)
            local dr = State.drag
            if dr.kind == "cable" then
                gf.spatial.cable_update_screen(ev.x, ev.y, pk.hit == "widget" and pk.id or -1, dr.dist)
            elseif dr.kind == "ring" then
                local ng = math.max(-2.0, math.min(2.0, dr.start_gain + (dr.start_y - ev.y) / 95.0))
                gf.patchbay.set_gain(dr.cable_id, ng)
            elseif dr.kind == "value" then
                local n = dr.start_norm + (dr.start_y - ev.y) / 140.0
                apply_widget_norm(dr.info, n)
            elseif dr.kind == "env_node" then
                local lx, ly = gf.spatial.ray_to_rack(dr.rack, ev.x, ev.y)
                if lx and ly then R3.on_node_drag(dr.aux, lx, ly) end
            elseif dr.kind == "xyz_orb" then
                local nx = clamp01(dr.ox + (ev.x - dr.start_x) / 180.0)
                local ny = clamp01(dr.oy + (dr.start_y - ev.y) / 180.0)
                local nz = clamp01(dr.oz + (dr.start_y - ev.y) * 0.5 / 180.0)
                gf.synth.set_xyz(nx, ny, nz)
                gf.spatial.sync_widgets()
            elseif dr.kind == "piano_key" and pk.hit == "widget" and pk.kind == "piano_key" and pk.aux ~= dr.note then
                State.note_off(dr.note)
                dr.note = pk.aux
                State.note_on(dr.note, 0.88)
            end
            return
        end

        local st = State.scene_touches[pid]
        if st then
            local prev_x, prev_y = st.x, st.y
            st.x, st.y = ev.x, ev.y
            local cnt, a, b = scene_touch_pair()
            if cnt == 1 then
                local sx = (State.mode == "touch") and (1.55 / math.max(320.0, vw)) or 0.0042
                local sy = (State.mode == "touch") and (1.25 / math.max(320.0, vh)) or 0.0035
                local dx = math.max(-0.11, math.min(0.11, (ev.x - prev_x) * sx))
                local dy = math.max(-0.10, math.min(0.10, -(ev.y - prev_y) * sy))
                gf.spatial.look(dx, dy)
            elseif cnt >= 2 and a and b then
                local ddx, ddy = b.x - a.x, b.y - a.y
                local mx, my = (a.x + b.x) * 0.5, (a.y + b.y) * 0.5
                local dist = math.sqrt(ddx * ddx + ddy * ddy)
                if State.pinch_prev then
                    local pinch_d = dist - State.pinch_prev.dist
                    local pan_x = mx - State.pinch_prev.mx
                    local pan_y = State.pinch_prev.my - my
                    local fwd = math.max(-0.35, math.min(0.35, pinch_d * 0.015 + pan_y * 0.009))
                    local rgt = math.max(-0.30, math.min(0.30, pan_x * 0.008))
                    if math.abs(fwd) > 1e-4 or math.abs(rgt) > 1e-4 then
                        gf.spatial.walk(fwd, rgt, 0.16)
                    end
                end
                State.pinch_prev = {mx = mx, my = my, dist = dist}
            end
            return
        end

        if not State.drag and State.mode ~= "touch" then
            local pk = gf.spatial.pick_screen(ev.x, ev.y)
            gf.spatial.set_hover(pk.hit == "widget" and pk.id or -1, -1)
            gf.spatial.set_ring_hover(pk.hit == "ring" and pk.cable_id or -1, -1)
        end
    elseif ev.type == "pointer_up" then
        if State.scene_touches[pid] then
            State.scene_touches[pid] = nil
            sync_pinch_anchor()
        end
        if State.drag and (State.drag.id == nil or State.drag.id == pid) then
            if State.drag.kind == "cable" then
                gf.spatial.cable_finish()
            elseif State.drag.kind == "piano_key" then
                State.note_off(State.drag.note)
            end
            State.drag = nil
            gf.spatial.set_hover(-1, -1)
            gf.spatial.set_ring_hover(-1, -1)
            gf.spatial.set_tooltip(false)
        end
    end
end

function gf_on_xr_ray(ev)
    local pk = gf.spatial.pick_ray(ev.ox, ev.oy, ev.oz, ev.dx, ev.dy, ev.dz)
    local hit_d = pk.dist or 3.0
    gf.scene.set_xr_laser(true, ev.ox, ev.oy, ev.oz,
                          ev.ox + ev.dx * hit_d, ev.oy + ev.dy * hit_d, ev.oz + ev.dz * hit_d)

    if ev.type == "xr_down" then
        begin_3d_pick(pk, ev.ox * 200.0, -ev.oy * 200.0, false)
    elseif ev.type == "xr_move" and State.drag then
        if State.drag.kind == "cable" then
            gf.spatial.cable_update_ray(ev.ox, ev.oy, ev.oz, ev.dx, ev.dy, ev.dz,
                                        pk.hit == "widget" and pk.id or -1, State.drag.dist)
        elseif State.drag.kind == "value" then
            local n = State.drag.start_norm + ((-ev.oy * 200.0) - State.drag.start_y) * -0.01
            apply_widget_norm(State.drag.info, n)
        end
    elseif ev.type == "xr_up" and State.drag then
        if State.drag.kind == "cable" then gf.spatial.cable_finish() end
        if State.drag.kind == "piano_key" then State.note_off(State.drag.note) end
        State.drag = nil
        gf.spatial.set_tooltip(false)
    elseif ev.type == "xr_teleport" then
        local next_rm = ((gf.spatial.camera_state().room) % 7) + 1
        gf.spatial.teleport_room(ev.id >= 1 and ev.id <= 7 and ev.id or next_rm)
    end
end

function gf_select_patch(idx)
    local ok = R7.select_global(idx)
    if ok then
        State.preset_name = R7.active_name
        R1.refresh_curves(); R2.refresh_curves(); R3.sync_rail()
    end
    return ok
end

function gf_get_state()
    return State
end
