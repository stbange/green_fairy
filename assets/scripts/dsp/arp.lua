-- Green Fairy: Scriptable Arpeggiator & Musical Sequencer
local Arp = {}
Arp.__index = Arp

function Arp.new()
    return setmetatable({
        enabled = false,
        bpm = 124,
        step_index = 0,
        timer = 0.0,
        held = {},
        last_note = nil,
    }, Arp)
end

function Arp:set_enabled(flag)
    self.enabled = flag and true or false
    if not self.enabled and self.last_note then
        gf.synth.note_off(self.last_note)
        self.last_note = nil
    end
end

function Arp:note_on(note, vel)
    if not self.enabled then
        gf.synth.note_on(note, vel or 0.85)
        return
    end
    for _, n in ipairs(self.held) do
        if n == note then return end
    end
    table.insert(self.held, note)
    table.sort(self.held)
end

function Arp:note_off(note)
    if not self.enabled then
        gf.synth.note_off(note)
        return
    end
    for i, n in ipairs(self.held) do
        if n == note then
            table.remove(self.held, i)
            break
        end
    end
    if #self.held == 0 and self.last_note then
        gf.synth.note_off(self.last_note)
        self.last_note = nil
    end
end

function Arp:update(dt)
    if not self.enabled or #self.held == 0 then
        return
    end
    local step_dur = 60.0 / math.max(self.bpm, 40.0) * 0.25
    self.timer = self.timer + dt
    if self.timer >= step_dur then
        self.timer = self.timer - step_dur
        if self.last_note then
            gf.synth.note_off(self.last_note)
        end
        self.step_index = (self.step_index % #self.held) + 1
        local note = self.held[self.step_index]
        gf.synth.note_on(note, 0.82)
        self.last_note = note
    end
end

return Arp
