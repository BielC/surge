# Live LFO position prototype

## Live parameter markers

Modulated floating-point sliders also show a live dot and a line from their base
handle to the current modulated value. The main handle stays at the value you set;
modulation-depth editing remains unchanged. Markers use the skin's positive
modulation color and refresh at about 30 Hz.

The engine publishes the combined value after scene and voice modulation, using
the newest held voice (then the newest releasing voice). Global/FX parameters use
the engine's global modulation data. Muted/removed routings and stale snapshots
hide their markers. Voice-only modulation needs an active or releasing voice.
Scene-only modulation can still be shown when there are no voices.

Filter cutoff includes its built-in envelope amount and key tracking, using the
same calculation as the filter coefficient maker. In filter 2 offset mode the
marker shows the resulting offset in semitones, matching that slider's scale.
Linked filter resonance follows filter 1's modulated resonance.

These are parameter targets before DSP smoothing and algorithm-specific limits,
not per-sample measurements of the eventual audio signal. Values outside the
slider range pin the marker to the end stop. Discrete controls, other internal
modulation paths (such as oscillator drift/pitch bend), and per-voice clouds of
markers are outside this prototype's scope. Fast modulation can alias visually.

The parameter publisher uses one relaxed atomic load per block without a request.
On a request, it reads existing engine values under the engine's existing routing
lock, without allocation or adding locks. Packed atomic values carry a refresh
generation so removed routings and partial refreshes cannot leave old values.

The modulation suite passed 17 test cases, including 54 parameter-display
assertions covering summed sources, mute/removal, voice priority, scene/global
identity, cutoff envelope/key tracking, offset scale, and non-mutating polling.
A local 30-sample microbenchmark measured approximately 2 ns without a request and
113 ns per requested refresh for eight modulated parameters with 16 voices.
These figures exclude GUI rendering and do not establish total DAW CPU impact.
Standalone and VST3 builds pass. The new parameter markers still need a visual
check: the standalone restart paused opening its user-preferences file, before
creating its window. This startup issue was observed outside the new display code.

```sh
build/src/surge-testrunner/surge-testrunner '[parameter-display]'
build/src/surge-testrunner/surge-testrunner '[parameter-display-perf]' --benchmark-samples 30
```

## LFO waveform marker

Sine, triangle, square and saw LFOs show a moving dot and a numeric live value.
The horizontal axis is the oscillator phase. The curve is a cached one-cycle
reference before the LFO envelope and parameter modulation; the dot includes both.
Envelope or amplitude modulation can therefore move the dot away from the curve.
Values outside +/-1 are clipped visually, while the number retains the actual value.

Voice LFOs follow the newest held voice, falling back to the newest releasing voice.
Scene LFOs follow the selected scene. A voice LFO needs a sounding/releasing voice;
LFO2-6 also need to be in use because Surge does not process unused voice LFOs.
Inactive sources and audio engines that stop responding hide the dot.

Right-click the waveform and disable **Show Live LFO Position** to return to the
original envelope preview. This switch lasts for the current editor session.
Noise, sample-and-hold, envelope, step sequencer, MSEG and formula retain their
original previews in this first version. Fast LFOs can alias visually at the UI
refresh rate; this display is intended mainly for slow modulation.

## Implementation

The editor requests one selected source at about 30 Hz. After processing a block,
the audio thread reads its existing phase/output and publishes one packed,
lock-free atomic snapshot, tagged with scene, LFO and output index. No voice
pointers cross threads. Sampling does not process the LFO, allocate or take a lock.
When the editor is inactive there is one relaxed atomic load per audio block.
The reference curve is rebuilt only when parameters or the display change.

This avoids repeatedly regenerating the simulated waveform discussed in
[issue #7895](https://github.com/surge-synthesizer/surge/issues/7895) and the
broader realtime waveform request in
[issue #139](https://github.com/surge-synthesizer/surge/issues/139).

## Checks

Built the ARM64 Release standalone and VST3 on macOS, with plugin copying disabled.
The standalone is copied to `~/Applications/Surge XT LFO Prototype.app` and
ad-hoc signed; its signature verifies. The VST3 has not been installed or tested
in a DAW. Existing installed plugins are untouched.
The modulation suite passed all 16 test cases, including snapshot selection,
engine value/phase agreement, inactive sources and non-mutating polling.
The GUI was checked with a slow 0.125 Hz sine LFO, including the Purple skin.
Automated native UI checks could not confirm the right-click toggle, so that
interaction still needs a manual check. Pre-commit could not run because
upstream has no `.pre-commit-config.yaml`; changed C++ lines were clang-formatted
and `git diff --check` passed.

A local Catch2 microbenchmark (30 samples) measured about 3.3 ns for the
no-request path and 26 ns for a selected voice LFO with 16 voices. These figures
measure snapshot sampling only, not total GUI rendering or DAW CPU usage.
A controlled live-display-on/off CPU comparison remains to be done.

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_LTO=OFF -DSURGE_COPY_AFTER_BUILD=OFF
cmake --build build --target surge-xt_Standalone surge-xt_VST3 surge-testrunner --parallel 8
build/src/surge-testrunner/surge-testrunner '[mod]'
build/src/surge-testrunner/surge-testrunner '[lfo-display-perf]' --benchmark-samples 30
```
