# Design

## Context

Green Fairy v0.1 established a clean C11 + SDL3 + OpenGL/WebGL2 + Lua 5.4 architecture with external sibling assets, >=95% unit coverage, and automated native + Playwright E2E suites. However, its synthesis feature set, 4-patch library, and single-room decorative 3D stage with 2D side-sliders are still rudimentary compared to Native Instruments Absynth 5. See [proposal.md](proposal.md) for motivation and capability scope.

## Goals / Non-Goals

**Goals:**
- Achieve and surpass Native Instruments Absynth 5 synthesis depth (9 oscillator modes including Fractalize & Hilbert Frequency Shifter, 14 filter modes including Comb, Formant & Cloud Filter, Wave-Transfer Shapers, 12x 68-stage envelopes with Embedded Env-LFOs & Control-Scrubbing, 16-step polyrhythmic sequencer, Microtuning, Mutator, and Aetherizer Pro / Pipe / Multi-Comb / 8x8 FDN Space Reverb).
- Replace flat 2D parameter screens with **7 interconnected 3D rooms** populated by raycast-interactive 3D knobs, faders, switches, holographic 3D envelope/wavetable sculptures, and physical catenary 3D patch cables that dynamically rewire a true Audio + CV modular patch graph.
- Deliver **320+ curated, external `.lua` atmospheric presets** across 8 realms of Fantasy and Deep Space with fast on-demand HTTP hydration on Web / GitHub Pages.
- Maintain strict real-time audio safety (zero audio-thread locks/allocations), >=95% C and Lua line coverage, and comprehensive native + browser E2E verification.

**Non-Goals:**
- Binary compatibility with proprietary NI `.ksd`/`.nabs` encrypted patch files.
- Photorealistic PBR/raytraced materials (preserve crisp, stylized retro-futuristic 3D aesthetics that run at 60 FPS across desktop, mobile Web, and WebXR).

## Decisions

### 1. Cycle-Safe Modular Audio + CV Patchbay Graph (`src/dsp/patchbay.c`)

- **Architecture**: Every per-voice and master DSP block reads its inputs from and writes its outputs to a flat, cache-aligned signal bus array (`float bus[GF_PORT_COUNT]`, ~96 typed ports covering Osc 1–3 Audio/Pitch/FM/Morph/Fractal/Grain, Filter 1–3 In/Out/Cutoff/Res/Morph, Shaper 1–3, Env 1–12, LFO 1–3, 16-Step Seq Pitch/Gate/Mod, VCA/Attenuverters, XYZ Orb, Macros 1–8, MIDI Velocity/Key/Aftertouch, and Master FX Send/Return buses).
- **Lock-Free Rewiring**: When the user plugs, unplugs, stacks, or adjusts an attenuverter ring on a 3D cable (or loads a preset), the control thread compiles a topologically sorted `GFPatchPlan` (up to 128 active `GFCableConn` edges `{src_port, dst_port, gain, cable_id, is_feedback}`) into an inactive plan slot and publishes the slot index atomically to the audio thread.
- **Feedback Cycle Handling**: Kahn's topological sort orders modules; any residual cycle edge is flagged `is_feedback = true` and reads from the previous sample's soft-saturated (`tanhf`) unit-delay state, enabling authentic Absynth self-oscillating feedback patches without NaNs or runaway blowups.
- **Live Cable Telemetry**: During `gf_synth_render_block`, peak/RMS signal levels for each active cable are accumulated into the lock-free double-buffered telemetry struct so 3D cables pulse with real voltage intensity.

### 2. Full Absynth 5+ DSP Engine Expansion (`src/dsp/`)

- **Oscillators (`src/dsp/oscillator.c`)**:
  1. `WAVETABLE_SINGLE` & `WAVETABLE_DOUBLE`: Mip-mapped 8-octave band-limited wavetable sets generated via harmonic pruning (`gf_wavetable_build_mipmaps`) plus live 64-partial harmonic spectrum sculpting.
  2. `FM`: Carrier/Modulator wavetable FM with self-feedback and external `fm_in` audio-rate cross-modulation port.
  3. `RINGMOD` & `FREQ_SHIFT`: Analytic signal single-sideband Bode frequency shifter using a matched 4th-order polyphase all-pass Hilbert network ($90^\circ$ quadrature pair) for bipolar Hz shifting (`[-2000, +2000] Hz`) and stereo opposite-sideband widening.
  4. `FRACTALIZE`: Recursive waveform contraction/folding across $2..6$ octaves (`fractal_depth`, `fractal_ratio`, `fractal_decay`).
  5. `SAMPLE` & `GRANULAR_CLOUD`: Up to 16 simultaneous stereo grains per channel with Hann/Tukey window morph, grain pitch, density, duration, position spray, reverse grain probability, and cloud freeze.
  6. `SUPER_UNISON`: 8 detuned stereo voices per oscillator channel with pan spread and detune curve.
- **Filters (`src/dsp/filter.c`) & Shaper (`src/dsp/waveshaper.c`)**:
  - 14 topologies: Bypass, 12 dB SVF (LP/HP/BP/Notch), 24 dB ZDF Ladder (LP24/HP24/BP24 with nonlinear saturation), 4-Stage All-Pass Phaser, Key-Tracked Damped Karplus-Strong `COMB_POS` & `COMB_NEG`, 5-Vowel (`A-E-I-O-U`) continuous Formant Filter, and Granular `CLOUD_FILTER`.
  - Waveshaper upgraded with 256-sample custom wave-transfer curve, phase offset, wavefolder, and asymmetric tube warmth.
- **12x 68-Stage Envelopes (`src/dsp/envelope.c`) & Polyrhythmic Modulators (`src/dsp/sequencer.c`, `src/dsp/tuning.c`)**:
  - Each of the 12 envelopes supports `SUSTAIN`, `LOOP`, `RHYTHMIC_STEP` (tempo-grid synced), `ENV_LFO` (per-stage embedded LFO wave/rate/depth riding the envelope segment), and `CONTROL_SCRUB` (direct X/Y/Macro/Velocity lookup along the 68 breakpoints).
  - 16-step polyrhythmic modulator + 8 microtuning scales (`12-TET`, `Just`, `Pythagorean`, `Bohlen-Pierce`, `Quarter-Tone`, `Pelog`, `Carlos Alpha`, `Harmonic`) + polyphonic portamento + **Drone Hold Latch** (essential so users can latch a chord and walk freely through the 7 rooms re-patching cables).
- **Studio Effects Suite (`src/dsp/fx_*.c`)**:
  - `aetherizer.c`: Upgraded to 24-grain stereo pitch/crystal cloud + resonant bandpass bank + reverse grains.
  - `fx_pipe.c`: Physical-modeling bidirectional acoustic waveguide pipe with variable excitation/pickup points and bore modulation.
  - `fx_resonators.c`: 6-Tap Stereo Multitap Delay + 6-Voice Chordal Multi-Comb Resonator.
  - `fx_reverb.c`: 8x8 Hadamard Feedback Delay Network (FDN) Space Cathedral Reverb with modulated prime delay lines, shimmer octave injection, and freeze.

### 3. Seven-Room 3D Synth Sanctum, Raycast Controls & Catenary Cables (`src/render/`)

```text
                     [Room 1: Oscillator Conservatory]
                                (0, 0, -18)
                       /             |             \
    [Room 7: Astral Archive]         |         [Room 2: Filter & Shaper Foundry]
        (-16, 0, -10)                |                  (16, 0, -10)
              |                      |                        |
              |      [Room 6: Performance Bridge & Mutator]   |
              +---------------   (0, 0, 0)   -----------------+
              |                      |                        |
    [Room 5: Aetherium FX]           |         [Room 3: 68-Pt Envelope Dome]
        (-16, 0, 10)                 |                  (16, 0, 10)
                       \             |             /
                     [Room 4: Clockwork Mod & Trunk Patchbay]
                                 (0, 0, 18)
```

- **Spatial Widget & Raycast System (`src/render/spatial_ui.c`)**:
  - Defines `GFWidget3D` records in C (`id`, `room_id`, `rack_id`, `kind`, world transform, OBB half-extents, bound `param_id` or `port_id`, normalized value, label).
  - Unprojects pointer/touch coordinates (`gf_ray_from_screen`) or uses WebXR controller pose rays (`gf_ray_from_xr`) to intersect OBBs (`gf_ray_intersect_obb`) sorted by ray distance `t`.
  - Renders 3D procedural/loaded hardware parts (beveled rack chassis, rotary knobs with indicator needles, glowing fader caps, toggle levers, color-coded hex patch jacks, 3D world-space bitmap text labels, and dynamic 3D holographic line-strips for 68-stage envelopes, wavetables, and filter responses).
- **3D Catenary Cable & Conduit Renderer (`src/render/cable_renderer.c`)**:
  - Intra-room cables generate smooth 12-segment catenary tubes with a draggable mid-span **3D Attenuverter Ring** (drag up/down to scale cable gain `[-2.0, +2.0]`).
  - Cross-room cables route cleanly up into overhead glowing ceiling conduits between rooms AND appear on **Room 4's Central Trunk Patchbay** (and per-room satellite trunk strips) so users can patch across rooms either by walking while holding a live cable or via local trunk jacks.
  - Cable shader (`assets/shaders/cable.frag.glsl`) animates traveling light pulses whose speed and brightness reflect live per-cable telemetry from `GFPatchPlan`.
- **Dual-Mode 3D Navigation (`src/render/camera_rig.c`)**:
  - **Free-Walk Mode**: Desktop `WASD`/Arrows + right-mouse/drag look (with simple radial/corridor floorplan bounds collision), Mobile dual-zone thumb walk/look, WebXR thumbstick/ray teleport.
  - **Rack-Focus Mode**: Clicking any rack header (or tapping a rack on mobile) smoothly interpolates (`cubic ease-out`, 0.35s) the camera in front of that rack so every 3D knob, 68-stage breakpoint, and patch jack fills the viewport cleanly for effortless precision control; `Esc` / right-click-bg / `Back` badge returns to Free-Walk.
  - **Minimal Compass & Room Quick-Jump**: A compact corner compass shows current room name, active preset, Drone Latch toggle, and 7 tiny room nodes for optional instant teleport.

### 4. 320+ Atmospheric Preset Vault & Mutator (`assets/patches/`, `assets/scripts/presets/`)

- **8 Curated Realms (40 External `.lua` Presets Each = 320 Presets)**:
  1. `fantasy_realms/` — Elven glass harps, enchanted flutes, runesong choirs, sunken cathedrals, crystal caverns.
  2. `deep_space/` — Pulsar telemetry, event-horizon sub-monoliths, Dyson sphere harmonics, nebula clouds, cryo-sleep pads.
  3. `bioluminescent_abyss/` — Whale-song vowel formants, trench pressure drones, jellyfish arps, shimmering coral clouds.
  4. `crystalline_sanctum/` — Hilbert prism bells, quartz multi-combs, ice-cavern echoes, diamond granular dust.
  5. `ancient_mythology/` — Singing bowls, throat-singing overtones, bronze gong washes, astral lyres, oracle whispers.
  6. `cybernetic_ruins/` — Mainframe polyrhythms, tape-warble ghosts, ring-mod transmissions, neon horizon brass.
  7. `astral_choirs/` — Seraphim A-E-I-O-U choirs, cosmic harmonium, starlight sopranos, monastic space chants.
  8. `rhythmic_odysseys/` — Evolving 68-stage rhythmic landscapes, clockwork sequences, pulsar kalimbas.
- **External Sound & Wavetable Expansion**: 16 external stereo/mono `.wav` textures in `assets/sounds/` (bowed glass, choir vowels, waterphone, metallic bell, breath flute, cosmic sweep, plucked string, crystal grains, etc.) + 16 external wavetable definitions in `assets/wavetables/`.
- **Fast Web On-Demand Hydration**: `assets/manifest.json` lists core engine assets + `patches/catalog.json` (~35 requests at boot). Selecting any of the 320 presets on Web checks `Module.FS`; if absent, `bridge.js` fetches `assets/patches/<realm>/<slug>.lua`, writes it into `/assets/patches/...`, and invokes `gf_web_apply_preset_path`, keeping GitHub Pages load time under 1 second while preserving 100% external individual `.lua` files!
- **Absynth Mutator (`assets/scripts/dsp/mutator.lua`)**: Mutates oscillator modes/ratios/morphs, filter cutoffs/formants, 68-stage envelope times/levels, FX parameters, and modulation cable weights within musically clamped envelopes according to `depth` and `randomness`, maintaining a 16-slot undo/redo Mutation History stack.

## Risks / Trade-offs

- **[Risk] Draw call count across 7 rooms with hundreds of 3D knobs and cables** → **Mitigation**: Partition widgets and rack meshes by `room_id` (culling non-visible rooms), batch all 3D knob/fader/jack instances per room into dynamic/instanced vertex buffers, and batch all catenary cables into a single `GL_TRIANGLES` VBO draw call per frame.
- **[Risk] CPU budget with 16 voices × 3 channels + FDN Reverb + Aetherizer on Mobile Web** → **Mitigation**: Run voice-level DSP (oscillators, per-channel filters, envelopes) per voice and run heavy space effects (Aetherizer Pro, Pipe, Multi-Comb, 8x8 FDN Reverb) on the stereo Master/Send buses, keeping 16-voice polyphony under ~15% single-core CPU in WASM.
- **[Risk] 320 presets feeling repetitive if generated naively** → **Mitigation**: Design 32 distinct multi-module synthesis & cable-routing blueprints (4 distinct synthesis blueprints per realm × 10 curated harmonic/envelope/scale/FX voicings each) and enforce an automated spectral-centroid, temporal-variance, and zero-clipping test across all 320 presets.
