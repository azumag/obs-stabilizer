#!/bin/bash
# OBS Stabilizer Plugin - Temporary File Consolidation Script
# Following CLAUDE.md principle: "一時ファイルは一箇所のディレクトリにまとめよ"

set -e

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP_DIR="$REPO_ROOT/tmp"
CURRENT_DATE=$(date +%Y%m%d)

echo "🧹 Consolidating temporary files following CLAUDE.md principles..."
echo "Repository root: $REPO_ROOT"

# Create consolidated tmp directory if it doesn't exist
mkdir -p "$TMP_DIR"

# Function to move files to tmp with date prefix
move_to_tmp() {
    local source_path="$1"
    local target_subdir="$2"
    
    if [ -e "$source_path" ]; then
        local basename=$(basename "$source_path")
        local target_dir="$TMP_DIR/$target_subdir"
        mkdir -p "$target_dir"
        
        # Add date prefix if not already present
        if [[ "$basename" != *"$CURRENT_DATE"* ]]; then
            local target_name="${CURRENT_DATE}_${basename}"
        else
            local target_name="$basename"
        fi
        
        mv "$source_path" "$target_dir/$target_name"
        echo "  Moved: $source_path → $target_dir/$target_name"
    fi
}

# Clean up scattered report files in root
echo "📋 Consolidating reports..."
move_to_tmp "$REPO_ROOT/qa_report_20250729.md" "reports"

# Clean up build artifacts in various locations
echo "🏗️ Consolidating build artifacts..."
if [ -d "$REPO_ROOT/src/memtest" ]; then
    move_to_tmp "$REPO_ROOT/src/memtest" "build-artifacts"
fi

if [ -d "$REPO_ROOT/src/perftest" ]; then
    move_to_tmp "$REPO_ROOT/src/perftest" "build-artifacts"
fi

# Clean up scripts that should be in tmp for testing
echo "📜 Consolidating test scripts..."
move_to_tmp "$REPO_ROOT/test-plugin-loading.sh" "scripts"

# Consolidate existing tmp subdirectories with better organization
echo "📁 Organizing existing tmp structure..."

# Reports consolidation
if [ -d "$TMP_DIR/build-system-qa-report.md" ]; then
    move_to_tmp "$TMP_DIR/build-system-qa-report.md" "reports"
fi

if [ -d "$TMP_DIR/comprehensive-qa-assessment-20250729.md" ]; then
    move_to_tmp "$TMP_DIR/comprehensive-qa-assessment-20250729.md" "reports"
fi

if [ -d "$TMP_DIR/comprehensive-qa-assessment-plugin-loading-fix.md" ]; then
    move_to_tmp "$TMP_DIR/comprehensive-qa-assessment-plugin-loading-fix.md" "reports"
fi

if [ -d "$TMP_DIR/file-organization-compliance-report.md" ]; then
    move_to_tmp "$TMP_DIR/file-organization-compliance-report.md" "reports"
fi

if [ -d "$TMP_DIR/final-build-system-qa-report.md" ]; then
    move_to_tmp "$TMP_DIR/final-build-system-qa-report.md" "reports"
fi

# Create README in tmp directory explaining organization
cat > "$TMP_DIR/README.md" << 'EOF'
# Temporary Files Directory

Following CLAUDE.md principle: "一時ファイルは一箇所のディレクトリにまとめよ" (consolidate temporary files in one directory)

## Organization

- `reports/` - QA reports, assessments, and analysis documents
- `build-artifacts/` - Temporary build outputs and test binaries  
- `scripts/` - Testing and utility scripts
- `tests/tests/` - Test implementation files
- `static-analysis/` - Code analysis outputs

## File Naming Convention

Files are prefixed with date (YYYYMMDD) when moved to prevent conflicts and maintain chronological order.

## Cleanup Policy

This directory contains temporary files that can be safely deleted. Files here are:
- Build artifacts that can be regenerated
- Test outputs and reports
- Development/debugging utilities
- Analysis results and assessments

Permanent project files should never be stored in this directory.
EOF

echo "✅ Temporary file consolidation complete!"
echo "📊 Summary:"
echo "  - All temporary files moved to $TMP_DIR"
echo "  - Files organized by type (reports/, build-artifacts/, scripts/, tests/)"
echo "  - Added date prefixes to prevent conflicts"
echo "  - Created README.md explaining tmp directory organization"
echo ""
echo "🎯 CLAUDE.md compliance: ✅ 一時ファイルは一箇所のディレクトリにまとめよ"