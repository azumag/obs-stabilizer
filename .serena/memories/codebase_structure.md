# OBS Stabilizer Codebase Structure

## Root Directory (11 essential files)
- `CMakeLists.txt` - Main build configuration
- `README.md` - Project documentation
- `CLAUDE.md` - Technical specifications
- `AGENTS.md` - Agent workflow and issue tracking
- `GEMINI.md` - Japanese development guidelines
- `IMPLEMENTATION_GUIDE.md` - Implementation guide
- `CONTRIBUTING.md` - Contribution guidelines
- `LICENSE` - GPL-2.0 license
- `STATUS.md` - Project status
- `opencode.json` - Opencode configuration

## Directory Structure
```
obs-stabilizer/
├── src/                    # Source code
│   ├── core/             # Core stabilization components (20 files)
│   │   ├── adaptive_stabilizer.cpp/.hpp
│   │   ├── benchmark.cpp/.hpp
│   │   ├── frame_utils.cpp/.hpp
│   │   ├── logging.hpp
│   │   ├── motion_classifier.cpp/.hpp
│   │   ├── neon_feature_detection.cpp/.hpp
│   │   ├── performance_regression.cpp/.hpp
│   │   ├── stabilizer_constants.hpp
│   │   ├── stabilizer_core.cpp/.hpp
│   │   └── stabilizer_wrapper.cpp/.hpp
│   ├── plugin-support.c    # Symbol bridge for OBS API
│   ├── plugin-support.h    # Bridge header
│   └── stabilizer_opencv.cpp    # OBS filter implementation and entry point
├── docs/                   # Documentation (38 files)
│   ├── architecture.md     # System architecture
│   ├── testing/           # Testing documentation
│   ├── history/           # Historical documentation
│   ├── performance-testing-*.md
│   ├── motion-classifier-*.md
│   └── ...
├── scripts/                # Build and test scripts (7 files)
│   ├── run-tests.sh       # Main test runner
│   ├── run-perf-benchmark.sh  # Performance benchmarks
│   ├── quick-perf.sh      # Quick performance validation
│   ├── run-perf-regression.sh  # Regression detection
│   ├── test-core-only.sh  # Core compilation test
│   ├── bundle_opencv.sh  # OpenCV bundling script
│   └── fix-plugin-loading.sh  # macOS plugin loading fix
├── tools/                  # Development tools (1 file)
│   └── performance_benchmark.cpp  # Modern benchmark executable
├── security/               # Security audit tools
│   └── security-audit.sh  # Security audit script
├── tests/                  # Test infrastructure (7 files)
│   ├── test_adaptive_stabilizer.cpp
│   ├── test_basic.cpp
│   ├── test_data_generator.cpp/.hpp
│   ├── test_motion_classifier.cpp
│   ├── test_stabilizer_core.cpp
│   └── test_constants.hpp
├── cmake/                  # CMake configuration (3 files)
│   ├── BundledOpenCV.cmake
│   ├── Info.plist.in
│   ├── Info.plist.minimal
│   └── verify-bundled-libs.sh.in
├── .github/               # GitHub workflows
│   ├── workflows/          # CI/CD workflows
│   ├── actions/            # GitHub actions
│   └── ISSUE_TEMPLATE/     # Issue templates
└── .serena/               # Serena project config
    └── memories/          # Serena memories
```

## Key Components

1. **Plugin System**: OBS module interface with proper symbol exports
   - Entry point: `src/stabilizer_opencv.cpp`
   - OBS symbol bridge: `src/plugin-support.c/h`

2. **Stabilization Core**: OpenCV-based Point Feature Matching
   - Core engine: `src/core/stabilizer_core.cpp/hpp`
   - Adaptive stabilization: `src/core/adaptive_stabilizer.cpp/hpp`
   - Motion classifier: `src/core/motion_classifier.cpp/hpp`

3. **Build System**: Simple CMake with OBS framework detection
   - Direct `find_package(OpenCV)` without deployment strategies
   - No presets or complex configurations
   - Simple workflow: `cmake -B build && cmake --build build`

4. **Testing**: Google Test framework (71 tests)
   - Test executable: `build/stabilizer_tests`
   - Test files in `tests/` directory
   - Performance benchmarks in `src/core/` and `tools/`

5. **Logging**: Unified logging interface
   - OBS logging when headers available: `src/core/logging.hpp`
   - Console output for standalone builds

## Codebase Statistics

- **Source Code**: 4,541 lines (21 files: 9 .cpp, 10 .hpp, 1 .c, 1 .h)
- **Test Code**: 1,347 lines (7 files: 5 .cpp, 2 .hpp)
- **Tools**: 324 lines (2 files: 2 .cpp)
- **Documentation**: 9,750 lines (38 files)
- **Total Code**: 6,212 lines (src + tests + tools)

## Development Philosophy

- **YAGNI**: Simple, minimal features - no deployment strategies or complex presets
- **DRY**: Single source of truth for build system
- **KISS**: Direct CMake workflow without abstraction layers
- **TDD**: Comprehensive test suite with 100% pass rate
