#!/bin/bash

# Fix script for build issues
# Automatically resolves common build problems

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "Attempting to fix build issues..."

FIXES_APPLIED=0

# Fix 1: Install missing dependencies
if ! command -v cmake &>/dev/null; then
	echo "Fixing: Installing CMake..."
	brew install cmake
	FIXES_APPLIED=$((FIXES_APPLIED + 1))
fi

if ! command -v ninja &>/dev/null; then
	echo "Fixing: Installing Ninja..."
	brew install ninja
	FIXES_APPLIED=$((FIXES_APPLIED + 1))
fi

# Fix 2: Install OpenCV if missing
if ! pkg-config --exists opencv4 2>/dev/null && ! pkg-config --exists opencv 2>/dev/null; then
	echo "Fixing: Installing OpenCV..."
	brew install opencv
	FIXES_APPLIED=$((FIXES_APPLIED + 1))
fi

# Fix 3: Create and prepare build directory
BUILD_DIR="$PROJECT_ROOT/build"
if [ ! -d "$BUILD_DIR" ]; then
	echo "Fixing: Creating build directory..."
	mkdir -p "$BUILD_DIR"
	FIXES_APPLIED=$((FIXES_APPLIED + 1))
fi

# Fix 4: Clean corrupted build
if [ -d "$BUILD_DIR" ] && [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
	echo "Fixing: Cleaning corrupted build cache..."
	cd "$BUILD_DIR"
	rm -rf CMakeCache.txt CMakeFiles/
	FIXES_APPLIED=$((FIXES_APPLIED + 1))
fi

# Fix 5: Fix CMakeLists.txt if OBS library path is incorrect
if [ -f "$PROJECT_ROOT/CMakeLists.txt" ]; then
	CMAKELISTS="$PROJECT_ROOT/CMakeLists.txt"

	# Check if OBS library path needs updating
	if ! grep -q 'Applications/OBS.app' "$CMAKELISTS" 2>/dev/null; then
		echo "Fixing: Updating CMakeLists.txt with correct OBS library path..."

		# Backup original
		cp "$CMAKELISTS" "$CMAKELISTS.backup"

		# Update OBS library path (using sed)
		sed -i '' 's|/usr/lib|/Applications/OBS.app/Contents/Frameworks|g' "$CMAKELISTS" 2>/dev/null || true

		echo "✓ Updated CMakeLists.txt"
		FIXES_APPLIED=$((FIXES_APPLIED + 1))
	fi
fi

# Fix 6: Fix plugin-support.c symbol bridging if missing
if [ -f "$PROJECT_ROOT/src/plugin-support.c" ]; then
	PLUGIN_SUPPORT="$PROJECT_ROOT/src/plugin-support.c"

	# Check if obs_register_source function is defined
	if ! grep -q 'bool obs_register_source' "$PLUGIN_SUPPORT" 2>/dev/null; then
		echo "Fixing: Adding symbol bridging to plugin-support.c..."

		# Backup original
		cp "$PLUGIN_SUPPORT" "$PLUGIN_SUPPORT.backup"

		# Add symbol bridging functions
		cat >>"$PLUGIN_SUPPORT" <<'EOF'

// Symbol bridging for OBS API compatibility
#ifdef HAVE_OBS_HEADERS
bool obs_register_source(struct obs_source_info *info)
{
    extern bool obs_register_source_s(struct obs_source_info *info, size_t size);
    return obs_register_source_s(info, sizeof(*info));
}

void obs_log(int log_level, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    extern void blogva(int log_level, const char *format, va_list args);
    blogva(log_level, format, args);
    va_end(args);
}
#endif
EOF

		echo "✓ Added symbol bridging"
		FIXES_APPLIED=$((FIXES_APPLIED + 1))
	fi
fi

# Fix 7: Update RPATH for OpenCV libraries
if [ -f "$PROJECT_ROOT/CMakeLists.txt" ]; then
	CMAKELISTS="$PROJECT_ROOT/CMakeLists.txt"

	if ! grep -q 'homebrew/lib' "$CMAKELISTS" 2>/dev/null; then
		echo "Fixing: Adding Homebrew library path to RPATH..."

		# Backup original
		cp "$CMAKELISTS" "$CMAKELISTS.rpath_backup"

		# Add Homebrew paths
		if grep -q 'INSTALL_RPATH' "$CMAKELISTS"; then
			sed -i '' 's|INSTALL_RPATH "@loader_path/\.\./Frameworks;@loader_path/\.\./Resources/lib"|INSTALL_RPATH "@loader_path/../Frameworks;@loader_path/../Resources/lib;/opt/homebrew/lib;/usr/local/lib"|g' "$CMAKELISTS"
		fi

		echo "✓ Updated RPATH"
		FIXES_APPLIED=$((FIXES_APPLIED + 1))
	fi
fi

echo ""
echo "Build fixes complete. Applied $FIXES_APPLIED fixes."

if [ $FIXES_APPLIED -eq 0 ]; then
	echo "No fixes were needed. The issue may be in the source code."
	exit 1
else
	exit 0
fi
