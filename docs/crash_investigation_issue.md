# OBS Stabilizer Plugin Crash Investigation Issue

## Issue: Filter Update Crash When Applied Through UI

### Current Status
- **Safe Version**: Works without crash (minimal_filter_update does nothing)
- **Step 1 Test**: Basic parameter reading works without crash

### Investigation Progress

#### Step 0: Baseline (Safe)
- **File**: `src/minimal_safe_plugin.cpp`
- **Function**: `minimal_filter_update()`
- **Behavior**: Returns immediately after NULL checks
- **Result**: ✅ No crash

#### Step 1: Basic Parameter Reading
- **File**: `src/minimal_step1_plugin.cpp`
- **Changes**: Added `obs_data_get_bool()` call and filter data update
- **Test Code**:
```cpp
bool new_enabled = obs_data_get_bool(settings, "enabled");
filter->enabled = new_enabled;
```
- **Result**: ✅ No crash

### Next Steps

#### Step 2: Testing with existing plugin
- Modify existing `minimal_safe_plugin.cpp` to add parameter reading
- Test with UI filter application
- Identify exact crash point

#### Step 3: Add more complex operations
- Test string parameter reading
- Test memory allocation
- Test pointer dereferencing

### Crash Symptoms (Original)
- **When**: Applying filter through UI (right-click → Filters → Add)
- **Type**: SIGSEGV
- **Address**: 0x0000000000000008
- **Function**: `minimal_filter_update`

### Hypothesis
1. Settings object might be invalid when called from UI
2. Race condition during UI update
3. Memory alignment issue
4. Incorrect casting of data pointer

### Test Plan
1. Add comprehensive logging to track execution flow
2. Test parameter access patterns
3. Verify pointer validity at each step
4. Test with different data types

### Related Files
- `/Users/azumag/work/obs-stabilizer/src/minimal_safe_plugin.cpp`
- `/Users/azumag/work/obs-stabilizer/src/minimal_step1_plugin.cpp`
- `/Users/azumag/work/obs-stabilizer/tmp/e2e/auto_apply_filter.sh`