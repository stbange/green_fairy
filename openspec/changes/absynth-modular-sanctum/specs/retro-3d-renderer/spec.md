# Spec Delta

## MODIFIED Requirements

### Requirement: Hybrid Lua-driven 2D HUD and spatial WebXR panels
The renderer SHALL prioritize diegetic 3D room panels, 3D rotary/fader/switch meshes, and 3D patchbay jacks across all viewports while compositing only a minimal room-compass/minimap HUD over the 3D scene on desktop and mobile, and rendering full stereoscopic 3D room interaction in WebXR.

#### Scenario: Rendering HUD controls over the 3D scene
- **GIVEN** a Lua room script declaring 3D synth racks in world space and a compact compass/room-map indicator
- **WHEN** a frame is rendered in desktop or mobile viewport mode
- **THEN** the 3D room racks, knobs, holographic curves, and patch cables render first and the minimal compass/minimap HUD composites cleanly on top

#### Scenario: Stereoscopic WebXR frame rendering
- **GIVEN** an active WebXR immersive session inside a 3D synth room providing left-eye and right-eye view and projection matrices
- **WHEN** an XR frame callback executes
- **THEN** the renderer draws both eye viewports with stereo room geometry, 3D controls, catenary cables, and XR controller laser rays

## ADDED Requirements

### Requirement: Procedural 3D catenary cable rendering with live signal pulses
The renderer SHALL generate and draw smooth 3D catenary tube geometry for all active modular patch cables and overhead cross-room conduits, shading each cable with animated directional luminance pulses driven by real-time cable voltage telemetry.

#### Scenario: Rendering an active signal-carrying patch cable
- **GIVEN** a connected cable between two 3D jacks with non-zero telemetry amplitude
- **WHEN** a frame is rendered
- **THEN** a sagging 3D tube mesh connects the two jack world positions and animates glowing signal pulses proportional to signal strength

### Requirement: 3D ray-picking and spatial holographic curve displays
The renderer SHALL provide screen-ray and XR-ray intersection against 3D oriented bounding boxes for knobs, faders, switches, envelope breakpoints, harmonic bars, and patch jacks, and SHALL render dynamic 3D wireframe/ribbon displays for 68-stage envelopes, wavetables, and filter responses.

#### Scenario: Ray-picking a 3D knob vs adjacent patch jack
- **GIVEN** a 3D module panel containing closely spaced knobs and patch jacks
- **WHEN** a pointer ray is cast through screen coordinates over a specific jack
- **THEN** the ray-picker returns the closest intersected jack ID accurately respecting depth occlusion

### Requirement: Multi-room portal and frustum visibility culling
The renderer SHALL partition scene geometry by room and skip draw calls for rooms outside the active camera frustum and non-visible rooms, maintaining smooth frame rates across all seven rooms on desktop, mobile Web, and WebXR.

#### Scenario: Culling distant synth rooms
- **GIVEN** the camera focused on a rack inside the Oscillator Conservatory facing away from distant rooms
- **WHEN** a frame is rendered
- **THEN** entities belonging to non-visible distant rooms are culled from draw submission
