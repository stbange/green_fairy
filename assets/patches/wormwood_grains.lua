-- Green Fairy Patch: Wormwood Grains (Granular sample cloud + sub wavetable)
local Gen = require("dsp.generators")
local pts, sus, ls, le = Gen.build_aether_envelope()

return {
    name = "GRAINS",
    samples = { [1] = "sounds/absinthe_bell.wav" },
    envelopes = {
        amp = { points = pts, sustain = sus, loop_start = ls, loop_end = le },
    },
    params = {
        ["ch1.mode"] = 6, ["ch1.level"] = 0.85, ["ch1.grain_size"] = 0.09,
        ["ch1.grain_density"] = 18.0, ["ch1.grain_pos"] = 0.20, ["ch1.sample"] = 0,
        ["ch2.mode"] = 2, ["ch2.level"] = 0.45, ["ch2.detune"] = -12.0, ["ch2.morph"] = 0.60,
        ["ch3.mode"] = 0, ["ch3.level"] = 0.0,
        ["master.filter.mode"] = 1, ["master.filter.cutoff"] = 6800.0,
        ["aether.time"] = 0.44, ["aether.feedback"] = 0.62, ["aether.mix"] = 0.52,
    },
    xy = { 0.45, 0.72 },
}
