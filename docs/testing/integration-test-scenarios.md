# E2E Integration Test Scenarios

This document provides detailed test scenarios for end-to-end (E2E) integration testing of the OBS Stabilizer plugin. Use these scenarios to systematically verify plugin functionality in real OBS Studio environments.

## Scenario Categories

1. **Basic Functionality Scenarios** - Core feature validation
2. **Performance Scenarios** - Resource usage and optimization verification
3. **Stabilization Quality Scenarios** - Output quality assessment
4. **Edge Cases** - Boundary conditions and unusual situations
5. **Multi-Platform Scenarios** - Cross-platform behavior verification
6. **Regression Scenarios** - Version-to-version comparison

---

## 1. Basic Functionality Scenarios

### BF-001: Plugin Installation and Loading

**Objective**: Verify plugin installs and loads without errors

**Steps**:
1. Clean install plugin on Windows/macOS/Linux
2. Launch OBS Studio
3. Verify plugin appears in Filters list
4. Check OBS logs for errors

**Expected Result**: Plugin loads successfully, "Video Stabilizer" appears in Filters list

**Success Criteria**:
- [ ] No startup errors in OBS
- [ ] Plugin appears in Filters list
- [ ] OBS logs show no plugin errors
- [ ] System Report shows no plugin errors

**Failure Modes**:
- Plugin doesn't appear in Filters list
- OBS crashes on startup
- "Failed to load module" error
- Symbol not found errors

**Notes**:
- Test on all three platforms (Windows, macOS, Linux)
- Use both standard and OpenCV-bundled builds
- Check OBS version compatibility (30.0+ required)

---

### BF-002: Filter Addition to Video Source

**Objective**: Verify stabilizer filter can be added to any video source

**Test Sources**:
1. Game Capture (DirectX, Vulkan)
2. Window Capture (Game, Browser)
3. Video Capture Device (Webcam, Camera)
4. Media Source (Video File)
5. Display Capture (Monitor)
6. Color Source (Solid Color)

**Steps**:
1. Add video source to OBS Scene
2. Right-click source → Filters → Add Filter → "Video Stabilizer"
3. Verify filter appears in Filters panel
4. Check OBS Preview for changes

**Expected Result**: Filter successfully added to source, stabilization active

**Success Criteria**:
- [ ] Filter can be added to all 6 source types
- [ ] No OBS crashes when adding filter
- [ ] Filter appears in Filters panel
- [ ] OBS Preview shows stabilized output

**Failure Modes**:
- Filter doesn't appear in list
- OBS crashes when adding filter
- Source stops rendering
- Properties panel fails to open

**Notes**:
- Test with sources at different resolutions
- Test with active streams/recording

---

### BF-003: Enable/Disable Toggle

**Objective**: Verify stabilization can be toggled on/off

**Steps**:
1. Add stabilizer filter to source
2. Record baseline video (stabilization enabled)
3. Uncheck "Enable Stabilization"
4. Record disabled state (pass-through)
5. Re-enable stabilization
6. Record re-enabled state

**Expected Result**: Toggle works smoothly, no artifacts when disabled

**Success Criteria**:
- [ ] Unchecking disables stabilization immediately
- [ ] Disabled state shows pass-through (no processing)
- [ ] Re-enabling restores stabilization
- [ ] No lag when toggling
- [ ] Settings preserved between states

**Failure Modes**:
- Toggle doesn't work
- Stabilization continues when disabled
- OBS crashes when toggling
- Settings reset after toggle

**Notes**:
- Test with different video content types
- Monitor CPU usage difference between enabled/disabled

---

### BF-004: Preset Application

**Objective**: Verify all presets apply correct parameters

**Test Presets**:
1. **Gaming**: Smoothing=15, Features=150, Max Correction=10%
2. **Streaming**: Smoothing=30, Features=200, Max Correction=20%
3. **Recording**: Smoothing=50, Features=350, Max Correction=35%

**Steps**:
1. Add filter to source
2. Select each preset
3. Verify parameters match expected values
4. Record 10 seconds for each preset
5. Compare output quality

**Expected Result**: Each preset applies correct configuration

**Success Criteria**:
- [ ] Gaming preset applies low-latency parameters
- [ ] Streaming preset applies balanced parameters
- [ ] Recording preset applies high-quality parameters
- [ ] Preset switching is instant
- [ ] Output quality appropriate for each preset

**Failure Modes**:
- Preset doesn't apply
- Parameters don't match expected values
- Preset switching causes crashes
- Quality not appropriate for preset type

**Notes**:
- Test with different content types for each preset
- Compare CPU usage between presets

---

### BF-005: Real-Time Parameter Adjustment

**Objective**: Verify parameters update in real-time without issues

**Test Parameters**:
1. Smoothing Radius (5-200 range)
2. Feature Count (50-1000 range)
3. Max Correction (5-50% range)
4. Quality Level (0.001-0.1 range)

**Steps**:
1. Add filter to source with Gaming preset
2. Adjust each parameter incrementally
3. Verify changes apply immediately in OBS Preview
4. Test extreme values (min/max)
5. Record parameter changes

**Expected Result**: All parameters update smoothly in real-time

**Success Criteria**:
- [ ] All parameters update without lag
- [ ] OBS Preview shows changes instantly
- [ ] No crashes when adjusting parameters
- [ ] Extreme values work correctly
- [ ] Parameter validation prevents invalid inputs

**Failure Modes**:
- Changes don't apply
- Lag between UI change and visual update
- OBS crashes when adjusting parameters
- Invalid values cause undefined behavior

**Notes**:
- Monitor CPU usage during parameter adjustments
- Test with both mouse and keyboard input
- Test slider dragging and value typing

---

### BF-006: Properties Panel UI

**Objective**: Verify all UI elements work correctly

**Test UI Elements**:
1. Enable/Disable checkbox
2. Adaptive Stabilization checkbox
3. All sliders (7 elements)
4. Quality Level dropdown
5. Preset dropdown
6. All labels and descriptions

**Steps**:
1. Open Properties panel
2. Interact with each UI element
3. Verify labels and tooltips display
4. Test keyboard navigation
5. Verify tab order

**Expected Result**: All UI elements accessible and functional

**Success Criteria**:
- [ ] All sliders work (drag, click, type)
- [ ] Checkboxes toggle correctly
- [ ] Dropdowns load all options
- [ ] Labels and descriptions display correctly
- [ ] UI is responsive and doesn't lag
- [ ] Panel resizes correctly

**Failure Modes**:
- UI elements don't respond
- Panel layout broken
- Missing or incorrect labels
- Values don't display correctly

**Notes**:
- Test on different screen resolutions
- Test with OBS at different window sizes
- Verify accessibility (screen readers if available)

---

## 2. Performance Scenarios

### PF-001: Static Scene Performance

**Objective**: Verify minimal resource usage for static content

**Test Content**:
1. Solid color source (black screen)
2. Static image source
3. Text source with no movement

**Steps**:
1. Add stabilizer filter with Recording preset
2. Record 60 seconds of static content
3. Monitor CPU usage, memory, FPS
4. Check for frame drops

**Expected Result**: Minimal processing overhead, stable performance

**Success Criteria**:
- [ ] CPU usage <5%
- [ ] Memory usage <50MB
- [ ] Zero frame drops
- [ ] Stable 60fps (or target frame rate)
- [ ] No memory leaks detected

**Failure Modes**:
- High CPU usage (>10%)
- Memory grows continuously
- Frame drops or missed frames
- CPU usage spikes unexpectedly

**Notes**:
- Compare with and without stabilization
- Test at different resolutions (720p, 1080p, 1440p)

---

### PF-002: Slow Movement Performance

**Objective**: Verify acceptable performance for slow pans

**Test Content**:
1. Slow panning test video (30 seconds)
2. Horizontal camera sweep
3. Diagonal camera movement

**Steps**:
1. Add filter with Streaming preset
2. Record 30 seconds each
3. Monitor CPU, memory, FPS
4. Check output quality

**Expected Result**: Smooth panning with moderate resource usage

**Success Criteria**:
- [ ] CPU usage 10-25%
- [ ] Memory usage <100MB
- [ ] Stable frame rate (30fps)
- [ ] Smooth output with minimal jitter
- [ ] No over-correction

**Failure Modes**:
- CPU usage >40%
- Jittery or choppy panning
- Over-correction artifacts
- Frame drops

**Notes**:
- Compare Gaming vs Streaming presets
- Test at 720p and 1080p

---

### PF-003: Fast Movement Performance

**Objective**: Verify low-latency performance for gaming

**Test Content**:
1. Fast FPS game footage (60fps)
2. Action game with rapid camera turns
3. Test video with sudden movements

**Steps**:
1. Add filter with Gaming preset
2. Record 30 seconds each
3. Monitor CPU, memory, FPS
4. Check for input lag

**Expected Result**: Reduced shake with minimal latency

**Success Criteria**:
- [ ] CPU usage 20-30% at 1080p
- [ ] Memory usage <100MB
- [ ] Stable 60fps at 1080p
- [ ] Input lag <50ms
- [ ] Acceptable artifacts for gaming

**Failure Modes**:
- CPU usage >40%
- Input lag >100ms
- Significant frame drops
- Excessive artifacts

**Notes**:
- Critical scenario for gaming users
- Test at 720p, 1080p, 1440p
- Monitor frame time graph

---

### PF-004: Camera Shake Reduction

**Objective**: Verify shake reduction with acceptable quality

**Test Content**:
1. Simulated camera shake test video
2. Real shaky handheld footage
3. Test video with vibration

**Steps**:
1. Add filter with Recording preset
2. Record 30 seconds each
3. Evaluate shake reduction
4. Check for over-correction

**Expected Result**: Significant shake reduction without over-correction

**Success Criteria**:
- [ ] Camera shake reduced by >70%
- [ ] Smooth output
- [ ] No over-correction (unnatural floaty movement)
- [ ] CPU usage 20-40%
- [ ] Acceptable quality for post-production

**Failure Modes**:
- Insufficient shake reduction
- Over-correction artifacts
- Output looks worse than input
- Excessive CPU usage

**Notes**:
- Test different shake intensities
- Compare Recording vs Streaming presets
- Evaluate quality vs. performance trade-off

---

### PF-005: Multi-Source Performance

**Objective**: Verify CPU usage scales correctly with multiple sources

**Test Configurations**:
1. Single source with filter
2. 2 sources with filters
3. 3 sources with filters
4. 5 sources with filters

**Steps**:
1. Add sources with filters
2. Monitor total CPU usage
3. Check individual CPU per source
4. Verify linear scaling

**Expected Result**: CPU usage increases proportionally

**Success Criteria**:
- [ ] CPU usage scales linearly with source count
- [ ] Each source with filter ~10-15% CPU
- [ ] Total CPU usage with 5 sources <80%
- [ ] No frame drops
- [ ] Stable frame rates

**Failure Modes**:
- CPU usage grows exponentially
- Some sources consume disproportionate CPU
- Frame drops with multiple sources
- System instability

**Notes**:
- Test with Gaming preset for low per-source CPU
- Test at 720p resolution
- Monitor memory usage scaling

---

### PF-006: Resolution Performance

**Objective**: Verify performance targets met across resolutions

**Test Resolutions**:
1. 480p (640x480 @ 60fps)
2. 720p (1280x720 @ 60fps)
3. 1080p (1920x1080 @ 30fps)
4. 1440p (2560x1440 @ 24fps)

**Steps**:
1. Use Gaming preset for each resolution
2. Record 30 seconds
3. Measure average processing time per frame
4. Monitor CPU usage

**Expected Performance Targets**:
- 480p: <5ms per frame, <10% CPU
- 720p: <8ms per frame, <25% CPU
- 1080p: <12ms per frame, <40% CPU
- 1440p: <20ms per frame, <60% CPU

**Success Criteria**:
- [ ] All targets met or exceeded
- [ ] Stable frame rate maintained
- [ ] No memory leaks
- [ ] Acceptable quality

**Failure Modes**:
- Processing time exceeds target
- CPU usage exceeds target
- Frame rate drops
- Memory grows continuously

**Notes**:
- Test on all three platforms
- Compare different presets
- Document actual performance vs. targets

---

## 3. Stabilization Quality Scenarios

### SQ-001: Static Scene Quality

**Objective**: Verify no artifacts in static scenes

**Test Content**:
1. Solid color source (black)
2. Static image (test pattern)
3. Text source (no movement)

**Steps**:
1. Add filter with Gaming preset (most sensitive)
2. Record 30 seconds
3. Analyze output frame-by-frame
4. Compare with input

**Expected Result**: Clean output, no artifacts

**Success Criteria**:
- [ ] No visible jitter or shaking
- [ ] No color shifts or artifacts
- [ ] Output identical to input (minus noise)
- [ ] No processing artifacts

**Failure Modes**:
- Visible jitter in static scene
- Color banding or shifts
- Ghosting or trailing artifacts
- Unexpected processing

**Notes**:
- Use Recording preset for comparison
- Test at different resolutions
- Zoom into output for detailed analysis

---

### SQ-002: Slow Pan Quality

**Objective**: Verify smooth panning without over-correction

**Test Content**:
1. Slow horizontal pan (30 seconds)
2. Slow vertical pan (30 seconds)
3. Diagonal pan (30 seconds)

**Steps**:
1. Use Streaming preset
2. Record each pan type
3. Analyze smoothness
4. Check for artifacts

**Expected Result**: Smooth camera movement, no jitter

**Success Criteria**:
- [ ] Pan is smooth and continuous
- [ ] No visible jitter or stuttering
- [ ] No over-correction (floaty movement)
- [ ] Maintains motion direction correctly
- [ ] No edge artifacts

**Failure Modes**:
- Jerky or choppy panning
- Over-correction (unnatural movement)
- Loss of fine detail
- Edge artifacts

**Notes**:
- Compare Gaming vs Streaming presets
- Test at 720p and 1080p
- Analyze frame-by-frame for issues

---

### SQ-003: Fast Motion Quality

**Objective**: Verify reduced shake without excessive latency

**Test Content**:
1. Fast FPS game (60fps gameplay)
2. Sudden camera turns
3. Rapid movements

**Steps**:
1. Use Gaming preset
2. Record 30 seconds
3. Analyze shake reduction
4. Measure perceived latency

**Expected Result**: Reduced shake with minimal lag

**Success Criteria**:
- [ ] Camera shake significantly reduced
- [ ] Input lag <50ms (not perceptible in gaming)
- [ ] Minor artifacts acceptable
- [ ] No over-correction
- - Smooth gameplay experience

**Failure Modes**:
- Excessive input lag (>100ms)
- Little or no shake reduction
- Over-correction artifacts
- Unplayable lag

**Notes**:
- Critical scenario for gaming
- Test at 720p and 1080p
- Use frame timing graph for latency analysis
- Compare with and without stabilization

---

### SQ-004: Complex Motion Quality

**Objective**: Verify handling of mixed motion types

**Test Content**:
1. Gaming footage with variable motion
2. Camera shake with sudden pans
3. Zoom in/out during movement

**Steps**:
1. Use Recording preset
2. Record 30 seconds
3. Analyze different motion segments
4. Evaluate quality transitions

**Expected Result**: Appropriate handling for all motion types

**Success Criteria**:
- [ ] Slow motion is smooth
- [ ] Fast motion has minimal lag
- [ ] Camera shake is reduced
- [ ] Transitions between motion types are smooth
- [ ] No over-correction
- [ ] Overall quality improvement

**Failure Modes**:
- Inappropriate smoothing for motion type
- Sudden quality changes
- Artifacts during transitions
- Over-correction in some segments

**Notes**:
- Test Adaptive Stabilization if available
- Compare preset performance
- Document optimal settings for each motion type

---

### SQ-005: Resolution Quality

**Objective**: Verify quality consistency across resolutions

**Test Content**:
1. Test video with complex motion
2. Test at 480p, 720p, 1080p, 1440p
3. Compare output quality

**Steps**:
1. Use Recording preset for each resolution
2. Record 30 seconds
3. Compare quality side-by-side
4. Analyze artifacts per resolution

**Expected Result**: Consistent quality across resolutions

**Success Criteria**:
- [ ] Quality comparable across resolutions
- [ ] No resolution-specific artifacts
- [ ] Appropriate smoothing for each resolution
- [ ] Fine details preserved at high resolution
- [ ] Stable frame rate

**Failure Modes**:
- Significant quality degradation at higher resolutions
- Resolution-specific artifacts
- Loss of fine details
- Inconsistent smoothing

**Notes**:
- Use same test video scaled to different resolutions
- Test with both Gaming and Recording presets
- Document optimal settings per resolution

---

### SQ-006: Edge Cases Quality

**Objective**: Verify robustness in unusual situations

**Test Cases**:
1. Empty source (black frames)
2. Scene switching during stabilization
3. Resolution change mid-stream
4. Very high feature count (1000)
5. Very low smoothing (5)

**Steps**:
1. Execute each edge case scenario
2. Monitor for crashes or artifacts
3. Verify graceful handling

**Expected Result**: No crashes, acceptable artifacts

**Success Criteria**:
- [ ] Empty source handled without crashes
- [ ] Scene switching doesn't cause artifacts
- [ ] Resolution changes work smoothly
- [ ] Extreme values don't crash
- [ ] Graceful degradation in edge cases

**Failure Modes**:
- Crash in any edge case
- Severe artifacts
- OBS hangs or freezes
- Memory leaks

**Notes**:
- Test each edge case independently
- Monitor system resources
- Document behavior for future reference

---

## 4. Edge Cases Scenarios

### EC-001: Empty Source Handling

**Objective**: Verify plugin handles empty/black frames gracefully

**Test Sources**:
1. Black screen color source
2. Disabled video source
3. Empty video file

**Steps**:
1. Add stabilizer filter
2. Enable OBS Stats (Frame Drops, Missed Frames)
3. Record 30 seconds
4. Monitor for crashes or artifacts

**Expected Result**: No crashes, minimal processing

**Success Criteria**:
- [ ] No OBS crashes
- [ ] No frame drops
- [ ] No excessive CPU usage
- [ ] Minimal processing overhead
- [ ] Stable frame rate

**Failure Modes**:
- OBS crashes on empty source
- Excessive CPU usage (should be near 0%)
- Frame drops occur
- Artifacts in output

**Notes**:
- Critical for reliability
- Test at all resolutions
- Compare with and without filter

---

### EC-002: Scene Switching

**Objective**: Verify smooth transitions when switching scenes

**Test Setup**:
1. Create Scene A with video source + stabilizer
2. Create Scene B with different video source + stabilizer
3. Rapidly switch between scenes
4. Record transitions

**Steps**:
1. Set Gaming preset on both filters
2. Switch A→B→A every 5 seconds for 1 minute
3. Monitor for artifacts during transitions
4. Check CPU usage spikes

**Expected Result**: Smooth scene changes without artifacts

**Success Criteria**:
- [ ] Scene transitions are smooth
- [ ] No artifacts at scene switch
- [ ] No CPU usage spikes
- [ ] Stabilization continues correctly in new scene
- [ ] No frame drops during transitions

**Failure Modes**:
- Artifacts at scene boundaries
- Jitter when switching scenes
- CPU spikes on transitions
- Stabilization stops working

**Notes**:
- Test with active streaming
- Test with different resolutions
- Monitor frame timing graph

---

### EC-003: Resolution Changes

**Objective**: Verify plugin handles resolution changes mid-stream

**Test Procedure**:
1. Start streaming at 720p
2. Change to 1080p during stream
3. Change back to 720p during stream
4. Record entire process

**Steps**:
1. Set Gaming preset
2. Start OBS Preview
3. Change source resolution (if supported by source)
4. Monitor for crashes or quality issues
5. Check OBS logs for errors

**Expected Result**: Smooth resolution changes

**Success Criteria**:
- [ ] Resolution changes don't crash OBS
- [ ] No artifacts during resolution switch
- [ ] Stabilization adapts to new resolution
- [ ] Frame rate adjusts appropriately
- [ ] No memory leaks

**Failure Modes**:
- OBS crashes on resolution change
- Artifacts or corruption
- Stabilization stops working
- Memory leak detected

**Notes**:
- Critical for VOD/streaming
- Test with different source types
- Monitor memory usage during changes

---

### EC-004: Multiple Filters

**Objective**: Verify plugin works with multiple other filters

**Test Configurations**:
1. Stabilizer + Color Correction
2. Stabilizer + Crop
3. Stabilizer + Sharpen
4. Stabilizer + Chroma Key
5. All 4 filters combined

**Steps**:
1. Add stabilizer to source
2. Add additional filter(s)
3. Test each combination
4. Reorder filters to test effect order

**Expected Result**: Stable output with all filters active

**Success Criteria**:
- [ ] All filter combinations work
- [ ] No crashes with multiple filters
- [ ] Order doesn't cause issues
- - CPU usage acceptable (<60%)
- - Output quality preserved

**Failure Modes**:
- OBS crashes with certain filter combinations
- Filters conflict or cancel each other
- Order-dependent artifacts
- Excessive CPU usage

**Notes**:
- Test each combination independently
- Monitor processing order
- Document any filter order dependencies

---

### EC-005: OBS Start/Stop

**Objective**: Verify plugin initializes/cleans up correctly

**Test Procedure**:
1. Start OBS with filter active
2. Start streaming or recording
3. Stop OBS
4. Restart OBS
5. Check plugin state

**Steps**:
1. Configure filter with Recording preset
2. Start recording 30-second video
3. Stop OBS mid-recording
4. Restart OBS and check filter state

**Expected Result**: Clean initialization and shutdown

**Success Criteria**:
- [ ] Plugin initializes correctly on OBS start
- [ ] No crashes during recording
- [ ] Clean shutdown on OBS stop
- [ ] No memory leaks detected
- [ ] Settings persist across sessions

**Failure Modes**:
- OBS crashes on start/stop
- Memory leaks detected
- Settings lost on restart
- Plugin state inconsistent

**Notes**:
- Check OBS logs for plugin messages
- Monitor memory usage over time
- Test all three platforms

---

## 5. Multi-Platform Scenarios

### MP-001: Windows Compatibility

**Objective**: Verify plugin works correctly on Windows

**Test Environments**:
- Windows 10
- Windows 11
- OBS Studio 30.0, 31.0
- Both standard and OpenCV-bundled builds

**Test Matrix**:
| Scenario | Windows 10 | Windows 11 | OBS 30.0 | OBS 31.0 | Notes |
|----------|-------------|-------------|-----------|-------|
| Installation | ✅ | ✅ | ✅ | ✅ | Plugin loads correctly |
| Basic Functionality | ✅ | ✅ | ✅ | ✅ | All features work |
| Performance 720p | ✅ | ✅ | ✅ | ✅ | <8ms, <25% CPU |
| Performance 1080p | ✅ | ✅ | ✅ | ✅ | <12ms, <40% CPU |
| Presets | ✅ | ✅ | ✅ | ✅ | All presets work |
| UI | ✅ | ✅ | ✅ | ✅ | All controls work |

**Success Criteria**:
- [ ] Plugin installs on Windows 10 and 11
- [ ] Compatible with OBS 30.0 and 31.0
- [ ] All basic functionality works
- [ ] Performance targets met
- [ ] No platform-specific crashes
- [ ] Presets work correctly

**Notes**:
- Test both x64 and ARM64 builds (if applicable)
- Verify OpenCV dependencies are correct

---

### MP-002: macOS Compatibility

**Objective**: Verify plugin works correctly on macOS

**Test Environments**:
- macOS Ventura (13.0)
- macOS Sonoma (14.0)
- macOS Sequoia (15.0)
- OBS Studio 30.0, 31.0
- Intel Mac and Apple Silicon (M1/M2/M3/M4)
- Both standard and OpenCV-bundled builds

**Test Matrix**:
| Scenario | Intel | Apple Silicon | Ventura | Sonoma | Sequoia | Notes |
|----------|-------|--------------|----------|----------|---------|-------|
| Installation | ✅ | ✅ | ✅ | ✅ | ✅ | Plugin loads |
| Basic Functionality | ✅ | ✅ | ✅ | ✅ | ✅ | All features work |
| Performance 720p | ✅ | ✅ | ✅ | ✅ | ✅ | <8ms, <25% CPU |
| Performance 1080p | ✅ | ✅ | ✅ | ✅ | ✅ | <12ms, <40% CPU |
| Presets | ✅ | ✅ | ✅ | ✅ | ✅ | All presets work |
| ARM64 Native | - | ✅ | ✅ | ✅ | ✅ | Native performance |
| Plugin Loading Fix | ✅ | ✅ | ✅ | ✅ | ✅ | Script works |

**Success Criteria**:
- [ ] Plugin installs on Intel and Apple Silicon Macs
- [ ] Compatible with macOS 13.0, 14.0, 15.0
- [ ] Compatible with OBS 30.0 and 31.0
- [ ] ARM64 native performance verified
- [ ] Plugin loading fix script works
- [ ] All basic functionality works
- [ ] Performance targets met
- [ ] No macOS-specific crashes

**Notes**:
- Test plugin loading fix script on all macOS versions
- Verify Frameworks are loaded correctly
- Test OpenCV bundle with Frameworks directory
- Monitor Activity Monitor for ARM64 efficiency

---

### MP-003: Linux Compatibility

**Objective**: Verify plugin works correctly on Linux

**Test Environments**:
- Ubuntu 22.04 LTS
- Ubuntu 24.04 LTS
- Debian 12
- Fedora 39
- OBS Studio 30.0, 31.0
- Both standard and OpenCV-bundled builds

**Test Matrix**:
| Scenario | Ubuntu 22.04 | Ubuntu 24.04 | Debian 12 | Fedora 39 | Notes |
|----------|----------------|----------------|-----------|----------|---------|
| Installation | ✅ | ✅ | ✅ | ✅ | Plugin loads |
| Dependencies | ✅ | ✅ | ✅ | ✅ | ldd shows correct deps |
| Basic Functionality | ✅ | ✅ | ✅ | ✅ | All features work |
| Performance 720p | ✅ | ✅ | ✅ | ✅ | <8ms, <25% CPU |
| Performance 1080p | ✅ | ✅ | ✅ | ✅ | <12ms, <40% CPU |
| Presets | ✅ | ✅ | ✅ | ✅ | All presets work |

**Success Criteria**:
- [ ] Plugin installs on all Linux distributions
- [ ] Compatible with OBS 30.0 and 31.0
- [ ] No missing dependencies
- [ ] All basic functionality works
- [ ] Performance targets met
- [ ] No Linux-specific crashes
- [ ] All presets work correctly

**Notes**:
- Verify OpenCV dependencies with `ldd`
- Test with Wayland and X11 (if applicable)
- Monitor CPU usage with `htop` or `top`

---

### MP-004: Cross-Platform Consistency

**Objective**: Verify behavior is consistent across platforms

**Test Procedure**:
1. Run same test scenarios on Windows, macOS, Linux
2. Compare results
3. Verify identical behavior

**Test Cases**:
- BF-001: Installation and Loading
- BF-004: Preset Application
- PF-001: Static Scene Performance
- SQ-002: Slow Pan Quality

**Expected Result**: Consistent behavior across platforms

**Success Criteria**:
- [ ] All three platforms pass same tests
- [ ] Performance within expected ranges (±20%)
- [ ] Quality metrics consistent
- [ ] No platform-specific bugs
- [ ] Presets apply identically

**Failure Modes**:
- Platform-specific failures
- Significant performance differences
- Quality variation between platforms
- Inconsistent feature behavior

**Notes**:
- Use identical test content on all platforms
- Document any necessary platform-specific adjustments
- Test at same resolutions

---

## 6. Regression Scenarios

### RG-001: Previous Version Comparison

**Objective**: Verify no regression from previous plugin version

**Test Procedure**:
1. Install previous plugin version (known good)
2. Record baseline test results
3. Install new plugin version
4. Run same tests
5. Compare results

**Test Cases**:
1. Basic functionality
2. Performance (CPU, memory, FPS)
3. Quality (stabilization effectiveness)
4. UI functionality
5. Stability (crashes, hangs)

**Expected Result**: New version equals or exceeds previous version

**Success Criteria**:
- [ ] All features work as well or better
- [ ] Performance is equal or better
- [ ] Quality is equal or better
- [ ] No new bugs introduced
- [ ] No regressions detected

**Failure Modes**:
- Performance degradation (>20% slower)
- Quality degradation
- New bugs or crashes
- Missing features

**Notes**:
- Document exact versions tested
- Use same test content for fair comparison
- Compare both standard and OpenCV-bundled builds

---

### RG-002: Fixed Issues Validation

**Objective**: Verify previously fixed issues remain fixed

**Previously Fixed Issues**:
- Issue #218: apple_accelerate.hpp removal
- Issue #217: Apple Accelerate code cleanup
- Issue #214: Memory leak in exception handling
- Issue #213: CMakeLists.txt test file references

**Test Procedure**:
1. Install latest plugin version
2. Run tests that would have caught original issues
3. Verify no symptoms of original bugs

**Expected Result**: Issues remain fixed

**Success Criteria**:
- [ ] apple_accelerate.hpp file does not exist in build
- [ ] No memory leaks from exception handling
- [ ] Build system works correctly
- [ ] All unit tests pass
- [ ] No crashes related to fixed issues

**Failure Modes**:
- Original bug reappears
- Related issue discovered
- New regression introduced

**Notes**:
- Use commit history to identify all fixed issues
- Test on all three platforms
- Document findings

---

## Running E2E Tests

### Pre-Test Checklist

**Environment Setup**:
- [ ] OBS Studio version verified (30.0+)
- [ ] Test video content prepared
- [ ] Test results template ready
- [ ] System monitoring tools running
- [ ] OBS logs location identified

**System State**:
- [ ] Clean OBS profile (no conflicting plugins)
- [ ] Adequate system resources (8GB+ RAM, 4+ cores)
- [ ] Sufficient disk space for recordings
- [ ] Network stable (if streaming)

**Test Session Preparation**:
- [ ] Test scenario identified
- [ ] Success criteria documented
- [ ] Expected behavior defined
- [ ] Known issues documented

### During-Test Notes Template

**Scenario**: [Name]
**Date/Time**: [YYYY-MM-DD HH:MM]
**OBS Version**: [Version]
**Platform**: [Windows/macOS/Linux]
**Plugin Version**: [Version]

**Pre-Test State**:
- [ ] OBS restarted
- [ ] Clean test environment
- [ ] Expected behavior reviewed

**Test Execution**:
1. [ ] Steps 1-N executed
2. [ ] Observations recorded
3. [ ] Deviations noted

**Test Results**:
- [ ] Pass/Fail for each criterion
- [ ] CPU usage: [X]%
- [ ] Memory usage: [X]MB
- [ ] Frame drops: [X]
- [ ] Artifacts: [Description]
- [ ] Crash: Yes/No

**Post-Test State**:
- [ ] OBS closed gracefully
- [ ] No system issues
- [ ] Test results documented

**Issues Found**:
- [ ] Issue description
- [ ] Severity (Critical/High/Medium/Low)
- [ ] Steps to reproduce

**Recommendations**:
- [ ] Suggestion 1
- [ ] Suggestion 2

---

## Test Results Aggregation

### Daily Test Summary

**Date**: [YYYY-MM-DD]
**Tester**: [Name]
**Platform**: [Windows/macOS/Linux]

**Summary**:
- Total Scenarios Run: [X]
- Scenarios Passed: [X]
- Scenarios Failed: [X]
- Pass Rate: [X]%
- Critical Issues Found: [X]
- High Issues Found: [X]
- Medium Issues Found: [X]
- Low Issues Found: [X]

**Scenario Breakdown**:
| Category | Passed | Failed | Notes |
|----------|--------|--------|-------|
| Basic Functionality | [X]/[Y] | [Details] |
| Performance | [X]/[Y] | [Details] |
| Quality | [X]/[Y] | [Details] |
| Edge Cases | [X]/[Y] | [Details] |
| Multi-Platform | [X]/[Y] | [Details] |

### Regression Tracking

| Version | Release Date | Critical Issues | High Issues | Medium Issues | Low Issues | Notes |
|---------|--------------|---------------|-------------|-------------|----------|-------|
| [v1.0] | [Date] | [X] | [X] | [X] | [X] | [Details] |
| [v1.1] | [Date] | [X] | [X] | [X] | [X] | [Details] |

---

## Continuous Testing Integration

### Automated Smoke Tests

**Purpose**: Quick validation on every commit

**Test Suite**: Minimal critical path tests

**Tests**:
1. **Load Test**: Plugin loads in OBS without crashes
2. **Filter Test**: Can add filter to source
3. **Basic Stabilization**: Filter processes static scene
4. **Presets**: Can switch between presets

**Automation Level**: Manual (run on each commit before PR)

**Success Criteria**:
- [ ] All smoke tests pass on all platforms
- [ ] No crashes in basic operations
- [ ] Filter functional in OBS

**Integration with CI/CD**:
- [ ] Tests added to CI/CD pipeline (future)
- [ ] Tests run on every commit
- [ ] Failures block merges
- [ ] Results reported in PR

---

## Best Practices

### Test Content Preparation

**Guidelines**:
1. **Use Real-World Content**: Avoid synthetic patterns when possible
2. **Multiple Test Types**: Include gaming, vlogging, screen recording
3. **Vary Resolutions**: Test at 480p, 720p, 1080p, 1440p
4. **Vary Motion Types**: Static, slow pan, fast, shake, complex
5. **Consistent Conditions**: Use same OBS settings for all tests
6. **Document Everything**: Record all parameters, OBS version, system specs

### Test Execution

**Guidelines**:
1. **Start Fresh**: Clean OBS profile each test session
2. **Monitor Resources**: Watch CPU, memory, FPS continuously
3. **Test One Thing at a Time**: Change only one variable at a time
4. **Document Observations**: Note deviations immediately
5. **Reproduce Bugs**: Don't assume, verify with exact steps
6. **Test Edge Cases**: Don't ignore corner cases
7. **Use System Monitor**: Activity Monitor, Task Manager, or htop
8. **Check OBS Logs**: Always review OBS logs for errors

### Result Reporting

**Guidelines**:
1. **Be Specific**: Include exact numbers (CPU: 23%, not "medium")
2. **Include Context**: OBS version, platform, test video
3. **Evidence-Based**: Describe artifacts, don't just say "bad quality"
4. **Compare Baselines**: Always note before/after comparison
5. **Suggest Solutions**: Don't just report problems
6. **Categorize Issues**: Separate by severity and category
7. **Use Templates**: Fill out test results template completely
8. **Take Screenshots**: Capture artifacts and UI states
9. **Record Videos**: Keep 10-30s clips for reference
10. **Verify All Platforms**: Test on Windows, macOS, Linux

---

## Related Documentation

- **docs/testing/e2e-testing-guide.md** - Complete E2E testing procedures
- **docs/architecture/ARCHITECTURE.md** - System design and components
- **README.md** - User guide and troubleshooting
- **AGENTS.md** - Development workflow and completed issues

---

## Version History

- **1.0** - Initial E2E test scenarios (Issue #220 Phase 1)
  - 6 scenario categories
  - 30+ detailed test scenarios
  - Test results templates
  - Best practices and procedures
  - Integration with CI/CD considerations
