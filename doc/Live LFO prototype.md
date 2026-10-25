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
Standalone and VST3 builds pass. Visual checks confirmed moving parameter
markers in the Classic and Purple skins, a changed cutoff base handle, and
modulation-edit colours. The standalone preferences-access issue was resolved.

```sh
build/src/surge-testrunner/surge-testrunner '[parameter-display]'
build/src/surge-testrunner/surge-testrunner '[parameter-display-perf]' --benchmark-samples 30
```

## LFO waveform marker

All ten LFO types now have a live view. Sine, triangle, square and saw LFOs show
a moving dot and a numeric live value.
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
Noise, sample-and-hold, envelope and formula show the last eight seconds of actual
output with the current value at the right edge. Sample-and-hold uses a stepped
trace, and the envelope also names its current stage. History is collected on the
message thread in a fixed 256-sample buffer. Switching scene, source, shape or
output, editing the reference parameters, and stale/invalid data reset the trace.
An editor that resumes after a pause discards its old pending snapshot.

The step sequencer highlights the currently playing step and shows a moving
position/output dot over its editable bars. Loop bounds, shuffle and rate-zero
scrubbing use the engine's playback state. Its expensive simulated preview runs
only when live display is disabled. Step and trigger editing remain available.

MSEG shows one pass over its custom curve with the actual position/output dot.
The position follows segment time, loop playback and gated release. Completed
one-shot and release traversals hold at the endpoint; a gated loop with no
release segment holds its last evaluated position. The cached curve precedes the live
LFO envelope and amplitude. Stochastic MSEG segments are a reference, so the
actual dot can differ. Clicking the preview still opens the MSEG/formula editor;
the live toggle is added to their existing menus and the step-value menu.

All views sample at about 30 Hz. Fast changes can be missed or alias visually;
these views are intended mainly for slow modulation. They show the selected
output of the newest held/releasing voice, rather than every voice at once.

## Implementation

The editor requests one selected source at about 30 Hz. After processing a block,
the audio thread reads its existing phase/output and publishes one packed,
lock-free atomic snapshot, tagged with scene, LFO, output index and shape. No voice
pointers cross threads. Sampling does not process the LFO, allocate or take a lock.
When the editor is inactive there is one relaxed atomic load per audio block.
The reference curve is rebuilt only when parameters or the display change.

This avoids repeatedly regenerating the simulated waveform discussed in
[issue #7895](https://github.com/surge-synthesizer/surge/issues/7895) and the
broader realtime waveform request in
[issue #139](https://github.com/surge-synthesizer/surge/issues/139).

## Checks

Built the ARM64 Release standalone and VST3 on macOS, with plugin copying disabled.
The personal VST3 is installed as
`~/Library/Audio/Plug-Ins/VST3/mySurgeXT.vst3`, with bundle identifier
`org.bielc.mysurgext`, manufacturer code `Biel` and plugin code `Msur`.
These compiled identities let hosts distinguish it from official Surge XT.
Its ad-hoc signature and installed binary hash were verified. Only the VST3 was
installed; existing standalone apps and official plugins were preserved.
Tracktion pluginval 1.0.4 passed at strictness 5, including editor opening during
processing, automation and state restoration, at 44.1/48 kHz and 64/512-sample
blocks. Loading it in Ableton remains a manual check after rescanning plugins.
This build is ARM64 and requires a native Apple Silicon host.

The modulation suite passed all 21 test cases (about 1.49 million assertions).
The parameter and LFO display checks passed 20,936 assertions in six cases,
including all shapes, stale shape requests, step loops/scrubbing, MSEG endpoints
and byte-identical audio with display requests on/off for 512 blocks per shape.
History and artwork-cache checks passed 40 assertions in four cases, including
joined/stepped trace geometry, bounded history and stale sample resets.
The GUI was checked with a slow 0.125 Hz LFO, including moving LFO/parameter dots,
sine-to-triangle selection, cutoff changes and modulation editing. New native
checks confirmed noise and sample-and-hold traces, envelope output/stage text,
formula output, moving step markers with editable step values, MSEG playback
position, and access to both formula and MSEG editors. The optimized standalone
also opens with the Purple skin. Native automation could not confirm
the context-menu toggle or interactive zoom, so those remain manual checks.
Pre-commit could not run because upstream has no `.pre-commit-config.yaml`;
C++ formatting and `git diff --check` passed.

A local Catch2 microbenchmark (30 samples) measured about 3.3 ns for the
no-request path and 26 ns for a selected voice LFO with 16 voices. Those figures
measure snapshot sampling only. The native editor comparison found much larger
rendering costs, and a shared immutable-artwork raster cache now reduces repeated
SVG painting without changing the audio engine or the display refresh rate.
The measured process CPU fell by about 25% for one voice and 22% for 16 voices
relative to the previous prototype. The 64-voice case did not improve, and peak
process memory rose. See [the performance notes](Live%20modulation%20performance.md)
for the comparison and its limits.

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_LTO=OFF -DSURGE_COPY_AFTER_BUILD=OFF
cmake --build build --target surge-xt_Standalone surge-xt_VST3 surge-testrunner --parallel 8
build/src/surge-testrunner/surge-testrunner '[mod]'
build/src/surge-testrunner/surge-testrunner '[lfo-display-perf]' --benchmark-samples 30
```

To reproduce the separately identified personal VST3 (defaults retain official
Surge XT naming and identity):

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_LTO=OFF -DSURGE_COPY_AFTER_BUILD=OFF \
  -DSURGE_XT_PRODUCT_NAME=mySurgeXT -DSURGE_XT_BUNDLE_ID=org.bielc.mysurgext \
  -DSURGE_XT_MANUFACTURER_CODE=Biel -DSURGE_XT_PLUGIN_CODE=Msur
cmake --build build --target surge-xt_VST3 --parallel 8
```

The personal plugin shares Surge's patch library and preferences. Ableton projects
using official Surge XT continue to refer to that plugin; load mySurgeXT as a new
instrument and use saved Surge patches to transfer sounds.
