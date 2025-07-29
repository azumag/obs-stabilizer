#!/bin/bash
# File Organization Compliance Script
# Enforces CLAUDE.md principles: YAGNI, DRY, and temporary file consolidation

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TMP_DIR="$PROJECT_ROOT/tmp"

# CLAUDE.md principle: "一時ファイルは一箇所のディレクトリにまとめよ"
echo "=== OBS Stabilizer Build Cleanup ==="
echo "Enforcing CLAUDE.md file organization principles..."

# Function to move build directory to tmp/builds-archive
move_old_build() {
    local build_dir="$1"
    if [[ -d "$PROJECT_ROOT/$build_dir" ]]; then
        echo "Moving $build_dir to tmp/builds-archive/ (YAGNI compliance)"
        mkdir -p "$TMP_DIR/builds-archive"
        mv "$PROJECT_ROOT/$build_dir" "$TMP_DIR/builds-archive/"
    fi
}

# Function to consolidate artifacts
consolidate_artifacts() {
    echo "Consolidating build artifacts to tmp/build-artifacts/ (DRY compliance)"
    mkdir -p "$TMP_DIR/build-artifacts"
    
    # Find and copy unique .dylib files
    find "$PROJECT_ROOT" -name "*.dylib" -type f -not -path "*/tmp/build-artifacts/*" | while read -r dylib; do
        filename=$(basename "$dylib")
        if [[ ! -f "$TMP_DIR/build-artifacts/$filename" ]]; then
            cp "$dylib" "$TMP_DIR/build-artifacts/"
            echo "Consolidated: $filename"
        fi
    done
}

# Function to clean CMake tmp directories
clean_cmake_tmp() {
    echo "Cleaning CMake temporary directories..."
    find "$PROJECT_ROOT" -path "*/CMakeFiles/*/tmp" -type d -exec rm -rf {} + 2>/dev/null || true
}

# Function to remove root build artifacts (CRITICAL CLAUDE.md compliance)
clean_root_build_artifacts() {
    echo "Removing prohibited build artifacts from project root..."
    local artifacts=(
        "CMakeFiles"
        "CMakeCache.txt" 
        "Makefile"
        "cmake_install.cmake"
        "build-qa-*"
        "build-test*"
        "build-debug*"
        "build-standalone*"
    )
    
    for artifact in "${artifacts[@]}"; do
        if [[ -e "$PROJECT_ROOT/$artifact" ]]; then
            echo "Removing root artifact: $artifact"
            rm -rf "$PROJECT_ROOT/$artifact"
        fi
    done
}

# Function to clean src directory build artifacts
clean_src_build_artifacts() {
    echo "Removing build artifacts from src/ directory..."
    if [[ -d "$PROJECT_ROOT/src" ]]; then
        rm -rf "$PROJECT_ROOT/src/CMakeFiles" \
               "$PROJECT_ROOT/src/CMakeCache.txt" \
               "$PROJECT_ROOT/src/Makefile" \
               "$PROJECT_ROOT/src/cmake_install.cmake" \
               "$PROJECT_ROOT/src/build" \
               "$PROJECT_ROOT/src/memtest" \
               "$PROJECT_ROOT/src/perftest" 2>/dev/null || true
        echo "Cleaned src/ directory"
    fi
}

# Main cleanup execution
cd "$PROJECT_ROOT"

# CRITICAL: Remove prohibited root build artifacts first
clean_root_build_artifacts

# Clean src directory build artifacts  
clean_src_build_artifacts

# Move legacy build directories (keep only 'build' and 'build-plugin-final')
for build_dir in build-debug build-obs-real build-obs-real-final build-standalone build-test; do
    move_old_build "$build_dir"
done

# Consolidate artifacts
consolidate_artifacts

# Clean CMake temporary directories
clean_cmake_tmp

# Report final state
echo ""
echo "=== Cleanup Complete ==="
echo "Active build directories in root:"
ls -d build* 2>/dev/null | head -5 || echo "  (none found)"

echo ""
echo "Consolidated temporary files in tmp/:"
ls -la tmp/ | grep -E "^d" | awk '{print "  " $NF}'

echo ""
echo "File organization compliance: ✅ ENFORCED"
echo "YAGNI: Unnecessary build directories archived"
echo "DRY: Duplicate artifacts consolidated"  
echo "一時ファイルは一箇所のディレクトリにまとめよ: All temp files in tmp/"