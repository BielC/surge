# Live modulation rendering performance

On an Apple M4 Max (14 CPU cores, 36 GiB RAM), caching immutable skin artwork
reduced process CPU in the single-voice and 16-voice native editor tests by 25.4%
and 21.8% relative to the previous live-display prototype. Both live displays
still refresh at about 30 Hz. This optimization changes only UI rendering.

| Workload | Vanilla | Previous prototype | Optimized |
|---|---:|---:|---:|
| Idle, 0 voices / 0 routes | 4.18% | 4.01% | 4.31% |
| 1 voice / 8 routes | 15.51% | 43.90% | 32.73% |
| 16 voices / 8 routes | 22.66% | 47.14% | 36.86% |
| 64 voices / 32 routes | 44.04% | 57.68% | 59.96% |

These are medians of three runs per variant, expressed as a percentage of **one
CPU core**. 100% is one full core, not the entire machine. The optimized version
still costs more than vanilla. The 64-voice median increased by 4.0% relative to
the previous prototype, so this is not a universal performance improvement.
Its optimized trials ranged from 44.64% to 60.15%; background/scheduler variation
was substantial. Idle also had an optimized 9.09% outlier; all trials were kept.

The host embeds the actual processor and visible native editor at 48 kHz with
256-sample callbacks, a deterministic Classic saw/LP12 patch and a 0.125 Hz LFO.
Each case has 2 seconds of warmup and 8 measured seconds. Cases reverse order in
repeat 1. Variant order rotates across repeats. Development and diagnostic runs
occurred between the previous and optimized trials in repeat 0. Builds use ARM64
Release, `-O3 -DNDEBUG`, with LTO off. Background apps remained running.

The native window can merge repaint regions from live markers and redraw many
neighbouring controls. `SurgeImage` now rasterizes immutable SVG artwork at the
current physical scale and reuses it. It keeps mutable or transformed source
drawables, existing raster images and allocations above 4,194,304 pixels on their
original path. Scale/zoom changes replace the cache. LFO-only repaint clips also
skip the type selector when it lies outside the clip. Control state remains live.

The tradeoff is memory. Median process peak RSS was about 190-205 MiB previously
and 231-251 MiB with caching. This is a process high-water mark shared across
cases in a run, not a per-patch allocation measurement.

The cache tests passed 21 assertions in two cases, the LFO/parameter-display tests
passed 101 assertions in two cases, and the modulation suite passed 17 cases.
Standalone and VST3 Release builds passed. The first second of deterministic
stereo float output for each of the four workloads was byte-identical to vanilla
and the previous prototype, with both display publishers requested in the new
build. This does not prove equivalence for every patch. Pre-commit was attempted
but upstream has no root configuration; formatting and `git diff --check` passed.

The UI was checked in Classic and Purple skins, including moving markers,
cutoff changes, waveform changes and modulation-edit colours. Interactive zoom
and context-menu automation remained inconclusive; scale/cache invalidation has
unit-test coverage. No installed plugin was replaced.

This is a synthetic host with a sleeping worker, not an Ableton realtime dropout
test. CPU excludes WindowServer/GPU work, and n=3 short desktop runs do not
establish a universal gain. Vanilla source is `f21a8046129edf5e2510772afa3cabc161a9c6d4`;
the previous prototype is `886ef9419fdaaf39f65c8131936a40d68b952cd4`.
Local raw trials, hashes, harness, charts, PCM checks and screenshots are preserved
under `surge-performance/2026-10-25/optimization/` alongside this checkout.

## Additional LFO shapes

The later extension covers all ten LFO shapes. Noise, sample-and-hold, envelope
and formula use a bounded 256-sample history on the message thread. The step
sequencer skips the simulated preview in live mode; MSEG caches one traversal.
The audio thread reads existing playback state without advancing the modulator
or allocating memory. Audio with display requests on/off is byte-identical for
512 blocks per shape in the new regression check.

The table above measured a periodic sine LFO before this extension. Its CPU
percentages have not been remeasured for the additional shapes. Formula scripts
and MSEG curves can have different DSP costs, so those percentages should not
be extrapolated to every waveform or patch.
