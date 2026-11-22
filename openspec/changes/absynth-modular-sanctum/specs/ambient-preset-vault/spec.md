# Spec Delta

## Purpose

Defines the 320+ external atmospheric preset library spanning Fantasy and Deep Space realms, metadata search/filtering, lazy Web loading, and the Absynth-inspired Mutator sound-breeding engine.

## ADDED Requirements

### Requirement: Curated 320+ atmospheric Fantasy and Space preset library
The external `assets/patches/` directory SHALL contain at least 320 distinct, musically playable presets organized across eight thematic realms (`fantasy_realms`, `deep_space`, `bioluminescent_abyss`, `crystalline_sanctum`, `ancient_mythology`, `cybernetic_ruins`, `astral_choirs`, and `rhythmic_odysseys`).

#### Scenario: Browsing and loading presets across all eight realms
- **GIVEN** the Astral Preset Archive initialized from `assets/patches/catalog.json`
- **WHEN** the user filters by any of the eight realms and selects a preset
- **THEN** the engine loads the external `.lua` preset file, configures all oscillators, envelopes, effects, and 3D patch cables, and produces non-silent, stable audio

### Requirement: Audible diversity and evolving atmospheric design
Presets across the vault SHALL utilize diverse synthesis topologies (wavetable morphing, FM bells, Hilbert frequency shifting, fractalized drones, granular clouds, Karplus-Strong combs, vowel formants, waveguide pipes, 68-stage rhythmic/looping envelopes, and FDN space reverbs) with verified spectral and temporal variety.

#### Scenario: Automated spectral and temporal preset validation
- **GIVEN** the automated preset verification suite sampling presets across all eight realms
- **WHEN** offline stereo audio is rendered for sustained notes on each sampled preset
- **THEN** every preset outputs finite, non-silent audio within safe headroom and exhibits measurable timbral diversity across realms

### Requirement: Fast on-demand external preset loading on Web
On the Web target, startup hydration SHALL fetch only core engine assets, shared wavetables/samples, and `patches/catalog.json`, fetching individual external preset `.lua` files over HTTP on demand with local caching so page boot remains fast despite hundreds of presets.

#### Scenario: Selecting an uncached preset on GitHub Pages Web build
- **GIVEN** the Web build running in a browser after initial boot
- **WHEN** the user selects a Deep Space preset from the 3D Astral Preset Archive
- **THEN** the runtime fetches the external `.lua` file over HTTP, caches it in the virtual filesystem, and applies the patch without blocking audio rendering

### Requirement: Absynth-style Mutator sound breeding engine
The synthesizer SHALL provide a Mutator engine capable of generating musical variations of the active patch with controllable Mutation Depth (`[0.0, 1.0]`), Character Preservation rules (protecting gain staging and pitch sanity), Mutation History stack, and AB comparison/save to user patch table.

#### Scenario: Mutating an ambient preset safely
- **GIVEN** an active preset loaded in the Performance Bridge & Mutator Lab with Mutation Depth set to `0.35`
- **WHEN** the user triggers the 3D Mutate lever
- **THEN** oscillator, filter, envelope, effect, and modulation parameters shift within musically bounded ranges, the previous state is pushed to Mutation History, and output remains finite and non-clipping
