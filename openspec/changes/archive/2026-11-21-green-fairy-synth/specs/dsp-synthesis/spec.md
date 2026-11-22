# Spec Delta

## Purpose

Defines the real-time Absynth-inspired polyphonic semi-modular audio synthesis engine, including multi-mode oscillators, multi-stage breakpoint envelopes, LFOs, filters, waveshapers, effects, and patch morphing.

## ADDED Requirements

### Requirement: Three-channel oscillator architecture
The synthesis engine SHALL provide three independent oscillator channels per voice, each selectable among Wavetable (single and double), Frequency Modulation (FM), Ring Modulation, Sample playback, and Granular synthesis modes.

#### Scenario: Multi-channel oscillator mixing
- **GIVEN** a voice with Channel 1 in Wavetable mode, Channel 2 in FM mode, and Channel 3 in Ring Modulation mode
- **WHEN** a note-on event triggers the voice and audio frames are rendered
- **THEN** the engine outputs a finite, non-silent stereo signal combining all active channels according to their mix levels and stereo pan settings

#### Scenario: Granular sample synthesis
- **GIVEN** an oscillator channel configured in Granular mode with a loaded external audio sample, grain size, density, and pitch ratio
- **WHEN** the voice renders audio across multiple blocks
- **THEN** overlapping windowed grains are synthesized without clicks, NaN values, or buffer out-of-bounds reads

### Requirement: Multi-stage breakpoint envelopes
The synthesis engine SHALL support per-voice multi-stage breakpoint envelopes with up to 68 breakpoints, configurable segment curvature, sustain point hold, and optional loop segments assignable to amplitude, pitch, filter cutoff, and modulation depth.

#### Scenario: Sustain hold and release progression
- **GIVEN** a breakpoint envelope with attack stages, a sustain breakpoint, and release stages
- **WHEN** a note-on occurs, holds past the sustain time, and then receives a note-off
- **THEN** the envelope holds at the sustain breakpoint level until note-off and then traverses the release stages to completion

#### Scenario: Looping envelope segment
- **GIVEN** a breakpoint envelope with a defined loop start and loop end breakpoint
- **WHEN** a note is held past the loop end time
- **THEN** the envelope continuously repeats the segment between loop start and loop end until note-off

### Requirement: LFO modulation matrix
The synthesis engine SHALL provide three independent Low-Frequency Oscillators (LFOs) per voice with selectable waveforms, frequency/tempo sync, phase offset, and routable modulation targets.

#### Scenario: LFO modulating filter cutoff
- **GIVEN** LFO 1 configured as a sine wave at 2 Hz routed to filter cutoff with positive modulation depth
- **WHEN** a sustained note is rendered over one second
- **THEN** the effective filter cutoff oscillates periodically around its base value within valid stability bounds

### Requirement: Per-channel and master signal processing
The synthesis engine SHALL provide per-channel resonant multimode filters (low-pass, high-pass, band-pass, notch) and waveshapers, feeding into a master bus with a master filter, waveshaper, and an Aetherizer-style granular/multitap feedback delay effect.

#### Scenario: Multimode filter attenuation
- **GIVEN** a harmonic-rich sawtooth oscillator routed through a low-pass filter set to 400 Hz
- **WHEN** audio is rendered and analyzed spectrally
- **THEN** high-frequency energy above 4 kHz is attenuated relative to the unfiltered signal while output remains stable

#### Scenario: Master feedback delay tails
- **GIVEN** the master Aetherizer-style multitap delay enabled with non-zero feedback and wet mix
- **WHEN** a short percussive note finishes its amplitude envelope
- **THEN** decaying delayed echoes remain audible after the dry voice becomes silent

### Requirement: Polyphonic voice management
The synthesis engine SHALL support polyphonic playback of at least 16 simultaneous voices with deterministic voice stealing when polyphony is exceeded and smooth voice transitions without clicks.

#### Scenario: Voice stealing under overload
- **GIVEN** a maximum polyphony of 16 voices and 16 active sustained notes
- **WHEN** a 17th note-on event arrives
- **THEN** the engine steals the oldest or quietest eligible voice and sounds the new note without crashing or clipping

### Requirement: XY patch morphing and macro controls
The synthesis engine SHALL support real-time interpolation across four corner parameter snapshots via a normalized 2D XY morph pad and assignable macro controls without audio dropouts.

#### Scenario: Continuous XY morph interpolation
- **GIVEN** four distinct parameter snapshots assigned to the four corners of the XY morph pad
- **WHEN** the XY coordinates move smoothly from (0.0, 0.0) to (1.0, 1.0) during playback
- **THEN** morphed synthesis parameters transition continuously between corner values with zero audio NaN or zipper artifacts

### Requirement: Real-time audio safety and deterministic offline rendering
The synthesis engine SHALL execute audio block rendering without dynamic heap allocation, mutex blocking, or filesystem I/O, and SHALL produce bit-identical output when driven deterministically in offline test mode.

#### Scenario: Deterministic offline block render
- **GIVEN** two synthesis engine instances initialized with identical seed, sample rate, patch parameters, and event timeline
- **WHEN** both instances render 1024 audio frames offline
- **THEN** both output buffers match sample-for-sample within exact floating-point equality
