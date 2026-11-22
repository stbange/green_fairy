-- Minimal Diegetic Sanctum Compass, 7-Room Quick-Jump Map & Mobile Multi-Touch Strip

local Compass = {
    mobile_keys_open = true,
    active_touches = {}, -- pointer_id -> {role="walk"|"look"|"key", start_x=..., start_y=..., last_x=..., last_y=..., note=...}
}

local ROOM_TAGS = {"1:OSC", "2:FLT", "3:ENV", "4:MOD", "5:FX", "6:BRG", "7:VLT"}

local function point_in_rect(px, py, r)
    return r and px >= r[1] and px <= (r[1] + r[3]) and py >= r[2] and py <= (r[2] + r[4])
end

function Compass.compute_hud(w, h, mode, cam)
    local is_touch = (mode == "touch")
    local bar_h = is_touch and 34 or 32
    local hud = {
        top_bar = {10, 8, math.min(w - 20, 760), bar_h},
        room_btns = {},
        exit_focus_btn = nil,
        rec_btn = nil,
        loop_btn = nil,
        clr_btn = nil,
        drone_btn = nil,
        keys_toggle_btn = nil,
        keyboard = nil,
        walk_pad = nil,
        look_pad = nil,
    }

    local gap = 4
    local bw = is_touch and math.max(38, math.min(50, math.floor((w - 20 - 6 * gap) / 7))) or 44
    local total_rw = 7 * bw + 6 * gap
    local rx = is_touch and math.max(10, math.floor((w - total_rw) * 0.5)) or math.max(12, w - total_rw - 12)
    local ry = is_touch and 46 or 8
    for r = 1, 7 do
        hud.room_btns[r] = {rx + (r - 1) * (bw + gap), ry, bw, bar_h}
    end

    local row3_y = ry + bar_h + 6
    local row3_h = is_touch and 34 or 28
    hud.drone_btn = {w - 102, row3_y, 92, row3_h}

    if is_touch then
        hud.keys_toggle_btn = {w - 190, row3_y, 84, row3_h}
        if cam and cam.mode == "focus" then
            hud.exit_focus_btn = {10, row3_y, 84, row3_h}
            hud.rec_btn  = {98,  row3_y, 46, row3_h}
            hud.loop_btn = {148, row3_y, 50, row3_h}
        else
            hud.rec_btn  = {10,  row3_y, 54, row3_h}
            hud.loop_btn = {68,  row3_y, 64, row3_h}
            hud.clr_btn  = {136, row3_y, 48, row3_h}
        end
        local kb_h = Compass.mobile_keys_open and math.min(132, math.floor(h * 0.19)) or 0
        if kb_h > 0 then
            hud.keyboard = {10, h - kb_h - 8, w - 20, kb_h}
        end
        if not cam or cam.mode ~= "focus" then
            local pad_y = h - kb_h - 122
            hud.walk_pad = {14, pad_y, 108, 108}
            hud.look_pad = {w - 122, pad_y, 108, 108}
        end
    else
        if cam and cam.mode == "focus" then
            hud.exit_focus_btn = {10, row3_y, 144, row3_h}
        end
        hud.clr_btn  = {w - 158, row3_y, 50, row3_h}
        hud.loop_btn = {w - 230, row3_y, 66, row3_h}
        hud.rec_btn  = {w - 292, row3_y, 56, row3_h}
    end
    return hud
end

function Compass.note_at_keyboard(kb, px, py)
    if not point_in_rect(px, py, kb) then return nil end
    local white = {0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23}
    local idx = math.floor(((px - kb[1]) / math.max(1, kb[3])) * #white) + 1
    if idx < 1 then idx = 1 end
    if idx > #white then idx = #white end
    return 48 + white[idx]
end

function Compass.render(w, h, mode, state)
    local cam = gf.spatial.camera_state()
    local hud = Compass.compute_hud(w, h, mode, cam)
    local narrow = (w < 540) or (mode == "touch")
    local lp_state = state.looper and state.looper.state or "stopped"

    -- 1. Sleek Top-Left Room & Preset/Melody Status Plaque
    local tb = hud.top_bar
    gf.ui.rect(tb[1], tb[2], tb[3], tb[4], 0.03, 0.07, 0.09, 0.82)
    local p_name = (state.preset_name or "Sanctum"):sub(1, narrow and 15 or 22):upper()
    local r_name = (cam.room_name or "BRIDGE"):sub(1, narrow and 11 or 22):upper()
    local lp_tag = (lp_state == "playing") and " [LOOP]" or ((lp_state == "recording" or lp_state == "rec_armed") and " [REC]" or "")
    local nav_hint = (mode == "desktop") and (cam.mode == "focus" and "[ESC: WALK]" or "[WASD+RMB | L:REC SPACE:LOOP]") or ""
    local line = narrow
        and string.format("R%d %s - %s - V:%d%s", cam.room, r_name, p_name, state.voices or 0, lp_tag)
        or  string.format("R%d %s - %s - V:%d%s %s", cam.room, r_name, p_name, state.voices or 0, lp_tag, nav_hint)
    gf.ui.text(tb[1] + 8, tb[2] + 8, narrow and 1.02 or 1.30, line, 0.52, 1.0, 0.84, 1.0)

    -- 2. 7-Room Quick-Teleport Compass Nodes
    for r = 1, 7 do
        local b = hud.room_btns[r]
        local cur = (cam.room == r)
        gf.ui.rect(b[1], b[2], b[3], b[4], cur and 0.16 or 0.04, cur and 0.52 or 0.12, cur and 0.42 or 0.16, 0.88)
        gf.ui.text(b[1] + 4, b[2] + 9, narrow and 0.98 or 1.15, ROOM_TAGS[r], cur and 1.0 or 0.68, 1.0, cur and 0.88 or 0.80, 1.0)
    end

    -- 3. Looper Transport (REC / LOOP / CLR), Drone Hold Latch & Exit Rack-Focus Badge
    if hud.rec_btn then
        local rb = hud.rec_btn
        local is_rec = (lp_state == "recording" or lp_state == "rec_armed")
        gf.ui.rect(rb[1], rb[2], rb[3], rb[4], is_rec and 0.72 or 0.18, is_rec and 0.18 or 0.08, 0.12, 0.90)
        gf.ui.text(rb[1] + 6, rb[2] + 8, narrow and 1.0 or 1.15, is_rec and "REC*" or "REC", 1.0, 0.88, 0.84, 1.0)
    end
    if hud.loop_btn then
        local lb = hud.loop_btn
        local is_play = (lp_state == "playing")
        gf.ui.rect(lb[1], lb[2], lb[3], lb[4], is_play and 0.14 or 0.06, is_play and 0.62 or 0.22, is_play and 0.42 or 0.26, 0.90)
        gf.ui.text(lb[1] + 6, lb[2] + 8, narrow and 1.0 or 1.15, is_play and "STOP" or "LOOP", 0.92, 1.0, 0.92, 1.0)
    end
    if hud.clr_btn then
        local cb = hud.clr_btn
        gf.ui.rect(cb[1], cb[2], cb[3], cb[4], 0.10, 0.16, 0.22, 0.88)
        gf.ui.text(cb[1] + 7, cb[2] + 8, narrow and 1.0 or 1.15, "CLR", 0.85, 0.92, 1.0, 1.0)
    end

    local db = hud.drone_btn
    local drone_on = (gf.synth.get_param("voice.drone_latch") or 0) >= 0.5
    gf.ui.rect(db[1], db[2], db[3], db[4], drone_on and 0.22 or 0.05, drone_on and 0.62 or 0.14, 0.35, 0.88)
    gf.ui.text(db[1] + 6, db[2] + 8, narrow and 1.0 or 1.15, drone_on and "DRONE:ON" or "DRONE", 0.95, 1.0, 0.9, 1.0)

    if hud.exit_focus_btn then
        local eb = hud.exit_focus_btn
        gf.ui.rect(eb[1], eb[2], eb[3], eb[4], 0.18, 0.45, 0.52, 0.92)
        gf.ui.text(eb[1] + 6, eb[2] + 8, narrow and 1.0 or 1.20, narrow and "< ROOM" or "< BACK TO ROOM", 1.0, 1.0, 0.85, 1.0)
    end

    -- 4. Mobile Touch Controls: Dual-Zone Walk/Look Pads & Collapsible Multi-Touch Keyboard
    if mode == "touch" then
        local kt = hud.keys_toggle_btn
        gf.ui.rect(kt[1], kt[2], kt[3], kt[4], 0.08, 0.28, 0.32, 0.88)
        gf.ui.text(kt[1] + 6, kt[2] + 8, 1.0, Compass.mobile_keys_open and "HIDE KEY" or "SHOW KEY", 0.7, 1.0, 0.9, 1.0)

        if hud.walk_pad then
            local wp, lp = hud.walk_pad, hud.look_pad
            gf.ui.rect(wp[1], wp[2], wp[3], wp[4], 0.04, 0.18, 0.16, 0.50)
            gf.ui.text(wp[1] + 24, wp[2] + 46, 1.25, "WALK", 0.55, 1.0, 0.82, 0.85)
            gf.ui.rect(lp[1], lp[2], lp[3], lp[4], 0.04, 0.16, 0.22, 0.50)
            gf.ui.text(lp[1] + 24, lp[2] + 46, 1.25, "LOOK", 0.55, 0.9, 1.0, 0.85)
        end

        if hud.keyboard then
            local kb = hud.keyboard
            gf.ui.rect(kb[1], kb[2], kb[3], kb[4], 0.03, 0.08, 0.10, 0.92)
            local white = {0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23}
            local kw = kb[3] / #white
            for i, semi in ipairs(white) do
                local note = 48 + semi
                local held = state.pressed_notes[note] == true
                local kx = kb[1] + (i - 1) * kw
                gf.ui.rect(kx + 1, kb[2] + 2, kw - 2, kb[4] - 4,
                           held and 0.28 or 0.12, held and 0.98 or 0.32, held and 0.72 or 0.28, 0.95)
            end
        end
    end
end

function Compass.handle_hud_pointer(ev, w, h, mode, state)
    local cam = gf.spatial.camera_state()
    local hud = Compass.compute_hud(w, h, mode, cam)
    local pid = ev.id or 0

    if ev.type == "pointer_down" then
        for r = 1, 7 do
            if point_in_rect(ev.x, ev.y, hud.room_btns[r]) then
                gf.spatial.teleport_room(r)
                return true
            end
        end
        if point_in_rect(ev.x, ev.y, hud.exit_focus_btn) then
            gf.spatial.exit_focus()
            return true
        end
        if point_in_rect(ev.x, ev.y, hud.rec_btn) and state.looper_action then
            state.looper_action("record")
            return true
        end
        if point_in_rect(ev.x, ev.y, hud.loop_btn) and state.looper_action then
            state.looper_action("play_toggle")
            return true
        end
        if point_in_rect(ev.x, ev.y, hud.clr_btn) and state.looper_action then
            state.looper_action("clear")
            return true
        end
        if point_in_rect(ev.x, ev.y, hud.drone_btn) then
            local cur = (gf.synth.get_param("voice.drone_latch") or 0) >= 0.5
            gf.synth.set_param("voice.drone_latch", cur and 0.0 or 1.0)
            if cur then gf.synth.all_notes_off() end
            gf.spatial.sync_widgets()
            return true
        end
        if point_in_rect(ev.x, ev.y, hud.keys_toggle_btn) then
            Compass.mobile_keys_open = not Compass.mobile_keys_open
            return true
        end
        if point_in_rect(ev.x, ev.y, hud.keyboard) then
            local n = Compass.note_at_keyboard(hud.keyboard, ev.x, ev.y)
            if n then
                Compass.active_touches[pid] = {role = "key", note = n}
                state.note_on(n, 0.88)
            end
            return true
        end
        if point_in_rect(ev.x, ev.y, hud.walk_pad) then
            Compass.active_touches[pid] = {role = "walk", start_x = ev.x, start_y = ev.y, dx = 0, dy = 0}
            return true
        end
        if point_in_rect(ev.x, ev.y, hud.look_pad) then
            Compass.active_touches[pid] = {role = "look", last_x = ev.x, last_y = ev.y}
            return true
        end
        return false
    end

    local tr = Compass.active_touches[pid]
    if not tr then return false end

    if ev.type == "pointer_move" then
        if tr.role == "key" and hud.keyboard then
            local nn = Compass.note_at_keyboard(hud.keyboard, ev.x, ev.y)
            if nn and nn ~= tr.note then
                state.note_off(tr.note)
                tr.note = nn
                state.note_on(nn, 0.88)
            end
        elseif tr.role == "walk" then
            tr.dx = math.max(-1.0, math.min(1.0, (ev.x - tr.start_x) / 42.0))
            tr.dy = math.max(-1.0, math.min(1.0, (tr.start_y - ev.y) / 42.0))
        elseif tr.role == "look" then
            local raw_dx = math.max(-36.0, math.min(36.0, ev.x - tr.last_x))
            local raw_dy = math.max(-36.0, math.min(36.0, ev.y - tr.last_y))
            tr.last_x, tr.last_y = ev.x, ev.y
            gf.spatial.look(raw_dx * 0.0036, -raw_dy * 0.0030)
        end
        return true
    elseif ev.type == "pointer_up" then
        if tr.role == "key" and tr.note then
            state.note_off(tr.note)
        end
        Compass.active_touches[pid] = nil
        return true
    end
    return false
end

function Compass.step_touch_walk(dt)
    for _, tr in pairs(Compass.active_touches) do
        if tr.role == "walk" and (math.abs(tr.dx or 0) > 0.05 or math.abs(tr.dy or 0) > 0.05) then
            gf.spatial.walk(tr.dy, tr.dx, dt)
        end
    end
end

return Compass
