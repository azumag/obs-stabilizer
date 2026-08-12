# OBS Stabilizer

[![Build](https://github.com/azumag/obs-stabilizer/actions/workflows/build.yml/badge.svg)](https://github.com/azumag/obs-stabilizer/actions/workflows/build.yml)
[![Quality Assurance](https://github.com/azumag/obs-stabilizer/actions/workflows/qa.yml/badge.svg)](https://github.com/azumag/obs-stabilizer/actions/workflows/qa.yml)
[![License: GPL-2.0](https://img.shields.io/badge/License-GPL--2.0-blue.svg)](LICENSE)

Real-time video stabilization filter plugin for OBS Studio, powered by OpenCV feature tracking.

## Features

- Point Feature Matching with Lucas-Kanade optical flow for real-time motion estimation
- Two trajectory smoothing strategies: windowed moving average and a constant-velocity Kalman filter
- Gaming, Streaming, and Recording presets, plus savable/loadable custom parameter sets
- Three edge-handling strategies for the borders stabilization exposes: black padding, crop, and scale-to-fit
- NV12 and I420 frame format support
- Windows, macOS (Apple Silicon native, no Qt dependency), and Linux

## Example Results

Measured on macOS 26.6.1, OBS Studio 31.1.2 (arm64), source at 640x360, 30 fps.
Four synthetic hand-held camera samples are generated from the bundled test
pattern with `docs/examples/generate_shake_samples.py`:

| Sample | Motion model | Filter settings |
|---|---|---|
| `fine-shake` | 2 px, 9 Hz micro-jitter | Streaming preset, smoothing 30, max correction 30%, edge padding |
| `large-shake` | 22 px, 3 Hz camera sway | Streaming preset, smoothing 30, max correction 30%, edge padding |
| `mixed-shake` | 10 px, 7 Hz jitter + 0.5 Hz sway | Streaming preset, smoothing 30, max correction 30%, edge padding |
| `pan-shake` | 80 px intentional pan and return + 2 px jitter | Streaming preset, smoothing 30, max correction 30%, edge padding |

Each sample is recorded twice in OBS: once with the filter disabled and once
with `Video Stabilizer` enabled. `docs/examples/measure_motion_v2.py` then
measures frame-to-frame camera motion with optical flow. The median is used
instead of the RMS because a small number of bad optical-flow tracks can
dominate the RMS; the median is stable and still drops sharply when jitter is
removed.

### Before / after videos

The animated comparisons below show the same 4-second segment with the filter
disabled on the left and `Video Stabilizer` enabled on the right. The shaking
edges of the grid and the outlined rectangles are visibly steadier on the
right.

The GIFs are regenerated from fresh OBS recordings with
`docs/examples/make_comparison_gifs.sh` (pass the six recordings as
baseline/filtered pairs for fine, large, and mixed in that order).
The motion chart uses the same six arguments with
`docs/examples/make_motion_chart.py`.

![fine-shake: no filter vs stabilizer](docs/examples/fine-comparison.gif)

![large-shake: no filter vs stabilizer](docs/examples/large-comparison.gif)

![mixed-shake: no filter vs stabilizer](docs/examples/mixed-comparison.gif)

![Frame-to-frame motion with and without the stabilizer](docs/examples/motion-comparison.png)

| Sample | Frame-to-frame motion (median) | Reduction | Pixel diff (mean) | Reduction |
|---|---|---:|---:|---:|
| `fine-shake` | 1.26 px -> 0.11 px | 91% | 8.9 -> 2.0 | 78% |
| `large-shake` | 7.96 px -> 0.77 px | 90% | 23.1 -> 8.1 | 65% |
| `mixed-shake` | 5.02 px -> 0.46 px | 91% | 20.3 -> 5.4 | 73% |
| `pan-shake` | 3.24 px -> 0.08 px | 98% | 18.4 -> 3.6 | 81% |

The same mixed sample rendered at 1920x1080 measures 5.06 px -> 0.47 px (91%)
with per-frame processing between 10 and 16 ms, inside the 33 ms budget of
30 fps. On the pan sample the filter follows the intentional 80 px camera move
(low-frequency trajectory correlation 0.98, amplitude ratio 0.99), settles on
the pan target within 0.4 px, and returns to the origin without drift or
pull-back.

Notes on reading these numbers:

- The stabilizer removes a substantial share of the high-frequency jitter while
  preserving low-frequency sway and intentional pans, which is the desired
  behavior for a moving camera, so the remaining frame-to-frame motion and
  pixel difference are not zero.
- `large-shake` has a strong low-frequency component, so a larger share of its
  motion is intentionally kept; its high-frequency content is still reduced.
- Use the same measurement script on your own footage: record the same source
  with the filter disabled and enabled, then run
  `python3 docs/examples/measure_motion_v2.py baseline.mov filtered.mov`.

## Requirements

- OBS Studio 30.0 or newer
- CMake 3.16+ and a C++17 compiler
- OpenCV 4.5+ with the `core imgproc video calib3d features2d flann` components
- GoogleTest (required at configure time)

## Installation

Build from source (see below), then install the plugin binary for your platform.

**macOS**: bundle OpenCV into the plugin and copy it into place:

```bash
./scripts/bundle_opencv.sh build/obs-stabilizer.plugin
mkdir -p ~/Library/Application\ Support/obs-studio/plugins
cp -R build/obs-stabilizer.plugin ~/Library/Application\ Support/obs-studio/plugins/
```

See [docs/MACOS_BUILD_INSTALL.md](docs/MACOS_BUILD_INSTALL.md) for the full
Apple Silicon build, code-signing, and system-vs-bundled OpenCV flow.

**Linux**:

```bash
cp build/obs-stabilizer-opencv.so ~/.config/obs-studio/plugins/
```

**Windows**:

```bash
copy build\Release\obs-stabilizer-opencv.dll %APPDATA%\obs-studio\plugins\
```

Restart OBS Studio; **Video Stabilizer** should then appear in the source
filter list.

## Building from source

```bash
git clone https://github.com/azumag/obs-stabilizer.git
cd obs-stabilizer
cmake -S . -B build
cmake --build build
```

macOS/Ubuntu need `cmake`, `pkg-config`, `opencv`, and `googletest` installed
beforehand (`brew install cmake pkg-config opencv googletest` / `sudo apt
install cmake pkg-config libopencv-dev libgtest-dev`); Windows can use vcpkg.
On macOS the build fetches OBS's own headers automatically over the network
during configure. If the build fails to find OBS, OpenCV, or GTest, see the
build troubleshooting section in
[docs/DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md).

## Usage

1. Add or select a video source in OBS.
2. Open **Filters** for that source.
3. Add **Video Stabilizer** as an effect filter.
4. Choose a preset: **Streaming** for general use, **Gaming** when latency
   matters most, **Recording** when stronger smoothing is acceptable.
5. Enable stabilization and adjust settings while watching the preview.

See [docs/USER_GUIDE.md](docs/USER_GUIDE.md) for scene recommendations,
troubleshooting, and an FAQ.

## Configuration

| Parameter | Range | Description |
|---|---|---|
| Enable Stabilization | on/off | Toggles processing without removing the filter |
| Preset | Gaming / Streaming / Recording / Custom | Applies a bundled parameter set, or keeps manual values |
| Smoothing Radius | 5-200 | Number of recent transforms averaged for motion smoothing |
| Max Correction | 1-100% | Upper bound on how far a frame may be shifted or transformed |
| Feature Count | 50-2000 | Target number of tracked visual points |
| Quality Level | 0.001-0.1 | Minimum accepted corner quality |
| Min Distance | 1-200 | Minimum pixel spacing between detected features |
| Block Size | 3-31 | Neighborhood size used by corner detection |
| Edge Handling | Padding / Crop / Scale | How exposed borders from stabilization are handled |
| Smoothing Mode | Moving Average / Kalman | Trajectory smoothing strategy |
| Use Harris Detector, Harris K | on/off, 0.01-0.1 | Optional Harris corner scoring and its sensitivity |
| Debug Mode | on/off | Additional diagnostic logging in the OBS log |

Edge handling trade-offs:

- **Black Padding**: no extra processing cost; may expose black borders.
- **Crop Borders**: removes unstable edges; slightly reduces the visible frame.
- **Scale to Fit**: fills the frame; large corrections can introduce mild distortion.

Full parameter descriptions live in [docs/USER_GUIDE.md](docs/USER_GUIDE.md#parameters).

## Performance

Stabilization runs on the CPU. On the mixed-shake sample above, 1920x1080
processing measures 10-16 ms per frame, within the 33 ms budget of 30 fps (see
[Example Results](#example-results)). If a scene is CPU-bound, reduce feature
count, smoothing radius, or source resolution first.

## Documentation

- [docs/USER_GUIDE.md](docs/USER_GUIDE.md) — installation, usage, parameters, troubleshooting, FAQ
- [docs/DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md) — architecture, build, testing, and PR checklist
- [docs/CURRENT_ARCHITECTURE.md](docs/CURRENT_ARCHITECTURE.md) — module boundaries and data flow
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) / [docs/ARCHITECTURE_DECISIONS.md](docs/ARCHITECTURE_DECISIONS.md) — design and decision records
- [docs/STABILIZATION_ALGORITHM.md](docs/STABILIZATION_ALGORITHM.md) — the stabilization algorithm
- [docs/API.md](docs/API.md) — internal C++ API reference
- [docs/MACOS_BUILD_INSTALL.md](docs/MACOS_BUILD_INSTALL.md) — macOS build and distribution
- [docs/performance-testing-guide.md](docs/performance-testing-guide.md) — performance test methodology
- [docs/testing/](docs/testing/) — end-to-end and integration test procedures

## Development

```bash
cmake -S . -B build
cmake --build build
./build/stabilizer_tests
```

`scripts/quick-perf.sh` and `scripts/run-perf-benchmark.sh` run performance
checks; `security/security-audit.sh` runs the security audit. See
[docs/DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md) for the full architecture,
testing strategy, and pull request checklist.

## Contributing

Contributions are welcome — see [CONTRIBUTING.md](CONTRIBUTING.md). When
reporting an issue, include the OS, OBS version, source resolution, preset,
and a reproducible sequence.

## Acknowledgments

This project was inspired by the [LiveVisionKit](https://github.com/Crowsinc/LiveVisionKit) plugin, which is no longer actively maintained. OBS Stabilizer aims to provide a modern, maintainable alternative with improved performance and user experience.

## License

GPL-2.0, compatible with OBS Studio. See [LICENSE](LICENSE).
