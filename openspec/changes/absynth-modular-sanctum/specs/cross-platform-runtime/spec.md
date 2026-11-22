# Spec Delta

## MODIFIED Requirements

### Requirement: Automatic mobile touch control switching
On the Web target, the runtime SHALL detect touch-primary or mobile viewports and automatically enable ergonomic mobile 3D touch controls: dual-zone virtual thumb-walk and look-orbit in free-roam mode, single-tap rack focus zoom, large-hit-radius 3D knob/cable dragging, and a toggleable bottom multi-touch performance keyboard + drone latch strip.

#### Scenario: Switching to touch layout on mobile device
- **GIVEN** the Web build opened in a mobile viewport with coarse touch pointer support
- **WHEN** the application initializes or the viewport transitions to mobile dimensions
- **THEN** the runtime activates `touch` input mode with dual-zone 3D walk/look navigation, tap-to-focus rack zoom, enlarged 3D knob/jack hit targets, and the multi-touch keyboard strip

#### Scenario: Multi-touch polyphonic playback
- **GIVEN** the mobile touch performance strip or 3D keyboard active on a touch-capable Web client
- **WHEN** two or more fingers press distinct virtual keyboard keys simultaneously
- **THEN** the runtime dispatches independent finger press/move/release events so multiple polyphonic voices sound simultaneously

### Requirement: WebXR immersive session support
On the Web target, the runtime SHALL detect `navigator.xr` availability, expose an Enter/Exit WebXR trigger when `immersive-vr` is supported, and bind WebXR stereo frame poses and controller laser rays to room-scale teleportation, 3D knob twisting, and physical 3D patch cable grabbing and re-plugging across all seven rooms.

#### Scenario: Entering and interacting in WebXR session
- **GIVEN** a WebXR-capable browser (or automated WebXR emulator) supporting `immersive-vr`
- **WHEN** the user activates WebXR mode, teleports to a room, and uses controller rays to twist a 3D knob and re-patch a 3D cable
- **THEN** the runtime renders stereo eye views of the 3D room and updates both the C modular patch graph and synth parameters

#### Scenario: Graceful fallback when WebXR is unavailable
- **GIVEN** a standard desktop or mobile browser without WebXR hardware or API support
- **WHEN** the Web application initializes
- **THEN** the WebXR entry button is disabled or hidden with clear status, and standard desktop or mobile touch 3D room navigation runs normally
