# macOS build and installation

This guide describes the supported local build flow for OBS Stabilizer on macOS, including Apple Silicon systems.

## Prerequisites

Install OBS Studio and the build dependencies first:

```bash
brew install --cask obs
brew install cmake ninja pkg-config opencv@4 googletest nlohmann-json
xcode-select --install
```

The project currently targets OpenCV 4 for the macOS build. Point CMake at the Homebrew OpenCV 4 package explicitly so a separately installed OpenCV 5 package is not selected:

```bash
export OpenCV_DIR="$(brew --prefix opencv@4)/lib/cmake/opencv4"
```

## Build

From the repository root:

```bash
cmake -S . -B build -G Ninja \
  -DOpenCV_DIR="$OpenCV_DIR"
cmake --build build --target obs-stabilizer-opencv
```

The raw CMake module is normally produced as:

```text
build/obs-stabilizer-opencv.so
```

A raw `.so` is an intermediate build product. For OBS on macOS, convert it to the plugin bundle before installation.

## Create the `.plugin` bundle

Run:

```bash
./scripts/fix-plugin-loading.sh build/obs-stabilizer-opencv.so
```

The script creates:

```text
build/obs-stabilizer.plugin/
└── Contents/
    ├── Info.plist
    └── MacOS/
        └── obs-stabilizer
```

It also normalizes Homebrew OpenCV references to `@rpath`, adds available OpenCV/OBS runtime search paths, validates the plist, ad-hoc signs the complete bundle, and verifies the signature.

If a `.plugin` bundle already exists, you can pass the bundle path directly:

```bash
./scripts/fix-plugin-loading.sh build/obs-stabilizer.plugin
```

To place the generated bundle somewhere else, set `OBS_STABILIZER_BUNDLE_PATH`:

```bash
OBS_STABILIZER_BUNDLE_PATH="$PWD/dist/obs-stabilizer.plugin" \
  ./scripts/fix-plugin-loading.sh build/obs-stabilizer-opencv.so
```

## Install

The script prints the exact installation commands. The user-local installation is:

```bash
mkdir -p "$HOME/Library/Application Support/obs-studio/plugins"
rm -rf "$HOME/Library/Application Support/obs-studio/plugins/obs-stabilizer.plugin"
cp -R build/obs-stabilizer.plugin \
  "$HOME/Library/Application Support/obs-studio/plugins/"
```

Restart OBS after installing the bundle, open a source, then open **Filters** and add OBS Stabilizer.

## OpenCV dependency note

The generated `.plugin` is a structurally complete macOS plugin bundle, but the current dynamic build still expects the matching Homebrew OpenCV libraries to be installed on the machine. The script supports both `opencv@4` and legacy `opencv` Homebrew prefixes.

For a redistributable bundle that does not depend on a local Homebrew OpenCV installation, the OpenCV dylibs and their transitive dependencies must be packaged and re-signed as a separate deployment step.

## Troubleshooting

If CMake selects OpenCV 5, remove the build directory and configure again with `OpenCV_DIR` pointing at `opencv@4`:

```bash
rm -rf build
export OpenCV_DIR="$(brew --prefix opencv@4)/lib/cmake/opencv4"
cmake -S . -B build -G Ninja -DOpenCV_DIR="$OpenCV_DIR"
```

If the script reports a missing Xcode command, install or refresh the Command Line Tools:

```bash
xcode-select --install
```

If OBS does not list the filter, inspect the plugin bundle and signature:

```bash
plutil -lint build/obs-stabilizer.plugin/Contents/Info.plist
codesign --verify --deep --strict --verbose=2 build/obs-stabilizer.plugin
otool -L build/obs-stabilizer.plugin/Contents/MacOS/obs-stabilizer
```
