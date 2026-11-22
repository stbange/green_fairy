# Tasks

## 1. Project Scaffolding & Cross-Platform Build System

- [x] 1.1 Create CMake project structure (`CMakeLists.txt`, `CMakePresets.json`, `src/`, `web/`, `assets/`, `tests/`) with pinned FetchContent setup for SDL3 and Lua 5.4, plus coverage build preset (`linux-coverage`); verify `cmake --preset linux-debug` configures cleanly.
- [x] 1.2 Implement core utilities in `src/core/` (3D math `vec3`/`mat4`/`quat`, lock-free SPSC ring buffer, deterministic PRNG, path normalization/traversal guard, structured logger) with C unit tests in `tests/unit/c/test_core.c`; verify all core unit tests pass with >=95% line coverage on `src/core/`.
- [x] 1.3 Author `README.md` build/run/test documentation and `.github/workflows/ci.yml` covering Linux, Windows, macOS, and Emscripten build matrices; verify workflow YAML parses cleanly.

## 2. External Asset Pipeline & Initial Asset Pack

- [x] 2.1 Implement external asset root resolution (`<exe_dir>/assets`, `--assets`, `GREEN_FAIRY_ASSETS`) and path-traversal protection in `src/assets/asset_manager.c` with unit tests in `tests/unit/c/test_assets.c`; verify missing/escaping paths fail cleanly with >=95% coverage.
- [x] 2.2 Implement typed external loaders for Wavefront `.obj` meshes, `.tga`/`.ppm` sprites, `.gfvox` optional voxel grids, PCM `.wav` audio samples, GLSL shaders (with desktop GL 3.3 / WebGL2 ES 3.0 version header injection), and bitmap font metadata, with unit tests covering valid and malformed files; verify >=95% coverage on `src/assets/`.
- [x] 2.3 Create the external `assets/` directory pack beside the binary (`manifest.json`, retro 3D `.obj` models, pixel sprite sheets, optional `.gfvox` voxel crystal/altar, external GLSL shaders, PCM `.wav` granular/sample sources, bitmap font atlas) and verify zero asset data arrays exist in `src/` via automated check.
- [x] 2.4 Implement file modification detection and manual hot-reload manager in `src/assets/hot_reload.c` with unit tests verifying change detection and safe fallback on invalid edits.

## 3. Absynth-Inspired DSP Synthesis Engine

- [x] 3.1 Implement multi-stage breakpoint envelopes (up to 68 breakpoints, segment curvature, sustain point hold, loop start/end) and 3 per-voice LFOs in `src/dsp/envelope.c` and `src/dsp/lfo.c` with unit tests in `tests/unit/c/test_dsp_mod.c`; verify sustain/loop accuracy and >=95% coverage.
- [x] 3.2 Implement 3-channel oscillator engine in `src/dsp/oscillator.c` supporting Wavetable (single/double, 2048-sample tables), FM, Ring Modulation, Sample playback, and Granular synthesis (windowed overlapping grains) with unit tests verifying finite non-silent output, frequency accuracy, and boundary safety at >=95% coverage.
- [x] 3.3 Implement per-channel and master resonant multimode State-Variable Filter (LP/HP/BP/Notch), Waveshaper, and Aetherizer-style granular/multitap feedback delay in `src/dsp/filter.c`, `src/dsp/waveshaper.c`, and `src/dsp/aetherizer.c` with spectral/tail unit tests achieving >=95% coverage.
- [x] 3.4 Implement 16-voice polyphonic voice allocator (with deterministic voice stealing), 4-corner XY patch morphing, 8 macro parameters, lock-free SPSC control queue, and double-buffered audio telemetry in `src/dsp/synth.c` with deterministic offline render unit tests achieving >=95% coverage across `src/dsp/`.

## 4. Retro 3D OpenGL / WebGL2 Renderer & Spatial UI

- [x] 4.1 Implement pure-C scene graph, camera/projection math, neighbor-culled voxel mesher (`src/render/voxel_mesher.c`), audio-reactive 3D ribbon builder, and 2D HUD command batcher/hit-tester with unit tests in `tests/unit/c/test_render_core.c`; verify >=95% coverage on non-GL renderer modules.
- [x] 4.2 Implement thin mockable OpenGL 3.3 Core / GLES 3.0 (WebGL2) backend (`GFGlApi` dispatch table) in `src/render/gl_renderer.c` supporting low-poly retro meshes (vertex lighting, vertex snap, color quantization), camera-facing animated sprites, toggleable voxel geometry, HUD overlay pass, and WebXR offscreen spatial UI panel quad + stereo eye rendering; verify unit tests with mock GL table (including shader error paths) reach >=95% coverage on `src/render/`.

## 5. Embedded Lua 5.4 Host & External Scripting Layer

- [x] 5.1 Implement sandboxed Lua 5.4 VM host, `gf.synth`, `gf.scene`, `gf.ui`, `gf.audio`, and `gf.assets` C bindings, protected error isolation, and hot-reload state preservation in `src/lua/lua_host.c` with C unit tests in `tests/unit/c/test_lua_host.c`; verify syntax/runtime errors never crash C and coverage is >=95% on `src/lua/`.
- [x] 5.2 Implement external Lua scripts under `assets/scripts/` (`main.lua`, `ui/layout.lua`, `ui/widgets.lua`, `ui/mobile_touch.lua`, `ui/xr_panel.lua`, `scene/stage.lua`, `dsp/generators.lua`, `dsp/arp.lua`) and initial Absynth-inspired patches in `assets/patches/` (`emerald_aether.lua`, `glass_spire.lua`, `wormwood_grains.lua`, `crystalline_fm.lua`); verify scripts load cleanly through the C host.
- [x] 5.3 Implement Lua unit test suite and line-coverage harness (`tests/unit/lua/run_lua_tests.lua` executed via `gf_lua_test_runner`) exercising all scripts and patches; verify all Lua tests pass with >=95% line coverage across `assets/scripts/` and `assets/patches/`.

## 6. Cross-Platform SDL3 Runtime, Mobile Touch & WebXR

- [x] 6.1 Implement unified input router (QWERTY musical keyboard, mouse, multi-touch fingers, MIDI messages, XR controller rays) and SDL3 application/audio lifecycle in `src/platform/` (`input.c`, `runtime.c`, `audio_sdl.c`) plus `src/main.c` (`--assets`, `--headless`, `--e2e-script`); verify unit tests achieve >=95% coverage on `src/platform/`.
- [x] 6.2 Implement Emscripten Web build target and external JS/HTML bridge (`web/shell.html`, `web/js/bridge.js`, `web/js/webxr.js`) that hydrates sibling `assets/` over HTTP fetch (zero embedded assets), unlocks Web Audio on gesture, auto-switches to mobile touch controls on touch/mobile viewports, and runs stereoscopic WebXR `immersive-vr` sessions with controller ray interaction; verify `emcmake cmake` compiles `green_fairy.wasm` + `green_fairy.js` cleanly.

## 7. Full-System Coverage Gates & Automated E2E Verification

- [x] 7.1 Run aggregated C coverage gate (`gcovr --root . --filter '^src/' --fail-under-line 95`) and Lua coverage gate (`gf_lua_test_runner --fail-under 95`) and verify both report >=95.0% line coverage with zero skipped or failing tests.
- [x] 7.2 Implement and run automated native headless E2E suite (`tests/e2e/native_e2e.py`) under `xvfb-run` + Mesa software GL verifying external asset loading, Lua execution, retro 3D frame rendering (non-blank + voxel toggle visual diff), audio output WAV verification (non-silent, finite, spectral response), and hot-reload fault recovery; verify all native E2E assertions pass.
- [x] 7.3 Implement and run automated Playwright browser E2E suite (`tests/e2e/web/e2e.spec.mjs`) serving the Emscripten build + sibling `assets/` over HTTP, verifying (a) desktop WebGL2 rendering & keyboard playback, (b) mobile viewport auto-switch to touch mode + multi-touch polyphony + XY pad drag, and (c) WebXR `immersive-vr` session with emulated stereo views and controller ray triggers; verify all Web E2E tests pass with zero console errors.

## Workflow follow-up

- Archive the change (`openspec archive green-fairy-synth`) after implementation and verification pass review.
- Verify archived specs in `openspec/specs/` with `openspec validate --strict`.
