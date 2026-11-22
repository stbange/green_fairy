# cross-platform-runtime Specification

## Purpose

Defines the SDL3-based cross-platform runtime supporting Windows, Linux, macOS, Web (Emscripten/WebAssembly), and WebXR, including real-time audio streaming, unified input, and automatic mobile touch controls.

## Requirements

### Requirement: Multi-platform SDL3 application lifecycle
The runtime SHALL build and execute via SDL3 across Windows, Linux, macOS, and Web (Emscripten/WebAssembly), using a unified frame-step main loop compatible with both native event loops and browser `requestAnimationFrame` / XR frame callbacks.

#### Scenario: Desktop window creation and clean shutdown
- **GIVEN** a desktop platform (Linux, Windows, or macOS) with an OpenGL 3.3 Core context
- **WHEN** the application starts, executes rendering and audio frames, and receives a quit signal
- **THEN** SDL3 creates the window and GL context cleanly, runs the loop without leaks, and exits with status 0

#### Scenario: WebAssembly Emscripten boot and asset fetch
- **GIVEN** the Emscripten build served alongside the external `assets/` directory over HTTP
- **WHEN** a browser loads the web entry page and completes asset hydration
- **THEN** the WASM runtime initializes WebGL2, SDL3 audio (unlocking on first user gesture), and Lua scripts without console errors

### Requirement: Real-time SDL3 audio output stream
The runtime SHALL stream floating-point stereo audio from the C DSP synthesis engine to the platform audio device via SDL3 audio streams with configurable sample rate and low-latency buffer sizing.

#### Scenario: Audio playback unlocked by user interaction on Web
- **GIVEN** the Web build loaded in a browser with autoplay audio restrictions
- **WHEN** the user taps a key, XY pad, or start audio control
- **THEN** the Web Audio / SDL3 stream resumes cleanly and outputs synthesized stereo audio without glitches

### Requirement: Automatic mobile touch control switching
On the Web target, the runtime SHALL detect touch-primary or mobile viewports (via pointer capability, touch support, and viewport heuristics) and automatically switch the Lua UI into touch-optimized controls featuring multi-touch piano keys, large rotary/slider targets, and a touch XY morph pad.

#### Scenario: Switching to touch layout on mobile device
- **GIVEN** the Web build opened in a mobile viewport with coarse touch pointer support
- **WHEN** the application initializes or the viewport transitions to mobile dimensions
- **THEN** the runtime notifies the Lua UI layer of `touch` input mode and activates multi-touch keyboard, touch XY pad, and large touch-friendly controls

#### Scenario: Multi-touch polyphonic playback
- **GIVEN** the mobile touch layout active on a touch-capable Web client
- **WHEN** two or more fingers press distinct virtual keyboard keys simultaneously
- **THEN** the runtime dispatches independent finger press/move/release events so multiple polyphonic voices sound simultaneously

### Requirement: WebXR immersive session support
On the Web target, the runtime SHALL detect `navigator.xr` availability, expose an Enter/Exit WebXR trigger when `immersive-vr` is supported, and bind WebXR stereo frame pose, view/projection matrices, and controller ray inputs to the renderer and Lua interaction layer.

#### Scenario: Entering and interacting in WebXR session
- **GIVEN** a WebXR-capable browser (or automated WebXR emulator) supporting `immersive-vr`
- **WHEN** the user activates the WebXR mode button
- **THEN** the runtime starts the XR session, switches rendering to stereo eye passes driven by XR poses, and maps XR controller trigger/ray inputs to spatial UI and synth note triggers

#### Scenario: Graceful fallback when WebXR is unavailable
- **GIVEN** a standard desktop or mobile browser without WebXR hardware or API support
- **WHEN** the Web application initializes
- **THEN** the WebXR entry button is disabled or hidden with clear status, and standard desktop or mobile touch mode runs normally

### Requirement: Unified keyboard, pointer, and MIDI input
The runtime SHALL normalize QWERTY musical keyboard input, mouse/touch pointer events, optional MIDI note/CC messages (Web MIDI on Web and platform/virtual MIDI interface), and XR controller actions into a single event queue consumed by the Lua UI and C DSP engine.

#### Scenario: Playing notes from computer keyboard
- **GIVEN** a focused Green Fairy window on desktop or Web
- **WHEN** the user presses and releases mapped musical keys (e.g., Z/S/X/D/C)
- **THEN** corresponding MIDI note-on and note-off events are dispatched to the synthesis engine and reflected in the UI keyboard state
