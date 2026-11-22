# Design

## Context

Green Fairy is a greenfield software synthesizer built in C11 with SDL3, OpenGL (Desktop GL 3.3 Core and WebGL2 / GLES 3.0), and an embedded Lua 5.4 runtime. See [proposal.md](proposal.md) for product motivation and capability scope.

Key architectural constraints:
- All assets (3D models, sprites, voxels, shaders, fonts, audio samples, patches, Lua scripts) MUST remain external in sibling directories beside the executable or Web bundle, never embedded into compiled binaries.
- Real-time DSP MUST execute lock-free and allocation-free in C while UI layout, 3D scene choreography, patches, macros/arpeggiator, and procedural wavetable/envelope generators live in hot-reloadable Lua scripts.
- Desktop (Windows, Linux, macOS), Web (Emscripten/WASM with automatic mobile touch switching), and WebXR MUST share one C/Lua core.
- Both C and Lua unit test suites MUST achieve >=95% line coverage, backed by automated native and browser E2E tests.

## Goals / Non-Goals

**Goals:**
- Clean separation of pure, deterministically testable C subsystems (`dsp`, `assets`, `scene`, `lua`, `ui_bridge`) from thin platform/GL adapters (`platform`, `gl_backend`).
- True Absynth-style semi-modular synthesis quality (3 oscillator channels, 68-stage looping breakpoint envelopes, 3 LFOs, per-channel and master filters/waveshapers, Aetherizer-style granular/multitap delay, 16-voice polyphony, 4-corner XY morphing).
- Distinct retro 3D aesthetic (low-poly meshes, camera-facing animated sprites, optional face-culled voxel structures, vertex lighting, optional retro resolution/dithering pass) with hybrid Lua HUD and WebXR spatial UI.
- Reproducible cross-platform CMake build using pinned FetchContent dependencies (SDL3, Lua 5.4) and Emscripten toolchain support.

**Non-Goals:**
- Supporting legacy OpenGL 2.1 fixed-function pipelines or Direct3D/Metal/Vulkan native backends in v1.
- Running Lua bytecode directly on the real-time audio callback thread (Lua stays strictly on the control/main thread to guarantee audio glitch freedom).
- Baking assets via Emscripten `--embed-file` or `--preload-file` into a monolithic `.data`/`.wasm` blob that hides external files from direct editing.

## Decisions

### 1. Repository & External Asset Layout

```text
green_fairy/
├── CMakeLists.txt
├── CMakePresets.json
├── assets/                     # Copied/served beside binary; NEVER baked in
│   ├── manifest.json           # Asset index for Web HTTP hydration & validation
│   ├── scripts/                # Editable Lua layer
│   │   ├── main.lua
│   │   ├── ui/                 # layout.lua, widgets.lua, mobile_touch.lua, xr_panel.lua
│   │   ├── scene/              # stage.lua (3D objects, sprites, optional voxels)
│   │   └── dsp/                # generators.lua (wavetables/envelopes), arp.lua
│   ├── patches/                # External Lua patch definitions (*.lua)
│   ├── models/                 # Low-poly Wavefront .obj meshes
│   ├── sprites/                # Retro pixel-art sprites (.tga / .ppm / .png)
│   ├── voxels/                 # Optional voxel volume files (.gfvox)
│   ├── shaders/                # External GLSL shader sources (shared GL3/GLES3)
│   ├── sounds/                 # External PCM .wav audio samples for Sample/Granular
│   └── fonts/                  # External bitmap font atlas + metrics (.json/.tga)
├── src/
│   ├── core/                   # Ring buffers, math (vec3/mat4/quat), logging, path safety
│   ├── dsp/                    # Synth engine, oscillators, envelopes, LFOs, filters, FX, morph
│   ├── assets/                 # Asset root resolver, OBJ/TGA/PPM/WAV/VOX/GLSL loaders, watcher
│   ├── render/                 # Scene graph, voxel mesher, HUD batcher, GL backend
│   ├── lua/                    # Lua 5.4 host, C API bindings, error isolation, hot-reload
│   ├── platform/               # SDL3 app loop, audio stream, input router, Web/WebXR hooks
│   └── main.c                  # CLI entry (--assets, --headless, --e2e-script)
├── web/                        # Emscripten HTML shell, WebXR & mobile-touch JS bridge
└── tests/
    ├── unit/c/                 # C unit tests + gcovr >=95% coverage gate
    ├── unit/lua/               # Lua unit tests + line coverage >=95% gate
    └── e2e/                    # Native headless E2E + Playwright Web/Mobile/WebXR E2E
```

- **Rationale**: Keeping `assets/` as a sibling directory (and syncing it next to build outputs via CMake post-build copy / symlink and static HTTP serving on Web) enforces true zero-embedding. On Emscripten, `web/js/bridge.js` fetches `assets/manifest.json` and each listed external file via HTTP `fetch()` into Emscripten's in-memory filesystem (`/assets/...`) before calling `gf_web_start()`, so editing any file in `assets/` on the server immediately changes behavior on page reload with zero recompilation.
- **Alternative Considered**: Emscripten `--preload-file`. Rejected because it packs assets into a binary `.data` bundle at link time, conflicting with the requirement that external assets sit openly in folders beside the runtime and remain directly editable.

### 2. Absynth-Inspired DSP Signal Flow & Threading Model

```text
[Main / Lua Thread]  --(Lock-Free SPSC Command Queue)-->  [Audio Thread (C11, zero-alloc)]
                                                                │
   Per Voice (up to 16 voices):                                 ▼
     ┌─ Ch 1: Osc (WT/FM/RM/Sample/Granular) -> Ch1 SVF -> Ch1 Waveshaper ─┐
     ├─ Ch 2: Osc (WT/FM/RM/Sample/Granular) -> Ch2 SVF -> Ch2 Waveshaper ─┼─► Voice Sum
     └─ Ch 3: Osc (WT/FM/RM/Sample/Granular) -> Ch3 SVF -> Ch3 Waveshaper ─┘       │
       Modulated by: 68-Stage Breakpoint Envelopes + 3 LFOs + XY Morph + Macros    ▼
                                                                           [Master Bus]
                                                             Master SVF -> Waveshaper ->
                                                             Aetherizer Multitap/Granular Delay ->
                                                             Soft Limiter -> Stereo Float Output
                                                                │
[Renderer / Lua Thread] <--(Lock-Free Double-Buffered Telemetry)--┘
```

- **Rationale**: Separating control-rate Lua from sample-rate C via a fixed-capacity lock-free SPSC ring buffer eliminates audio dropouts and race conditions while permitting arbitrary hot-reload of Lua patches, envelopes, and 2048-sample custom wavetables (swapped via atomic generation/slot indices).
- **DSP Determinism**: All per-voice PRNGs (for granular jitter and noise) use a deterministic xorshift32 state initialized from a configurable seed, allowing bit-identical offline rendering in unit and E2E tests.
- **Alternative Considered**: Executing Lua hooks directly per audio block. Rejected due to GC pauses and latency jitter under real-time Web Audio / SDL3 audio constraints.

### 3. Retro 3D Renderer & OpenGL 3.3 / WebGL2 Unification

- **Shader Portability**: All external shaders in `assets/shaders/` are written in the common subset of GLSL 330 core and GLSL ES 300. At load time, the C shader preprocessor prepends `#version 330 core\n` on desktop OpenGL and `#version 300 es\nprecision highp float;\n` on Emscripten/WebGL2.
- **Retro 3D Primitives**:
  - **3D Meshes**: Wavefront `.obj` parser producing indexed vertex buffers (`pos`, `normal`, `uv`, `color`). Vertex shader supports configurable PS1-style grid snapping (`u_snap_res`) and per-vertex Gouraud-style lighting + optional color quantization (`u_color_steps`).
  - **3D Sprites**: Camera-aligned quads referencing external `.tga`/`.ppm` sprite sheets with frame index UV offsets and alpha cutout/blending.
  - **Optional Voxels**: External `.gfvox` (compact ASCII/binary grid of palette-indexed voxels) compiled on load by a pure-C neighbor face-culling mesher (`src/render/voxel_mesher.c`) into indexed triangles. Toggleable at runtime via `gf.scene.set_voxels_enabled(bool)`.
  - **Audio-Reactive Visualizers**: Dynamic 3D ribbon vertex buffers updated each frame from the audio telemetry waveform & spectrum bands.
- **Hybrid HUD & WebXR Spatial Panels**:
  - In `desktop` and `touch` modes, Lua emits declarative 2D draw commands (`rect`, `rounded_rect`, `line`, `text`, `knob_arc`, `xy_pad`, `key`) rendered in a single batched 2D overlay pass after the 3D scene.
  - In `xr` mode, the exact same Lua UI draw pass renders into an offscreen FBO texture displayed on an interactive curved/planar 3D spatial panel in front of the user, intersected by WebXR controller rays.
- **Alternative Considered**: Immediate-mode C GUI library (e.g., Dear ImGui/Nuklear). Rejected because Dear ImGui is C++, and keeping UI layout/widgets in Lua directly satisfies the user's requirement for an extendable Lua-scripted UI layer.

### 4. Embedded Lua 5.4 Host & C API Contract

- **Dependency**: Lua 5.4.7 built as a static C library via CMake `FetchContent` so Windows, Linux, macOS, and Emscripten compile identically with zero external system Lua requirements.
- **Sandbox**: Exposes `base`, `table`, `string`, `math`, `utf8` (omitting `os.execute`/`io.popen` for safety, while providing `gf.assets` for asset-root-confined reads).
- **C-to-Lua Modules**:
  - `gf.synth`: `note_on(note, vel)`, `note_off(note)`, `all_notes_off()`, `set_param(id, val)`, `get_param(id)`, `set_xy(x, y)`, `set_macro(idx, val)`, `apply_patch(tbl)`, `set_wavetable(ch, samples_tbl)`, `set_envelope(target, points_tbl)`.
  - `gf.scene`: `clear()`, `add_mesh(path, opts)`, `add_sprite(path, opts)`, `add_voxel(path, opts)`, `set_transform(id, pos, rot, scale)`, `set_camera(pos, target, fov)`, `set_voxels_enabled(bool)`, `set_retro_opts(snap, quantize, dither)`.
  - `gf.ui`: `rect(...)`, `line(...)`, `text(...)`, `circle(...)`, `viewport()`, `input_mode()`.
  - `gf.audio`: `telemetry()` returning `{rms_l, rms_r, active_voices, bands = {...}, scope = {...}}`.
  - `gf.assets`: `list(subdir)`, `read_text(rel_path)`, `reload()`.
- **Lifecycle Hooks in Lua**: `gf_init()`, `gf_update(dt, telemetry)`, `gf_render_ui(ui_ctx)`, `gf_on_input(event)`, `gf_on_mode_change(mode)`. All invoked via `lua_pcall` with a stack-trace error handler; errors are logged and never abort C execution.

### 5. Cross-Platform SDL3, Mobile Touch & WebXR Integration

- **SDL3**: Pinned SDL3 release fetched via CMake `FetchContent` (or found via `find_package(SDL3)` when preinstalled). Handles windowing, OpenGL/WebGL2 context, `SDL_AudioStream` audio callback/feed, keyboard, mouse, and multi-touch (`SDL_EVENT_FINGER_*`) events.
- **Mobile Touch Auto-Detection (Web)**: `web/js/bridge.js` evaluates `window.matchMedia('(pointer: coarse)').matches || navigator.maxTouchPoints > 0 || window.innerWidth <= 820` on load and on `resize`/`orientationchange`, calling `_gf_platform_set_input_mode(mode)` in WASM (`0 = desktop`, `1 = touch`, `2 = xr`). Also allows query param `?mode=touch` or `?mode=desktop` for deterministic E2E testing.
- **WebXR Integration**:
  - `web/js/webxr.js` queries `navigator.xr.isSessionSupported('immersive-vr')`, enables the WebXR button when supported, calls `gl.makeXRCompatible()` on the WebGL2 context, creates an `XRWebGLLayer`, and drives `session.requestAnimationFrame`.
  - Each XR frame passes left/right eye viewports + 4x4 view & projection matrices + controller ray origin/direction/trigger state into exported C functions (`gf_webxr_begin_frame`, `gf_webxr_render_eye`, `gf_webxr_controller_input`, `gf_webxr_end_frame`), pausing the standard 2D `requestAnimationFrame` loop while immersive XR is active and restoring it cleanly on session end.
- **Cross-Platform Presets**: `CMakePresets.json` provides `linux-debug`, `linux-coverage`, `linux-release`, `windows-release`, `macos-release`, and `web-emscripten`, accompanied by a GitHub Actions workflow testing Linux, Windows, macOS, and Emscripten builds.

### 6. Testing & >=95% Coverage Strategy

- **Coverage Isolation**:
  - All algorithmic, DSP, parsing, meshing, scene, Lua binding, input routing, and renderer state logic lives in pure, platform-agnostic C compilation units (`libgreen_fairy_core`).
  - Hardware-bound GL/SDL entry wrappers are kept minimal and exercised via both mockable GL/SDL test seams (unit tests) and real Mesa software GL (`xvfb-run` + `LIBGL_ALWAYS_SOFTWARE=1`) native E2E runs so that aggregate C line coverage across `src/` is cleanly >=95% without dead branches.
- **C Unit Coverage**: Built with GCC `--coverage` (`-O0 -g --coverage`) and enforced via `gcovr --root . --filter '^src/' --fail-under-line 95`.
- **Lua Unit Coverage**: Executed via a dedicated `gf_lua_test_runner` C binary linking the real Lua 5.4 host and C bindings, installing a `debug.sethook("l")` line-coverage collector across `assets/scripts/` and `assets/patches/` and failing if executable line coverage is below 95.0%.
- **Native Headless E2E**: `--headless --e2e-script <file>` flag runs the full application with SDL3 + Mesa GL (under `xvfb-run`) and offline/dummy audio driver, executes scripted actions (load patch, trigger chord, morph XY, toggle voxels on/off, trigger hot-reload with valid & broken Lua, capture PNG/PPM framebuffers & WAV audio output), and asserts non-blank pixels, visual diff on voxel toggle, and non-silent/valid audio energy.
- **Web / Mobile / WebXR E2E**: Playwright + Chromium serving the Emscripten build directory over local HTTP:
  1. Desktop WebGL2 test: verifies zero console errors, external `/assets/` HTTP fetches, canvas non-blank render, keyboard note trigger, and synth telemetry update.
  2. Mobile Touch test: Chromium mobile device emulation (`hasTouch: true`, `viewport: 390x844`) verifying automatic switch to `touch` mode, multi-touch simultaneous piano key presses (polyphony >= 2), and XY pad drag updates.
  3. WebXR test: Injects a spec-compliant lightweight `navigator.xr` mock (`immersive-vr` session + stereo `XRView`s + controller `XRInputSource`), clicks Enter WebXR, verifies stereo eye draw calls and controller ray interaction, then exits XR cleanly.

## Risks / Trade-offs

- **[Risk] Emscripten SDK availability in local/CI environments** → **Mitigation**: Provide a scripted Emscripten toolchain setup (pinned `emsdk` version) and document/cache it in CI; keep CMake cleanly split so native builds work with zero Emscripten dependency.
- **[Risk] SDL3 API surface stability and build time via FetchContent** → **Mitigation**: Pin a specific stable SDL3 release tag (e.g., `release-3.2.x`), disable unused SDL3 subsystems (camera, joystick/haptic if unneeded) to speed up builds, and support preinstalled `find_package(SDL3)` as a fast path.
- **[Risk] WebGL2 + WebXR context handoff with SDL3** → **Mitigation**: Bind WebXR exclusively through the JS WebXR shim using the existing Emscripten WebGL2 canvas (`Module.canvas`) with `gl.makeXRCompatible()` and pause `emscripten_set_main_loop` while `XRSession.requestAnimationFrame` drives `gf_webxr_*` exports.
- **[Risk] Reaching >=95% line coverage across OpenGL and platform code** → **Mitigation**: Keep platform/GL call sites ultra-thin behind a small C function-pointer dispatch table (`GFGlApi`) so both unit tests (mock GL table testing error paths like shader compile failures) and Mesa E2E tests achieve >=95% real coverage with no untested dead code.
- **[Risk] External asset loading latency on Web** → **Mitigation**: Keep retro assets compact (low-poly OBJ, pixel-art sprites, compact `.gfvox`, short WAV samples) and fetch them concurrently via `Promise.all` from `assets/manifest.json` with a minimal loading indicator in `web/shell.html`.
