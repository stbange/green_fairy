# Spec Delta

## Purpose

Defines the real-time audio-rate and control-rate modular cable routing graph allowing arbitrary module-to-module patching, stackable jacks, attenuverters, and cycle-safe feedback paths across the synthesizer.

## ADDED Requirements

### Requirement: Typed audio and CV patchbay ports
The synthesis engine SHALL expose typed output ports (Audio, Unipolar/Bipolar CV, Gate/Clock) and input ports across all oscillator, filter, waveshaper, envelope, LFO, sequencer, macro, and effect modules.

#### Scenario: Connecting an LFO output jack to a filter cutoff input jack
- **GIVEN** an unpatched resonant low-pass filter and an active LFO output port
- **WHEN** a patch cable is connected from `lfo1.out` to `flt1.cutoff_cv` with positive gain
- **THEN** the filter cutoff modulates in real time according to the LFO signal without audio clicks

### Requirement: Stackable many-to-many cable connections with attenuverters
The patch graph SHALL support connecting one output jack to multiple input jacks and summing multiple cables into a single input jack, with an independent bipolar attenuverter weight (`[-2.0, +2.0]`) per cable.

#### Scenario: Summing two modulation cables into one target jack
- **GIVEN** `env2.out` connected to `osc1.fm_cv` with weight `+0.8` and `lfo2.out` connected to `osc1.fm_cv` with weight `-0.4`
- **WHEN** a voice renders audio frames
- **THEN** `osc1.fm_cv` receives the weighted sum of both modulation signals

### Requirement: Dynamic preset cable spawning and live rewiring
Loading any preset SHALL populate the patch graph with explicit cable connections matching that preset's audio and modulation routing, and unplugging or reconnecting cables at runtime SHALL immediately alter signal flow without heap allocation on the audio thread.

#### Scenario: Unplugging an oscillator audio cable silences its path
- **GIVEN** a loaded preset where `osc1.audio_out` is cabled to `flt1.audio_in` and onward to `master.in`
- **WHEN** the user unplugs the `osc1.audio_out` cable while a note is sounding
- **THEN** Channel 1's audio contribution ceases smoothly without crashing or muting other cabled channels

### Requirement: Cycle-safe feedback routing
The patch graph SHALL detect directed cycles when topological sorting cannot linearize a user-created feedback loop and SHALL insert a deterministic single-sample unit delay with soft saturation so self-patched feedback paths remain bounded and stable.

#### Scenario: Patching master effect output back into oscillator FM input
- **GIVEN** an active patch where `aether.out_l` is patched back into `osc3.audio_in` creating a cycle
- **WHEN** audio blocks are rendered continuously
- **THEN** the engine processes the feedback loop via a 1-sample delay bus and outputs finite bounded samples with zero NaNs

### Requirement: Live per-cable signal telemetry
The patch graph SHALL publish lock-free per-cable RMS/instantaneous signal amplitude snapshots to the main thread so rendered 3D patch cables animate real-time signal intensity and polarity.

#### Scenario: Active cable telemetry reflects signal level
- **GIVEN** an active cable carrying an oscillator signal during note playback
- **WHEN** telemetry is queried for the rendered frame
- **THEN** the cable's reported signal level is non-zero during playback and decays toward zero when silent
