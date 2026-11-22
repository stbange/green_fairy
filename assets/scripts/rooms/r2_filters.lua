-- Room 2: Filter & Shaper Foundry (16, 0, -10)
-- 3 Channel 14-Mode Filter + Waveshaper Racks & Master Filter/Shaper Console

local RB = require("rooms.rack_builder")

local R2 = {
    ch_racks = {},
    master_rack = -1,
}

function R2.build()
    R2.ch_racks = {}
    local cx, cz = 16.0, -10.0
    local angles = {0.25, 1.05, 1.85}

    for ch = 1, 3 do
        local ang = angles[ch]
        local rx = cx + math.sin(ang) * 4.55
        local rz = cz - math.cos(ang) * 4.55
        local yaw = math.atan(cx - rx, cz - rz)
        local rk = RB.add_rack(2, string.format("CH %d FILTER & SHAPER", ch), rx, 1.52, rz, yaw, 2.25, 1.78,
                               {0.98, 0.65, 0.24, 1.0})
        R2.ch_racks[ch] = rk

        local pfx = "ch" .. ch
        RB.add_controls(2, rk, {
            {kind="step_dial", x=-0.78, y= 0.45, steps=14, min=0, max=13,    param=pfx..".filter.mode",   label="FLT MODE", color=RB.COLOR_DIAL},
            {kind="knob",      x=-0.26, y= 0.45,           min=30,max=16000, param=pfx..".filter.cutoff", label="CUTOFF"},
            {kind="knob",      x= 0.26, y= 0.45,           min=0, max=0.95,  param=pfx..".filter.res",    label="RESON"},
            {kind="knob",      x= 0.78, y= 0.45,           min=0, max=1.0,   param=pfx..".filter.morph",  label="VOWEL/M"},
            {kind="step_dial", x=-0.55, y= 0.10, steps=6,  min=0, max=5,     param=pfx..".shaper.mode",   label="SHP MODE", color=RB.COLOR_DIAL},
            {kind="knob",      x= 0.00, y= 0.10,           min=1, max=16.0,  param=pfx..".shaper.drive",  label="DRIVE"},
            {kind="knob",      x= 0.55, y= 0.10,           min=0, max=1.0,   param=pfx..".shaper.mix",    label="SHP MIX"},
        })

        local fp, sp = "flt" .. ch, "shp" .. ch
        RB.add_jack_strip(2, rk, -0.68, {
            {fp .. ".in",        false, "FLT IN",  "audio"},
            {fp .. ".out",       true,  "FLT OUT", "audio"},
            {fp .. ".cutoff_cv", false, "CUT CV",  "cv"},
            {fp .. ".res_cv",    false, "RES CV",  "cv"},
            {fp .. ".morph_cv",  false, "MRPH CV", "cv"},
            {sp .. ".in",        false, "SHP IN",  "audio"},
            {sp .. ".out",       true,  "SHP OUT", "audio"},
            {sp .. ".drive_cv",  false, "DRV CV",  "cv"},
        })
    end

    -- Master Filter & Wave-Transfer Shaper Rack
    local mx = cx + math.sin(2.65) * 4.45
    local mz = cz - math.cos(2.65) * 4.45
    local myaw = math.atan(cx - mx, cz - mz)
    R2.master_rack = RB.add_rack(2, "MASTER FILTER & WAVE SHAPER", mx, 1.50, mz, myaw, 2.20, 1.72,
                                 {1.0, 0.76, 0.32, 1.0})
    RB.add_controls(2, R2.master_rack, {
        {kind="step_dial", x=-0.75, y= 0.42, steps=14, min=0, max=13,    param="master.filter.mode",   label="MFLT MOD", color=RB.COLOR_DIAL},
        {kind="knob",      x=-0.25, y= 0.42,           min=40,max=17000, param="master.filter.cutoff", label="M CUTOFF"},
        {kind="knob",      x= 0.25, y= 0.42,           min=0, max=0.95,  param="master.filter.res",    label="M RESON"},
        {kind="knob",      x= 0.75, y= 0.42,           min=0, max=1.0,   param="master.filter.morph",  label="M MORPH"},
        {kind="step_dial", x=-0.55, y= 0.06, steps=6,  min=0, max=5,     param="master.shaper.mode",   label="MSHP MOD", color=RB.COLOR_DIAL},
        {kind="knob",      x= 0.00, y= 0.06,           min=1, max=16.0,  param="master.shaper.drive",  label="M DRIVE"},
        {kind="knob",      x= 0.55, y= 0.06,           min=0, max=1.0,   param="master.shaper.mix",    label="M SHPMIX"},
    })
    RB.add_jack_strip(2, R2.master_rack, -0.66, {
        {"mflt.in",        false, "MFLT IN",  "audio"},
        {"mflt.out",       true,  "MFLT OUT", "audio"},
        {"mflt.cutoff_cv", false, "MCUT CV",  "cv"},
        {"mflt.res_cv",    false, "MRES CV",  "cv"},
    })

    R2.refresh_curves()
end

function R2.refresh_curves()
    for ch = 1, 3 do
        local cut = gf.synth.get_param(string.format("ch%d.filter.cutoff", ch)) or 2500.0
        local res = gf.synth.get_param(string.format("ch%d.filter.res", ch)) or 0.25
        local c_norm = math.min(0.95, math.max(0.05, math.log(math.max(30.0, cut) / 30.0) / math.log(600.0)))
        local ys = {}
        for i = 1, 32 do
            local x = (i - 1) / 31.0
            local lp = 1.0 / (1.0 + (math.max(0.001, x / c_norm) ^ 4.0))
            local pk = res * 0.75 * math.exp(-(((x - c_norm) * 9.0) ^ 2.0))
            ys[i] = math.min(1.0, math.max(0.04, lp * 0.65 + pk))
        end
        gf.spatial.set_curve(3 + ch, R2.ch_racks[ch], 0.0, -0.28, 1.85, 0.34, nil, ys, false, 1.0, 0.72, 0.25, 0.95)
    end
end

return R2
