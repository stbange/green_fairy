# Spec Delta

## MODIFIED Requirements

### Requirement: Three-channel oscillator architecture
The synthesis engine SHALL provide three independent oscillator channels per voice supporting nine professional modes: Single Wavetable (anti-aliased mip-mapped with phase/harmonic morph), Double Wavetable (sync/crossfade), FM (with feedback and cross-channel FM), Ring Modulation, Hilbert Single-Sideband Frequency Shifter, Recursive Fractalizer, Sample playback (forward/reverse/crossfade loop), Sync/Cloud Stereo Granular, and 8-Voice Stereo Super-Unison.

#### Scenario: Multi-channel oscillator mixing
- **GIVEN** a voice with Channel 1 in Fractalize mode, Channel 2 in Hilbert Frequency Shifter mode, and Channel 3 in Super-Unison mode
- **WHEN** a note-on event triggers the voice and audio frames are rendered
- **THEN** the engine outputs a finite, non-silent stereo signal combining all active channels according to their patched mix levels and stereo spread

#### Scenario: Granular sample synthesis
- **GIVEN** an oscillator channel configured in Cloud Granular mode with a loaded external sample, grain size, density, stereo pan spread, and reverse probability
- **WHEN** the voice renders audio across multiple blocks
- **THEN** overlapping windowed stereo grains are synthesized without clicks, NaN values, or buffer out-of-bounds reads

### Requirement: Multi-stage breakpoint envelopes
The synthesis engine SHALL provide twelve assignable 68-stage breakpoint envelopes per voice supporting five operating modes: Sustained, Looping, Tempo-Synced Rhythmic Step-Pulse, Per-Stage Embedded Envelope-LFO, and Macro/XY Control-Scrubbing.

#### Scenario: Sustain hold and release progression
- **GIVEN** a breakpoint envelope with attack stages, a sustain breakpoint, and release stages
- **WHEN** a note-on occurs, holds past the sustain time, and then receives a note-off
- **THEN** the envelope holds at the sustain breakpoint level until note-off and then traverses the release stages to completion

#### Scenario: Looping envelope segment
- **GIVEN** a breakpoint envelope with a defined loop start and loop end breakpoint
- **WHEN** a note is held past the loop end time
- **THEN** the envelope continuously repeats the segment between loop start and loop end until note-off

#### Scenario: Embedded Envelope-LFO and Control-Scrubbed modes
- **GIVEN** an envelope configured with per-stage embedded LFO modulation or Macro Control-Scrubbing
- **WHEN** a note sustains or the bound Macro scrubs from `0.0` to `1.0`
- **THEN** the envelope superimposes the stage LFO or evaluates the 68-stage curve at the scrubbed coordinate cleanly

### Requirement: Per-channel and master signal processing
The synthesis engine SHALL provide 14 filter topologies (12 dB and 24 dB ZDF Ladder LP/HP/BP/Notch, All-Pass Phaser, Positive and Negative Karplus-Strong Comb Resonators, A-E-I-O-U Formant Vowel Filter, and Granular Cloud Filter), custom Wave-Transfer Waveshapers, and a multi-stage stereo Master Effects Rack (Aetherizer Pro, Waveguide Pipe, 6-Tap Multitap Delay, 6-Voice Multi-Comb, Stereo Chorus/Phaser, and 8x8 Hadamard FDN Space Reverb).

#### Scenario: Multimode filter attenuation
- **GIVEN** a harmonic-rich sawtooth oscillator routed through a 24 dB Ladder low-pass or Formant Vowel filter
- **WHEN** audio is rendered and analyzed spectrally
- **THEN** out-of-band energy is attenuated or shaped into vowel formant peaks while output remains stable

#### Scenario: Master feedback delay tails
- **GIVEN** the master effects chain active with Aetherizer Pro shimmer, Waveguide Pipe resonance, Multi-Comb chordal tuning, and 8x8 FDN Space Reverb
- **WHEN** a short percussive note finishes its amplitude envelope
- **THEN** decaying pitched, resonant, and reverberant stereo echoes remain audible after the dry voice becomes silent

## ADDED Requirements

### Requirement: Polyrhythmic 16-step modulator and microtuning engine
The synthesis engine SHALL provide a tempo-synced 16-step polyrhythmic step modulator/sequencer (emitting pitch, gate, and CV) and selectable microtuning scales (12-TET, Just Intonation, Pythagorean, Bohlen-Pierce, Quarter-Tone, Pelog, Wendy Carlos Alpha, and Harmonic Series) plus polyphonic portamento and drone-hold latch.

#### Scenario: Playing notes in non-12-TET microtuning with drone latch
- **GIVEN** Bohlen-Pierce microtuning and Drone Latch enabled
- **WHEN** a chord of MIDI notes is triggered and released
- **THEN** voice fundamental frequencies follow the Bohlen-Pierce tuning ratios and sustain continuously until unlatched or replaced
