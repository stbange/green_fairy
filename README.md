# Green Fairy

**Green Fairy** is a professional Absynth 5+ caliber modular software synthesizer set inside an interconnected **7-Room Retro-3D Synth Sanctum**. Every synthesis, envelope, harmonic, sequencer, effect, mutator, and performance parameter is controlled directly in 3D world space via raycast-interactive hardware racks, holographic sculptures, and physical catenary patch cables that dynamically rewire a cycle-safe Audio + CV modular patch graph.

Built on a real-time **C11 + SDL3 + OpenGL (GL 3.3 Core / WebGL2 & GLES 3.0)** core engine paired with an external, hot-reloadable **Lua 5.4** scripting architecture, Green Fairy ships with **zero baked-in assets** and **320+ curated atmospheric presets** across 8 realms of Fantasy and Deep Space.

## Highlights

- **Interconnected Seven-Room 3D Synth Sanctum (`src/render/`, `assets/scripts/rooms/`)**
  1. **Room 1 — Oscillator Conservatory**: 3 curved Absynth oscillator racks + 32-bar interactive 3D Wavetable Harmonic Sculptor & live holographic waveforms.
  2. **Room 2 — Filter & Shaper Foundry**: 3 per-channel + Master 14-mode filter & 6-mode wave-transfer shaper racks with live 3D frequency response curves.
  3. **Room 3 — 68-Stage Envelope Observatory**: Giant 3D holographic breakpoint rail where stages can be dragged in 3D across 12 assignable envelopes.
  4. **Room 4 — Clockwork Mod & Trunk Patchbay**: Triple LFO + Quad VCA matrix, 16-Step Polyrhythmic Sequencer, and Central Cross-Room Satellite Trunk Patchbay.
  5. **Room 5 — Aetherium & Acoustics Chamber**: 24-grain Aetherizer Pro, Bidirectional Waveguide Pipe, 6-Voice Chordal Multi-Comb, 6-Tap Stereo Delay, and 8x8 Hadamard FDN Space Cathedral Reverb.
  6. **Room 6 — Performance Bridge & Mutator Lab**: Floating 3D XYZ Morph Orb, tilted 25-key 3D synthesizer console, Drone Hold Latch, 8-scale Microtuning, 8 Macro knobs, and the Absynth Sound-Breeding **Mutator** with 16-slot undo history.
  7. **Room 7 — Astral Preset Archive**: 3D browser across **8 thematic realms (320+ external `.lua` presets)**.
- **True Audio + CV Modular Patchbay & Catenary Cables (`src/dsp/patchbay.c`, `src/render/cable_renderer.c`)**
  - ~100 typed Audio, CV, and Trigger jacks supporting stackable many-to-many connections, draggable mid-cable bipolar **3D Attenuverter Rings** (`[-2.0, +2.0]`), overhead cross-room ceiling conduits, Kahn topological sorting, and 1-sample `tanhf`-saturated feedback loops.
  - Lock-free double-buffered `GFPatchPlan` swapping with live per-cable voltage luminance pulses.
- **Professional Absynth 5+ DSP Engine (`src/dsp/`)**
  - **9 Oscillator Modes**: Band-limited mip-mapped Single & Double Wavetables, Carrier/Mod + External-Bus FM, RingMod, Crossfaded Sample, 16-Grain Stereo Cloud Granular, Recursive Fractalizer, 4th-Order Polyphase Hilbert Single-Sideband Frequency Shifter, and 8-Voice Stereo Super-Unison.
  - **14 Filter Topologies & 6 Waveshapers**: 12 dB SVF (LP/HP/BP/Notch), 24 dB Nonlinear ZDF Ladder (LP24/HP24/BP24), 4-Stage Allpass, Key-Tracked Damped Karplus-Strong `Comb+` / `Comb-`, 5-Vowel (`A-E-I-O-U`) Formant Filter, Granular Cloud Filter, and 256-point Wave-Transfer Shapers.
  - **12x 68-Stage Envelopes, 16-Step Polyrhythmic Sequencer & 8 Microtunings**: Sustain, Loop, Tempo-Synced Rhythmic Pulse, Per-Stage Embedded `Env-LFO`, and CV/Macro `Scrub` modes; `12-TET`, `Just`, `Pythagorean`, `Bohlen-Pierce`, `Quarter-Tone`, `Pelog`, `Carlos Alpha`, and `Harmonic` scales.
- **320+ Atmospheric Presets Across 8 Realms (`assets/patches/`)**
  - `fantasy_realms/`, `deep_space/`, `bioluminescent_abyss/`, `crystalline_sanctum/`, `ancient_mythology/`, `cybernetic_ruins/`, `astral_choirs/`, and `rhythmic_odysseys/` (40 external `.lua` presets each + on-demand HTTP hydration on Web).
- **Dual-Mode Cross-Platform Navigation**
  - **Desktop (Windows, Linux, macOS, Web)**: `WASD`/Arrows + right-drag mouse-look free-walk, `1`–`7` room teleport, click any 3D rack header for smooth cubic **Rack-Focus** zoom (`Esc` to return).
  - **Mobile Touch Web**: Automatic touch mode with dual-zone virtual thumb walk/look pads, tap-to-focus rack framing, enlarged 3D control hitboxes, and collapsible multi-touch polyphonic keyboard + Drone Latch strip.
  - **WebXR (`immersive-vr`)**: Stereoscopic 3D room rendering, controller room teleportation, and 6DoF XR laser ray knob twisting & 3D cable patching.

## Directory Structure

```text
green_fairy/
├── assets/                 # 100% external assets beside the binary (zero baked-in data)
│   ├── manifest.json       # Core boot asset list for fast Web HTTP hydration
│   ├── scripts/            # Lua 7-room sanctum, Mutator, generators, arpeggiator, compass
│   ├── patches/            # catalog.json + 320+ external .lua presets across 8 realms
│   ├── wavetables/         # 16 external harmonic/formant wavetable definitions
│   ├── sounds/             # 16 external acoustic & spectral .wav textures
│   ├── models/             # Retro-3D room shells, archways, racks, knobs, faders, jacks
│   ├── sprites/            # Animated pixel-art sprites (.tga)
│   ├── voxels/             # Optional neighbor-culled voxel spires (.gfvox)
│   ├── shaders/            # GLSL shaders (mesh, catenary cable pulse, sprite, HUD)
│   └── fonts/              # External bitmap font atlas & metrics (2D + 3D world plaques)
├── src/                    # C11 core engine (core, assets, dsp, render, lua, platform)
├── web/                    # Emscripten HTML shell & WebXR / mobile-touch / preset bridge
├── tests/                  # C unit tests, Lua unit tests, preset vault & E2E suites
└── openspec/               # OpenSpec living specifications & change history
```

## Building & Running

### Desktop (Linux / macOS / Windows)

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
./build/linux-debug/green_fairy
```

Windows and macOS use `cmake --preset windows-release` and `cmake --preset macos-release`.

### Web & WebXR (Emscripten)

```bash
source /path/to/emsdk/emsdk_env.sh
cmake --preset web-emscripten
cmake --build --preset web-emscripten
python3 -m http.server 8080 --directory build/web-emscripten
```

## Testing & >=95% Coverage Gates

```bash
# 1. C unit tests + 320-preset vault audit + >=95% C line coverage gate
cmake --preset linux-coverage
cmake --build --preset linux-coverage
ctest --test-dir build/linux-coverage --output-on-failure
gcovr --root . build/linux-coverage --filter '^src/' --fail-under-line 95

# 2. Lua 7-room & Mutator unit tests + >=95% Lua line coverage gate
./build/linux-coverage/gf_lua_test_runner --assets assets --fail-under 95

# 3. Native headless 7-room 3D + DSP + cable re-patching E2E suite
xvfb-run -a env LIBGL_ALWAYS_SOFTWARE=1 python3 tests/e2e/native_e2e.py ./build/linux-coverage/green_fairy

# 4. Playwright Desktop, Mobile Touch & WebXR E2E suite
node tests/e2e/web/run_web_e2e.mjs build/web-emscripten
```
