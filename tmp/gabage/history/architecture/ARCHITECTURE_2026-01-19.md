# OBS Stabilizer Plugin Architecture - Comprehensive Design

## Executive Summary

This architecture addresses the critical technical debt issues identified in the OBS Stabilizer plugin, focusing on memory safety, logging standardization, and code quality improvements while maintaining full functionality.

## Current State Analysis

### Functional Status
- ✅ Core stabilization algorithm works correctly
- ✅ OBS plugin integration functional
- ✅ OpenCV-based feature tracking operational
- ✅ Configuration UI with presets working
- ❌ CI/CD pipeline has dependency issues (OBS headers, Google Test)

### Critical Issues Requiring Immediate Attention

#### 1. Memory Safety Issues (Issue #167)
**Problem**: Mixed C++/C memory management in OBS callbacks creates crash risks
- `new VideoStabilizer()` in C-style OBS callbacks
- No RAII protection for callback failures
- Exception safety concerns with pthread_mutex operations

#### 2. Logging Standardization (Issue #168)
**Problem**: Mixed printf()/obs_log() usage violates OBS plugin standards
- printf() statements in plugin_main.cpp and other files
- Messages don't appear in OBS Studio logs
- No log level control (debug, info, warning, error)

#### 3. File Organization (Issue #173)
**Problem**: tmp/ directory contains excessive files violating CLAUDE.md principles
- 1,574 files (67MB) in tmp/ directory
- Test files scattered across multiple locations
- Build system complexity with multiple CMakeLists.txt files

## Architecture Design

### 1. Memory Management Strategy

#### Solution: C++ Wrapper with RAII Pattern

```cpp
// Current problematic code:
data->stabilizer = new VideoStabilizer();  // C++ new in C callback

// New safe approach:
class StabilizerWrapper {
private:
    std::unique_ptr<StabilizerCore> stabilizer;
    std::mutex mutex;
    
public:
    // Safe initialization
    bool initialize(uint32_t width, uint32_t height, const StabilizerParams& params) {
        std::lock_guard<std::mutex> lock(mutex);
        stabilizer = std::make_unique<StabilizerCore>();
        return stabilizer->initialize(width, height, params);
    }
    
    // Exception-safe processing
    cv::Mat process_frame(cv::Mat frame) {
        try {
            std::lock_guard<std::mutex> lock(mutex);
            return stabilizer->process_frame(frame);
        } catch (const std::exception& e) {
            obs_log(LOG_ERROR, "Stabilizer exception: %s", e.what());
            return frame; // Return original frame on error
        }
    }
    
    // Automatic cleanup via RAII
    ~StabilizerWrapper() {
        // No manual cleanup needed - unique_ptr handles it
    }
};
```

#### Implementation Plan:
1. Create `StabilizerWrapper` class in `src/core/stabilizer_wrapper.hpp`
2. Replace direct `new/delete` calls with `std::unique_ptr`
3. Add mutex protection for all shared state access
4. Implement exception-safe boundaries
5. Add RAII guards for OBS callback cleanup

### 2. Logging Standardization Strategy

#### Solution: Complete obs_log() Migration

```cpp
// Current problematic code:
printf("[obs-stabilizer] Video stabilizer filter registered\n");

// New standardized approach:
obs_log(LOG_INFO, "[obs-stabilizer] Video stabilizer filter registered");

// Log level definitions:
#define LOG_ERROR 400  // Critical failures
#define LOG_WARNING 300 // Non-critical issues  
#define LOG_INFO 200    // General information
#define LOG_DEBUG 100   // Detailed debugging
```

#### Implementation Plan:
1. Replace all printf() calls with obs_log() in plugin files
2. Create logging macros for consistency:
   - `STABILIZER_LOG_ERROR(msg)`
   - `STABILIZER_LOG_WARNING(msg)`
   - `STABILIZER_LOG_INFO(msg)`
   - `STABILIZER_LOG_DEBUG(msg)`
3. Add compile-time log level control
4. Ensure all messages appear in OBS Studio log files

### 3. File Organization Strategy

#### Solution: Comprehensive Cleanup and Restructuring

```bash
# Current problematic structure:
/tmp/tests/                # 1,574 files, 67MB
  /tests/                  # Redundant nesting
  /legacy/                 # Historical duplicates

# New organized structure:
/tests/                   # All tests in standard location
  /unit/                  # Unit tests
  /integration/           # Integration tests
  /performance/           # Performance tests
  /data/                  # Test data and sample frames

/src/                    # Source files
  /core/                  # Core algorithms
  /obs/                   # OBS integration
  /utils/                 # Utilities and helpers
```

#### Implementation Plan:
1. Move all test files to proper `/tests/` directory structure
2. Remove duplicate and obsolete test files
3. Clean up tmp/ directory (reduce from 67MB to <10MB)
4. Consolidate CMakeLists.txt files (from 9 to 2)
5. Update .gitignore to prevent future misplacement

### 4. CI/CD Fix Strategy

#### Solution: Dependency Resolution for Build Pipeline

```cmake
# Fix OBS headers issue
find_path(OBS_INCLUDE_DIR obs-module.h
    PATHS
    ${CMAKE_SOURCE_DIR}/include/obs
    /usr/local/include/obs
    /usr/include/obs
)

# Fix Google Test issue
find_package(GTest REQUIRED)
if(NOT GTest_FOUND)
    message(FATAL_ERROR "Google Test required for testing")
endif()
```

#### Implementation Plan:
1. Fix OBS headers path resolution in CMake
2. Ensure Google Test is properly installed and found
3. Add proper error handling for missing dependencies
4. Create standalone test mode for CI environments

## Implementation Priority Matrix

| Priority | Issue | Effort | Impact |
|----------|-------|--------|--------|
| Critical | Memory safety audit | High | High |
| Critical | CI/CD dependency fixes | Medium | High |
| High | Logging standardization | Medium | Medium |
| High | File organization cleanup | Low | High |
| Medium | Magic numbers replacement | Low | Medium |
| Medium | Test coverage expansion | High | Medium |

## Success Criteria

### Memory Safety
- [ ] No memory leaks during plugin load/unload cycles
- [ ] Exception safety testing with invalid video frames
- [ ] Cross-platform memory management verification
- [ ] OBS integration testing under stress conditions
- [ ] Performance impact of `std::mutex` measured and evaluated to be within acceptable limits

### Logging Standardization
- [ ] All printf() statements replaced with obs_log()
- [ ] Messages appear in OBS Studio log files
- [ ] Different log levels working correctly
- [ ] No printf() statements remain in plugin code

### File Organization
- [ ] tmp/ directory reduced to <50 files, <10MB
- [ ] All tests consolidated in /tests/ directory
- [ ] CMakeLists.txt files consolidated to 2-3 essential files
- [ ] Clear documentation of test organization

### CI/CD Pipeline
- [ ] OBS headers found during CI builds
- [ ] Google Test framework available for testing
- [ ] All build configurations working
- [ ] Cross-platform compatibility verified

## Risk Assessment

### High Risk Areas
1. **Memory Management Changes**: Potential for introducing new crashes
2. **OBS Callback Modifications**: Could break plugin compatibility
3. **CI/CD Dependency Changes**: May require infrastructure updates

### Mitigation Strategies
1. **Incremental Implementation**: Change one subsystem at a time
2. **Comprehensive Testing**: Unit tests for each component
3. **Fallback Mechanisms**: Maintain backward compatibility
4. **CI/CD Validation**: Test all changes in automated pipeline

## Migration Path

### Phase 1: Critical Fixes (Week 1)
1. Implement memory safety wrapper (Issue #167)
2. Fix CI/CD dependency issues
3. Replace printf() with obs_log() (Issue #168)
4. Basic testing and validation

### Phase 2: Quality Improvements (Week 2)
1. File organization cleanup (Issue #173)
2. CMakeLists.txt consolidation (Issue #169)
3. Magic numbers replacement (Issue #170)
4. Expanded test coverage

### Phase 3: Long-term Maintenance (Week 3+)
1. Performance optimization
2. Additional test coverage
3. Documentation improvements
4. User experience enhancements

## Compliance with CLAUDE.md Principles

- ✅ **KISS Principle**: Simplified architecture with clear separation
- ✅ **DRY Principle**: Eliminated code duplication
- ✅ **Memory Safety**: RAII pattern for automatic cleanup
- ✅ **Error Handling**: Comprehensive exception safety
- ✅ **File Organization**: Clean directory structure
- ✅ **Standards Compliance**: OBS plugin best practices

## Expected Benefits

1. **Improved Stability**: No crashes from memory management issues
2. **Better Debugging**: Standardized logging with OBS integration
3. **Easier Maintenance**: Clean code organization and structure
4. **CI/CD Reliability**: Consistent build pipeline
5. **Code Quality**: Higher standards compliance
6. **Developer Experience**: Clear architecture and documentation

## Implementation Timeline

- **Day 1-3**: Memory safety wrapper implementation
- **Day 4-5**: Logging standardization
- **Day 6-7**: CI/CD fixes and validation
- **Week 2**: File organization and cleanup
- **Week 3**: Testing and quality assurance
- **Week 4**: Documentation and final validation

This architecture provides a comprehensive solution to the critical technical debt while maintaining full functionality and improving overall code quality and maintainability.