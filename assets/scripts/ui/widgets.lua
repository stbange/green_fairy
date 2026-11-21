-- Green Fairy: Lua UI Widget & Multi-Touch Geometry Helpers
local W = {}

function W.hit(px, py, x, y, w, h)
    return px >= x and px <= (x + w) and py >= y and py <= (y + h)
end

function W.clamp(v, lo, hi)
    if v < lo then return lo end
    if v > hi then return hi end
    return v
end

function W.draw_button(x, y, w, h, label, active)
    if active then
        gf.ui.rect(x, y, w, h, 0.12, 0.58, 0.34, 0.92)
    else
        gf.ui.rect(x, y, w, h, 0.06, 0.14, 0.14, 0.84)
    end
    gf.ui.rect(x, y, w, 2, 0.30, 0.95, 0.58, 0.9)
    gf.ui.text(x + 8, y + (h - 10) * 0.5, 1.25, label, 0.88, 1.0, 0.92, 1.0)
end

function W.draw_slider(x, y, w, h, label, norm_val)
    local v = W.clamp(norm_val or 0.0, 0.0, 1.0)
    gf.ui.rect(x, y, w, h, 0.05, 0.11, 0.12, 0.84)
    gf.ui.rect(x + 2, y + 2, (w - 4) * v, h - 4, 0.14, 0.72, 0.44, 0.90)
    gf.ui.text(x + 6, y + (h - 10) * 0.5, 1.2, label, 0.92, 1.0, 0.95, 1.0)
end

function W.draw_xy_pad(x, y, w, h, nx, ny)
    gf.ui.rect(x, y, w, h, 0.04, 0.11, 0.11, 0.88)
    gf.ui.line(x + w * 0.5, y, x + w * 0.5, y + h, 1.0, 0.2, 0.55, 0.4, 0.6)
    gf.ui.line(x, y + h * 0.5, x + w, y + h * 0.5, 1.0, 0.2, 0.55, 0.4, 0.6)
    local cx = x + W.clamp(nx, 0.0, 1.0) * w
    local cy = y + W.clamp(ny, 0.0, 1.0) * h
    gf.ui.rect(cx - 7, cy - 7, 14, 14, 0.45, 1.0, 0.72, 1.0)
    gf.ui.text(x + 6, y + 6, 1.15, "XY MORPH", 0.65, 1.0, 0.82, 0.95)
end

W.WHITE_OFFSETS = {0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19}

function W.draw_keyboard(x, y, w, h, base_note, active_notes)
    local count = #W.WHITE_OFFSETS
    local kw = w / count
    for i = 1, count do
        local note = base_note + W.WHITE_OFFSETS[i]
        local kx = x + (i - 1) * kw
        local pressed = active_notes[note] == true
        if pressed then
            gf.ui.rect(kx + 1, y, kw - 2, h, 0.25, 0.95, 0.58, 0.96)
        else
            gf.ui.rect(kx + 1, y, kw - 2, h, 0.10, 0.20, 0.19, 0.90)
        end
        gf.ui.rect(kx + 1, y, kw - 2, 2, 0.35, 0.90, 0.62, 0.75)
    end
end

function W.keyboard_note_at(px, py, x, y, w, h, base_note)
    if not W.hit(px, py, x, y, w, h) then return nil end
    local count = #W.WHITE_OFFSETS
    local idx = math.floor(((px - x) / w) * count) + 1
    idx = W.clamp(idx, 1, count)
    return base_note + W.WHITE_OFFSETS[idx]
end

return W
