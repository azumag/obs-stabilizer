# OBS Stabilizer Codebase Structure

## Root Directory (9 essential files)
- `CMakeLists.txt` - Main build configuration
- `CMakeLists-minimal.txt` - Minimal build config
- `README.md` - Project documentation
- `CLAUDE.md` - Technical specifications
- `LICENSE` - GPL-2.0 license
- `CONTRIBUTING.md` - Contribution guidelines
- `.gitignore` - Git ignore rules
- `Info.plist.in` - macOS plugin template
- `Info-minimal.plist.in` - Minimal plugin template

## Directory Structure
```
obs-stabilizer/
├── src/                    # Source code
│   ├── plugin_main.cpp     # Plugin entry point
│   ├── obs_plugin.cpp      # OBS filter implementation
│   ├── plugin-support.c    # Symbol bridge for OBS API
│   ├── plugin-support.h    # Bridge header
│   ├── stabilizer.cpp      # Core stabilization algorithm
│   ├── stabilizer.h        # Core header
│   ├── minimal_plugin_main.cpp  # Minimal test plugin
│   ├── performance-test.cpp     # Performance benchmarks
│   ├── memory-test.cpp          # Memory leak tests
│   └── tests/              # Unit tests
├── docs/                   # Documentation
│   ├── architecture.md     # System architecture
│   ├── plugin-loading-*    # Troubleshooting docs
│   └── ...
├── scripts/                # Build and test scripts
│   ├── run-tests.sh       # Main test runner
│   ├── run-perftest.sh    # Performance tests
│   └── fix-plugin-loading.sh  # macOS fix
├── security/               # Security audit tools
├── tests/                  # Test infrastructure
├── data/                   # Plugin resources
│   └── locale/            # Localization files
├── include/               # OBS headers (local copy)
├── tmp/                   # Temporary files
├── .github/               # GitHub workflows
└── .serena/               # Serena project config
```

## Key Components
1. **Plugin System**: OBS module interface with proper symbol exports
2. **Stabilization Core**: OpenCV-based Point Feature Matching
3. **Build System**: CMake with OBS framework detection
4. **Testing**: Google Test framework + custom test scripts
5. **Symbol Bridge**: Compatibility layer for OBS API differences