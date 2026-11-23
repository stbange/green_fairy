-- Minimal Diegetic Sanctum Compass, 7-Room Quick-Jump Map & Grand Chromatic Piano Strip

local Compass = {
    keys_open = true,
    desktop_octave = 2, -- C2 (36) .. C6 (84) = 49 keys (4 full octaves)
    touch_octave = 3,   -- C3 (48) .. C5 (72) = 25 keys (2 full octaves)
    active_touches = {}, -- pointer_id -> {role="walk"|"look"|"key", ...}
}

local ROOM_TAGS = {"1:OSC", "2:FLT", "3:ENV", "4:MOD", "5:FX", "6:BRG", "7:VLT"}
local WHITE_OFFSETS = {0, 2, 4, 5, 7, 9, 11}
local BLACK_AFTER = {[0] = 1, [2] = 3, [5] = 6, [7] = 8, [9] = 10}
local QWERTY_HINTS = {
    [48] = "Z", [50] = "X", [52] = "C", [53] = "V", [55] = "B", [57] = "N", [59] = "M",
    [60] = "Q", [62] = "E", [64] = "R", [65] = "T", [67] = "Y", [69] = "U", [71] = "I",
    [72] = "O", [74] = "P",
}

local function point_in_rect(px, py, r)
    return r and px >= r[1] and px <= (r[1] + r[3]) and py >= r[2] and py <= (r[2] + r[4])
end

function Compass.build_keyboard_layout(kb)
    local octaves = kb.octaves or 2
    local base_note = kb.base_note or 48
    local num_white = octaves * 7 + 1
    local kw = kb[3] / num_white
    local bh = math.floor(kb[4] * 0.58)
    local bw = kw * 0.62
    local whites, blacks = {}, {}

    for i = 0, num_white - 1 do
        local oct = math.floor(i / 7)
        local deg = (i % 7) + 1
        local semi = WHITE_OFFSETS[deg]
        local note = base_note + oct * 12 + semi
        local kx = kb[1] + i * kw
        whites[#whites + 1] = {
            note = note, x = kx, y = kb[2], w = kw, h = kb[4],
            is_c = (semi == 0), oct_num = math.floor(note / 12) - 1,
        }
        local b_semi = BLACK_AFTER[semi]
        if b_semi and i < num_white - 1 then
            local b_note = base_note + oct * 12 + b_semi
            blacks[#blacks + 1] = {
                note = b_note, x = kx + kw - bw * 0.5, y = kb[2], w = bw, h = bh,
            }
        end
    end
    return whites, blacks
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
        oct_dn_btn = nil,
        oct_up_btn = nil,
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
        hud.keys_toggle_btn = {w - 186, row3_y, 80, row3_h}
        if cam and cam.mode == "focus" then
            hud.exit_focus_btn = {10, row3_y, 82, row3_h}
            hud.rec_btn  = {96,  row3_y, 46, row3_h}
            hud.loop_btn = {146, row3_y, 50, row3_h}
        else
            hud.rec_btn  = {10,  row3_y, 50, row3_h}
            hud.loop_btn = {64,  row3_y, 58, row3_h}
            hud.clr_btn  = {126, row3_y, 44, row3_h}
        end
        local kb_h = Compass.keys_open and math.min(158, math.floor(h * 0.21)) or 0
        if kb_h > 0 then
            hud.keyboard = {
                8, h - kb_h - 6, w - 16, kb_h,
                octaves = 2, base_note = (Compass.touch_octave + 1) * 12,
            }
            hud.oct_dn_btn = {math.floor(w * 0.5) - 54, h - kb_h - 36, 50, 26}
            hud.oct_up_btn = {math.floor(w * 0.5) + 4,  h - kb_h - 36, 50, 26}
        end
        if not cam or cam.mode ~= "focus" then
            local pad_y = h - kb_h - 118
            hud.walk_pad = {14, pad_y, 108, 108}
            hud.look_pad = {w - 122, pad_y, 108, 108}
        end
    else
        if cam and cam.mode == "focus" then
            hud.exit_focus_btn = {10, row3_y + 38, 148, row3_h}
        end
        hud.keys_toggle_btn = {w - 168, row3_y, 62, row3_h}
        hud.oct_up_btn      = {w - 216, row3_y, 44, row3_h}
        hud.oct_dn_btn      = {w - 264, row3_y, 44, row3_h}
        hud.clr_btn         = {w - 318, row3_y, 50, row3_h}
        hud.loop_btn        = {w - 388, row3_y, 66, row3_h}
        hud.rec_btn         = {w - 448, row3_y, 56, row3_h}
        if Compass.keys_open and mode ~= "xr" then
            local kb_h = math.max(128, math.min(168, math.floor(h * 0.21)))
            local octs = (w >= 900) and 4 or 2
            local base_oct = (w >= 900) and Compass.desktop_octave or Compass.touch_octave
            hud.keyboard = {
                12, h - kb_h - 8, w - 24, kb_h,
                octaves = octs, base_note = (base_oct + 1) * 12,
            }
        end
    end
    return hud
end

function Compass.note_at_keyboard(kb, px, py)
    if not point_in_rect(px, py, kb) then return nil end
    local whites, blacks = Compass.build_keyboard_layout(kb)
    -- Black keys sit physically on top of the upper portion of white keys
    for _, b in ipairs(blacks) do
        if px >= b.x and px <= (b.x + b.w) and py >= b.y and py <= (b.y + b.h) then
            return b.note
        end
    end
    for _, wt in ipairs(whites) do
        if px >= wt.x and px <= (wt.x + wt.w) then
            return wt.note
        end
    end
    return whites[#whites] and whites[#whites].note or nil
end

local function render_chromatic_keyboard(kb, state, narrow)
    gf.ui.rect(kb[1] - 2, kb[2] - 3, kb[3] + 4, kb[4] + 6, 0.02, 0.06, 0.08, 0.94)
    local whites, blacks = Compass.build_keyboard_layout(kb)

    -- Pass 1: White keys (large ivory-mint concert keys with emerald glow when pressed)
    for _, wt in ipairs(whites) do
        local held = state.pressed_notes[wt.note] == true
        local r = held and 0.24 or (wt.is_c and 0.80 or 0.88)
        local g = held and 1.00 or (wt.is_c and 0.96 or 0.97)
        local b = held and 0.72 or (wt.is_c and 0.90 or 0.94)
        gf.ui.rect(wt.x + 1, wt.y + 2, math.max(2, wt.w - 2), wt.h - 4, r, g, b, 0.96)
        local bottom_y = wt.y + wt.h - (narrow and 20 or 24)
        if wt.is_c then
            gf.ui.text(wt.x + 3, bottom_y, narrow and 0.85 or 1.05, "C" .. wt.oct_num,
                       held and 0.02 or 0.08, held and 0.14 or 0.34, held and 0.10 or 0.30, 0.95)
        elseif not narrow and QWERTY_HINTS[wt.note] then
            gf.ui.text(wt.x + math.max(3, wt.w * 0.28), bottom_y, 0.92, QWERTY_HINTS[wt.note],
                       0.14, 0.38, 0.34, 0.65)
        end
    end

    -- Pass 2: Raised black chromatic keys (ebony-teal idle, luminous aqua when pressed)
    for _, bk in ipairs(blacks) do
        local held = state.pressed_notes[bk.note] == true
        gf.ui.rect(bk.x - 1, bk.y + 2, bk.w + 2, bk.h + 2, 0.02, 0.04, 0.06, 0.98)
        gf.ui.rect(bk.x, bk.y + 2, bk.w, bk.h - 2,
                   held and 0.18 or 0.06, held and 0.96 or 0.14, held and 0.86 or 0.17, 0.98)
    end
end

function Compass.render(w, h, mode, state)
    local cam = gf.spatial.camera_state()
    local hud = Compass.compute_hud(w, h, mode, cam)
    local narrow = (w < 540) or (mode == "touch")
    local lp_state = state.looper and state.looper.state or "stopped"

    -- 1. Sleek Top-Left Room, Preset & World Theme Status Plaque
    local tb = hud.top_bar
    gf.ui.rect(tb[1], tb[2], tb[3], tb[4], 0.03, 0.07, 0.09, 0.84)
    local p_name = (state.preset_name or "Sanctum"):sub(1, narrow and 15 or 24):upper()
    local r_name = (cam.room_name or "BRIDGE"):sub(1, narrow and 10 or 20):upper()
    local lp_tag = (lp_state == "playing") and " [LOOP]" or ((lp_state == "recording" or lp_state == "rec_armed") and " [REC]" or "")
    local nav_hint = (mode == "desktop") and (cam.mode == "focus" and "[ESC:WALK]" or "[WASD+RMB | L:REC SPACE:LOOP]") or ""
    local line = narrow
        and string.format("R%d %s - %s - V:%d%s", cam.room, r_name, p_name, state.voices or 0, lp_tag)
        or  string.format("R%d %s - %s - V:%d%s %s", cam.room, r_name, p_name, state.voices or 0, lp_tag, nav_hint)
    gf.ui.text(tb[1] + 8, tb[2] + 8, narrow and 1.02 or 1.28, line, 0.52, 1.0, 0.84, 1.0)

    -- 2. 7-Room Quick-Teleport Compass Nodes
    for r = 1, 7 do
        local b = hud.room_btns[r]
        local cur = (cam.room == r)
        gf.ui.rect(b[1], b[2], b[3], b[4], cur and 0.16 or 0.04, cur and 0.52 or 0.12, cur and 0.42 or 0.16, 0.88)
        gf.ui.text(b[1] + 4, b[2] + 9, narrow and 0.98 or 1.15, ROOM_TAGS[r], cur and 1.0 or 0.68, 1.0, cur and 0.88 or 0.80, 1.0)
    end

    -- 3. Looper Transport (REC / LOOP / CLR), Octave Shift, Keyboard Toggle & Drone Hold
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
    if hud.oct_dn_btn then
        local od = hud.oct_dn_btn
        gf.ui.rect(od[1], od[2], od[3], od[4], 0.06, 0.18, 0.24, 0.88)
        gf.ui.text(od[1] + 5, od[2] + 7, 1.0, "OCT-", 0.78, 0.96, 1.0, 1.0)
    end
    if hud.oct_up_btn then
        local ou = hud.oct_up_btn
        gf.ui.rect(ou[1], ou[2], ou[3], ou[4], 0.06, 0.18, 0.24, 0.88)
        gf.ui.text(ou[1] + 5, ou[2] + 7, 1.0, "OCT+", 0.78, 0.96, 1.0, 1.0)
    end
    if hud.keys_toggle_btn then
        local kt = hud.keys_toggle_btn
        gf.ui.rect(kt[1], kt[2], kt[3], kt[4], Compass.keys_open and 0.12 or 0.06, Compass.keys_open and 0.36 or 0.16, 0.34, 0.88)
        gf.ui.text(kt[1] + 6, kt[2] + 8, narrow and 1.0 or 1.10, Compass.keys_open and "KEYS" or "KEYS+", 0.76, 1.0, 0.92, 1.0)
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

    -- 4. Mobile Touch Walk/Look Pads
    if mode == "touch" and hud.walk_pad then
        local wp, lp = hud.walk_pad, hud.look_pad
        gf.ui.rect(wp[1], wp[2], wp[3], wp[4], 0.04, 0.18, 0.16, 0.50)
        gf.ui.text(wp[1] + 24, wp[2] + 46, 1.25, "WALK", 0.55, 1.0, 0.82, 0.85)
        gf.ui.rect(lp[1], lp[2], lp[3], lp[4], 0.04, 0.16, 0.22, 0.50)
        gf.ui.text(lp[1] + 24, lp[2] + 46, 1.25, "LOOK", 0.55, 0.9, 1.0, 0.85)
    end

    -- 5. Grand On-Screen Chromatic Piano (Desktop 49-key / Mobile 25-key)
    if hud.keyboard then
        render_chromatic_keyboard(hud.keyboard, state, narrow)
    end
end

function Compass.shift_octave(delta, mode)
    if mode == "touch" then
        Compass.touch_octave = math.max(1, math.min(5, Compass.touch_octave + delta))
    else
        Compass.desktop_octave = math.max(1, math.min(4, Compass.desktop_octave + delta))
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
        if point_in_rect(ev.x, ev.y, hud.oct_dn_btn) then
            Compass.shift_octave(-1, mode)
            return true
        end
        if point_in_rect(ev.x, ev.y, hud.oct_up_btn) then
            Compass.shift_octave(1, mode)
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
            Compass.keys_open = not Compass.keys_open
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
