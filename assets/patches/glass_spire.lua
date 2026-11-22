-- Green Fairy Patch: Glass Spire (FM crystal + resonant bell harmonics)
local Gen = require("dsp.generators")
local pts, sus, ls, le = Gen.build_pluck_envelope()

return {
    name = "SPIRE",
    samples = { [1] = "sounds/glass_pad.wav" },
    wavetables = {
        [1] = { a = Gen.gen_crystal_table(256) },
    },
    envelopes = {
        amp = { points = pts, sustain = sus, loop_start = ls, loop_end = le },
    },
    params = {
        ["ch1.mode"] = 3, ["ch1.level"] = 0.70, ["ch1.mod_ratio"] = 2.5, ["ch1.fm_index"] = 3.8,
        ["ch2.mode"] = 4, ["ch2.level"] = 0.45, ["ch2.detune"] = 7.0, ["ch2.mod_ratio"] = 2.0, ["ch2.fm_index"] = 0.65,
        ["ch3.mode"] = 5, ["ch3.level"] = 0.35, ["ch3.sample"] = 0,
        ["master.filter.mode"] = 1, ["master.filter.cutoff"] = 12000.0,
        ["aether.time"] = 0.24, ["aether.feedback"] = 0.58, ["aether.mix"] = 0.45,
    },
    xy = { 0.72, 0.30 },
}
