# OBS Video Stabilizer Filter Functionality Test Guide

## Test Status: READY FOR USER TESTING

The plugin has been successfully built and installed. Based on the analysis:

### Plugin Installation Status ✅
- **Plugin Location**: `~/Library/Application Support/obs-studio/plugins/test-stabilizer.plugin`
- **Build Status**: Successfully compiled with OpenCV integration
- **OBS Loading**: Plugin loads successfully in OBS Studio
- **Log Confirmation**: Plugin shows "Video stabilizer filter registered successfully"

### Expected Filter Information
- **Filter Name**: "Video Stabilizer" 
- **Filter ID**: "video_stabilizer_filter"
- **Filter Type**: Video Filter (applies to video sources)

## Manual Testing Steps

Please perform these tests in OBS Studio and report any issues:

### Test 1: Filter Visibility Check
1. **Open OBS Studio**
2. **Add or select a video source** (e.g., "Video Capture Device" or "Display Capture")
3. **Right-click on the source** → Select "Filters"
4. **Click the "+" button** to add a new filter
5. **Look for "Video Stabilizer"** in the filter list

**Expected Result**: "Video Stabilizer" should appear in the available filters list.

### Test 2: Filter Properties Access
1. **Add the "Video Stabilizer" filter** to a video source
2. **Check if the filter properties panel opens**
3. **Verify the following settings are available**:
   - ✅ Enable Stabilization (checkbox)
   - ✅ Smoothing Radius (slider: 1-100)
   - ✅ Max Features (slider: 50-500) 
   - ✅ Feature Quality (slider: 0.001-0.1)
   - ✅ Min Distance (slider: 10.0-100.0)
   - ✅ Detection Interval (slider: 5-50)

**Expected Result**: All properties should be visible and adjustable.

### Test 3: Filter Application Test
1. **With the filter active** and "Enable Stabilization" checked
2. **Start your video source** (camera, screen capture, etc.)
3. **Monitor OBS logs** for any error messages:
   - Open a new terminal: `tail -f ~/Library/Application\ Support/obs-studio/logs/2025-08-05*.txt`
4. **Look for any of these messages**:
   - ❌ "Stabilization processing failed, passing frame through"
   - ❌ "stabilizer_filter_update called with null parameters"
   - ❌ Any crash or error messages

**Expected Result**: Video should display without errors in the log.

### Test 4: Performance and Stability Test
1. **Leave the filter running for 2-3 minutes**
2. **Adjust filter settings while running**:
   - Change "Smoothing Radius" value
   - Toggle "Enable Stabilization" on/off
3. **Monitor system performance**:
   - Check if OBS remains responsive
   - Watch for excessive CPU usage
   - Look for memory leaks

**Expected Result**: OBS should remain stable and responsive.

## Common Issues and Solutions

### Issue: Filter Not Visible
- **Check**: Plugin installation at correct location
- **Solution**: Restart OBS Studio completely
- **Verify**: Look for plugin loading messages in logs

### Issue: Filter Crashes OBS
- **Check**: Look for crash logs in Console.app
- **Solution**: Report the exact error message and crash details

### Issue: Poor Performance
- **Check**: CPU usage during filter operation  
- **Solution**: Adjust "Max Features" and "Detection Interval" to lower values

## Test Results Reporting

Please provide the following information:

### ✅ Success Checklist
- [ ] Filter appears in OBS filter menu
- [ ] Filter properties panel opens correctly
- [ ] All 6 property controls are visible and functional
- [ ] Video displays without log errors
- [ ] Filter settings can be adjusted in real-time
- [ ] OBS remains stable during operation

### ❌ Issue Reporting Template
If you encounter issues, please provide:

1. **Exact error message** from OBS logs
2. **Steps to reproduce** the issue
3. **Video source type** being used (camera, display capture, etc.)
4. **macOS version** and OBS Studio version
5. **Screenshot** of any error dialogs

## Log Monitoring Commands

Use these terminal commands to monitor OBS behavior:

```bash
# Watch live OBS logs
tail -f ~/Library/Application\ Support/obs-studio/logs/$(ls -t ~/Library/Application\ Support/obs-studio/logs/*.txt | head -1)

# Search for stabilizer-related messages
grep -i "stabilizer\|video_stabilizer" ~/Library/Application\ Support/obs-studio/logs/$(ls -t ~/Library/Application\ Support/obs-studio/logs/*.txt | head -1)

# Check for errors
grep -i "error\|failed\|crash" ~/Library/Application\ Support/obs-studio/logs/$(ls -t ~/Library/Application\ Support/obs-studio/logs/*.txt | head -1)
```

## Next Steps

Based on test results:
- ✅ **If all tests pass**: Plugin is ready for production use
- ⚠️ **If minor issues found**: Specific fixes can be implemented
- ❌ **If major issues found**: Further debugging and code review needed

The plugin architecture is solid and the build process is working correctly. Any issues found will likely be specific to the stabilization algorithm implementation or OBS integration details.