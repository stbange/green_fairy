# Proposal

## Why

Green Fairy brings the deep, eerie, semi-modular sound-design ethos of Native Instruments Absynth into a lightweight, cross-platform, hackable software synthesizer paired with a tactile retro 3D visual world. By splitting a high-performance C/SDL3/OpenGL core from hot-reloadable external Lua scripts and folder-based assets, musicians and modders can sculpt evolving soundscapes, customize the interface and 3D stage, and perform across desktop, mobile Web, and WebXR without recompiling the binary.

## What Changes

- Introduce the **Green Fairy** C11 core engine providing real-time Absynth-inspired polyphonic DSP synthesis (3 oscillator channels with wavetable, FM, ring modulation, and sample/granular modes; multi-stage breakpoint envelopes up to 68 points with sustain and loop segments; 3 LFOs; per-channel and master filters, waveshapers, and Aetherizer-style granular/multitap delay; XY morphing).
- Introduce a **retro 3D OpenGL / WebGL2 renderer** combining low-poly 3D meshes, billboarded sprites, optional voxel grids, and retro post-processing shaders, integrated with a hybrid Lua-driven 2D HUD and spatial WebXR panels.
- Introduce an **embedded Lua 5.4 scripting host** exposing safe bindings so UI screens/widgets, 3D scene choreography, patches/presets, macros/arpeggiators, and custom wavetable/envelope generators live in user-editable, hot-reloadable `.lua` scripts outside the binary.
- Introduce an **external asset architecture** where all graphics (3D models, sprites, voxels, shaders, fonts), audio samples, patches, and scripts reside in dedicated directories beside the binary (or served alongside the WebAssembly bundle) and are never compiled into the executable.
- Introduce **cross-platform SDL3 build and runtime support** for Windows, Linux, macOS, Web (Emscripten/WASM), and WebXR, with automatic detection and switching to touch-optimized controls on mobile Web devices.
- Introduce a **comprehensive verification suite** enforcing >=95% unit test coverage across both C core modules and Lua scripts, alongside automated E2E tests covering native headless DSP/OpenGL execution and browser-based Web, mobile-touch, and WebXR flows.

### Scope & Non-Goals

- **In Scope (v1)**: Standalone desktop app (Windows, Linux, macOS), Web/WASM app with mobile touch UI, WebXR immersive stereoscopic mode, full 3-channel Absynth-inspired voice architecture, retro 3D objects + sprites + optional voxels, external hot-reloadable Lua scripting and asset folders, >=95% unit coverage, and automated native + web E2E tests.
- **Out of Scope (v1)**: DAW plugin formats (VST3/AU/CLAP/LV2), importing proprietary Native Instruments `.nks`/`.ksd` binary patch files, multi-user networked jamming, and hardware-accelerated raytracing.

## Capabilities

### New Capabilities
- `dsp-synthesis`: Polyphonic Absynth-inspired semi-modular audio synthesis engine (3 oscillator channels, multi-stage breakpoint envelopes, LFOs, filters, waveshaper, granular/multitap effects, and XY patch morphing).
- `retro-3d-renderer`: Retro 3D OpenGL/WebGL2 rendering pipeline supporting 3D meshes, billboarded sprites, optional voxel meshes, retro shaders, audio-reactive visualizers, and WebXR stereo projection.
- `lua-scripting`: Embedded Lua 5.4 scripting VM and C/Lua API powering hot-reloadable UI layouts, 3D scene choreography, patches/presets, macros/arpeggiators, and custom wavetable/envelope generators.
- `external-assets`: External directory-based asset discovery, loading, caching, and hot-reloading for models, sprites, voxels, shaders, fonts, audio samples, patches, and scripts sitting beside the binary.
- `cross-platform-runtime`: SDL3-based platform abstraction targeting Windows, Linux, macOS, Web (Emscripten), and WebXR, including real-time audio output, input handling (keyboard, mouse, MIDI, XR controllers), and automatic mobile touch-control switching.
- `quality-verification`: Automated unit testing with >=95% code coverage across C and Lua codebases, plus automated headless native and browser-based E2E test suites.

### Modified Capabilities
<!-- None — greenfield repository with no prior capabilities. -->

## Impact

- **Codebase**: Greenfield CMake + C11 project (`src/core/`, `src/dsp/`, `src/render/`, `src/lua/`, `src/platform/`, `web/`), external `assets/` directory tree (`scripts/`, `patches/`, `models/`, `sprites/`, `voxels/`, `shaders/`, `sounds/`, `fonts/`), and `tests/` (`unit/`, `e2e/`).
- **Dependencies**: SDL3, OpenGL 3.3 Core / GLES 3.0 (WebGL2), Lua 5.4, Emscripten SDK (for Web/WebXR builds), Unity/Check or custom zero-dependency C test harness + `gcov`/`gcovr` (or `llvm-cov`) + LuaCov, and Playwright/Chromium for Web/mobile/WebXR E2E tests.
- **CI & Builds**: Cross-platform build configurations and presets for Linux, Windows, macOS, and Emscripten Web/WebXR, plus coverage and E2E gates.
