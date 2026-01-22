# E2E Testing Guide for OBS Stabilizer Plugin

This guide provides comprehensive procedures for end-to-end (E2E) integration testing with OBS Studio to ensure the plugin works correctly in production environments.

## Overview

**Purpose**: Validate plugin functionality with actual OBS Studio integration beyond unit tests

**Scope**:
- Plugin installation and loading
- Filter configuration and application
- Real-time stabilization processing
- Multi-platform compatibility (Windows, macOS, Linux)
- UI component testing (Properties Panel)

**Prerequisites**:
- OBS Studio 30.0+ installed
- Built plugin binary (.dll, .so, .dylib)
- Test video content (short clips with various motion patterns)
- System monitoring tools (OBS Stats, Task Manager, htop/Activity Monitor)

---

## Manual E2E Testing Procedures

### Phase 1: Installation Testing

**Objective**: Verify plugin loads without errors on all supported platforms

#### Windows

1. **Clean Install**:
   ```bash
   # Remove old versions
   del "%APPDATA%\obs-studio\plugins\obs-stabilizer.dll"
   del "%APPDATA%\obs-studio\plugins\obs-stabilizer-opencv.dll"
   ```

2. **Copy New Binary**:
   ```bash
   # Using Release build
   copy build\Release\obs-stabilizer.dll "%APPDATA%\obs-studio\plugins\"
   
   # Or using OpenCV bundle
   copy build\obs-stabilizer-opencv.dll "%APPDATA%\obs-studio\plugins\"
   ```

3. **Launch OBS Studio**:
   - Check for startup errors in OBS Log (Help → Log Files)
   - Look for: "Failed to load module 'obs-stabilizer'"
   - Look for: "Symbol not found" errors
   - Verify plugin appears in Filters list

4. **Verification Checklist**:
   - [ ] Plugin loads without startup errors
   - [ ] "Video Stabilizer" appears in Filters list
   - [ ] OBS Settings → Plugins shows plugin without warnings
   - [ ] System Report shows no plugin errors
   - [ ] Version matches expected (check obs_module_version)

#### macOS

1. **Clean Install**:
   ```bash
   # Remove old versions
   rm -rf ~/Library/Application\ Support/obs-studio/plugins/obs-stabilizer*
   rm -rf ~/Library/Application\ Support/obs-studio/plugins/obs-stabilizer-opencv*
   ```

2. **Copy New Binary**:
   ```bash
   # Using standard build
   cp build/obs-stabilizer.so ~/Library/Application\ Support/obs-studio/plugins/
   
   # Using OpenCV bundle
   cp build/obs-stabilizer-opencv.so ~/Library/Application\ Support/obs-studio/plugins/
   cp -r build/Frameworks ~/Library/Application\ Support/obs-studio/plugins/obs-stabilizer-opencv/
   ```

3. **Run Plugin Loading Fix** (if needed):
   ```bash
   ./scripts/fix-plugin-loading.sh
   ```

4. **Launch OBS Studio**:
   - Open Console.app to monitor logs
   - Check for: "dyld: symbol not found" errors
   - Verify plugin appears in Filters list

5. **Verification Checklist**:
   - [ ] Plugin loads without startup errors
   - [ ] "Video Stabilizer" or "obs-stabilizer-opencv" appears in Filters list
   - [ ] Console shows no symbol resolution errors
   - [ ] System Report shows no plugin errors
   - [ ] ARM64 architecture verified for Apple Silicon

#### Linux

1. **Clean Install**:
   ```bash
   # Remove old versions
   rm -f ~/.config/obs-studio/plugins/obs-stabilizer.so
   rm -f ~/.config/obs-studio/plugins/obs-stabilizer-opencv.so
   ```

2. **Copy New Binary**:
   ```bash
   # Using standard build
   cp build/obs-stabilizer.so ~/.config/obs-studio/plugins/
   
   # Using OpenCV bundle
   cp build/obs-stabilizer-opencv.so ~/.config/obs-studio/plugins/
   cp -r build/Frameworks ~/.config/obs-studio/plugins/obs-stabilizer-opencv/
   ```

3. **Check Dependencies**:
   ```bash
   # Verify OpenCV libraries are available
   ldd ~/.config/obs-studio/plugins/obs-stabilizer.so | grep opencv
   ```

4. **Launch OBS Studio**:
   - Run from terminal to see error messages
   - Check for: "Failed to load module" errors
   - Verify plugin appears in Filters list

5. **Verification Checklist**:
   - [ ] Plugin loads without startup errors
   - [ ] "Video Stabilizer" appears in Filters list
   - [ ] No missing dependency errors (ldd output clean)
   - [ ] Terminal shows no load errors
   - [ ] System Report shows no plugin errors

**Phase 1 Acceptance Criteria**:
- [ ] Plugin loads on all three platforms (Windows, macOS, Linux)
- [ ] No startup errors or crashes
- [ ] Plugin appears in OBS Filters list
- [ ] No missing symbol or dependency errors
- [ ] OBS Logs show no plugin-related errors

---

### Phase 2: Basic Functionality Testing

**Objective**: Verify core stabilization features work correctly

#### Test 2.1: Filter Addition and Configuration

**Procedure**:
1. **Add Filter to Source**:
   - Right-click video source → Filters → Add Filter → "Video Stabilizer"
   - OR: Select source → Properties → Add Filter → "Video Stabilizer"

2. **Verify Filter Appears**:
   - Filter should appear in Filters panel
   - Filter name should be "Video Stabilizer"

3. **Open Filter Properties**:
   - Right-click filter → Properties
   - Properties panel should open without errors

4. **Test Enable/Disable Toggle**:
   - Uncheck "Enable Stabilization"
   - Verify: Stabilization is disabled (pass-through mode)
   - Re-check "Enable Stabilization"
   - Verify: Stabilization is re-enabled

5. **Verification Checklist**:
   - [ ] Filter can be added to any video source
   - [ ] Properties panel opens without errors
   - [ ] Enable/disable toggle works correctly
   - [ ] Setting is preserved when Properties closed
   - [ ] No OBS crashes during configuration

#### Test 2.2: Preset Application

**Procedure**:
1. **Test Each Preset**:
   - Select "Gaming" preset
   - Verify: Smoothing = 15, Features = 150, Max Correction = 10%
   - Select "Streaming" preset
   - Verify: Smoothing = 30, Features = 200, Max Correction = 20%
   - Select "Recording" preset
   - Verify: Smoothing = 50, Features = 350, Max Correction = 35%
   - Select "Custom" preset
   - Verify: Parameters remain at previous values

2. **Test Preset Switching**:
   - Switch between presets multiple times
   - Verify: Each preset applies correct parameters immediately

3. **Verification Checklist**:
   - [ ] Gaming preset applies correct low-latency parameters
   - [ ] Streaming preset applies balanced parameters
   - [ ] Recording preset applies high-quality parameters
   - [ ] Custom preset preserves user configuration
   - [ ] Preset switching is instant (no lag)
   - [ ] No OBS crashes when changing presets

#### Test 2.3: Real-Time Parameter Adjustment

**Procedure**:
1. **Adjust Each Parameter**:
   - **Smoothing Radius**: Slide from 5 to 200 in increments of 10
   - **Feature Count**: Adjust from 50 to 1000 in increments of 50
   - **Max Correction**: Slide from 5% to 50% in increments of 5%
   - **Quality Level**: Adjust from 0.001 to 0.1 in increments of 0.001

2. **Test Edge Cases**:
   - Set Smoothing Radius to minimum (5)
   - Set Smoothing Radius to maximum (200)
   - Set Feature Count to maximum (1000)
   - Set Max Correction to maximum (50%)

3. **Verification Checklist**:
   - [ ] All parameters update in real-time
   - [ ] OBS Preview shows changes immediately
   - [ ] No lag when adjusting parameters
   - [ ] Extreme values work without crashes
   - [ ] Parameter validation prevents invalid inputs

**Phase 2 Acceptance Criteria**:
- [ ] Filter can be added and configured
- [ ] All presets work correctly
- [ ] Real-time parameter adjustment works
- [ ] No OBS crashes or hangs during configuration
- [ ] Settings persist correctly after restart

---

### Phase 3: Stabilization Performance Testing

**Objective**: Verify stabilization quality and resource usage

#### Test 3.1: Static Scene (No Movement)

**Procedure**:
1. **Setup OBS**:
   - Create Scene with static image or color source
   - Add Stabilizer filter with Gaming preset
   - Record 30 seconds of static content

2. **Expected Behavior**:
   - No visible stabilization (no movement to stabilize)
   - Minimal CPU usage (<5%)
   - Stable output (no jitter)

3. **Verification Checklist**:
   - [ ] No visible artifacts in output
   - [ ] CPU usage remains low (<5%)
   - [ ] Output is stable and clear
   - [ ] No frame drops or missed frames
   - [ ] Memory usage stable (<50MB)

#### Test 3.2: Slow Movement (Panning)

**Procedure**:
1. **Setup OBS**:
   - Use camera source or test video with slow panning
   - Add Stabilizer filter with Streaming preset
   - Record 30 seconds of slow panning

2. **Expected Behavior**:
   - Smooth camera movement without jitter
   - Moderate smoothing applied
   - No over-correction

3. **Verification Checklist**:
   - [ ] Camera movement is smooth (no jerky pans)
   - [ ] No over-correction artifacts
   - [ ] CPU usage moderate (10-25%)
   - [ ] Memory usage stable (<100MB)
   - [ ] No frame drops

#### Test 3.3: Fast Movement (Gaming Action)

**Procedure**:
1. **Setup OBS**:
   - Use test video with fast camera movements
   - Add Stabilizer filter with Gaming preset
   - Record 30 seconds of fast motion

2. **Expected Behavior**:
   - Reduced shake without adding lag
   - Fast response to movements
   - Minor artifacts acceptable for gaming

3. **Verification Checklist**:
   - [ ] Camera shake significantly reduced
   - [ ] Minimal input lag (<50ms)
   - [ ] Acceptable artifacts for fast motion
   - [ ] CPU usage moderate (20-30%)
   - [ ] No frame drops at 30fps

#### Test 3.4: Camera Shake Simulation

**Procedure**:
1. **Setup OBS**:
   - Use test video with simulated camera shake
   - Add Stabilizer filter with Recording preset
   - Record 30 seconds of shaky content

2. **Expected Behavior**:
   - Shake substantially reduced
   - Smooth output
   - Some artifacts acceptable for quality vs. stabilization trade-off

3. **Verification Checklist**:
   - [ ] Significant reduction in camera shake
   - [ ] Output is smooth and watchable
   - [ ] CPU usage moderate (20-40%)
   - [ ] No over-correction (unnatural floaty movement)
   - [ ] Quality improvement justifies increased processing

#### Test 3.5: Resolution Testing

**Procedure**:
1. **Test Each Resolution**:
   - 480p: 640x480 @ 60fps
   - 720p: 1280x720 @ 60fps
   - 1080p: 1920x1080 @ 30fps
   - Use Gaming preset for all

2. **Expected Performance**:
   - 480p: <5ms per frame, <10% CPU
   - 720p: <8ms per frame, <20% CPU
   - 1080p: <12ms per frame, <30% CPU

3. **Verification Checklist**:
   - [ ] 480p: Stable at 60fps, low CPU
   - [ ] 720p: Stable at 60fps, moderate CPU
   - [ ] 1080p: Stable at 30fps, acceptable CPU
   - [ ] No resolution-specific crashes
   - [ ] Memory scales appropriately with resolution

**Phase 3 Acceptance Criteria**:
- [ ] Static scenes show minimal artifacts
- [ ] Slow movement is smooth without over-correction
- [ ] Fast movement has minimal lag
- [ ] Camera shake is substantially reduced
- [ ] Performance targets met for each resolution
- [ ] No memory leaks or excessive memory usage

---

### Phase 4: Multi-Platform Testing

**Objective**: Verify consistent behavior across platforms

#### Test 4.1: Windows OBS Studio

**Environment**:
- OBS Studio 30.0+ on Windows 10/11
- Clean OBS profile (no other plugins)
- Performance monitoring enabled

**Test Matrix**:
| Scenario | Expected Behavior | Pass/Fail | Notes |
|-----------|----------------|-----------|-------|
| Plugin loading | Loads without errors |  |  |
| Presets | All presets work |  |  |
| 720p @ 30fps | <8ms processing |  |  |
| 1080p @ 60fps | <12ms processing |  |  |

#### Test 4.2: macOS OBS Studio

**Environment**:
- OBS Studio 30.0+ on macOS (Intel/Apple Silicon)
- Clean OBS profile
- Activity Monitor for performance

**Test Matrix**:
| Scenario | Expected Behavior | Pass/Fail | Notes |
|-----------|----------------|-----------|-------|
| Plugin loading | Loads without errors |  |  |
| Presets | All presets work |  |  |
| 720p @ 30fps | <8ms processing |  |  |
| 1080p @ 60fps | <12ms processing |  |  |
| ARM64 native | Native ARM64 performance |  |  |

#### Test 4.3: Linux OBS Studio

**Environment**:
- OBS Studio 30.0+ on Ubuntu/Linux
- Clean OBS profile
- htop for performance

**Test Matrix**:
| Scenario | Expected Behavior | Pass/Fail | Notes |
|-----------|----------------|-----------|-------|
| Plugin loading | Loads without errors |  |  |
| Presets | All presets work |  |  |
| 720p @ 30fps | <8ms processing |  |  |
| 1080p @ 60fps | <12ms processing |  |  |

**Phase 4 Acceptance Criteria**:
- [ ] All three platforms tested (Windows, macOS, Linux)
- [ ] Consistent behavior across platforms
- [ ] No platform-specific crashes or hangs
- [ ] Performance within expected ranges for each platform
- [ ] OBS version compatibility verified (28.x, 29.x, 30.x)

---

### Phase 5: Edge Cases and Regression Testing

**Objective**: Verify robustness and prevent regressions

#### Test 5.1: Edge Cases

**Procedure**:
1. **Empty Source**:
   - Add filter to black screen or disabled source
   - Verify: No crashes, graceful handling

2. **Resolution Change During Stream**:
   - Start streaming at 720p
   - Switch to 1080p mid-stream
   - Verify: No crashes, smooth transition

3. **Scene Switching**:
   - Add stabilizer to sources in multiple scenes
   - Rapidly switch between scenes
   - Verify: No memory leaks, smooth scene changes

4. **Multiple Sources with Stabilizer**:
   - Add to 3-4 different sources
   - Enable all simultaneously
   - Verify: CPU usage scales linearly, no crashes

5. **High Feature Count Stress Test**:
   - Set Feature Count to 1000 (maximum)
   - Use complex test video
   - Monitor: Memory usage, stability, performance

6. **Extreme Parameter Values**:
   - Smoothing = 200 (maximum)
   - Max Correction = 50% (maximum)
   - Verify: Plugin remains stable, no crashes

**Verification Checklist**:
- [ ] Empty source handled gracefully
- [ ] Resolution changes don't cause crashes
- [ ] Scene switching works smoothly
- [ ] Multiple sources work correctly
- [ ] Extreme values don't crash OBS
- [ ] Memory usage remains reasonable with extreme parameters

#### Test 5.2: Regression Testing

**Procedure**:
1. **Compare with Previous Version**:
   - Record test video with previous plugin version
   - Record same video with new version
   - Compare: Quality, performance, CPU usage

2. **Previous Settings Compatibility**:
   - Export OBS settings from old version
   - Import to new version
   - Verify: All settings apply correctly

3. **Known Issues Validation**:
   - Re-test all previously fixed issues
   - Verify issues don't reappear

**Verification Checklist**:
- [ ] No regression in stabilization quality
- [ ] No performance degradation vs. previous version
- [ ] Settings migration works correctly
- [ ] No previously fixed bugs have reappeared

**Phase 5 Acceptance Criteria**:
- [ ] All edge cases handled gracefully
- [ ] No regressions from previous versions
- [ ] Plugin remains stable under stress conditions
- [ ] Memory management is correct (no leaks)

---

### Phase 6: UI Component Testing

**Objective**: Verify OBS Properties Panel functionality

#### Test 6.1: Properties Panel Layout

**Procedure**:
1. **Open Properties Panel**:
   - Right-click source with filter → Properties
   - Verify: All sections visible and accessible

2. **Verify UI Elements**:
   - Enable Stabilization checkbox
   - Adaptive Stabilization checkbox
   - Preset dropdown
   - All sliders (Smoothing, Features, Correction, Quality)
   - All parameter labels and descriptions

3. **Test UI Controls**:
   - Click and drag each slider
   - Type values in text boxes
   - Test preset dropdown selection
   - Test checkboxes

**Verification Checklist**:
- [ ] All UI elements are visible and accessible
- [ ] Sliders have correct ranges and increments
- [ ] Checkboxes work correctly
- [ ] Dropdown loads all presets
- [ ] No layout issues or overlapping elements
- [ ] Panel resizes correctly

#### Test 6.2: Real-Time UI Updates

**Procedure**:
1. **Update Parameters via UI**:
   - Adjust sliders while OBS Preview is running
   - Change presets
   - Toggle enable/disable

2. **Verify Live Updates**:
   - Changes apply immediately in OBS Preview
   - No lag between UI change and visual update
   - Performance Status updates (if implemented)

3. **Test Settings Persistence**:
   - Configure parameters
   - Close OBS Studio
   - Re-open OBS Studio
   - Verify: Settings are preserved

**Verification Checklist**:
- [ ] UI changes apply immediately in Preview
- [ ] No lag when adjusting parameters
- [ ] Settings persist after OBS restart
- [ ] No UI elements get stuck or become unresponsive

**Phase 6 Acceptance Criteria**:
- [ ] All UI components work correctly
- [ ] Real-time parameter adjustment works
- [ ] Settings persist properly
- [ ] No UI crashes or hangs

---

## Test Results Template

### Test Report

**Date**: [YYYY-MM-DD]
**Tester**: [Name]
**OBS Version**: [Version]
**Platform**: [Windows/macOS/Linux]
**Plugin Version**: [Version]

### Summary

- Total Tests Run: [Number]
- Tests Passed: [Number]
- Tests Failed: [Number]
- Pass Rate: [X%]

### Detailed Results

| Test Category | Status | Notes |
|---------------|--------|-------|
| Installation | ✅/❌ | [Details] |
| Basic Functionality | ✅/❌ | [Details] |
| Stabilization Quality | ✅/❌ | [Details] |
| Performance | ✅/❌ | [Details] |
| Edge Cases | ✅/❌ | [Details] |
| UI Components | ✅/❌ | [Details] |

### Issues Found

| Issue | Severity | Description |
|-------|----------|-------------|
| [Issue 1] | [Critical/High/Medium/Low] | [Details] |
| [Issue 2] | [Severity] | [Details] |

### Recommendations

- [Recommendation 1]
- [Recommendation 2]
- [Recommendation 3]

---

## Acceptance Criteria

**Minimum Viable Product (MVP)**:
- [ ] Manual E2E test procedures documented
- [ ] Installation verified on at least one platform
- [ ] Basic functionality tested (add filter, configure parameters)
- [ ] Test results template provided
- [ ] 10+ core test scenarios defined

**Full Implementation**:
- [ ] All manual E2E test procedures documented (6 phases)
- [ ] Installation tested on all three platforms (Windows, macOS, Linux)
- [ ] All core functionality tested (filter config, presets, real-time adjustment)
- [ ] All stabilization scenarios tested (static, slow, fast, shake, resolution)
- [ ] Edge cases tested (empty sources, scene switching, multiple sources, extreme values)
- [ ] UI components tested (properties panel, real-time updates)
- [ ] Multi-platform compatibility verified
- [ ] Test results template ready for use
- [ ] Regression testing procedures documented

**Quality Standards**:
- [ ] All procedures are clear and actionable
- [ ] Expected behaviors are well-defined
- [ ] Verification checklists are comprehensive
- [ ] Test results template includes all necessary metrics
- [ ] Documentation is easy to follow for testers

---

## Troubleshooting E2E Tests

### Common Issues and Solutions

**Issue**: Plugin Won't Load
- **Check**: Verify OBS version (30.0+ required)
- **Check**: Plugin file location (correct plugins directory)
- **Check**: OBS logs for specific error messages
- **Solution**: Reinstall plugin, verify dependencies

**Issue**: OBS Crashes When Adding Filter
- **Check**: System logs for crash details
- **Check**: Test video content compatibility
- **Solution**: Try different test video, verify plugin version

**Issue**: High CPU Usage
- **Check**: Current resolution and frame rate
- **Check**: Active feature count and smoothing radius
- **Solution**: Reduce complexity (Gaming preset, lower feature count)

**Issue**: No Stabilization Effect
- **Check**: Filter is enabled in properties
- **Check**: Test video has actual movement
- **Solution**: Increase smoothing radius, check preset suitability

**Issue**: Artifacts in Output
- **Check**: Preset matches content type
- **Check**: Max correction not too high
- **Solution**: Adjust parameters incrementally

### Performance Monitoring During Tests

**Windows**: Use Task Manager to monitor:
- OBS Studio CPU usage
- Memory usage
- GPU usage (if applicable)

**macOS**: Use Activity Monitor:
- CPU percentage per core
- Memory pressure
- Energy impact

**Linux**: Use htop or top:
- CPU usage percentage
- Memory usage
- Process priority

**Target Metrics**:
- CPU: <40% during 1080p processing
- Memory: <200MB steady state
- Frame drops: <1% at target frame rate

---

## Integration with Existing CI/CD

### Automated Testing Limitations

**Current State**:
- Unit tests run in CI/CD pipeline (71/71 passing)
- No OBS Studio integration testing in CI/CD

**Why No E2E in CI/CD**:
- E2E testing requires OBS Studio runtime
- Headless OBS builds have limitations
- Full UI testing requires interactive OBS sessions
- Cross-platform testing requires multiple OS environments

**Proposed Solution**:
- Manual E2E testing documented in this guide
- Test results documented using template
- Critical issues added to GitHub for tracking
- Consider automated E2E framework for future (see Issue #220 Phase 2)

### Continuous Testing Integration

**Future Improvements**:
- E2E tests run on every commit
- Automated smoke tests before releases
- Nightly build testing on multiple platforms
- Test result aggregation and trend analysis

---

## Best Practices for E2E Testing

1. **Start Fresh**: Clean OBS profile before testing
2. **Document Everything**: Record OBS version, plugin version, settings
3. **Use Consistent Content**: Same test videos across all tests
4. **Monitor Resources**: Watch CPU, memory, FPS during tests
5. **Test Edge Cases**: Don't just test happy paths
6. **Compare Versions**: Test against previous builds
7. **Report Issues Clearly**: Include steps to reproduce, OBS version, system specs
8. **Clean Up After Testing**: Remove test sources, close OBS
9. **Update Documentation**: Keep this guide current with findings
10. **Collaborate**: Share test results with team

---

## Related Documentation

- **README.md**: User guide and basic troubleshooting
- **docs/architecture/ARCHITECTURE.md**: System design
- **AGENTS.md**: Development workflow and completed issues

---

## Version History

- **1.0** - Initial E2E testing guide (Issue #220)
  - 6 testing phases defined
  - Manual testing procedures documented
  - Test results template provided
  - Integration with CI/CD considered
  - Best practices documented
