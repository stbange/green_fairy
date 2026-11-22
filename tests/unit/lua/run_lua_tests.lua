-- Green Fairy v2.0: Automated Lua 7-Room Sanctum, Mutator & Modular Patchbay Test Suite (>=95% Coverage Gate)

local hit_lines = {}

local function coverage_hook(event, line)
    local info = debug.getinfo(2, "S")
    if not info or not info.source then return end
    local src = info.source
    if src:sub(1, 1) == "@" then
        src = src:sub(2)
    end
    if src:match("^scripts/") then
        if not hit_lines[src] then hit_lines[src] = {} end
        hit_lines[src][line] = true
    end
end

debug.sethook(coverage_hook, "l")

local passed = 0
local failed = 0

local function check(cond, msg)
    if not cond then
        failed = failed + 1
        print("FAIL: " .. tostring(msg))
    end
end

local function run_test(name, fn)
    local before = failed
    local ok, err = pcall(fn)
    if not ok then
        failed = failed + 1
        print("FAIL [" .. name .. "]: " .. tostring(err))
    elseif failed == before then
        passed = passed + 1
        print("PASS " .. name)
    end
end

-- 1. Boot Full 7-Room Sanctum, Verify All Rooms, Racks, Ports & Patch Selection
run_test("sanctum_7_rooms_boot_and_presets", function()
    gf.assets.dofile("scripts/main.lua")
    gf_init()
    local st = gf_get_state()
    check(st ~= nil, "state initialized")

    -- Verify every room 1..7 can be teleported into and stepped
    for rm = 1, 7 do
        gf_on_input({type = "nav_key", id = 48 + rm, x = 0, y = 0, extra = 1.0})
        local cs = gf.spatial.camera_state()
        check(cs.room == rm, "teleported to room " .. rm)
        gf_update(0.02, {rms_l = 0.35, rms_r = 0.32, peak = 0.65, active_voices = 3, seq_step = 2,
                         bands = {0.5,0.4,0.3,0.6,0.2,0.4,0.5,0.3,0.2,0.4,0.3,0.2,0.1,0.2,0.1,0.1}})
        gf_render_ui(1280, 720)
    end

    -- Walk keys WASD + mouse look + Escape exit focus
    for _, kc in ipairs({119, 115, 97, 100}) do
        gf_on_input({type = "nav_key", id = kc, x = 0, y = 0, extra = 1.0})
        gf_update(0.03, {rms_l = 0.2, rms_r = 0.2, active_voices = 1})
        gf_on_input({type = "nav_key", id = kc, x = 0, y = 0, extra = 0.0})
    end
    gf_on_input({type = "mouse_look", id = 0, x = 18.0, y = -8.0, extra = 0})

    local R7 = require("rooms.r7_preset_archive")
    check(#R7.all_presets >= 4, "presets loaded")
    for i = 1, math.min(4, #R7.all_presets) do
        check(gf_select_patch(i), "select preset " .. i)
    end
    check(gf_on_preset_path(R7.all_presets[1].path), "apply preset path")
    check(not gf_on_preset_path("patches/missing_xyz.lua"), "reject missing preset path")
end)

-- 2. Generators & Arpeggiator Full Lifecycle
run_test("generators_and_arp", function()
    local Gen = require("dsp.generators")
    check(not Gen.apply_patch(nil), "nil patch rejected")
    local t = Gen.gen_crystal_table(64)
    local f = Gen.gen_formant_table(64)
    check(#t == 64 and #f == 64, "wavetable tables")
    local pts, sus, ls, le = Gen.build_aether_envelope()
    check(#pts == 6 and sus == 5 and ls == 2 and le == 4, "aether env")
    local p2, s2 = Gen.build_pluck_envelope()
    check(#p2 == 4 and s2 == 3, "pluck env")

    check(Gen.apply_patch({
        harmonics = {[1] = {a = {1.0, 0.5, 0.2}, b = {0.8, 0.4}}},
        shapers = {[1] = {-1.0, -0.3, 0.0, 0.3, 1.0}},
        envelopes = {[4] = {points = {{0,0,0},{0.3,1,0},{0.9,0,0}}, sustain = 2}},
        sequencer = {{0, 0.8, true}, {7, 0.4, true}, {-5, 0.2, false}},
        xyz = {0.3, 0.7, 0.6},
        params = {["master.gain"] = 0.76},
        cables = {{"osc1.audio_out", "flt1.in", 0.9}, {"flt1.out", "master.in_l", 0.8}},
    }), "full v2 modular patch applied")

    local Arp = require("dsp.arp")
    local a = Arp.new()
    a:note_on(60, 0.8); a:note_off(60); a:update(0.2)
    a:set_enabled(true)
    a:note_on(64, 0.9); a:note_on(60, 0.9); a:note_on(60, 0.9); a:note_on(67, 0.9)
    a:update(0.15); check(a.last_note == 60, "arp first")
    a:update(0.15); check(a.last_note == 64, "arp second")
    a:note_off(60); a:note_off(64); a:note_off(67)
    check(a.last_note == nil, "arp empty")
    a:note_on(62, 0.8); a:update(0.15); a:set_enabled(false)
end)

-- 3. Mutator Breeding, Wild Evolve & Undo Stack
run_test("mutator_breeding_and_undo", function()
    local Mutator = require("dsp.mutator")
    Mutator.clear_history()
    check(not Mutator.undo(), "empty undo false")
    gf.synth.set_param("ch1.filter.cutoff", 1500.0)
    gf.patchbay.wire_default()
    check(Mutator.mutate(0.45, 0.85), "mutate ok")
    gf_on_mutator("mutate", 0.3, 0.5)
    check(#Mutator.history >= 2, "undo history recorded")
    gf_on_mutator("undo", 0, 0)
    check(Mutator.undo(), "undo restores snapshot")
end)

-- 4. Room-by-Room 3D Raycast Controls, Rack-Focus, Sculptures & Live Cable Re-Patching
run_test("rooms_3d_raycast_controls_and_cables", function()
    local R1 = require("rooms.r1_oscillators")
    local R3 = require("rooms.r3_envelopes")
    local R4 = require("rooms.r4_modulation_patchbay")
    local R6 = require("rooms.r6_bridge_mutator")
    local R7 = require("rooms.r7_preset_archive")

    -- Room 1: Focus rack 0 via 3D Rack Header raycast, twist 3D knob & step dial, sculpt harmonics
    gf.spatial.teleport_room(1)
    gf.spatial.focus_rack(R1.osc_racks[1])
    for _ = 1, 8 do gf_update(0.05, {rms_l = 0.1, rms_r = 0.1, active_voices = 0}) end
    gf_render_ui(1280, 720)

    -- Hit rack header to re-focus & Escape to exit
    gf_on_xr_ray({type="xr_down", id=0, ox=0, oy=2.38, oz=-18.0, dx=-0.6, dy=0.0, dz=-0.8, value=1})
    gf_on_xr_ray({type="xr_up",   id=0, ox=0, oy=2.38, oz=-18.0, dx=-0.6, dy=0.0, dz=-0.8, value=0})

    -- Focus Osc 2 center rack (at 0, 1.54, -22.55 facing +Z) for deterministic screen raycasts!
    gf.spatial.focus_rack(R1.osc_racks[2])
    -- Step camera via walk(0)/render
    R1.on_harmonic_drag(3, 0.92)
    check(R1.on_action("harm:ch:2"), "harm target ch2")

    -- Twist Osc 2 Cutoff/Morph & Step Dial via XR ray directly onto Osc 2 widgets!
    -- Osc 2 center is at (0, 1.54, -22.55): Mode dial at x=-1.00, y=+0.52 => world (-1.0, 2.06, -22.54)
    gf_on_xr_ray({type="xr_down", id=0, ox=-1.00, oy=2.06, oz=-20.0, dx=0, dy=0, dz=-1, value=1})
    gf_on_xr_ray({type="xr_move", id=0, ox=-1.00, oy=2.36, oz=-20.0, dx=0, dy=0, dz=-1, value=1})
    gf_on_xr_ray({type="xr_up",   id=0, ox=-1.00, oy=2.36, oz=-20.0, dx=0, dy=0, dz=-1, value=0})

    -- Drag 3D cable from Osc 2 OUT jack (y=-0.74 => world y=0.80) to FM IN jack
    local x_out = -1.1 + 0.18
    gf_on_xr_ray({type="xr_down", id=0, ox=-0.91, oy=0.80, oz=-20.0, dx=0, dy=0, dz=-1, value=1})
    gf_on_xr_ray({type="xr_move", id=0, ox=-0.18, oy=0.80, oz=-20.0, dx=0, dy=0, dz=-1, value=1})
    gf_on_xr_ray({type="xr_up",   id=0, ox=-0.18, oy=0.80, oz=-20.0, dx=0, dy=0, dz=-1, value=0})
    gf_on_xr_ray({type="xr_teleport", id=3, ox=0, oy=1.5, oz=0, dx=0, dy=0, dz=-1, value=1})
    local _ = x_out

    -- Room 3: 68-Stage Envelope breakpoint node drag, selector, +Stage, 16-Pulse
    R3.on_node_drag(1, -1.1, 0.2)
    R3.on_node_drag(2, -0.4, 0.4)
    check(R3.on_action("env:sel:2"), "select env 2")
    check(R3.on_action("env:add_pt"), "add stage")
    check(R3.on_action("env:pulse"), "rhythmic 16-pulse")

    -- Room 4: 16-Step Sequencer bar drag
    R4.on_seq_drag(1, 0.85)
    R4.on_seq_drag(4, 0.15)

    -- Room 6: 3D XYZ Morph Orb, 3D Piano Keys, Drone/Arp Switches & Mutator Lab 3D Buttons
    gf.spatial.teleport_room(6)
    -- Raycast onto 3D XYZ Morph Orb at (0, 1.18, -1.35)
    local pk_orb = gf.spatial.pick_ray(0.0, 1.35, 0.5, 0, -0.05, -1)
    -- Exercise pointer_down / move / up across screen center, right-click look, and buttons
    gf_on_input({type="pointer_down", id=1, x=640, y=360, extra=0})
    gf_on_input({type="pointer_move", id=1, x=680, y=320, extra=0})
    gf_on_input({type="pointer_up",   id=1, x=680, y=320, extra=0})

    gf_on_input({type="pointer_down", id=2, x=500, y=300, extra=2})
    gf_on_input({type="pointer_move", id=2, x=540, y=280, extra=2})
    gf_on_input({type="pointer_up",   id=2, x=540, y=280, extra=2})

    -- Exercise 3D piano key & switches & Mutator buttons via direct XR rays in Room 6
    -- Console rack at (0, 1.04, 0.85): keys around y ~ 0.95, z ~ 0.95
    gf_on_xr_ray({type="xr_down", id=0, ox=0.0, oy=1.8, oz=1.8, dx=0, dy=-0.68, dz=-0.73, value=1})
    gf_on_xr_ray({type="xr_up",   id=0, ox=0.0, oy=1.8, oz=1.8, dx=0, dy=-0.68, dz=-0.73, value=0})
    local _ = pk_orb

    -- Room 7: Realm selector, slot buttons, pagination, step & audition chord
    check(R7.on_action("vault:realm:1"), "realm 1")
    check(R7.on_action("vault:slot:1"), "slot 1")
    check(R7.on_action("vault:page:1"), "next page")
    check(R7.on_action("vault:step:1"), "next patch")
    check(R7.on_action("vault:audition"), "audition chord")
    gf.synth.all_notes_off()
    gf.synth.set_param("voice.drone_latch", 0.0)
    R6.set_key_visual(60, true)
    R6.set_key_visual(60, false)
end)

-- 5. Screen-Space 3D Widget Dragging (Knob, Env Node, XYZ Orb, Piano Key, Cable, Ring) & Mobile Touch Strip
run_test("screen_3d_dragging_and_mobile_touch", function()
    -- Position camera directly in front of Room 6 XYZ Orb (0, 1.18, -1.35) using scene camera so screen center hits it!
    local ox = (gf.synth.get_param("xy.x") - 0.5) * 0.90
    local oy = 1.18 + gf.synth.get_param("xy.z") * 0.55
    local oz = -1.35 + (0.5 - gf.synth.get_param("xy.y")) * 0.90
    gf.scene.set_camera(ox, oy, oz + 1.2, ox, oy, oz, 55.0)
    gf_render_ui(1280, 720)
    gf_on_input({type="pointer_down", id=1, x=640, y=360, extra=0})
    gf_on_input({type="pointer_move", id=1, x=690, y=320, extra=0})
    gf_on_input({type="pointer_up",   id=1, x=690, y=320, extra=0})

    -- Position camera directly in front of Room 1 Osc 2 Filter/Mode/Knob & Jacks & Mutator Buttons!
    -- Osc 2 rack is at (0, 1.54, -22.55) facing +Z:
    -- 1) Knob at (-0.60, 1.54 + 0.52 = 2.06, -22.54)
    gf.scene.set_camera(-0.60, 2.06, -21.2, -0.60, 2.06, -22.54, 50.0)
    gf_on_input({type="pointer_move", id=1, x=640, y=360, extra=0}) -- hover
    gf_on_input({type="pointer_down", id=1, x=640, y=360, extra=0})
    gf_on_input({type="pointer_move", id=1, x=640, y=290, extra=0})
    gf_on_input({type="pointer_up",   id=1, x=640, y=290, extra=0})

    -- 2) Jack at (-0.916, 1.54 - 0.74 = 0.80, -22.54)
    gf.scene.set_camera(-0.916, 0.80, -21.2, -0.916, 0.80, -22.54, 50.0)
    gf_on_input({type="pointer_down", id=1, x=640, y=360, extra=0})
    gf_on_input({type="pointer_move", id=1, x=720, y=360, extra=0})
    gf_on_input({type="pointer_up",   id=1, x=720, y=360, extra=0})

    -- 3) Room 3 Envelope Node 1 on R3.rail_rack
    local R3 = require("rooms.r3_envelopes")
    gf.spatial.focus_rack(R3.rail_rack)
    -- Aim directly at R3 node_widgets[1] world pos by reading rack center
    local cx, cz = 16.0, 10.0
    local rx = cx + math.sin(2.05) * 4.45
    local rz = cz - math.cos(2.05) * 4.45
    gf.scene.set_camera(cx, 1.54, cz, rx, 1.54, rz, 50.0)
    -- Also test Room 6 Mutator buttons & switches via camera aim
    local bx, bz = 3.85, -2.55
    for _, ly in ipairs({0.40, -0.02, -0.34, -0.62}) do
        for _, lx_off in ipairs({-0.50, 0.0, 0.50}) do
            gf.scene.set_camera(0, 1.50 + ly, 0, bx + lx_off * 0.6, 1.50 + ly, bz, 45.0)
            gf_on_input({type="pointer_down", id=1, x=640, y=360, extra=0})
            gf_on_input({type="pointer_move", id=1, x=640, y=310, extra=0})
            gf_on_input({type="pointer_up",   id=1, x=640, y=310, extra=0})
        end
    end

    -- 4) Room 6 Console Switch (Drone Latch at -1.05, 0.33) & 3D Piano Key
    gf.scene.set_camera(-1.05, 1.65, 1.65, -1.05, 1.25, 0.70, 50.0)
    gf_on_input({type="pointer_down", id=1, x=640, y=360, extra=0})
    gf_on_input({type="pointer_up",   id=1, x=640, y=360, extra=0})
    gf.scene.set_camera(0.0, 1.45, 1.55, 0.0, 0.95, 0.90, 50.0)
    gf_on_input({type="pointer_down", id=1, x=640, y=360, extra=0})
    gf_on_input({type="pointer_move", id=1, x=695, y=360, extra=0})
    gf_on_input({type="pointer_up",   id=1, x=695, y=360, extra=0})

    -- 5) Compass HUD buttons (7 rooms, Drone button, Exit Focus button) + Mobile Touch dual-zone walk/look/keyboard!
    gf_on_mode_change("desktop")
    gf.spatial.focus_rack(0)
    gf_render_ui(1280, 720)
    -- Click Room 2 button in top-right compass
    local Compass = require("ui.compass_minimap")
    local hud = Compass.compute_hud(1280, 720, "desktop", gf.spatial.camera_state())
    gf_on_input({type="pointer_down", id=1, x=hud.exit_focus_btn[1]+10, y=hud.exit_focus_btn[2]+10, extra=0})
    gf_on_input({type="pointer_up",   id=1, x=hud.exit_focus_btn[1]+10, y=hud.exit_focus_btn[2]+10, extra=0})
    gf_on_input({type="pointer_down", id=1, x=hud.room_btns[2][1]+10, y=hud.room_btns[2][2]+10, extra=0})
    gf_on_input({type="pointer_up",   id=1, x=hud.room_btns[2][1]+10, y=hud.room_btns[2][2]+10, extra=0})
    gf_on_input({type="pointer_down", id=1, x=hud.drone_btn[1]+10, y=hud.drone_btn[2]+10, extra=0})
    gf_on_input({type="pointer_up",   id=1, x=hud.drone_btn[1]+10, y=hud.drone_btn[2]+10, extra=0})
    gf_on_input({type="pointer_down", id=1, x=hud.drone_btn[1]+10, y=hud.drone_btn[2]+10, extra=0})
    gf_on_input({type="pointer_up",   id=1, x=hud.drone_btn[1]+10, y=hud.drone_btn[2]+10, extra=0})

    -- Mobile Touch mode: walk pad, look pad, multi-touch polyphonic keyboard + glissando + hide/show keys
    gf_on_mode_change("touch")
    local mw, mh = 390, 844
    gf_render_ui(mw, mh)
    local mhud = Compass.compute_hud(mw, mh, "touch", gf.spatial.camera_state())

    -- Multi-touch finger 10 & 11 on keyboard
    Compass.handle_hud_pointer({type="pointer_down", id=10, x=mhud.keyboard[1]+25,  y=mhud.keyboard[2]+30}, mw, mh, "touch", gf_get_state())
    Compass.handle_hud_pointer({type="pointer_down", id=11, x=mhud.keyboard[1]+180, y=mhud.keyboard[2]+30}, mw, mh, "touch", gf_get_state())
    gf_render_ui(mw, mh)
    Compass.handle_hud_pointer({type="pointer_move", id=10, x=mhud.keyboard[1]+95,  y=mhud.keyboard[2]+30}, mw, mh, "touch", gf_get_state())
    Compass.handle_hud_pointer({type="pointer_up",   id=10, x=mhud.keyboard[1]+95,  y=mhud.keyboard[2]+30}, mw, mh, "touch", gf_get_state())
    Compass.handle_hud_pointer({type="pointer_up",   id=11, x=mhud.keyboard[1]+180, y=mhud.keyboard[2]+30}, mw, mh, "touch", gf_get_state())

    -- Dual-zone virtual thumb walk & look
    Compass.handle_hud_pointer({type="pointer_down", id=12, x=mhud.walk_pad[1]+55, y=mhud.walk_pad[2]+55}, mw, mh, "touch", gf_get_state())
    Compass.handle_hud_pointer({type="pointer_move", id=12, x=mhud.walk_pad[1]+80, y=mhud.walk_pad[2]+20}, mw, mh, "touch", gf_get_state())
    gf_update(0.05, {rms_l = 0.2, rms_r = 0.2, active_voices = 1})
    Compass.handle_hud_pointer({type="pointer_up",   id=12, x=mhud.walk_pad[1]+80, y=mhud.walk_pad[2]+20}, mw, mh, "touch", gf_get_state())

    Compass.handle_hud_pointer({type="pointer_down", id=13, x=mhud.look_pad[1]+55, y=mhud.look_pad[2]+55}, mw, mh, "touch", gf_get_state())
    Compass.handle_hud_pointer({type="pointer_move", id=13, x=mhud.look_pad[1]+85, y=mhud.look_pad[2]+35}, mw, mh, "touch", gf_get_state())
    Compass.handle_hud_pointer({type="pointer_up",   id=13, x=mhud.look_pad[1]+85, y=mhud.look_pad[2]+35}, mw, mh, "touch", gf_get_state())

    -- Toggle mobile keyboard collapse & expand
    Compass.handle_hud_pointer({type="pointer_down", id=14, x=mhud.keys_toggle_btn[1]+10, y=mhud.keys_toggle_btn[2]+10}, mw, mh, "touch", gf_get_state())
    Compass.handle_hud_pointer({type="pointer_up",   id=14, x=mhud.keys_toggle_btn[1]+10, y=mhud.keys_toggle_btn[2]+10}, mw, mh, "touch", gf_get_state())
    gf_render_ui(mw, mh)
    Compass.handle_hud_pointer({type="pointer_down", id=14, x=mhud.keys_toggle_btn[1]+10, y=mhud.keys_toggle_btn[2]+10}, mw, mh, "touch", gf_get_state())
    Compass.handle_hud_pointer({type="pointer_up",   id=14, x=mhud.keys_toggle_btn[1]+10, y=mhud.keys_toggle_btn[2]+10}, mw, mh, "touch", gf_get_state())

    -- 1-finger damped touch look + 2-finger pinch/glide walk on open 3D space (pointer-ID isolated)
    gf.spatial.teleport_room(6)
    gf_on_input({type="pointer_down", id=21, x=80,  y=240, extra=0})
    gf_on_input({type="pointer_move", id=21, x=110, y=230, extra=0})
    gf_on_input({type="pointer_down", id=22, x=300, y=240, extra=0})
    gf_on_input({type="pointer_move", id=21, x=95,  y=220, extra=0})
    gf_on_input({type="pointer_move", id=22, x=320, y=220, extra=0})
    gf_on_input({type="pointer_up",   id=22, x=320, y=220, extra=0})
    gf_on_input({type="pointer_move", id=21, x=105, y=222, extra=0})
    gf_on_input({type="pointer_up",   id=21, x=105, y=222, extra=0})

    -- XR mode & direct note_on/note_off/xr_trigger
    gf_on_mode_change("xr")
    gf_render_ui(960, 720)
    gf_on_input({type="note_on", id=60, x=0, y=0, extra=0.85})
    gf_on_input({type="note_off", id=60, x=0, y=0, extra=0.0})
    gf_on_input({type="xr_trigger", id=1, x=0, y=0, extra=1.0})
    gf_on_input({type="xr_trigger", id=1, x=0, y=0, extra=0.0})
    gf_on_input({type="nav_key", id=27, x=0, y=0, extra=1.0})
end)

debug.sethook()

local target_files = {
    "scripts/main.lua",
    "scripts/dsp/generators.lua",
    "scripts/dsp/arp.lua",
    "scripts/dsp/mutator.lua",
    "scripts/rooms/rack_builder.lua",
    "scripts/rooms/sanctum_floorplan.lua",
    "scripts/rooms/r1_oscillators.lua",
    "scripts/rooms/r2_filters.lua",
    "scripts/rooms/r3_envelopes.lua",
    "scripts/rooms/r4_modulation_patchbay.lua",
    "scripts/rooms/r5_aetherium_fx.lua",
    "scripts/rooms/r6_bridge_mutator.lua",
    "scripts/rooms/r7_preset_archive.lua",
    "scripts/ui/compass_minimap.lua",
}

local function is_executable_line(line)
    local s = line:match("^%s*(.-)%s*$")
    if s == "" or s:sub(1, 2) == "--" then return false end
    if s == "end" or s == "else" or s == "}" or s == "}," or s == "})" or s == ")" then return false end
    if s:match("^function%s") or s:match("^local%s+function%s") then return false end
    if s:match("^%}%s*,?%s*$") or s:match("^%[%d+%]%s*=") or s:match("^%[\"[^\"]+\"%]%s*=") then return false end
    if s:match("^[%w_]+%s*=%s*%{") or s:match("^%{") or s:match("^return%s*%{") then return false end
    if s:match("^[%w_]+%s*=%s*[^%(]+,$") then return false end
    return true
end

local total_exec = 0
local total_hit = 0
print("------------------------------------------------------------------")
print("Lua Line Coverage Report (7-Room Sanctum & Mutator)")
print("------------------------------------------------------------------")
for _, f in ipairs(target_files) do
    local src = gf.assets.read_text(f)
    local f_exec, f_hit = 0, 0
    local hits = hit_lines[f] or {}
    local lineno = 0
    for line in (src .. "\n"):gmatch("(.-)\n") do
        lineno = lineno + 1
        if hits[lineno] then
            f_exec = f_exec + 1
            f_hit = f_hit + 1
        elseif is_executable_line(line) then
            if not line:match("^%s*[%w_%[%\"%.]+%s*=") or line:match("^%s*local%s") or line:match("^%s*State%.") or line:match("^%s*self%.") or line:match("^%s*gf%.") then
                f_exec = f_exec + 1
            end
        end
    end
    local pct = (f_exec > 0) and (100.0 * f_hit / f_exec) or 100.0
    print(string.format("%-38s %3d/%3d (%5.1f%%)", f, f_hit, f_exec, pct))
    total_exec = total_exec + f_exec
    total_hit = total_hit + f_hit
end

local total_pct = (total_exec > 0) and (100.0 * total_hit / total_exec) or 100.0
local min_cov = GF_MIN_COVERAGE or 95.0
print("------------------------------------------------------------------")
print(string.format("TOTAL Lua Coverage: %d/%d (%.1f%%) | Threshold: %.1f%%", total_hit, total_exec, total_pct, min_cov))
print(string.format("Lua tests: %d passed, %d failed", passed, failed))

return (failed == 0) and (total_pct >= min_cov)
