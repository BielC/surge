# Live LFO position prototype

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
