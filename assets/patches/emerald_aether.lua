-- Green Fairy Patch: Emerald Aether (Absynth-inspired glassy breathing pad)
local Gen = require("dsp.generators")
local pts, sus, ls, le = Gen.build_aether_envelope()

return {
    name = "EMERALD",
    samples = { [1] = "sounds/absinthe_bell.wav" },
    wavetables = {
        [1] = { a = Gen.gen_crystal_table(256), b = Gen.gen_formant_table(256) },
    },
    envelopes = {
        amp = { points = pts, sustain = sus, loop_start = ls, loop_end = le },
    },
    params = {
        ["ch1.mode"] = 2, ["ch1.level"] = 0.72, ["ch1.morph"] = 0.40,
        ["ch1.filter.mode"] = 1, ["ch1.filter.cutoff"] = 4800.0, ["ch1.filter.res"] = 0.28,
        ["ch2.mode"] = 3, ["ch2.level"] = 0.42, ["ch2.detune"] = 12.0, ["ch2.mod_ratio"] = 3.0, ["ch2.fm_index"] = 2.4,
        ["ch3.mode"] = 4, ["ch3.level"] = 0.30, ["ch3.detune"] = -12.0, ["ch3.mod_ratio"] = 1.5, ["ch3.fm_index"] = 0.45,
        ["master.filter.mode"] = 1, ["master.filter.cutoff"] = 9200.0, ["master.filter.res"] = 0.20,
        ["aether.time"] = 0.34, ["aether.feedback"] = 0.50, ["aether.mod"] = 0.45, ["aether.mix"] = 0.38,
        ["master.gain"] = 0.68,
    },
    xy = { 0.30, 0.25 },
}
