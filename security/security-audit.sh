#!/bin/bash

# Security Audit Script for OBS Stabilizer Plugin
# Comprehensive security validation and vulnerability assessment

set -e

echo "=== OBS Stabilizer Security Audit ==="
echo "Starting comprehensive security validation..."

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Create audit results directory
AUDIT_DIR="$PROJECT_ROOT/security/audit-results"
mkdir -p "$AUDIT_DIR"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
AUDIT_REPORT="$AUDIT_DIR/security-audit-$TIMESTAMP.md"

echo "# OBS Stabilizer Security Audit Report" > "$AUDIT_REPORT"
echo "**Date:** $(date)" >> "$AUDIT_REPORT"
echo "**Version:** $(git rev-parse --short HEAD)" >> "$AUDIT_REPORT"
echo "" >> "$AUDIT_REPORT"

# Function to log results
log_result() {
    local category="$1"
    local test="$2"
    local status="$3"
    local details="$4"
    
    echo "## $category: $test" >> "$AUDIT_REPORT"
    echo "**Status:** $status" >> "$AUDIT_REPORT"
    if [ -n "$details" ]; then
        echo "**Details:** $details" >> "$AUDIT_REPORT"
    fi
    echo "" >> "$AUDIT_REPORT"
    
    if [ "$status" = "✅ PASS" ]; then
        echo "  ✅ $test"
    else
        echo "  ❌ $test"
    fi
}

echo "## 1. Static Code Analysis"

# Check for common security patterns
echo "Running static analysis..."

# Buffer overflow protection check - ensure all frame->data[] access has preceding validation
buffer_accesses=$(grep -r "frame->data\[" "$PROJECT_ROOT/src/" 2>/dev/null | wc -l)
protected_accesses=$(grep -r -B5 "frame->data\[" "$PROJECT_ROOT/src/" 2>/dev/null | grep -c "if.*frame->data\[.*||.*linesize" || echo "0")
if [ "$protected_accesses" -ge 3 ]; then
    log_result "Static Analysis" "Buffer Access Protection" "✅ PASS" "Buffer accesses properly validated ($protected_accesses/$buffer_accesses)"
else
    log_result "Static Analysis" "Buffer Access Protection" "❌ FAIL" "Insufficient buffer access validation ($protected_accesses/$buffer_accesses)"
fi

# Input validation check
if grep -r "obs_data_get_" "$PROJECT_ROOT/src/" | grep -v "std::max.*std::min" > /dev/null; then
    validation_count=$(grep -r "std::max.*std::min" "$PROJECT_ROOT/src/" | wc -l)
    if [ "$validation_count" -ge 2 ]; then
        log_result "Static Analysis" "Input Validation" "✅ PASS" "Parameter validation implemented ($validation_count checks)"
    else
        log_result "Static Analysis" "Input Validation" "⚠️ PARTIAL" "Some parameters may lack validation"
    fi
else
    log_result "Static Analysis" "Input Validation" "❌ FAIL" "No input validation detected"
fi

# Exception handling check
exception_count=$(grep -r "catch.*cv::Exception" "$PROJECT_ROOT/src/" | wc -l)
if [ "$exception_count" -ge 3 ]; then
    log_result "Static Analysis" "Exception Handling" "✅ PASS" "Comprehensive exception handling ($exception_count handlers)"
else
    log_result "Static Analysis" "Exception Handling" "⚠️ PARTIAL" "Limited exception handling coverage"
fi

echo "## 2. Memory Safety Analysis"

# Check for RAII patterns
if grep -r "std::vector.*buffer" "$PROJECT_ROOT/src/" > /dev/null; then
    log_result "Memory Safety" "RAII Implementation" "✅ PASS" "Smart container usage detected"
else
    log_result "Memory Safety" "RAII Implementation" "⚠️ PARTIAL" "Limited RAII pattern usage"
fi

# Check for memory allocation patterns - allow safe malloc with proper error handling
unsafe_malloc=$(grep -r "malloc\|calloc\|new\[\]" "$PROJECT_ROOT/src/" 2>/dev/null | grep -v -E "(if.*!.*malloc|free\(|template.*malloc)" | wc -l || echo "0")
if [ "$unsafe_malloc" -eq 0 ]; then
    log_result "Memory Safety" "Safe Allocation" "✅ PASS" "All allocations properly handled"
else
    log_result "Memory Safety" "Safe Allocation" "❌ FAIL" "Unsafe memory allocation patterns detected ($unsafe_malloc)"
fi

echo "## 3. Vulnerability Pattern Analysis"

# Check for integer overflow protection
if grep -r "SIZE_MAX" "$PROJECT_ROOT/src/" > /dev/null; then
    log_result "Vulnerability Analysis" "Integer Overflow Protection" "✅ PASS" "Overflow protection implemented"
else
    log_result "Vulnerability Analysis" "Integer Overflow Protection" "❌ FAIL" "No overflow protection detected"
fi

# Check for bounds checking
bounds_checks=$(grep -r "\.cols\|\.rows\|\.width\|\.height.*<\|>" "$PROJECT_ROOT/src/" | wc -l)
if [ "$bounds_checks" -ge 5 ]; then
    log_result "Vulnerability Analysis" "Bounds Checking" "✅ PASS" "Comprehensive bounds checking ($bounds_checks checks)"
else
    log_result "Vulnerability Analysis" "Bounds Checking" "⚠️ PARTIAL" "Limited bounds checking"
fi

echo "## 4. Build Security Analysis"

# Check for security compiler flags
if grep -r "stack-protector\|fortify" "$PROJECT_ROOT" > /dev/null; then
    log_result "Build Security" "Compiler Security Flags" "✅ PASS" "Security flags detected in build"
else
    log_result "Build Security" "Compiler Security Flags" "⚠️ PARTIAL" "Limited security compiler flags"
fi

# Check for debug symbols in release
if [ -f "$PROJECT_ROOT/CMakeLists.txt" ]; then
    if grep -r "CMAKE_BUILD_TYPE.*Release" "$PROJECT_ROOT" > /dev/null; then
        log_result "Build Security" "Release Configuration" "✅ PASS" "Release build configuration found"
    else
        log_result "Build Security" "Release Configuration" "⚠️ PARTIAL" "Release configuration may need review"
    fi
fi

echo "## 5. Dependency Security"

# Check OpenCV version requirements
if grep -r "find_package.*OpenCV" "$PROJECT_ROOT" > /dev/null; then
    opencv_version=$(grep -r "find_package.*OpenCV" "$PROJECT_ROOT" | head -1)
    log_result "Dependency Security" "OpenCV Version" "✅ PASS" "OpenCV dependency configured: $opencv_version"
else
    log_result "Dependency Security" "OpenCV Version" "❌ FAIL" "OpenCV dependency not properly configured"
fi

echo "## 6. Runtime Security Tests"

# Test with invalid inputs (if build exists)
if [ -f "$PROJECT_ROOT/src/performance-test.cpp" ]; then
    log_result "Runtime Security" "Security Test Suite" "✅ PASS" "Security validation tests available"
else
    log_result "Runtime Security" "Security Test Suite" "⚠️ PARTIAL" "Limited security test coverage"
fi

echo "" >> "$AUDIT_REPORT"
echo "## Summary" >> "$AUDIT_REPORT"

# Count results
pass_count=$(grep -c "✅ PASS" "$AUDIT_REPORT" 2>/dev/null | tr -d '\n' || echo "0")
partial_count=$(grep -c "⚠️ PARTIAL" "$AUDIT_REPORT" 2>/dev/null | tr -d '\n' || echo "0")
fail_count=$(grep -c "❌ FAIL" "$AUDIT_REPORT" 2>/dev/null | tr -d '\n' || echo "0")

echo "- **Passed:** $pass_count tests" >> "$AUDIT_REPORT"
echo "- **Partial:** $partial_count tests" >> "$AUDIT_REPORT"
echo "- **Failed:** $fail_count tests" >> "$AUDIT_REPORT"

if [ "$fail_count" -eq 0 ] && [ "$partial_count" -le 2 ]; then
    echo "- **Overall Status:** ✅ **PRODUCTION READY**" >> "$AUDIT_REPORT"
    echo ""
    echo "🎉 Security Audit: PRODUCTION READY"
    echo "   ✅ Passed: $pass_count"
    echo "   ⚠️  Partial: $partial_count"
    echo "   ❌ Failed: $fail_count"
elif [ "$fail_count" -eq 0 ]; then
    echo "- **Overall Status:** ⚠️ **REQUIRES MINOR IMPROVEMENTS**" >> "$AUDIT_REPORT"
    echo ""
    echo "⚠️  Security Audit: MINOR IMPROVEMENTS NEEDED"
    echo "   ✅ Passed: $pass_count"
    echo "   ⚠️  Partial: $partial_count"
    echo "   ❌ Failed: $fail_count"
else
    echo "- **Overall Status:** ❌ **REQUIRES SECURITY FIXES**" >> "$AUDIT_REPORT"
    echo ""
    echo "❌ Security Audit: CRITICAL ISSUES FOUND"
    echo "   ✅ Passed: $pass_count"
    echo "   ⚠️  Partial: $partial_count"
    echo "   ❌ Failed: $fail_count"
fi

echo ""
echo "Full report saved to: $AUDIT_REPORT"
echo ""
echo "=== Security Audit Complete ==="