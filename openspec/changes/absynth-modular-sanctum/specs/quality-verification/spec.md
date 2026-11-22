# Spec Delta

## MODIFIED Requirements

### Requirement: Automated native headless end-to-end tests
The project SHALL provide automated native headless E2E tests verifying startup across all seven 3D rooms, camera room-to-room traversal and rack-focus zooming, 3D raycast knob manipulation, live modular cable unplugging and re-patching, Mutator execution, and non-blank frame + stereo WAV output.

#### Scenario: Native headless audio and visual E2E verification
- **GIVEN** a built native Green Fairy binary and its sibling external `assets/` directory
- **WHEN** the automated native E2E harness traverses all seven 3D rooms, focuses racks, drags 3D controls, re-patches modular cables, triggers Mutator, and captures frames and WAV output
- **THEN** each room renders distinct 3D geometry, cable changes measurably alter audio output, and the binary exits cleanly with zero errors

### Requirement: Automated Web, mobile touch, and WebXR end-to-end tests
The project SHALL provide automated browser E2E tests verifying fast catalog boot, on-demand HTTP loading of presets across the 320+ preset vault, 3D room navigation and rack-focus raycast editing on desktop, touch 3D navigation and multi-touch playback on mobile viewports, and stereo WebXR room teleportation + 3D cable interaction.

#### Scenario: Mobile touch mode E2E verification in headless browser
- **GIVEN** the Emscripten Web build served over HTTP and opened in a headless browser configured with a mobile touch viewport
- **WHEN** the E2E suite verifies touch 3D navigation mode, taps a 3D rack to focus, drags a 3D knob/cable, loads presets on demand, and dispatches multi-touch notes
- **THEN** the app runs with zero console/page errors, updates 3D room views and modular cables, and renders WebGL2 frames cleanly

#### Scenario: WebXR session E2E verification with XR polyfill/emulator
- **GIVEN** the Emscripten Web build opened in a headless browser injected with a WebXR `immersive-vr` test emulator
- **WHEN** the E2E suite enters WebXR mode and simulates room teleportation, stereo headset pose updates, and XR controller ray knob/cable interaction
- **THEN** the WebXR session executes stereo 3D room render frames, updates modular synth state, and exits cleanly without errors

## ADDED Requirements

### Requirement: Automated 320+ preset integrity and diversity verification
The test suite SHALL validate the entire 320+ external preset catalog to ensure every preset parses cleanly, creates valid modular cable connections, renders finite non-silent stereo audio without clipping, and demonstrates timbral diversity across the eight realms.

#### Scenario: Full catalog validation and spectral diversity gate
- **GIVEN** all external presets registered in `assets/patches/catalog.json`
- **WHEN** the automated preset validator loads every preset and renders test notes across realms
- **THEN** 100% of presets load without errors, produce finite non-clipping audio, and pass spectral/temporal diversity assertions
