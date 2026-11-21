-- Green Fairy Patch: Crystalline FM (Wavefolded retro cyber lead)
local Gen = require("dsp.generators")
local pts, sus, ls, le = Gen.build_pluck_envelope()

return {
    name = "CYBERFM",
    wavetables = {
        [1] = { a = Gen.gen_formant_table(256) },
    },
    envelopes = {
        amp = { points = pts, sustain = sus, loop_start = ls, loop_end = le },
    },
    params = {
        ["ch1.mode"] = 1, ["ch1.level"] = 0.75, ["ch1.shaper.mode"] = 2, ["ch1.shaper.drive"] = 3.2,
        ["ch2.mode"] = 3, ["ch2.level"] = 0.55, ["ch2.mod_ratio"] = 4.0, ["ch2.fm_index"] = 5.2,
        ["ch3.mode"] = 1, ["ch3.level"] = 0.40, ["ch3.detune"] = -12.0,
        ["master.filter.mode"] = 1, ["master.filter.cutoff"] = 10500.0, ["master.filter.res"] = 0.35,
        ["aether.time"] = 0.18, ["aether.feedback"] = 0.35, ["aether.mix"] = 0.25,
    },
    xy = { 0.85, 0.15 },
}
