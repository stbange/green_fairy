# lua-scripting Specification

## Purpose

Defines the embedded Lua 5.4 scripting host and C-to-Lua API that allows UI screens, 3D scene choreography, patches, macros, arpeggiators, and custom wavetable/envelope generators to live in editable external Lua scripts.

## Requirements

### Requirement: Sandboxed Lua host initialization
The core engine SHALL initialize an embedded Lua 5.4 runtime with standard safe libraries and register Green Fairy C API modules for synthesis control, 3D scene management, UI layout, asset queries, and input events.

#### Scenario: Loading entrypoint scripts at startup
- **GIVEN** a valid external scripts directory containing the main Lua bootstrap script
- **WHEN** the engine starts and initializes the scripting subsystem
- **THEN** the Lua runtime loads the bootstrap script, registers active screens, patches, and scene hooks, and reports zero runtime errors

### Requirement: Lua-driven UI layout and input dispatch
Lua scripts SHALL define UI pages (Patch, Oscillators, Envelopes, Perform/XY, and Mobile Touch layout), widget geometry, visual styling, and parameter bindings, receiving normalized pointer, keyboard, touch, and XR ray events from C.

#### Scenario: Adjusting a synth parameter via a Lua widget
- **GIVEN** a Lua UI script exposing a filter cutoff knob bound to engine parameter `ch1.filter.cutoff`
- **WHEN** the user drags the knob widget via mouse or touch input
- **THEN** the Lua callback updates the parameter through the C API and the synthesis engine reflects the new cutoff value on the next audio block

### Requirement: Lua-driven 3D scene choreography
Lua scripts SHALL create, configure, animate, and destroy 3D mesh objects, billboarded sprites, optional voxel structures, lights, and camera rigs each frame using audio-reactive telemetry supplied by the C engine.

#### Scenario: Audio-reactive scene update script
- **GIVEN** a Lua scene script that scales and rotates a 3D crystal mesh proportionally to current RMS energy
- **WHEN** the engine invokes the Lua update hook with an audio telemetry table containing non-zero RMS
- **THEN** the script updates the mesh transform via the C scene API and the new transform is applied on the next rendered frame

### Requirement: Lua patch, macro, and arpeggiator scripting
Lua scripts SHALL define synthesizer patches, preset banks, XY morph corner snapshots, macro mappings, and musical event generators such as arpeggiators or step sequencers that emit note and parameter events to the C engine.

#### Scenario: Loading a Lua-defined patch
- **GIVEN** an external Lua patch file returning a valid patch table with oscillator modes, envelopes, LFOs, and XY corners
- **WHEN** the user or test harness selects that patch
- **THEN** the C engine applies all validated patch parameters atomically without interrupting audio stability

### Requirement: Lua custom wavetable and envelope generation
Lua scripts SHALL be able to compute custom wavetable sample arrays and multi-stage breakpoint envelope definitions and upload them to the C synthesis engine safely outside the real-time audio thread.

#### Scenario: Uploading a procedural Lua wavetable
- **GIVEN** a Lua generator script that synthesizes a 2048-sample harmonic wavetable
- **WHEN** the script calls the C API to register the wavetable on Oscillator Channel 1
- **THEN** the C core normalizes, validates, and swaps the wavetable atomically for subsequent audio rendering

### Requirement: Hot-reloading and fault isolation
The scripting host SHALL support reloading modified external Lua scripts at runtime without restarting the application, and SHALL catch Lua syntax or runtime errors without crashing the C engine or interrupting audio playback.

#### Scenario: Recovering from a broken user-edited Lua script
- **GIVEN** a running synthesizer with an active audio stream
- **WHEN** a user introduces a syntax or runtime error in an external Lua script and triggers reload
- **THEN** the C host logs a clear error message with file and line number, preserves the previous valid state or safe fallback, and continues audio and rendering without crashing
