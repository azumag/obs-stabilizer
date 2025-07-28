# OBS Stabilizer Plugin Loading Test Results

**Test Date:** 2025-07-28
**Environment:** macOS 15.4.1 (Apple M4), OBS Studio 31.1.1
**Plugin Version:** 0.1.0

## Executive Summary

The obs-stabilizer plugin fails to load in OBS due to an incorrect install name in the binary. The plugin binary has `@rpath/libobs-stabilizer.0.dylib` as its install name instead of the expected plugin bundle path. This causes dlopen to fail when OBS attempts to load the plugin.

## Test Results

### 1. Plugin Bundle Structure Verification ✅ PASS

**Test:** Verify correct macOS plugin bundle structure
**Result:** Structure is correct

```
obs-stabilizer.plugin/
├── Contents/
│   ├── Info.plist          ✅ Present and valid
│   ├── MacOS/
│   │   └── obs-stabilizer  ✅ Binary present
│   └── Resources/
│       └── locale/
│           └── en-US.ini   ✅ Localization present
```

### 2. Binary Architecture and Dependencies ❌ FAIL

**Test:** Check binary compatibility and dependencies
**Result:** Critical issues found

**Architecture:** ✅ Correct (arm64, matches OBS)
```
Mach-O 64-bit dynamically linked shared library arm64
```

**Install Name:** ❌ INCORRECT
```
Current: @rpath/libobs-stabilizer.0.dylib
Expected: obs-stabilizer.plugin/Contents/MacOS/obs-stabilizer
```

**Dependencies:** ⚠️ WARNING - Hardcoded paths
```
/opt/homebrew/opt/opencv/lib/libopencv_*.412.dylib
```

### 3. Info.plist Validation ✅ PASS

**Test:** Validate bundle metadata
**Result:** Info.plist is properly formatted

Key fields verified:
- CFBundleExecutable: obs-stabilizer ✅
- CFBundleIdentifier: com.obsproject.obs-stabilizer ✅
- CFBundlePackageType: BNDL ✅
- CFBundleVersion: 0.1.0 ✅

### 4. Code Signing Status ⚠️ WARNING

**Test:** Verify code signature
**Result:** Ad-hoc signing only

```
Signature=adhoc
Info.plist=not bound
```

Recommendation: Sign with developer certificate for production.

### 5. Symbol Exports ✅ PASS

**Test:** Verify required OBS module symbols
**Result:** All required symbols exported correctly

```
✅ _obs_module_load
✅ _obs_module_unload
✅ _obs_module_name
✅ __Z22obs_module_descriptionv (C++ mangled, but MODULE_EXPORT should handle)
```

### 6. Installation Location ✅ PASS

**Test:** Verify plugin installed to correct directory
**Result:** Correctly installed

```
Location: /Applications/OBS.app/Contents/PlugIns/obs-stabilizer.plugin
Permissions: drwxr-xr-x (755) ✅
```

## Root Cause Analysis

The plugin fails to load due to the following chain of events:

1. OBS scans `/Applications/OBS.app/Contents/PlugIns/` directory
2. OBS finds `obs-stabilizer.plugin` bundle
3. OBS attempts to dlopen `obs-stabilizer.plugin/Contents/MacOS/obs-stabilizer`
4. dlopen reads the binary's install name: `@rpath/libobs-stabilizer.0.dylib`
5. dlopen attempts to resolve this path using OBS's rpath
6. Resolution fails because the path doesn't exist
7. Plugin loading fails silently (OBS doesn't log dlopen failures by default)

## Immediate Fix

Run the provided fix script:
```bash
./fix-plugin-loading.sh
```

This script:
1. Corrects the install name using `install_name_tool`
2. Bundles OpenCV dependencies with @loader_path references
3. Properly signs the bundle

## Permanent Fix

Add to CMakeLists.txt:
```cmake
include(cmake/macOS-plugin-fix.cmake)
```

This ensures all future builds have the correct install name and bundled dependencies.

## Verification Steps

After applying the fix:

1. Check install name:
   ```bash
   otool -D obs-stabilizer.plugin/Contents/MacOS/obs-stabilizer
   ```
   Should show: `obs-stabilizer.plugin/Contents/MacOS/obs-stabilizer`

2. Verify in OBS:
   - Restart OBS
   - Check Tools → Log Files → View Current Log
   - Look for: "Loading OBS Stabilizer Plugin v0.1.0"
   - Check Filters → Video Filters for "Stabilizer" option

## Severity Assessment

- **Overall Severity:** CRITICAL - Plugin completely non-functional
- **User Impact:** Plugin appears to install but never loads
- **Fix Complexity:** Low - Simple install name correction
- **Risk:** Low - Changes only affect plugin loading mechanism

## Recommendations

1. **Immediate:** Apply the fix script to correct install name
2. **Short-term:** Update CMakeLists.txt with permanent fix
3. **Long-term:** Add automated tests for plugin loading
4. **Future:** Consider using OBS Plugin Template's CMake helpers for macOS