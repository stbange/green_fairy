# Spec Delta

## Purpose

Defines the interconnected 7-room 3D synthesizer architecture, raycast-interactive 3D controls, physical catenary patch cables, and dual-mode free-walk and rack-focus navigation.

## ADDED Requirements

### Requirement: Interconnected seven-room 3D synth architecture
The synthesizer SHALL present seven themed, spatially connected 3D rooms—Oscillator Conservatory, Filter & Shaper Foundry, 68-Stage Envelope Observatory, Clockwork Modulation & Patchbay Room, Aetherium & Acoustics Chamber, Performance Bridge & Mutator Lab, and Astral Preset Archive—accessible through open archways and a quick-teleport room map.

#### Scenario: Walking between adjacent 3D rooms
- **GIVEN** the user camera positioned inside the Performance Bridge room
- **WHEN** the user navigates forward through the archway into the Oscillator Conservatory
- **THEN** the camera transitions smoothly into the Oscillator Conservatory with room-bounded collision and visible oscillator racks

### Requirement: Complete 3D hardware control coverage
Every user-controllable synthesis, envelope, sequencer, effect, mutator, and performance parameter SHALL be represented by an interactive 3D control (rotary knob, linear fader, toggle switch, stepped dial, 3D envelope node, 3D harmonic bar, or 3D XYZ morph orb) selectable via 3D raycast.

#### Scenario: Turning a 3D rotary knob in world space
- **GIVEN** the user raycasts onto the 3D Filter Cutoff knob in the Filter Foundry and drags vertically
- **WHEN** pointer movement is processed
- **THEN** the 3D knob mesh rotates proportionally, a diegetic value readout updates, and the underlying C DSP parameter changes in real time

### Requirement: Interactive 3D 68-stage envelope and wavetable sculptures
The Envelope Observatory and Oscillator Conservatory SHALL provide interactive 3D curve and harmonic sculptures where users drag 3D breakpoint nodes, curvature handles, loop/sustain flags, and wavetable harmonic pillars directly in 3D space.

#### Scenario: Dragging a 3D envelope breakpoint node
- **GIVEN** an active envelope displayed on the 3D rail in the Envelope Observatory
- **WHEN** the user grabs breakpoint 3 in 3D space and drags its time and level coordinates
- **THEN** the 3D envelope geometry updates immediately and the updated 68-stage envelope definition is uploaded to the C synthesis engine

### Requirement: Physical 3D catenary patch cables and cross-room conduits
All active modular connections SHALL render as sagging 3D catenary tube cables between 3D input and output jacks (or overhead inter-room cable conduits and trunk patchbays for cross-room patches), supporting click-and-drag plugging, unplugging, jack stacking, and attenuverter ring adjustment.

#### Scenario: Re-patching a 3D cable between jacks
- **GIVEN** a 3D cable plugged into `flt1.cutoff_cv`
- **WHEN** the user grabs the cable plug in 3D space, drags it to `osc2.fm_cv`, and releases
- **THEN** the catenary cable snaps to the new jack3D position and the C modular patch graph rewires the connection to `osc2.fm_cv`

### Requirement: Dual-mode free-walk and click-to-focus rack navigation
The 3D sanctum SHALL support free-roam movement (WASD/mouse-look, mobile touch dual-zone walk/look, WebXR teleport) AND one-click rack focus that smoothly dollies and frames any clicked 3D rack panel for precision parameter and cable manipulation, returning to free-roam on Escape or Back.

#### Scenario: Focusing onto a 3D synth rack for precision editing
- **GIVEN** the user in free-walk mode viewing the Aetherizer rack from across the room
- **WHEN** the user clicks the rack header or triggers rack-focus
- **THEN** the camera smoothly interpolates to an ergonomic framing of that rack, unlocks the cursor for precise 3D knob/cable dragging, and restores free-walk when exited

### Requirement: Preset-inspired music-reactive 3D surroundings
For every loaded preset, the synthesizer SHALL dynamically compose external retro-3D surroundings (non-living structures, living plants, living animals, sky gradient, and distance fog) inspired by the preset's name and realm, and animate them in response to keystrokes, volume/spectrum, and pitch.

#### Scenario: Switching presets transforms the 3D surroundings
- **GIVEN** the synthesizer running with *Emerald Sanctum Choir* displaying giant glowing emerald crystal caverns and soaring choir wisps
- **WHEN** the user loads *Starfall Sanctuary* and plays notes across low and high registers
- **THEN** the surroundings switch to deep space with flying star streaks and ringed planets whose warp speed, glow, and elevation react to keystrokes, pitch, and volume

### Requirement: Expanded playable chromatic piano keyboards
The synthesizer SHALL provide large authentic chromatic keyboards (white keys plus raised black keys) as a 61-key (5-octave, `C2`–`C7`) 3D console in Room 6 and as a collapsible, octave-shiftable on-screen 2D piano on Desktop (49-key) and Mobile Touch (25-key).

#### Scenario: Playing white and raised black keys on the expanded chromatic piano
- **GIVEN** the synthesizer running in Desktop or Mobile Touch mode
- **WHEN** the user clicks or touches white keys and raised black keys across octaves outside the computer QWERTY keymap
- **THEN** the synthesizer triggers the corresponding chromatic pitches with visual key illumination and octave-shift navigation


