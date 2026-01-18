# Architecture Document

## Project Overview

**OBS Stabilizer Plugin** - A real-time video stabilization plugin for OBS Studio using OpenCV computer vision algorithms.

**Status**: Functional with minor technical debt items to address

## Functional Requirements

### Core Functionality
- **Real-time Video Stabilization**: Apply video stabilization to live streams and recordings in OBS Studio
- **Point Feature Matching**: Use OpenCV's feature detection and tracking algorithms (Point Feature Matching + Lucas-Kanade Optical Flow)
- **Transform Smoothing**: Apply configurable smoothing to stabilization transforms
- **User Interface**: Provide OBS properties panel for configuration
- **Multi-format Support**: Support NV12 and I420 video formats

### User-Configurable Parameters
- **Enable Stabilization**: Main toggle for stabilization processing
- **Smoothing Radius**: Transform smoothing window (10-100 frames)
- **Feature Points**: Number of tracking points (100-1000)
- **Stability Threshold**: Error threshold for tracking quality (10.0-100.0)
- **Edge Handling**: Crop borders/Black padding/Scale to fit options

## Non-Functional Requirements

### Performance Requirements
- **720p**: <2ms/frame (60fps+ capable)
- **1080p**: <4ms/frame (30fps+ capable)
- **1440p**: <8ms/frame
- **4K**: <15ms/frame

### Quality Attributes
- **Reliability**: No crashes or major memory issues during normal operation
- **Maintainability**: Follow YAGNI, KISS, DRY principles
- **Code Quality**: Consistent coding standards, proper error handling
- **Thread Safety**: Thread-safe configuration updates and frame processing
- **Memory Safety**: RAII resource management, bounds checking, buffer overflow protection

### Standards Compliance
- **OBS Plugin Standards**: Use obs_log() for logging (not printf) in production code
- **Cross-Platform**: Support macOS, Windows, Linux
- **Build System**: Use CMake with proper dependency management

## Acceptance Criteria

### Functional Acceptance
- [x] Plugin loads successfully in OBS Studio
- [x] "Stabilizer" filter appears in OBS filters list
- [x] Stabilization processing works correctly
- [x] Configuration UI is accessible and functional
- [x] All performance targets met

### Code Quality Acceptance
- [x] No compiler warnings
- [x] All tests passing
- [x] Code follows project coding standards
- [ ] Logging standardized to obs_log() in production code
- [ ] Build system consolidated to essential files

### Integration Acceptance
- [x] CI/CD pipeline operational
- [x] Cross-platform builds successful
- [x] Plugin installation working on target platforms

## Design Principles

### Core Principles
1. **YAGNI (You Aren't Gonna Need It)**: Implement only what's needed
2. **KISS (Keep It Simple, Stupid)**: Prioritize simplicity and clarity
3. **DRY (Don't Repeat Yourself)**: Eliminate code duplication
4. **TDD (Test-Driven Development)**: Write tests before implementation (when applicable)

### Architectural Guidelines
- Keep components focused and single-purpose
- Use minimal abstractions
- Prefer direct implementation over complex patterns
- Maintain clear separation of concerns

## Current Architecture

### Source File Structure
```
src/
├── plugin_main.cpp              # OBS module entry point
├── obs_plugin.cpp               # Minimal test implementation (no OpenCV)
├── obs_module_exports.c         # C linkage layer for OBS module functions
├── obs_stubs.c                  # OBS stubs for standalone builds
├── plugin-support.c             # OBS API compatibility bridge
├── stabilizer_opencv.cpp        # OpenCV-based stabilizer (main implementation)
├── stabilizer_filter.cpp        # OBS filter integration
├── stabilizer.cpp               # Legacy stabilizer (may be removed)
├── minimal_*.cpp                # Test/standalone implementations
└── tests/
    └── ...                     # Test files
```

### Component Overview

#### 1. OBS Module Layer (plugin_main.cpp, obs_module_exports.c)
- Purpose: OBS module entry point and C linkage
- Exported functions: obs_module_load(), obs_module_unload(), obs_module_name()
- Status: Functional

#### 2. OBS API Bridge (plugin-support.c, obs_stubs.c)
- Purpose: Compatibility layer for OBS API differences
- Functions: obs_register_source(), obs_log()
- Status: Functional

#### 3. Stabilizer Implementation (stabilizer_opencv.cpp)
- Purpose: Core stabilization logic using OpenCV
- Algorithm: Point Feature Matching + Lucas-Kanade Optical Flow
- Status: Functional

#### 4. OBS Filter Integration (stabilizer_filter.cpp, obs_plugin.cpp)
- Purpose: OBS filter registration and callbacks
- Status: obs_plugin.cpp is minimal test, stabilizer_filter.cpp needs verification

## Technical Debt Items

### High Priority

#### 1. Logging Standardization (Issue #168)
**Problem**: Mixed usage of printf() and obs_log() across codebase

**Current State**:
- obs_log: stabilizer_opencv.cpp (correct)
- printf: obs_plugin.cpp, plugin_main.cpp (incorrect for production)
- printf: obs_stubs.c, minimal_*.cpp (acceptable for tests)

**Solution**:
- Replace printf() with obs_log() in obs_plugin.cpp
- Replace printf() with obs_log() in plugin_main.cpp
- Keep printf() in test files (obs_stubs.c, minimal_*.cpp)

**Impact**: Medium effort, Medium impact

#### 2. Build System Consolidation (Issue #169)
**Problem**: Multiple CMakeLists.txt files create maintenance burden

**Current State**:
- CMakeLists.txt (root) - Main build configuration
- src/CMakeLists.txt - Source-specific configuration
- src/tests/CMakeLists.txt - Test configuration
- tmp/tests/CMakeLists.txt - Temporary test configuration (can be removed)

**Solution**:
- Evaluate if src/CMakeLists.txt can be merged into root CMakeLists.txt
- Evaluate if src/tests/CMakeLists.txt can be merged
- Remove tmp/tests/CMakeLists.txt (temporary directory)

**Target**: Consolidate to 1-2 essential CMakeLists.txt files

**Impact**: Medium effort, Medium impact

### Medium Priority

#### 3. Memory Management Audit (Issue #167)
**Problem**: C++ new/delete mixing with OBS C callbacks

**Current State**: Functional but needs formal audit

**Solution**: Review memory management patterns, ensure proper RAII usage

**Impact**: High effort, High impact

#### 4. Deployment Strategy (Issue #171)
**Problem**: OpenCV dependency creates end-user installation complexity

**Current State**: Plugin requires OpenCV runtime libraries

**Solution**: Implement static linking or bundled distribution strategy

**Impact**: High effort, Medium impact

### Low Priority

#### 5. Test Coverage Expansion (Issue #172)
**Problem**: Limited automated testing of stabilization algorithms

**Current State**: Manual testing only for OBS integration

**Solution**: Expand automated test coverage

**Impact**: High effort, Medium impact

## Design Decisions and Trade-offs

### 1. OpenCV Integration
**Decision**: Use OpenCV for computer vision algorithms
**Trade-off**:
- Pro: Powerful, well-tested algorithms
- Con: Creates deployment complexity, large dependency

### 2. C/C++ Hybrid
**Decision**: Use C++ for core logic, C for OBS module interface
**Trade-off**:
- Pro: Proper C linkage for OBS module functions
- Con: Added complexity in memory management

### 3. Minimal Test Implementation
**Decision**: Keep obs_plugin.cpp as minimal test without OpenCV
**Trade-off**:
- Pro: Allows development without OpenCV dependency
- Con: Maintains two implementations

## Implementation Roadmap

### Phase 1: High Priority Items (Current Sprint)
- [ ] Standardize logging to obs_log() in production code
- [ ] Consolidate CMakeLists.txt files

### Phase 2: Medium Priority Items
- [ ] Complete memory management audit
- [ ] Design deployment strategy

### Phase 3: Low Priority Items
- [ ] Expand test coverage
- [ ] Performance optimization

## Metrics and Success Criteria

### Code Quality Metrics
- **Total Lines of Code**: Currently ~2,277 lines (reasonable)
- **Complexity**: Maintain cyclomatic complexity < 10
- **Test Coverage**: Increase from current level (target to be defined)

### Performance Metrics
- **720p Processing Time**: <2ms/frame ✅
- **1080p Processing Time**: <4ms/frame ✅
- **Memory Usage**: Stable over long periods ✅

### Build Metrics
- **CMakeLists.txt Files**: Target 1-2 files (current: 3-4)
- **Build Time**: <5 minutes on CI

## Dependencies

### External Dependencies
- **OBS Studio**: 30.0+
- **OpenCV**: 4.5+ (with 5.x experimental support)
- **CMake**: 3.16+
- **C++17**: Compatible compiler

### Internal Dependencies
- Core logic depends on OpenCV
- OBS integration depends on OBS headers
- Tests depend on Google Test framework

## Future Considerations

### Potential Improvements
- GPU acceleration for OpenCV operations
- Enhanced algorithms (rolling shutter correction, etc.)
- Preset system for different use cases
- Advanced diagnostic and monitoring tools

### Scalability
- Multi-threaded processing for higher resolutions
- Adaptive algorithms based on camera movement
- Machine learning-based stabilization

## Related Documentation

- **README.md**: Project overview and build instructions
- **CLAUDE.md**: Development workflow and principles
- **docs/architecture.md**: Detailed technical architecture
- **docs/ARCHITECTURE_SIMPLIFICATION.md**: Simplification plan
- **docs/TDD_METHODOLOGY.md**: Test-driven development guidelines

## Change History

| Date | Version | Author | Description |
|------|---------|--------|-------------|
| 2025-08-04 | 1.0 | azumag | Initial architecture documentation |
| 2025-08-04 | 1.1 | azumag | Updated with Issue #173 technical debt items |
| 2026-01-18 | 1.2 | azumag | Added tmp directory cleanup design (Issue #166) |

---

# Issue #166: tmp Directory Cleanup Design

## Overview

This section outlines the architecture and implementation strategy for cleaning up the `tmp/` directory to resolve issue #166: "[CRITICAL CLEANUP] tmp directory cleanup - 1,482 files (64MB) violating CLAUDE.md principles".

## Current State Analysis

### Current Directory Structure

```
tmp/
├── build/                      # CMake build artifacts + plugin binary
├── scripts/                     # 10 test/cleanup scripts
├── tests/                       # Test source files + CMake artifacts
├── PLUGIN_LOADING_SOLUTION.md
├── plugin_test_framework_design.md
├── PLUGIN_TEST_FRAMEWORK_GUIDE.md
├── README.md
└── TEST_FRAMEWORK_IMPLEMENTATION_REPORT.md
```

### Current Metrics
- **Total files**: 124 files
- **Total size**: 1.3MB
- **Markdown documentation**: 5 files
- **Shell scripts**: 10 files
- **Test source files**: 7 files
- **CMake build artifacts**: 3 CMakeFiles directories + associated files

### Problems Identified

1. **Documentation scattered**: 5 markdown files in tmp/ instead of docs/
2. **Duplicate scripts**: tmp/scripts/ contains scripts that duplicate functionality in scripts/
3. **Build artifacts**: CMakeFiles and intermediate build files in tmp/
4. **Test files in wrong location**: Test source files in tmp/tests/ instead of tests/
5. **CLAUDE.md violation**: Violates "一時ファイルは一箇所のディレクトリにまとめよ" principle

## Functional Requirements

### FR-1: Documentation Organization
- Move all markdown files from tmp/ to docs/ directory
- Ensure documentation is properly integrated with existing docs/

### FR-2: Script Consolidation
- Remove duplicate scripts from tmp/scripts/
- Preserve any unique functionality from tmp/scripts/
- Ensure all scripts follow project conventions

### FR-3: Build Artifact Cleanup
- Remove all CMakeFiles directories from tmp/
- Remove intermediate build artifacts (.ninja_*, cmake_install.cmake, etc.)
- Preserve actual plugin binaries needed for testing

### FR-4: Test File Organization
- Move test source files from tmp/tests/ to tests/ directory
- Preserve test functionality while removing build artifacts

### FR-5: Directory Compliance
- Reduce tmp/ to <50 files
- Reduce tmp/ to <10MB
- Ensure tmp/ only contains legitimate temporary files

## Non-Functional Requirements

### NFR-1: Performance
- Cleanup operation must complete in <5 seconds
- No impact on build system functionality

### NFR-2: Compatibility
- Preserve all existing functionality
- No breaking changes to existing scripts or tests

### NFR-3: Maintainability
- Follow CLAUDE.md principles
- Adhere to YAGNI, DRY, KISS principles
- Ensure future tmp/ usage policies are clear

### NFR-4: Safety
- No deletion of files that might be needed
- Backup strategy for any files being removed
- Reversible operations where possible

## Acceptance Criteria

### AC-1: Documentation
- [ ] All 5 markdown files moved to docs/
- [ ] No markdown files remain in tmp/
- [ ] Documentation is accessible and organized

### AC-2: Scripts
- [ ] Duplicate scripts removed from tmp/scripts/
- [ ] Unique functionality preserved in scripts/
- [ ] No test scripts remain in tmp/scripts/

### AC-3: Build Artifacts
- [ ] All CMakeFiles directories removed from tmp/
- [ ] Intermediate build files removed
- [ ] Plugin binaries preserved if needed for testing

### AC-4: Test Files
- [ ] Test source files moved to tests/ directory
- [ ] Build artifacts removed from tmp/tests/
- [ ] Tests remain functional

### AC-5: Directory Size
- [ ] tmp/ contains <50 files
- [ ] tmp/ size <10MB
- [ ] tmp/ structure is clean and organized

### AC-6: Project Compliance
- [ ] Follows CLAUDE.md principles
- [ ] Documentation updated (README.md if needed)
- [ ] Git commit with cleanup changes

## Design Architecture

### File Migration Strategy

#### Phase 1: Documentation Migration

**Source**: tmp/*.md
**Destination**: docs/

**Actions**:
1. `PLUGIN_LOADING_SOLUTION.md` → `docs/plugin-loading-solution.md`
2. `plugin_test_framework_design.md` → `docs/plugin-test-framework-design.md`
3. `PLUGIN_TEST_FRAMEWORK_GUIDE.md` → `docs/plugin-test-framework-guide.md`
4. `README.md` → `docs/tmp-README.md` (if unique content exists)
5. `TEST_FRAMEWORK_IMPLEMENTATION_REPORT.md` → `docs/test-framework-implementation-report.md`

**Rationale**: Documentation should live in the docs/ directory for better organization and discoverability.

#### Phase 2: Script Consolidation

**Analysis**:
Compare tmp/scripts/ with scripts/ to identify duplicates and unique functionality.

**Duplicate Scripts (to be removed)**:
- `clean_plugins.sh` - Duplicates cleanup functionality
- `test_plugin_loading.sh` - Duplicates existing test scripts
- `monitor_obs_logs.sh` - May have unique utility

**Unique Scripts (to be preserved)**:
- `fix_obs_crash.sh` - Unique crash fix utility
- `interactive_filter_test.sh` - Interactive testing utility
- `test_filter_functionality.sh` - Specific filter testing
- `test_plugin_crash_fix.sh` - Crash fix testing
- `verify_filter_status.sh` - Status verification

**Actions**:
1. Compare scripts with existing ones in scripts/
2. Remove clear duplicates
3. Move unique scripts to scripts/ or scripts/integration/
4. Remove tmp/scripts/ directory

**Rationale**: Consolidate all scripts in one location (scripts/) to follow DRY principle.

#### Phase 3: Build Artifact Cleanup

**Source**: tmp/build/, tmp/tests/CMakeFiles/

**Actions**:
1. Remove all CMakeFiles directories
2. Remove intermediate build files:
   - `.ninja_deps`
   - `.ninja_log`
   - `build.ninja`
   - `cmake_install.cmake`
   - `CMakeCache.txt`
   - `Makefile`
   - `CTestTestfile.cmake`

**Preserve**:
- `test-stabilizer.plugin` binary if needed for current testing
- Any other required binaries

**Rationale**: Build artifacts should not be tracked in version control and should be cleaned up.

#### Phase 4: Test File Organization

**Source**: tmp/tests/*.cpp, tmp/tests/CMakeLists.txt

**Destination**: tests/

**Actions**:
1. Move test source files to tests/:
   - `stabilizer_core.cpp` → `tests/stabilizer_core.cpp`
   - `test_feature_tracking.cpp` → `tests/test_feature_tracking.cpp`
   - `test_main.cpp` → `tests/test_main.cpp`
   - `test_mocks.hpp` → `tests/test_mocks.hpp`
   - `test_stabilizer_core.cpp` → `tests/test_stabilizer_core.cpp`
   - `test_transform_smoothing.cpp` → `tests/test_transform_smoothing.cpp`
   - `test-ui-implementation.cpp` → `tests/test-ui-implementation.cpp`

2. Remove build artifacts from tmp/tests/
3. Remove tmp/tests/ directory after migration

**Rationale**: Test files belong in the tests/ directory for consistent project structure.

### Implementation Plan

#### Step 1: Backup and Verification
```bash
# Create backup of tmp/ directory
cp -r tmp tmp_backup_$(date +%Y%m%d)

# Count files before cleanup
find tmp -type f | wc -l
du -sh tmp
```

#### Step 2: Documentation Migration
```bash
# Move markdown files to docs/
mv tmp/PLUGIN_LOADING_SOLUTION.md docs/plugin-loading-solution.md
mv tmp/plugin_test_framework_design.md docs/plugin-test-framework-design.md
mv tmp/PLUGIN_TEST_FRAMEWORK_GUIDE.md docs/plugin-test-framework-guide.md
mv tmp/TEST_FRAMEWORK_IMPLEMENTATION_REPORT.md docs/test-framework-implementation-report.md

# Check if tmp/README.md has unique content before moving
if [ -s tmp/README.md ]; then
    mv tmp/README.md docs/tmp-README.md
fi
```

#### Step 3: Script Analysis and Consolidation
```bash
# Analyze scripts for duplicates
for script in tmp/scripts/*.sh; do
    script_name=$(basename "$script")
    if [ -f "scripts/$script_name" ]; then
        echo "Duplicate found: $script_name"
        # Compare content to determine if truly duplicate
        diff "$script" "scripts/$script_name"
    fi
done

# Move unique scripts to scripts/
mv tmp/scripts/fix_obs_crash.sh scripts/
mv tmp/scripts/interactive_filter_test.sh scripts/integration/
mv tmp/scripts/test_filter_functionality.sh scripts/integration/
mv tmp/scripts/test_plugin_crash_fix.sh scripts/integration/
mv tmp/scripts/verify_filter_status.sh scripts/integration/

# Remove duplicate scripts
rm tmp/scripts/clean_plugins.sh
rm tmp/scripts/test_plugin_loading.sh
rm tmp/scripts/monitor_obs_logs.sh  # After verifying it's a duplicate

# Remove empty tmp/scripts/ directory
rmdir tmp/scripts
```

#### Step 4: Build Artifact Cleanup
```bash
# Remove CMakeFiles directories
find tmp -type d -name "CMakeFiles" -exec rm -rf {} +

# Remove intermediate build files
find tmp -name ".ninja_deps" -delete
find tmp -name ".ninja_log" -delete
find tmp -name "build.ninja" -delete
find tmp -name "cmake_install.cmake" -delete
find tmp -name "CMakeCache.txt" -delete
find tmp -name "Makefile" -delete
find tmp -name "CTestTestfile.cmake" -delete

# Remove build directory if empty or only contains plugin binary
if [ -f tmp/build/test-stabilizer.plugin ]; then
    # Keep plugin binary, remove other files
    cd tmp/build
    rm -rf CMakeFiles .ninja_* build.ninja cmake_install.cmake CMakeCache.txt
    cd ../..
else
    # Remove entire build directory
    rm -rf tmp/build
fi
```

#### Step 5: Test File Migration
```bash
# Move test source files to tests/
mv tmp/tests/stabilizer_core.cpp tests/
mv tmp/tests/test_feature_tracking.cpp tests/
mv tmp/tests/test_main.cpp tests/
mv tmp/tests/test_mocks.hpp tests/
mv tmp/tests/test_stabilizer_core.cpp tests/
mv tmp/tests/test_transform_smoothing.cpp tests/
mv tmp/tests/test-ui-implementation.cpp tests/

# Remove build artifacts from tmp/tests/
rm -rf tmp/tests/CMakeFiles
rm -rf tmp/tests/build-test
rm tmp/tests/cmake_install.cmake
rm tmp/tests/CMakeCache.txt
rm tmp/tests/CMakeLists.txt
rm tmp/tests/Makefile
rm tmp/tests/CTestTestfile.cmake

# Remove empty directories
rmdir tmp/tests/tests 2>/dev/null || true
rmdir tmp/tests/legacy 2>/dev/null || true
rmdir tmp/tests
```

#### Step 6: Final Verification
```bash
# Count files after cleanup
find tmp -type f | wc -l
du -sh tmp

# Verify no documentation files remain
ls tmp/*.md 2>/dev/null && echo "ERROR: Markdown files remain in tmp/" || echo "OK: No markdown files in tmp/"

# Verify no scripts remain
ls tmp/scripts/ 2>/dev/null && echo "ERROR: Scripts remain in tmp/scripts/" || echo "OK: No scripts in tmp/scripts/"

# Verify no CMakeFiles remain
find tmp -type d -name "CMakeFiles" && echo "ERROR: CMakeFiles directories remain in tmp/" || echo "OK: No CMakeFiles in tmp/"
```

### Trade-offs and Considerations

#### Trade-off 1: Build Artifact Preservation
**Option A**: Remove all build artifacts
- **Pros**: Cleanest result, smallest tmp/ size
- **Cons**: May require rebuilding for testing

**Option B**: Preserve plugin binaries
- **Pros**: Faster testing, no rebuild needed
- **Cons**: Larger tmp/ size, not following strict YAGNI

**Decision**: Option B - Preserve plugin binaries for current testing convenience

#### Trade-off 2: Script Consolidation
**Option A**: Move all scripts to scripts/
- **Pros**: Single location for all scripts
- **Cons**: May clutter scripts/ with rarely used utilities

**Option B**: Keep some scripts in tmp/ for quick access
- **Pros**: Convenience for active development
- **Cons**: Violates "one directory" principle

**Decision**: Option A - Move all scripts to scripts/ to follow CLAUDE.md principles

#### Trade-off 3: Test File Migration
**Option A**: Merge with existing tests/
- **Pros**: Consistent test organization
- **Cons**: Potential conflicts with existing test files

**Option B**: Keep separate tmp_tests/ directory
- **Pros**: Clear separation, no conflicts
- **Cons**: Additional directory, violates "single location" principle

**Decision**: Option A - Merge with existing tests/ for consistency

## Testing Strategy

### Pre-Cleanup Validation
1. Count files and size of tmp/ directory
2. Document all files in tmp/ directory
3. Create backup of tmp/ directory

### Post-Cleanup Validation
1. Verify all documentation files moved to docs/
2. Verify all scripts moved to scripts/ or scripts/integration/
3. Verify test files moved to tests/
4. Verify no CMakeFiles directories remain
5. Verify tmp/ size <50 files and <10MB
6. Run existing tests to ensure functionality preserved
7. Verify scripts still work after migration

### Rollback Plan
If cleanup causes issues:
1. Restore from backup: `cp -r tmp_backup_YYYYMMDD tmp`
2. Identify problematic changes
3. Fix issues and retry cleanup

## Future Prevention

### Git Configuration
Add entries to .gitignore to prevent future accumulation:
```gitignore
# Build artifacts
tmp/build/
tmp/tests/CMakeFiles/
tmp/tests/build-test/
tmp/**/CMakeFiles/
tmp/**/.ninja*
tmp/**/build.ninja
tmp/**/cmake_install.cmake
tmp/**/CMakeCache.txt
tmp/**/Makefile

# Documentation (should be in docs/)
tmp/*.md

# Scripts (should be in scripts/)
tmp/scripts/
```

### Policies
1. Documentation must go in docs/ directory
2. Scripts must go in scripts/ or scripts/integration/
3. Test files must go in tests/ directory
4. Build artifacts should be cleaned up after use
5. tmp/ is for temporary, ephemeral files only

### Automated Cleanup
Create a script to periodically clean tmp/:
```bash
scripts/clean-tmp.sh
```

## Related Issues

- Issue #166: [CRITICAL CLEANUP] tmp directory cleanup
- Issue #93: [Technical Debt] Test files scattered in tmp directory

## Success Metrics

- tmp/ contains <50 files
- tmp/ size <10MB
- All documentation in docs/
- All scripts in scripts/
- All tests in tests/
- No CMakeFiles in tmp/
- All existing functionality preserved
- README.md updated if needed

## Timeline Estimate

- **Phase 1 (Documentation Migration)**: 15 minutes
- **Phase 2 (Script Consolidation)**: 30 minutes
- **Phase 3 (Build Artifact Cleanup)**: 15 minutes
- **Phase 4 (Test File Migration)**: 20 minutes
- **Testing and Validation**: 30 minutes

**Total Estimated Time**: 2 hours
