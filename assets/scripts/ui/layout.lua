-- Green Fairy: Adaptive Hybrid UI Layout (Desktop, Mobile Touch, and WebXR)
local W = require("ui.widgets")
local Layout = {}

function Layout.compute_rects(w, h, mode)
    if mode == "touch" then
        -- Mobile touch layout: large bottom multi-touch keyboard + big touch XY pad & sliders
        local pad = 10
        local top_h = 44
        local kb_h = math.floor(h * 0.34)
        local mid_y = top_h + pad
        local mid_h = h - top_h - kb_h - pad * 3
        local xy_w = math.floor((w - pad * 3) * 0.48)
        local sl_x = pad * 2 + xy_w
        local sl_w = w - sl_x - pad
        return {
            top_bar = {pad, 6, w - pad * 2, top_h},
            btn_patch = {pad, 6, math.floor((w - pad * 4) * 0.42), top_h},
            btn_vox   = {pad * 2 + math.floor((w - pad * 4) * 0.42), 6, math.floor((w - pad * 4) * 0.29), top_h},
            btn_arp   = {pad * 3 + math.floor((w - pad * 4) * 0.71), 6, math.floor((w - pad * 4) * 0.29), top_h},
            xy_pad    = {pad, mid_y, xy_w, mid_h},
            sl_cutoff = {sl_x, mid_y, sl_w, math.floor(mid_h * 0.30)},
            sl_fm     = {sl_x, mid_y + math.floor(mid_h * 0.35), sl_w, math.floor(mid_h * 0.30)},
            sl_aether = {sl_x, mid_y + math.floor(mid_h * 0.70), sl_w, math.floor(mid_h * 0.30)},
            keyboard  = {pad, h - kb_h - pad, w - pad * 2, kb_h},
        }
    end

    -- Desktop and WebXR spatial panel layout (keeps center clear for the retro 3D crystal stage!)
    local pad = 12
    local kb_h = (mode == "xr") and 76 or 84
    local side_w = math.min(230, math.floor(w * 0.26))
    return {
        top_bar   = {pad, 8, w - pad * 2, 34},
        btn_patch = {pad + 170, 10, 160, 30},
        btn_vox   = {pad + 340, 10, 115, 30},
        btn_arp   = {pad + 465, 10, 95, 30},
        sl_cutoff = {pad, 56, side_w, 30},
        sl_fm     = {pad, 94, side_w, 30},
        sl_aether = {pad, 132, side_w, 30},
        xy_pad    = {w - side_w - pad, 56, side_w, 150},
        keyboard  = {pad, h - kb_h - pad, w - pad * 2, kb_h},
    }
end

function Layout.render(state, w, h)
    local r = Layout.compute_rects(w, h, state.mode)

    -- Header bar
    gf.ui.rect(r.top_bar[1], r.top_bar[2], r.top_bar[3], r.top_bar[4], 0.03, 0.09, 0.09, 0.84)
    if state.mode ~= "touch" then
        gf.ui.text(r.top_bar[1] + 10, r.top_bar[2] + 10, 1.5, "GREEN FAIRY", 0.42, 1.0, 0.68, 1.0)
    end

    W.draw_button(r.btn_patch[1], r.btn_patch[2], r.btn_patch[3], r.btn_patch[4],
        "PATCH:" .. (state.patch_name or "DEFAULT"), false)
    W.draw_button(r.btn_vox[1], r.btn_vox[2], r.btn_vox[3], r.btn_vox[4],
        gf.scene.get_voxels_enabled() and "VOXEL:ON" or "VOXEL:OFF", gf.scene.get_voxels_enabled())
    W.draw_button(r.btn_arp[1], r.btn_arp[2], r.btn_arp[3], r.btn_arp[4],
        state.arp.enabled and "ARP:ON" or "ARP:OFF", state.arp.enabled)

    -- Sliders & XY Morph
    local cut_norm = W.clamp((gf.synth.get_param("master.filter.cutoff") - 80.0) / 15000.0, 0.0, 1.0)
    local fm_norm  = W.clamp(gf.synth.get_param("ch2.fm_index") / 10.0, 0.0, 1.0)
    local ae_norm  = W.clamp(gf.synth.get_param("aether.mix"), 0.0, 1.0)

    W.draw_slider(r.sl_cutoff[1], r.sl_cutoff[2], r.sl_cutoff[3], r.sl_cutoff[4], "CUTOFF", cut_norm)
    W.draw_slider(r.sl_fm[1],     r.sl_fm[2],     r.sl_fm[3],     r.sl_fm[4],     "FM SHIMMER", fm_norm)
    W.draw_slider(r.sl_aether[1], r.sl_aether[2], r.sl_aether[3], r.sl_aether[4], "AETHER MIX", ae_norm)
    W.draw_xy_pad(r.xy_pad[1], r.xy_pad[2], r.xy_pad[3], r.xy_pad[4], state.xy_x, state.xy_y)

    -- Multi-touch / clickable virtual keyboard
    W.draw_keyboard(r.keyboard[1], r.keyboard[2], r.keyboard[3], r.keyboard[4], 48, state.active_notes)
end

function Layout.handle_pointer(state, ev, w, h)
    local r = Layout.compute_rects(w, h, state.mode)
    local pid = ev.id or 0
    local px, py = ev.x or 0, ev.y or 0

    if ev.type == "pointer_up" then
        local prev = state.pointer_notes[pid]
        if prev then
            state.pointer_notes[pid] = nil
            state.active_notes[prev] = nil
            state.arp:note_off(prev)
        end
        state.pointer_drag[pid] = nil
        return
    end

    if ev.type == "pointer_down" then
        if W.hit(px, py, r.btn_patch[1], r.btn_patch[2], r.btn_patch[3], r.btn_patch[4]) then
            state.next_patch()
            return
        elseif W.hit(px, py, r.btn_vox[1], r.btn_vox[2], r.btn_vox[3], r.btn_vox[4]) then
            gf.scene.set_voxels_enabled(not gf.scene.get_voxels_enabled())
            return
        elseif W.hit(px, py, r.btn_arp[1], r.btn_arp[2], r.btn_arp[3], r.btn_arp[4]) then
            state.arp:set_enabled(not state.arp.enabled)
            return
        elseif W.hit(px, py, r.xy_pad[1], r.xy_pad[2], r.xy_pad[3], r.xy_pad[4]) then
            state.pointer_drag[pid] = "xy"
        elseif W.hit(px, py, r.sl_cutoff[1], r.sl_cutoff[2], r.sl_cutoff[3], r.sl_cutoff[4]) then
            state.pointer_drag[pid] = "cutoff"
        elseif W.hit(px, py, r.sl_fm[1], r.sl_fm[2], r.sl_fm[3], r.sl_fm[4]) then
            state.pointer_drag[pid] = "fm"
        elseif W.hit(px, py, r.sl_aether[1], r.sl_aether[2], r.sl_aether[3], r.sl_aether[4]) then
            state.pointer_drag[pid] = "aether"
        else
            local note = W.keyboard_note_at(px, py, r.keyboard[1], r.keyboard[2], r.keyboard[3], r.keyboard[4], 48)
            if note then
                state.pointer_notes[pid] = note
                state.active_notes[note] = true
                state.arp:note_on(note, 0.88)
            end
            return
        end
    end

    local drag = state.pointer_drag[pid]
    if drag == "xy" then
        state.xy_x = W.clamp((px - r.xy_pad[1]) / r.xy_pad[3], 0.0, 1.0)
        state.xy_y = W.clamp((py - r.xy_pad[2]) / r.xy_pad[4], 0.0, 1.0)
        gf.synth.set_xy(state.xy_x, state.xy_y)
    elseif drag == "cutoff" then
        local t = W.clamp((px - r.sl_cutoff[1]) / r.sl_cutoff[3], 0.0, 1.0)
        gf.synth.set_param("master.filter.cutoff", 80.0 + t * 15000.0)
    elseif drag == "fm" then
        local t = W.clamp((px - r.sl_fm[1]) / r.sl_fm[3], 0.0, 1.0)
        gf.synth.set_param("ch2.fm_index", t * 10.0)
    elseif drag == "aether" then
        local t = W.clamp((px - r.sl_aether[1]) / r.sl_aether[3], 0.0, 1.0)
        gf.synth.set_param("aether.mix", t)
    elseif ev.type == "pointer_move" and state.pointer_notes[pid] then
        local note = W.keyboard_note_at(px, py, r.keyboard[1], r.keyboard[2], r.keyboard[3], r.keyboard[4], 48)
        if note and note ~= state.pointer_notes[pid] then
            local old = state.pointer_notes[pid]
            state.active_notes[old] = nil
            state.arp:note_off(old)
            state.pointer_notes[pid] = note
            state.active_notes[note] = true
            state.arp:note_on(note, 0.85)
        end
    end
end

return Layout
