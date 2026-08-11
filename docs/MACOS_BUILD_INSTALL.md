# macOS build and installation

This is the supported local build flow for OBS Stabilizer on macOS, including Apple Silicon.

## Prerequisites

Install OBS Studio and the build dependencies:

```bash
brew install --cask obs
brew install cmake ninja pkg-config opencv googletest nlohmann-json
xcode-select --install
```

If your Homebrew setup provides `opencv@4` instead of `opencv`, use that formula name in the commands below.

## Build

From the repository root:

```bash
OPENCV_PREFIX="$(brew --prefix opencv)"
cmake -S . -B build -G Ninja \
  -DOpenCV_DIR="$OPENCV_PREFIX/lib/cmake/opencv4"
cmake --build build --target obs-stabilizer-opencv
```

CMake produces a complete plugin structure:

```text
build/obs-stabilizer.plugin/
  Contents/
    Info.plist
    MacOS/
      obs-stabilizer
    Resources/
```

The build output can use the local Homebrew OpenCV installation during development:

```bash
./scripts/fix-plugin-loading.sh build/obs-stabilizer.plugin
```

## Create a redistributable bundle

For CI artifacts, releases, or another Mac, bundle OpenCV and its non-system transitive dependencies:

```bash
./scripts/bundle_opencv.sh build/obs-stabilizer.plugin
```

The command copies dependencies into `Contents/Frameworks`, rewrites load paths to relative references, removes Homebrew RPATHs, ad-hoc signs the bundle, and verifies its signature. The resulting `.plugin` does not require Homebrew OpenCV on the target Mac.

## Install

Move any older OBS Stabilizer `.plugin` or loose `.so` installation aside first so OBS does not discover two copies. Then install the completed bundle:

```bash
mkdir -p "$HOME/Library/Application Support/obs-studio/plugins"
cp -R build/obs-stabilizer.plugin \
  "$HOME/Library/Application Support/obs-studio/plugins/"
```

Restart OBS, open a source's **Filters** dialog, and add OBS Stabilizer.

## Legacy raw module compatibility

Older build directories may contain `obs-stabilizer-opencv.so` instead of a `.plugin` directory. The repair script can wrap that module without overwriting an existing bundle:

```bash
OBS_STABILIZER_BUNDLE_PATH="$PWD/dist/obs-stabilizer.plugin" \
  ./scripts/fix-plugin-loading.sh build/obs-stabilizer-opencv.so
./scripts/bundle_opencv.sh dist/obs-stabilizer.plugin
```

## Verification

```bash
plutil -lint build/obs-stabilizer.plugin/Contents/Info.plist
codesign --verify --deep --strict --verbose=2 build/obs-stabilizer.plugin
otool -L build/obs-stabilizer.plugin/Contents/MacOS/obs-stabilizer
```

The bundled OpenCV references must use `@loader_path`; `@executable_path` points at the OBS application rather than the loaded plugin and must not remain in the distributable bundle.
