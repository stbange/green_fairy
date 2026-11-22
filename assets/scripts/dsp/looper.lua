-- Green Fairy v2.1: Polyphonic Phrase Recorder, Overdub Looper & 108-Melody Hans Zimmer Score Engine

local Looper = {}
Looper.__index = Looper

local SUITE_ORDER = {
    {slug = "organ_cathedrals",    title = "ORGAN CATHEDRALS"},
    {slug = "desert_prophecies",   title = "DESERT PROPHECIES"},
    {slug = "neon_dystopia",       title = "NEON DYSTOPIA"},
    {slug = "dream_architectures", title = "DREAM ARCHITECTURES"},
    {slug = "elegiac_hymns",       title = "ELEGIAC HYMNS"},
    {slug = "deep_cosmos",         title = "DEEP COSMOS"},
}

local function clamp(v, lo, hi)
    if v < lo then return lo end
    if v > hi then return hi end
    return v
end

function Looper.new()
    local self = setmetatable({
        suites = SUITE_ORDER,
        catalog = {},
        by_suite = {},
        active_idx = 1,
        active_suite_idx = 1,
        state = "stopped", -- "stopped" | "rec_armed" | "recording" | "playing"
        overdub = false,
        melody_name = "None",
        recommended_patch = nil,
        bpm = 54.0,
        speed = 1.0,
        transpose = 0,
        vel_scale = 1.0,
        loop_beats = 16.0,
        playhead = 0.0,
        events = {},     -- {{beat, dur, note, vel}, ...}
        rec_elapsed = 0.0,
        rec_open = {},   -- note -> {t_sec = ..., beat = ..., vel = ...}
        rec_raw = {},    -- {{t_sec = ..., dur_sec = ..., note = ..., vel = ...}, ...}
        sounding = {},   -- {{played_note = ..., rem_beats = ...}, ...}
        live_held = {},  -- note -> true
    }, Looper)
    self:load_catalog()
    self:sync_status()
    return self
end

function Looper:sync_status()
    if gf and gf.ui and gf.ui.set_looper_status then
        gf.ui.set_looper_status(self.state, self.melody_name, #self.events)
    end
end

function Looper:load_catalog()
    self.catalog = {}
    self.by_suite = {}
    for _, s in ipairs(self.suites) do
        self.by_suite[s.slug] = {}
    end

    local raw = gf.assets.read_text("melodies/catalog.json")
    if raw then
        for obj in raw:gmatch("%{(.-)%}") do
            local path  = obj:match('"path"%s*:%s*"([^"]+)"')
            local name  = obj:match('"name"%s*:%s*"([^"]+)"')
            local suite = obj:match('"suite"%s*:%s*"([^"]+)"')
            local patch = obj:match('"recommendedPatch"%s*:%s*"([^"]+)"')
            if path and name and suite then
                local entry = {path = path, name = name, suite = suite, recommended_patch = patch}
                self.catalog[#self.catalog + 1] = entry
                if not self.by_suite[suite] then self.by_suite[suite] = {} end
                local sl = self.by_suite[suite]
                sl[#sl + 1] = entry
            end
        end
    end
end

function Looper:is_note_still_sounding(played_note, ignore_idx)
    if self.live_held[played_note] then return true end
    for i, s in ipairs(self.sounding) do
        if i ~= ignore_idx and s.played_note == played_note and s.rem_beats > 0.0 then
            return true
        end
    end
    return false
end

function Looper:release_all_sounding(visual_cb)
    for i = #self.sounding, 1, -1 do
        local n = self.sounding[i].played_note
        table.remove(self.sounding, i)
        if not self.live_held[n] then
            gf.synth.note_off(n)
            if visual_cb then visual_cb(n, false) end
        end
    end
end

function Looper:stop(visual_cb)
    if self.state == "recording" then
        return self:finish_recording(visual_cb)
    end
    self:release_all_sounding(visual_cb)
    self.rec_open = {}
    self.state = "stopped"
    self.playhead = 0.0
    self:sync_status()
    return true
end

function Looper:play(visual_cb)
    if self.state == "recording" then
        return self:finish_recording(visual_cb)
    end
    if #self.events == 0 and #self.catalog > 0 then
        return self:select_index(self.active_idx, true, visual_cb)
    end
    if #self.events == 0 then return false end
    self:release_all_sounding(visual_cb)
    self.playhead = 0.0
    self.state = "playing"
    self:sync_status()
    return true
end

function Looper:play_toggle(visual_cb)
    if self.state == "playing" then
        return self:stop(visual_cb)
    elseif self.state == "recording" then
        return self:finish_recording(visual_cb)
    elseif self.state == "rec_armed" then
        self.state = "stopped"
        self:sync_status()
        return true
    end
    return self:play(visual_cb)
end

function Looper:clear(visual_cb)
    self:release_all_sounding(visual_cb)
    self.events = {}
    self.rec_raw = {}
    self.rec_open = {}
    self.rec_elapsed = 0.0
    self.playhead = 0.0
    self.state = "stopped"
    self.melody_name = "Empty Loop"
    self.recommended_patch = nil
    self:sync_status()
    return true
end

function Looper:record_toggle(visual_cb)
    if self.state == "recording" then
        return self:finish_recording(visual_cb)
    elseif self.state == "rec_armed" then
        self.state = "stopped"
        self:sync_status()
        return true
    end
    self:release_all_sounding(visual_cb)
    self.events = {}
    self.rec_raw = {}
    self.rec_open = {}
    self.rec_elapsed = 0.0
    self.playhead = 0.0
    self.melody_name = "Recorded Loop"
    self.recommended_patch = nil
    self.state = "rec_armed"
    self:sync_status()
    return true
end

function Looper:finish_recording(visual_cb)
    for note, o in pairs(self.rec_open) do
        if o.t_sec ~= nil then
            self.rec_raw[#self.rec_raw + 1] = {
                t_sec = o.t_sec,
                dur_sec = math.max(0.15, self.rec_elapsed - o.t_sec),
                note = note,
                vel = o.vel,
            }
        end
    end
    self.rec_open = {}

    if #self.rec_raw == 0 then
        self.state = "stopped"
        self:sync_status()
        return false
    end

    local eff_bpm = math.max(32.0, self.bpm * self.speed)
    local bps = eff_bpm / 60.0
    local span_sec = math.max(0.5, self.rec_elapsed)
    for _, r in ipairs(self.rec_raw) do
        local end_s = r.t_sec + r.dur_sec
        if end_s > span_sec then span_sec = end_s end
    end

    local raw_beats = span_sec * bps
    self.loop_beats = math.max(2.0, math.floor(raw_beats * 2.0 + 0.85) * 0.5)
    self.events = {}
    for i, r in ipairs(self.rec_raw) do
        local bt = clamp(r.t_sec * bps, 0.0, self.loop_beats - 0.05)
        local du = math.max(0.15, math.min(self.loop_beats - 0.05, r.dur_sec * bps))
        self.events[i] = {bt, du, r.note, r.vel}
    end
    table.sort(self.events, function(a, b) return a[1] < b[1] end)

    self:release_all_sounding(visual_cb)
    self.playhead = 0.0
    self.state = "playing"
    self.melody_name = "Recorded Loop"
    self:sync_status()
    return true
end

function Looper:on_live_note_on(note, vel)
    local v = clamp(vel or 0.85, 0.1, 1.0)
    self.live_held[note] = true
    if self.state == "rec_armed" then
        self.state = "recording"
        self.rec_elapsed = 0.0
        self.rec_open[note] = {t_sec = 0.0, vel = v}
        self:sync_status()
    elseif self.state == "recording" then
        self.rec_open[note] = {t_sec = self.rec_elapsed, vel = v}
    elseif self.state == "playing" and self.overdub then
        self.rec_open[note] = {beat = self.playhead, vel = v}
    end
end

function Looper:on_live_note_off(note)
    self.live_held[note] = nil
    local o = self.rec_open[note]
    if not o then return end
    self.rec_open[note] = nil

    if self.state == "recording" and o.t_sec ~= nil then
        self.rec_raw[#self.rec_raw + 1] = {
            t_sec = o.t_sec,
            dur_sec = math.max(0.14, self.rec_elapsed - o.t_sec),
            note = note,
            vel = o.vel,
        }
    elseif self.state == "playing" and self.overdub and o.beat ~= nil then
        local d = self.playhead - o.beat
        if d <= 0.02 then d = d + self.loop_beats end
        self.events[#self.events + 1] = {
            clamp(o.beat, 0.0, self.loop_beats - 0.02),
            clamp(d, 0.16, self.loop_beats - 0.05),
            note,
            o.vel,
        }
        table.sort(self.events, function(a, b) return a[1] < b[1] end)
        self:sync_status()
    end
end

function Looper:load_melody_file(rel_path, auto_play, visual_cb)
    if not rel_path or rel_path == "" then return false end
    local ok, tbl = pcall(gf.assets.dofile, rel_path)
    if not ok or type(tbl) ~= "table" or type(tbl.events) ~= "table" or #tbl.events == 0 then
        return false
    end

    self:release_all_sounding(visual_cb)
    self.rec_open = {}
    self.rec_raw = {}
    self.events = {}
    for i, ev in ipairs(tbl.events) do
        self.events[i] = {
            tonumber(ev[1]) or 0.0,
            math.max(0.12, tonumber(ev[2]) or 1.0),
            math.floor(tonumber(ev[3]) or 60),
            clamp(tonumber(ev[4]) or 0.75, 0.1, 1.0),
        }
    end
    table.sort(self.events, function(a, b) return a[1] < b[1] end)

    self.bpm = clamp(tonumber(tbl.bpm) or 54.0, 28.0, 180.0)
    self.loop_beats = math.max(2.0, tonumber(tbl.beats) or 16.0)
    self.melody_name = tbl.name or rel_path
    self.recommended_patch = tbl.recommended_patch
    self.playhead = 0.0

    for idx, it in ipairs(self.catalog) do
        if it.path == rel_path then
            self.active_idx = idx
            for si, s in ipairs(self.suites) do
                if s.slug == it.suite then self.active_suite_idx = si; break end
            end
            break
        end
    end

    self.state = (auto_play ~= false) and "playing" or "stopped"
    self:sync_status()
    return true
end

function Looper:select_index(idx, auto_play, visual_cb)
    if #self.catalog == 0 then return false end
    local n = #self.catalog
    local wrapped = ((idx - 1) % n) + 1
    for attempt = 0, n - 1 do
        local cand = ((wrapped - 1 + attempt) % n) + 1
        local entry = self.catalog[cand]
        if entry and self:load_melody_file(entry.path, auto_play, visual_cb) then
            return true
        end
    end
    return false
end

function Looper:step_melody(delta, visual_cb)
    return self:select_index(self.active_idx + (delta or 1), true, visual_cb)
end

function Looper:step_suite(delta, visual_cb)
    local ns = #self.suites
    if ns == 0 then return false end
    self.active_suite_idx = ((self.active_suite_idx - 1 + (delta or 1)) % ns) + 1
    local target_slug = self.suites[self.active_suite_idx].slug
    for i, it in ipairs(self.catalog) do
        if it.suite == target_slug then
            return self:select_index(i, true, visual_cb)
        end
    end
    return false
end

function Looper:trigger_window(lo, hi, visual_cb)
    for _, ev in ipairs(self.events) do
        local bt = ev[1]
        if bt >= lo and bt < hi then
            local played = clamp(math.floor(ev[3] + self.transpose + 0.5), 24, 96)
            local vel = clamp(ev[4] * self.vel_scale, 0.10, 1.0)
            gf.synth.note_on(played, vel)
            if visual_cb then visual_cb(played, true) end
            self.sounding[#self.sounding + 1] = {
                played_note = played,
                rem_beats = math.max(0.12, ev[2]),
            }
        end
    end
end

function Looper:update(dt, visual_cb)
    if dt <= 0 then return end
    if self.state == "recording" then
        self.rec_elapsed = self.rec_elapsed + dt
        if self.rec_elapsed >= 64.0 then
            self:finish_recording(visual_cb)
        end
        return
    end

    if self.state ~= "playing" or #self.events == 0 then return end

    local eff_bpm = math.max(24.0, self.bpm * self.speed)
    local d_beats = dt * (eff_bpm / 60.0)
    if d_beats > self.loop_beats * 0.5 then d_beats = self.loop_beats * 0.5 end

    for i = #self.sounding, 1, -1 do
        local s = self.sounding[i]
        s.rem_beats = s.rem_beats - d_beats
        if s.rem_beats <= 0.0 then
            local n = s.played_note
            table.remove(self.sounding, i)
            if not self:is_note_still_sounding(n, 0) then
                gf.synth.note_off(n)
                if visual_cb then visual_cb(n, false) end
            end
        end
    end

    local b0 = self.playhead
    local b1 = b0 + d_beats
    if b1 < self.loop_beats then
        self:trigger_window(b0, b1, visual_cb)
        self.playhead = b1
    else
        self:trigger_window(b0, self.loop_beats, visual_cb)
        local wrapped = b1 % self.loop_beats
        self:trigger_window(0.0, wrapped, visual_cb)
        self.playhead = wrapped
    end
end

function Looper:build_curve(num_pts)
    local n = num_pts or 40
    local ys = {}
    local lb = math.max(1.0, self.loop_beats)
    local ph_bin = (self.state == "playing") and (math.floor((self.playhead / lb) * (n - 1)) + 1) or -1

    for i = 1, n do
        local b_lo = ((i - 1) / n) * lb
        local b_hi = (i / n) * lb
        local top_n = nil
        for _, ev in ipairs(self.events) do
            local e0, e1 = ev[1], ev[1] + ev[2]
            if e0 < b_hi and e1 > b_lo then
                if not top_n or ev[3] > top_n then top_n = ev[3] end
            end
        end
        local v = top_n and clamp((top_n - 32.0) / 56.0, 0.08, 0.92) or 0.04
        if i == ph_bin then v = math.min(1.0, v + 0.22) end
        ys[i] = v
    end
    return ys
end

return Looper
