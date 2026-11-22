# Tasks

## 1. Modular Audio + CV Patchbay Graph (`src/dsp/patchbay.*`)

- [x] 1.1 Implement typed ~96-port registry (`GFPortId`), many-to-many stackable cable connections with bipolar attenuverters, Kahn topological sort, and 1-sample soft-saturated cycle feedback buses in `src/dsp/patchbay.c` with unit tests in `tests/unit/c/test_patchbay.c`; verify cycle stability, multi-cable summing, and >=95% line coverage on `src/dsp/patchbay.c`.
- [x] 1.2 Integrate lock-free double-buffered `GFPatchPlan` swapping and per-cable live RMS/peak signal telemetry into `src/dsp/synth.c` with unit tests verifying click-free live cable unplugging/re-patching during active playback at >=95% coverage.

## 2. Professional Absynth 5+ DSP Upgrade (`src/dsp/`)

- [x] 2.1 Upgrade `src/dsp/oscillator.c` to 9 modes: mip-mapped anti-aliased Single/Double Wavetables + 64-partial harmonic sculpting, Feedback/Cross-Channel FM, RingMod, 4th-order Hilbert Single-Sideband Frequency Shifter, Recursive Fractalizer, Crossfade Sample, 16-grain Stereo Cloud Granular, and 8-Voice Stereo Super-Unison; verify spectral/anti-aliasing unit tests in `tests/unit/c/test_dsp.c` at >=95% coverage.
- [x] 2.2 Upgrade `src/dsp/filter.c` to 14 topologies (12 dB SVF, 24 dB ZDF Ladder LP/HP/BP, 4-stage All-Pass, Damped Karplus-Strong Comb+ / Comb-, 5-Vowel A-E-I-O-U Formant Filter, and Granular Cloud Filter) and upgrade `src/dsp/waveshaper.c` with 256-point custom wave-transfer curves, phase offset, and asymmetric folding; verify unit tests achieve >=95% coverage.
- [x] 2.3 Upgrade `src/dsp/envelope.c` to 12 assignable 68-stage envelopes supporting Sustain, Loop, Tempo-Synced Rhythmic Step-Pulse, Per-Stage Embedded Envelope-LFO, and Macro/XY Control-Scrubbing, plus implement `src/dsp/sequencer.c` (16-step polyrhythmic modulator) and `src/dsp/tuning.c` (8 microtuning scales, polyphonic glide, Drone Hold Latch); verify unit tests achieve >=95% coverage.
- [x] 2.4 Implement the full stereo studio FX rack (`src/dsp/aetherizer.c` Pro 24-grain crystal cloud, `src/dsp/fx_pipe.c` waveguide acoustic pipe, `src/dsp/fx_resonators.c` 6-tap delay + 6-voice chordal Multi-Comb, `src/dsp/fx_reverb.c` 8x8 Hadamard FDN Space Cathedral Reverb, and stereo chorus/phaser) with unit tests verifying finite resonant/reverberant tails and >=95% coverage across `src/dsp/`.

## 3. Seven-Room 3D Sanctum, Raycast Controls & Catenary Cables (`src/render/`, `assets/models/`, `assets/shaders/`)

- [x] 3.1 Author external 3D architectural room & modular hardware meshes in `assets/models/` (vaulted room shell/archway, rack chassis, rotary knob, fader cap, toggle switch, hex jack, keyboard key, pedestal) and cable pulse shaders (`assets/shaders/cable.vert.glsl`, `assets/shaders/cable.frag.glsl`); verify asset loader tests read all new models and shaders cleanly.
- [x] 3.2 Implement Dual-Mode 3D Camera Rig (`src/render/camera_rig.c`) supporting WASD/mouse-look free-walk with 7-room floorplan collision, smooth cubic click-to-focus rack framing, room frustum/portal visibility culling, and WebXR room teleport poses, with unit tests in `tests/unit/c/test_spatial_ui.c` achieving >=95% coverage.
- [x] 3.3 Implement 3D Widget & Raycast Picking Engine (`src/render/spatial_ui.c`) supporting OBB ray intersection, 3D knobs/faders/switches/step-dials/jacks/piano-keys/XYZ-orb, 3D world-space text readouts, and interactive 3D 68-stage envelope & 64-bar wavetable sculptures, with unit tests achieving >=95% coverage.
- [x] 3.4 Implement 3D Catenary Patch Cable & Cross-Room Conduit Renderer (`src/render/cable_renderer.c`) generating batched sagging tube geometry, overhead inter-room cable conduits, draggable mid-cable 3D attenuverter rings, and live telemetry pulse shading; verify unit tests in `tests/unit/c/test_render.c` achieve >=95% coverage across `src/render/`.

## 4. Lua Seven-Room Architecture, Mutator & Cross-Platform Runtime (`src/lua/`, `src/platform/`, `assets/scripts/`, `web/`)

- [x] 4.1 Extend `src/lua/lua_host.c` with `gf.patchbay`, `gf.spatial`, `gf.mutator`, and `gf.presets` C bindings and C unit tests in `tests/unit/c/test_lua_host.c` achieving >=95% coverage on `src/lua/`.
- [x] 4.2 Implement the external Lua 7-room sanctum & Mutator suite under `assets/scripts/` (`rooms/sanctum_floorplan.lua`, `rooms/r1_oscillators.lua`, `rooms/r2_filters.lua`, `rooms/r3_envelopes.lua`, `rooms/r4_modulation_patchbay.lua`, `rooms/r5_aetherium_fx.lua`, `rooms/r6_bridge_mutator.lua`, `rooms/r7_preset_archive.lua`, `dsp/mutator.lua`, `ui/compass_minimap.lua`) exposing every DSP parameter and patch jack in 3D; verify Lua unit tests pass with >=95% line coverage.
- [x] 4.3 Upgrade `src/platform/` and `web/js/` (`bridge.js`, `webxr.js`, `shell.html`) for desktop pointer-lock/WASD + rack-focus, mobile dual-zone touch walk/look + tap-to-focus + collapsible multi-touch keyboard/drone strip, WebXR room teleport + 3D ray knob/cable grabbing, and on-demand HTTP preset fetching; verify `test_platform` achieves >=95% coverage and Emscripten compiles cleanly.

## 5. 320+ Atmospheric Fantasy & Space Preset Vault (`assets/patches/`, `assets/sounds/`, `assets/wavetables/`)

- [x] 5.1 Expand external `assets/sounds/` (16 acoustic/spectral `.wav` textures: bowed glass, choir vowels, waterphone, metallic bell, breath flute, cosmic sweep, plucked harp, crystal grains, etc.) and `assets/wavetables/` (16 harmonic/formant wavetables) and update `assets/manifest.json`; verify zero embedded binary assets in `src/`.
- [x] 5.2 Create the 320+ curated external `.lua` preset library across 8 directories (`fantasy_realms`, `deep_space`, `bioluminescent_abyss`, `crystalline_sanctum`, `ancient_mythology`, `cybernetic_ruins`, `astral_choirs`, `rhythmic_odysseys` — 40 presets each) plus `assets/patches/catalog.json`, and update `README.md` documenting all 7 rooms, modular patching, DSP modules, and preset realms.
- [x] 5.3 Implement automated preset catalog & audio diversity validator (`tests/unit/c/test_preset_vault.c`) verifying all 320+ presets load cleanly, wire valid modular cables, produce finite non-silent non-clipping stereo audio, and exhibit distinct spectral/temporal characteristics across realms.
- [x] 5.4 Implement polyphonic phrase recorder & overdub looper (`assets/scripts/dsp/looper.lua`, Room 6 3D Looper Rack, Compass/Touch HUD `REC`/`LOOP`/`CLR`, and Web toolbar) plus 108 external slow ambient cinematic Hans Zimmer-inspired `.lua` melodies across 6 suites in `assets/melodies/` (`catalog.json`), verified via unit and E2E suites.

## 6. Full-System >=95% Coverage Gates, E2E Suites & GitHub Pages Update

- [x] 6.1 Run aggregated C coverage gate (`gcovr --root . build/linux-coverage --filter '^src/' --fail-under-line 95`) and Lua coverage gate (`gf_lua_test_runner --fail-under 95`) and verify >=95.0% coverage on both C and Lua codebases.
- [x] 6.2 Upgrade and run automated native headless E2E suite (`tests/e2e/native_e2e.py`) verifying distinct 3D rendering across all 7 rooms, rack-focus zoom, 3D raycast knob turns, live cable unplugging/re-patching with measurable audio change, Mutator breeding, and WAV verification.
- [x] 6.3 Upgrade and run automated Playwright browser E2E suite (`tests/e2e/web/run_web_e2e.mjs`) verifying sub-second Web boot, on-demand HTTP loading of presets across realms, desktop 3D room navigation + cable re-patching, mobile touch 3D navigation + rack focus + multi-touch notes, and WebXR stereo room teleport + XR ray cable patching; redeploy the verified Web build to GitHub Pages.

## Workflow follow-up

- Archive `absynth-modular-sanctum` (`openspec archive absynth-modular-sanctum`) after user review of the completed implementation.
