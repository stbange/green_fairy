# Proposal

## Why

The initial v0.1 Green Fairy prototype demonstrated the cross-platform C/SDL3/OpenGL/Lua foundation, but its DSP depth, 4-patch library, and decorative 3D stage with flat 2D side-sliders fall far short of a professional Absynth-caliber instrument. Transforming Green Fairy into an interconnected 7-room 3D Modular Synth Sanctum—where every synthesis, envelope, modulation, and acoustic-effect parameter is physically controlled by 3D hardware and patchable catenary cables, backed by a full Absynth 5+ DSP engine and 320+ curated Fantasy & Space ambient presets—makes the 3D world essential to sound design and delivers a truly professional, boundless atmospheric synthesizer.

## What Changes

- **BREAKING** Replace the flat 2D HUD control paradigm with a **7-Room 3D Synth Sanctum** (`Oscillator Conservatory`, `Filter & Shaper Foundry`, `68-Stage Envelope Observatory`, `Clockwork Modulation & Patchbay Room`, `Aetherium & Acoustics Chamber`, `Performance Bridge & Mutator Lab`, and `Astral Preset Archive`), retaining only a minimal compass/minimap overlay while moving all synth controls into raycast-interactive 3D knobs, faders, switches, breakpoint sculptures, and patchbay jacks.
- Introduce **Dual-Mode 3D Navigation**: seamless free-roam walking through connected rooms (WASD + mouse-look on desktop, dual-zone touch walk/look on mobile, room-scale teleport in WebXR) combined with one-click smooth camera dolly-focus onto any 3D rack for precision editing.
- Introduce a **True Audio + CV Modular Patch Graph** where presets spawn physical 3D catenary patch cables (with animated real-time signal pulses and draggable attenuverter rings) that can be unplugged, stacked (many-to-many), or routed across rooms via overhead cable conduits and trunk patchbays to dynamically rewire the C DSP engine.
- Upgrade the C DSP engine to **full Absynth 5+ professional capability**:
  - **9 Oscillator Modes**: Anti-aliased Mip-Mapped Single & Double Wavetables with Harmonic Spectrum Morphing, 3-Op/Cross-Channel FM with Feedback, Ring Modulation, Hilbert Single-Sideband Frequency Shifter, Recursive Waveform Fractalizer, Sync/Cloud Stereo Granular, Crossfade-Loop Sample Playback, 8-Voice Stereo Super-Unison, and Cycle-Safe Feedback Bus.
  - **14 Filter Topologies & Wave-Transfer Shaper**: 12 dB & 24 dB ZDF Ladder LP/HP/BP/Notch, All-Pass Phaser, Positive/Negative Karplus-Strong Comb Resonators, A-E-I-O-U Formant Vowel Filter, Granular Cloud Filter, and arbitrary Wavetable Transfer-Curve Waveshaper.
  - **12 Assignable 68-Stage Envelopes**: Supporting Sustain, Loop, Tempo-Synced Rhythmic Step-Pulse, Embedded Per-Stage Envelope-LFOs, and Macro/XY Control-Scrubbing.
  - **Expanded Modulators & Performance**: 3 Wavetable LFOs with fade-in & sync, 16-Step Polyrhythmic Sequencer, Microtuning Scale Engine (Just, Pythagorean, Bohlen-Pierce, Quarter-Tone, Pelog, Harmonic), Polyphonic Glide, Drone Hold Latch, 3D XYZ Morph Orb, and the **Absynth Mutator** patch-breeding engine.
  - **Deep Stereo Effects Rack**: Aetherizer Pro (multi-grain pitch/crystal shimmer + filter bank), Physical-Modeling Waveguide Pipe, 6-Tap Multitap Delay, 6-Voice Chordal Multi-Comb, 8x8 Hadamard FDN Space Cathedral Reverb, Stereo Chorus/Phaser, and Master Limiter.
- Introduce the **320+ Atmospheric Ambient Preset Vault** (`assets/patches/<realm>/*.lua`) across 8 curated realms of Fantasy and Deep Space, supported by expanded external acoustic/spectral `.wav` samples and wavetables, plus fast on-demand HTTP loading on Web.
- Expand **Unit (>=95% C & Lua coverage) and Automated E2E Verification** to test all new DSP modules, modular cable patching, 3D raycast controls, room navigation, Mutator, and spectral/musical diversity across all 320+ presets.

## Capabilities

### New Capabilities
- `modular-patch-graph`: Real-time cycle-safe audio-rate and control-rate modular cable routing graph with attenuverters, stackable jacks, normalled preset wiring, and cross-room trunk buses.
- `spatial-3d-sanctum`: Interconnected 7-room 3D synth architecture, raycast-interactive 3D hardware controls, catenary 3D patch cables with live signal visualization, and dual-mode free-walk + rack-focus navigation.
- `ambient-preset-vault`: 320+ external atmospheric presets organized into 8 Fantasy & Space realms with tag/search catalog, lazy Web hydration, and the Absynth Mutator sound-breeding engine.

### Modified Capabilities
- `dsp-synthesis`: Upgrade oscillators, filters, waveshapers, 68-stage envelopes (adding Env-LFO, step-rhythm, and scrub modes), polyrhythmic sequencer, microtuning, drone latch, and studio effects rack to full Absynth 5+ parity.
- `retro-3d-renderer`: Add portal/room culling, procedural catenary cable geometry with live signal shaders, 3D ray-picking, 3D holographic envelope/wavetable/spectrum displays, and minimal compass/minimap overlay.
- `lua-scripting`: Extend Lua bindings to define the 7-room 3D architecture, 3D widget/jack bindings, cable patching interactions, Mutator logic, and preset catalog management externally.
- `cross-platform-runtime`: Add desktop WASD/mouse-look + click-focus camera, mobile touch walk/look + rack-zoom gestures, WebXR room teleport + 3D ray cable/knob grabbing, and on-demand Web preset fetching.
- `quality-verification`: Extend >=95% C/Lua unit coverage and native/Playwright E2E suites to verify modular cable rewiring, 3D room navigation, diegetic 3D widget control, and automated audio validation of the 320+ preset library.

## Impact

- **C Engine (`src/`)**: Major expansion of `src/dsp/` (fractalizer, Hilbert freq-shifter, comb/formant/cloud filters, 12x 68-pt Env-LFOs, modular patch graph, Pipe, Multi-Comb, 8x8 FDN Reverb, microtuning), `src/render/` (3D raycaster, 3D widget/jack renderer, catenary cable mesher, room culling, 3D curve displays), `src/lua/`, and `src/platform/`.
- **External Assets (`assets/`)**: 7 room architectural 3D meshes & modular rack kits (`assets/models/`), 16+ external spectral/acoustic `.wav` grain/sample sources (`assets/sounds/`), external wavetables (`assets/wavetables/`), Lua room/widget/mutator scripts (`assets/scripts/`), and 320+ external `.lua` patches across 8 realm directories (`assets/patches/`).
- **Web & GitHub Pages (`web/`)**: Fast boot via `assets/patches/catalog.json` + on-demand HTTP fetch of individual preset `.lua` files so 320+ external presets load instantaneously on GitHub Pages.
