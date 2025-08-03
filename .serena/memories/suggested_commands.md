# Suggested Commands for OBS Stabilizer Development

## Build Commands
```bash
# Simple build (recommended)
cmake -B build
cmake --build build

# Clean and rebuild
rm -rf build
cmake -B build
cmake --build build

# Debug build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# macOS: Fix plugin loading after build
./scripts/fix-plugin-loading.sh
```

## Test Commands
```bash
# Run comprehensive test suite
./scripts/run-tests.sh

# Run core compilation test only (no dependencies)
./scripts/test-core-only.sh

# Run performance tests
./scripts/run-perftest.sh

# Run integration tests
./scripts/run-integration-test.sh

# Run UI implementation test
./scripts/run-ui-test.sh

# Run security audit
./security/security-audit.sh
```

## Installation Commands
```bash
# macOS
cp build/test-stabilizer.plugin/Contents/MacOS/test-stabilizer ~/Library/Application\ Support/obs-studio/plugins/test-stabilizer.plugin/Contents/MacOS/

# Linux
cp build/obs-stabilizer.so ~/.config/obs-studio/plugins/

# Windows
copy build\Release\obs-stabilizer.dll %APPDATA%\obs-studio\plugins\
```

## Development Tools
```bash
# Format code (if clang-format available)
find src -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# Check for memory leaks (macOS)
leaks --atExit -- ./build/test-stabilizer

# Profile performance
instruments -t "Time Profiler" ./build/test-stabilizer
```

## Git Commands
```bash
# Check status
git status

# Stage changes
git add -A

# Commit with descriptive message
git commit -m "feat: implement feature X"

# Push to remote
git push origin main
```

## Utility Commands (macOS/Darwin)
```bash
# List files
ls -la

# Find files
find . -name "*.cpp"

# Search in files (use ripgrep)
rg "pattern" --type cpp

# Check library dependencies
otool -L build/test-stabilizer.plugin/Contents/MacOS/test-stabilizer

# Check exported symbols
nm -gU build/test-stabilizer.plugin/Contents/MacOS/test-stabilizer

# Monitor OBS process
ps aux | grep -i obs

# Kill OBS (use carefully)
pkill -f OBS
```