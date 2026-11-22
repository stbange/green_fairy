# Spec Delta

## Purpose

Defines the automated quality gates requiring at least 95% unit test coverage across both the C core engine and Lua scripts, plus automated end-to-end verification of native and Web/mobile/WebXR targets.

## ADDED Requirements

### Requirement: C core unit test suite with >=95% coverage
The project SHALL maintain an automated unit test suite exercising all C core modules (DSP oscillators, envelopes, LFOs, filters, waveshapers, effects, voice manager, XY morpher, asset loaders, Lua bindings, scene graph, and math/renderer logic) and SHALL enforce at least 95% line coverage via coverage tooling.

#### Scenario: Running C coverage gate
- **GIVEN** the project compiled with coverage instrumentation enabled
- **WHEN** the automated unit test suite executes and coverage is aggregated
- **THEN** all C unit tests pass and reported line coverage across project C source files is at least 95.0%

### Requirement: Lua script unit test suite with >=95% coverage
The project SHALL maintain an automated unit test suite exercising all external Lua scripts (UI layout and widget logic, 3D scene choreography, patch definitions, macro/arpeggiator logic, and custom wavetable/envelope generators) and SHALL enforce at least 95% line coverage.

#### Scenario: Running Lua coverage gate
- **GIVEN** the Lua scripts under the external `assets/scripts/` and `assets/patches/` directories
- **WHEN** the automated Lua test runner executes with line coverage tracking
- **THEN** all Lua tests pass and reported line coverage across project Lua files is at least 95.0%

### Requirement: Automated native headless end-to-end tests
The project SHALL provide automated native E2E tests that launch the compiled Green Fairy executable against the external `assets/` directory in headless/offscreen mode to verify startup, Lua initialization, audio waveform rendering, patch switching, asset hot-reload, and retro 3D OpenGL frame output.

#### Scenario: Native headless audio and visual E2E verification
- **GIVEN** a built native Green Fairy binary and its sibling external `assets/` directory
- **WHEN** the automated native E2E harness drives scripted note-on, XY morph, voxel toggle, and frame-capture steps
- **THEN** the binary renders non-blank retro 3D frames, writes verifiable non-silent audio output, and exits cleanly with zero errors

### Requirement: Automated Web, mobile touch, and WebXR end-to-end tests
The project SHALL provide automated browser E2E tests driving the Emscripten Web build served alongside external `assets/` to verify desktop WebGL2 rendering, mobile viewport touch-control switching and multi-touch interaction, audio state activation, and WebXR immersive session rendering with an emulated XR device.

#### Scenario: Mobile touch mode E2E verification in headless browser
- **GIVEN** the Emscripten Web build served over HTTP and opened in a headless browser configured with a mobile touch viewport
- **WHEN** the E2E suite loads the page, verifies touch layout activation, and dispatches multi-touch note and XY pad gestures
- **THEN** the app runs with zero console/page errors, reports touch UI mode active, updates synth voices/parameters, and renders updated WebGL2 frames

#### Scenario: WebXR session E2E verification with XR polyfill/emulator
- **GIVEN** the Emscripten Web build opened in a headless browser injected with a WebXR `immersive-vr` test emulator
- **WHEN** the E2E suite enters WebXR mode and simulates headset pose updates and controller trigger input
- **THEN** the WebXR session starts cleanly, executes stereo render frames, triggers spatial synth interactions, and exits cleanly without errors

### Requirement: Cross-platform build verification
The project SHALL define repeatable build configurations and CI workflows targeting Linux, Windows, macOS, and Emscripten Web/WebXR so regressions in platform compilation or external asset packaging are caught automatically.

#### Scenario: External asset packaging check
- **GIVEN** any built target output directory (native or Emscripten web bundle)
- **WHEN** the build verification script inspects the output artifacts
- **THEN** the external `assets/` folder tree exists alongside the binary or web bundle and all required asset paths resolve cleanly
