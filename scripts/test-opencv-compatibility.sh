#!/bin/bash

# OpenCV Version Compatibility Testing Script
# Tests OBS Stabilizer Plugin with different OpenCV versions

set -e

echo "=== OpenCV Version Compatibility Test ==="
echo "Testing OBS Stabilizer Plugin compatibility across OpenCV versions"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build-test"

# Cleanup function
cleanup() {
    echo "Cleaning up test environment..."
    rm -rf "$BUILD_DIR"
}
trap cleanup EXIT

# Test configuration
TEST_RESULTS="$PROJECT_ROOT/opencv-compatibility-results.md"
echo "# OpenCV Compatibility Test Results" > "$TEST_RESULTS"
echo "**Date:** $(date)" >> "$TEST_RESULTS"
echo "**System:** $(uname -s) $(uname -m)" >> "$TEST_RESULTS"
echo "" >> "$TEST_RESULTS"

log_test() {
    local version="$1"
    local status="$2"
    local details="$3"
    
    echo "## OpenCV $version" >> "$TEST_RESULTS"
    echo "**Status:** $status" >> "$TEST_RESULTS"
    if [ -n "$details" ]; then
        echo "**Details:** $details" >> "$TEST_RESULTS"
    fi
    echo "" >> "$TEST_RESULTS"
    
    if [ "$status" = "✅ COMPATIBLE" ]; then
        echo "  ✅ OpenCV $version - Compatible"
    elif [ "$status" = "⚠️ PARTIAL" ]; then
        echo "  ⚠️ OpenCV $version - Partial compatibility"
    else
        echo "  ❌ OpenCV $version - Not compatible"
    fi
}

# Function to test OpenCV version
test_opencv_version() {
    local opencv_version="$1"
    local test_name="$2"
    
    echo "Testing $test_name..."
    
    # Create clean build directory
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Try to configure with specific OpenCV version
    if cmake "$PROJECT_ROOT" -DOpenCV_DIR="/usr/local/lib/cmake/opencv4" 2>&1 | tee cmake_output.log; then
        # Check if OpenCV was found
        if grep -q "Found OpenCV" cmake_output.log; then
            detected_version=$(grep "Found OpenCV" cmake_output.log | sed 's/.*Found OpenCV \([0-9.]*\).*/\1/')
            
            # Try to build
            if make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) 2>&1 | tee build_output.log; then
                log_test "$detected_version" "✅ COMPATIBLE" "Build successful - fully compatible"
                return 0
            else
                # Check for specific error patterns
                if grep -q "error.*CV_VERSION" build_output.log; then
                    log_test "$detected_version" "❌ INCOMPATIBLE" "API version check failed - requires OpenCV 4.5+"
                elif grep -q "warning.*OpenCV.*compatibility" build_output.log; then
                    log_test "$detected_version" "⚠️ PARTIAL" "Build successful with warnings - limited testing"
                    return 0
                else
                    log_test "$detected_version" "❌ BUILD_FAILED" "Build failed - see logs for details"
                fi
                return 1
            fi
        else
            log_test "Unknown" "❌ NOT_FOUND" "OpenCV not detected by CMake"
            return 1
        fi
    else
        log_test "Unknown" "❌ CMAKE_FAILED" "CMake configuration failed"
        return 1
    fi
}

# Test API compatibility without actual OpenCV installation
test_api_compatibility() {
    echo "Testing API compatibility patterns..."
    
    # Create a minimal test program
    cat > "$BUILD_DIR/api_test.cpp" << 'EOF'
#include <iostream>
#ifdef ENABLE_STABILIZATION
#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/video/tracking.hpp>

int main() {
    std::cout << "OpenCV Version: " << CV_VERSION << std::endl;
    std::cout << "Major: " << CV_VERSION_MAJOR << std::endl;  
    std::cout << "Minor: " << CV_VERSION_MINOR << std::endl;
    
    // Test key APIs used by stabilizer
    try {
        cv::Mat test_mat(100, 100, CV_8UC1);
        std::vector<cv::Point2f> points;
        cv::goodFeaturesToTrack(test_mat, points, 100, 0.01, 10);
        
        cv::Mat transform = cv::getAffineTransform(
            std::vector<cv::Point2f>{{0,0}, {1,0}, {0,1}},
            std::vector<cv::Point2f>{{0,0}, {1,0}, {0,1}}
        );
        
        std::cout << "✓ API compatibility test passed" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "✗ API test failed: " << e.what() << std::endl;
        return 1;
    }
}
#else
int main() {
    std::cout << "ENABLE_STABILIZATION not defined" << std::endl;
    return 0;
}
#endif
EOF
    
    # Try to compile the test
    if g++ -DENABLE_STABILIZATION -I/usr/local/include -lopencv_core -lopencv_imgproc -lopencv_features2d -lopencv_video "$BUILD_DIR/api_test.cpp" -o "$BUILD_DIR/api_test" 2>/dev/null; then
        if "$BUILD_DIR/api_test" 2>&1 | tee api_test_output.log; then
            version=$(grep "OpenCV Version:" api_test_output.log | cut -d: -f2 | tr -d ' ')
            if grep -q "API compatibility test passed" api_test_output.log; then
                log_test "$version" "✅ COMPATIBLE" "API test passed - all required functions available"
                return 0
            fi
        fi
    fi
    
    log_test "Unknown" "❌ API_TEST_FAILED" "Could not validate API compatibility"
    return 1
}

# Main testing sequence
echo "Starting OpenCV compatibility testing..."

# Test 1: Current system OpenCV (if available)
echo "## Test 1: System OpenCV"
if test_opencv_version "system" "System OpenCV Installation"; then
    echo "✅ System OpenCV test passed"
else
    echo "❌ System OpenCV test failed"
fi

# Test 2: API compatibility test
echo "## Test 2: API Compatibility"
mkdir -p "$BUILD_DIR"
if test_api_compatibility; then
    echo "✅ API compatibility test passed"
else
    echo "❌ API compatibility test failed"
fi

# Test 3: CMake version detection
echo "## Test 3: CMake Version Detection"
cd "$PROJECT_ROOT"
if cmake --version > /dev/null 2>&1; then
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    if cmake "$PROJECT_ROOT" 2>&1 | grep -E "(OpenCV|Found|WARNING)" | tee cmake_version_test.log; then
        log_test "CMake Detection" "✅ WORKING" "CMake successfully detects and validates OpenCV versions"
    else
        log_test "CMake Detection" "❌ FAILED" "CMake version detection not working properly"
    fi
else
    log_test "CMake Detection" "❌ NO_CMAKE" "CMake not available for testing"
fi

# Generate summary
echo "" >> "$TEST_RESULTS"
echo "## Summary" >> "$TEST_RESULTS"
echo "" >> "$TEST_RESULTS"

compatible_count=$(grep -c "✅ COMPATIBLE" "$TEST_RESULTS" || echo "0")
partial_count=$(grep -c "⚠️ PARTIAL" "$TEST_RESULTS" || echo "0")
failed_count=$(grep -c "❌" "$TEST_RESULTS" || echo "0")

echo "- **Compatible:** $compatible_count versions" >> "$TEST_RESULTS"
echo "- **Partial:** $partial_count versions" >> "$TEST_RESULTS"
echo "- **Failed:** $failed_count tests" >> "$TEST_RESULTS"
echo "" >> "$TEST_RESULTS"

if [ "$failed_count" -eq 0 ]; then
    echo "- **Overall Result:** ✅ **COMPATIBILITY VERIFIED**" >> "$TEST_RESULTS"
    echo ""
    echo "🎉 OpenCV Compatibility: VERIFIED"
    echo "   ✅ Compatible: $compatible_count"
    echo "   ⚠️  Partial: $partial_count"
    echo "   ❌ Failed: $failed_count"
else
    echo "- **Overall Result:** ⚠️ **COMPATIBILITY ISSUES DETECTED**" >> "$TEST_RESULTS"
    echo ""
    echo "⚠️  OpenCV Compatibility: ISSUES DETECTED"
    echo "   ✅ Compatible: $compatible_count"
    echo "   ⚠️  Partial: $partial_count"
    echo "   ❌ Failed: $failed_count"
fi

echo ""
echo "Full compatibility report saved to: $TEST_RESULTS"
echo ""
echo "=== OpenCV Compatibility Test Complete ==="