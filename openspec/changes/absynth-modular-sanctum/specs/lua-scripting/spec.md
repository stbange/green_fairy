# Spec Delta

## MODIFIED Requirements

### Requirement: Lua-driven UI layout and input dispatch
Lua scripts SHALL declaratively define all seven 3D rooms, rack modules, 3D hardware widgets, 3D envelope/wavetable editors, patchbay jacks, and minimal compass/room-teleport indicators, receiving 3D raycast hit events, drag deltas, cable plug/unplug events, keyboard/MIDI notes, and XR controller actions from C.

#### Scenario: Adjusting a synth parameter via a Lua widget
- **GIVEN** a Lua room script exposing a 3D filter cutoff knob bound to engine parameter `ch1.filter.cutoff`
- **WHEN** the user raycast-drags the 3D knob widget via mouse, touch, or XR ray input
- **THEN** the Lua callback updates both the 3D knob rotation and the C DSP parameter on the next audio block

## ADDED Requirements

### Requirement: Lua modular cable and Mutator bindings
The C-to-Lua API SHALL expose `gf.patchbay` functions to query, connect, disconnect, stack, and attenuate modular patch cables, plus `gf.mutator` and `gf.presets` functions to browse the 320+ preset catalog, load patches on demand, and breed/revert mutated patch states.

#### Scenario: Rewiring modular cables from Lua interaction script
- **GIVEN** a user dragging a 3D cable from `lfo3.out` to `pipe.tension_cv`
- **WHEN** the Lua interaction handler invokes `gf.patchbay.connect("lfo3.out", "pipe.tension_cv", 0.65)`
- **THEN** the C patch graph adds the connection, assigns a cable ID, and exposes its live signal telemetry to the 3D cable renderer
