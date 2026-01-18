# Issue #001: Crash when accessing settings in filter update

## Summary
Plugin crashes with SIGSEGV when attempting to read parameters from `obs_data_t *settings` in the `minimal_filter_update()` function.

## Reproduction Steps
1. Build and install the minimal safe plugin
2. Start OBS with a scene containing the filter
3. Filter update is called during initialization
4. Crash occurs at `obs_data_get_bool(settings, "enabled")`

## Technical Details

### Crash Location
```cpp
static void minimal_filter_update(void *data, obs_data_t *settings)
{
    // ... validation code ...
    
    // CRASH HAPPENS HERE:
    bool new_enabled = obs_data_get_bool(settings, "enabled");  // <- SIGSEGV
}
```

### Log Analysis
```
info: === FILTER UPDATE CALLED ===
info: Data pointer: 0x600002840c20     // Valid heap address
info: Settings pointer: 0x16d8a1d18    // Suspicious stack-like address
info: Filter pointer valid: 0x600002840c20
info: Attempting to read 'enabled' parameter...
[CRASH]
```

### Root Cause Analysis
The `settings` pointer appears to be invalid:
- Address `0x16d8a1d18` looks like a stack address, not a heap address
- The pointer passes NULL check but is not a valid `obs_data_t` object
- This suggests the settings object may be corrupted or incorrectly passed

## Hypothesis
1. **Stack Corruption**: The settings object might be allocated on the stack and becomes invalid
2. **ABI Mismatch**: Function signature might not match OBS's expectations
3. **Thread Safety**: Settings might be accessed from wrong thread
4. **Lifecycle Issue**: Settings object might be freed before use

## Workaround Attempts

### Attempt 1: Validate settings pointer range
Check if settings pointer is in valid heap range before accessing.

### Attempt 2: Use different settings access method
Try using `obs_data_get_default_bool` or check if key exists first.

### Attempt 3: Defer settings access
Store settings for later use instead of immediate access.

## Impact
- **Severity**: Critical
- **Affected Version**: All versions that attempt to read settings
- **Workaround**: Return immediately without reading settings (current safe version)

## Next Steps
1. Investigate OBS source code for correct settings handling
2. Check if settings lifetime is managed correctly
3. Test with different OBS API patterns
4. Consider using properties callback instead of update