-- Green Fairy: External Lua Entry Script (Hot-Reloadable)
local Generators = require("dsp.generators")
local Arp        = require("dsp.arp")
local Stage      = require("scene.stage")
local Layout     = require("ui.layout")

local State = {
    mode = "desktop",
    patch_files = {},
    patch_index = 1,
    patch_name = "EMERALD",
    xy_x = 0.30,
    xy_y = 0.25,
    arp = Arp.new(),
    active_notes = {},
    pointer_notes = {},
    pointer_drag = {},
}

function gf_select_patch(index)
    if #State.patch_files == 0 then return false end
    State.patch_index = ((index - 1) % #State.patch_files) + 1
    local fname = State.patch_files[State.patch_index]
    local ok, patch = pcall(gf.assets.dofile, "patches/" .. fname)
    if ok and type(patch) == "table" then
        Generators.apply_patch(patch)
        State.patch_name = patch.name or fname
        if patch.xy then
            State.xy_x = patch.xy[1] or State.xy_x
            State.xy_y = patch.xy[2] or State.xy_y
        end
        return true
    end
    return false
end

State.next_patch = function()
    gf_select_patch(State.patch_index + 1)
end

function gf_init()
    State.mode = gf.ui.input_mode()
    Stage.init()
    State.patch_files = {}
    local files = gf.assets.list("patches")
    for _, f in ipairs(files) do
        if f:sub(-4) == ".lua" then
            table.insert(State.patch_files, f)
        end
    end
    if #State.patch_files > 0 then
        gf_select_patch(1)
    end
    gf.log("Green Fairy Lua initialized (" .. #State.patch_files .. " patches, mode=" .. State.mode .. ")")
end

function gf_update(dt, telemetry)
    State.arp:update(dt)
    Stage.update(dt, telemetry, State.xy_x, State.xy_y)
end

function gf_render_ui(w, h)
    Layout.render(State, w, h)
end

function gf_on_mode_change(mode)
    State.mode = mode
    gf.log("Input mode changed to: " .. tostring(mode))
end

function gf_on_input(ev)
    if ev.type == "note_on" then
        State.active_notes[ev.id] = true
        State.arp:note_on(ev.id, ev.extra or 0.85)
    elseif ev.type == "note_off" then
        State.active_notes[ev.id] = nil
        State.arp:note_off(ev.id)
    elseif ev.type == "xr_trigger" then
        -- WebXR controller trigger plays emerald chord / interacts with spatial UI
        if ev.extra > 0.5 then
            local note = 55 + (ev.id % 5) * 2
            State.active_notes[note] = true
            gf.synth.note_on(note, 0.85)
        else
            gf.synth.all_notes_off()
            State.active_notes = {}
        end
    elseif ev.type == "pointer_down" or ev.type == "pointer_move" or ev.type == "pointer_up" then
        local w, h = gf.ui.viewport()
        Layout.handle_pointer(State, ev, w, h)
    end
end

function gf_get_state()
    return State
end
