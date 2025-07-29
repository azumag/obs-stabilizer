/*
OBS Stabilizer Plugin - Exception Safety Test Suite
Copyright (C) 2025 azumag <azumag@users.noreply.github.com>

Following t-wada TDD methodology for exception safety verification.
Tests written BEFORE any corresponding implementation changes.
*/

#include <gtest/gtest.h>
#include <stdexcept>
#include <memory>
#include <cstdint>
#include <signal.h>
#include <setjmp.h>

// Project headers
#include "../obs/obs_integration.hpp"
#include "../core/error_handler.hpp"
#include "../core/parameter_validator.hpp"
#include "../core/stabilizer_core.hpp"

#ifdef ENABLE_STABILIZATION
#include <opencv2/opencv.hpp>
#endif

using namespace obs_stabilizer;

// Mock OBS structures for testing
struct MockOBSData {
    void* mock_data;
    size_t data_size;
};

struct MockOBSFrame {
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint8_t* data[4];
    uint32_t linesize[4];
    
    MockOBSFrame() {
        width = 640; height = 480; format = 25; // VIDEO_FORMAT_NV12
        for (int i = 0; i < 4; i++) { data[i] = nullptr; linesize[i] = 0; }
    }
};

class ExceptionSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create valid test structures
        valid_filter = std::make_unique<StabilizerFilter>();
        
        // Setup mock frame data
        frame_data = std::unique_ptr<uint8_t[]>(new uint8_t[640 * 480 * 2]);
        mock_frame.data[0] = frame_data.get();
        mock_frame.linesize[0] = 640;
        mock_frame.data[1] = frame_data.get() + (640 * 480);
        mock_frame.linesize[1] = 640;
    }

    void TearDown() override {
        valid_filter.reset();
        frame_data.reset();
    }

    std::unique_ptr<StabilizerFilter> valid_filter;
    MockOBSFrame mock_frame;
    std::unique_ptr<uint8_t[]> frame_data;
};

// ====================
// TDD Red Phase: Write tests that should fail
// ====================

TEST_F(ExceptionSafetyTest, ValidateFilterDataIntegrity_NullPointer_ReturnsFalse) {
    // RED: This test should fail initially if validate_filter_data_integrity doesn't exist
    // or doesn't handle null pointers correctly
    
    // Test null pointer validation
    bool result = validate_filter_data_integrity(nullptr);
    EXPECT_FALSE(result) << "validate_filter_data_integrity should reject null pointers";
}

TEST_F(ExceptionSafetyTest, ValidateFilterDataIntegrity_InvalidAlignment_ReturnsFalse) {
    // RED: Test should fail if alignment validation isn't implemented
    
    // Create misaligned pointer (odd address on systems requiring alignment)
    char buffer[sizeof(StabilizerFilter) + 8];
    void* misaligned_ptr = reinterpret_cast<void*>(
        reinterpret_cast<uintptr_t>(buffer) + 1); // +1 to misalign
    
    bool result = validate_filter_data_integrity(misaligned_ptr);
    EXPECT_FALSE(result) << "validate_filter_data_integrity should reject misaligned pointers";
}

TEST_F(ExceptionSafetyTest, ValidateFilterDataIntegrity_InvalidAddressSpace_ReturnsFalse) {
    // RED: Test should fail if address space validation isn't implemented
    
    // Test obviously invalid address spaces
    void* low_addr = reinterpret_cast<void*>(0x100);  // Too low
    void* high_addr = reinterpret_cast<void*>(0xFFFFFFFFFFFFFFFFULL); // Too high
    
    bool result1 = validate_filter_data_integrity(low_addr);
    bool result2 = validate_filter_data_integrity(high_addr);
    
    EXPECT_FALSE(result1) << "validate_filter_data_integrity should reject low memory addresses";
    EXPECT_FALSE(result2) << "validate_filter_data_integrity should reject high memory addresses";
}

TEST_F(ExceptionSafetyTest, ValidateFilterDataIntegrity_CorruptedStabilizerCore_ReturnsFalse) {
    // RED: This test should fail if structure validation isn't thorough enough
    
    // Create a structure that looks like StabilizerFilter but has corrupted stabilizer_core
    struct FakeStabilizerFilter {
        void* source;
        bool enabled;
        std::unique_ptr<StabilizerCore> stabilizer_core; // This will be null
        StabilizerConfig config;
    };
    
    FakeStabilizerFilter fake_filter;
    fake_filter.source = nullptr;
    fake_filter.enabled = true;
    fake_filter.stabilizer_core = nullptr; // Intentionally corrupted
    
    bool result = validate_filter_data_integrity(&fake_filter);
    EXPECT_FALSE(result) << "validate_filter_data_integrity should detect corrupted stabilizer_core";
}

TEST_F(ExceptionSafetyTest, ValidateFilterDataIntegrity_ValidFilter_ReturnsTrue) {
    // RED: This should initially fail if the function doesn't properly validate valid structures
    
    bool result = validate_filter_data_integrity(valid_filter.get());
    EXPECT_TRUE(result) << "validate_filter_data_integrity should accept valid filter structures";
}

TEST_F(ExceptionSafetyTest, FilterVideo_InvalidData_HandlesGracefully) {
    // RED: Test should fail if filter_video doesn't handle invalid data gracefully
    
    struct obs_source_frame* result = OBSIntegration::filter_video(nullptr, &mock_frame);
    EXPECT_EQ(result, &mock_frame) << "filter_video should pass through frame when data is invalid";
}

TEST_F(ExceptionSafetyTest, FilterVideo_CorruptedFilter_HandlesGracefully) {
    // RED: Test should fail if filter_video doesn't validate filter integrity
    
    // Create corrupted filter structure
    struct FakeStabilizerFilter {
        void* source;
        bool enabled;
        std::unique_ptr<StabilizerCore> stabilizer_core;
        StabilizerConfig config;
    };
    
    FakeStabilizerFilter corrupted_filter;
    corrupted_filter.source = nullptr;
    corrupted_filter.enabled = true;
    corrupted_filter.stabilizer_core = nullptr; // Corrupted
    
    struct obs_source_frame* result = OBSIntegration::filter_video(
        &corrupted_filter, &mock_frame);
    EXPECT_EQ(result, &mock_frame) << "filter_video should handle corrupted filter gracefully";
}

#ifdef ENABLE_STABILIZATION
TEST_F(ExceptionSafetyTest, FilterVideo_OpenCVException_HandlesGracefully) {
    // RED: Test should fail if OpenCV exceptions aren't properly caught
    
    // This test requires a way to trigger OpenCV exception during frame processing
    // We'll create a mock scenario that could trigger cv::Exception
    
    // Create a filter with invalid configuration that might trigger OpenCV errors
    StabilizerConfig bad_config;
    bad_config.max_features = -1; // Invalid value that might cause OpenCV error
    bad_config.smoothing_radius = -1; // Invalid value
    
    auto test_filter = std::make_unique<StabilizerFilter>();
    
    // Mock frame that might cause processing errors
    MockOBSFrame problematic_frame;
    problematic_frame.width = 0; // Invalid dimension
    problematic_frame.height = 0; // Invalid dimension
    
    struct obs_source_frame* result = OBSIntegration::filter_video(
        test_filter.get(), reinterpret_cast<struct obs_source_frame*>(&problematic_frame));
    
    // Should not crash and should return the original frame
    EXPECT_EQ(result, reinterpret_cast<struct obs_source_frame*>(&problematic_frame))
        << "filter_video should handle OpenCV exceptions gracefully";
}
#endif

TEST_F(ExceptionSafetyTest, FilterUpdate_InvalidData_HandlesGracefully) {
    // RED: Test should fail if filter_update doesn't handle invalid data
    
    MockOBSData mock_settings;
    mock_settings.mock_data = nullptr;
    mock_settings.data_size = 0;
    
    // This should not crash
    EXPECT_NO_THROW(OBSIntegration::filter_update(
        nullptr, reinterpret_cast<obs_data_t*>(&mock_settings)))
        << "filter_update should handle null data gracefully";
}

TEST_F(ExceptionSafetyTest, FilterUpdate_CorruptedFilter_HandlesGracefully) {
    // RED: Test should fail if filter_update doesn't validate filter structure
    
    struct FakeStabilizerFilter {
        void* source;
        bool enabled;
        std::unique_ptr<StabilizerCore> stabilizer_core;
        StabilizerConfig config;
    };
    
    FakeStabilizerFilter corrupted_filter;
    corrupted_filter.stabilizer_core = nullptr; // Corrupted
    
    MockOBSData mock_settings;
    
    EXPECT_NO_THROW(OBSIntegration::filter_update(
        &corrupted_filter, reinterpret_cast<obs_data_t*>(&mock_settings)))
        << "filter_update should handle corrupted filter gracefully";
}

TEST_F(ExceptionSafetyTest, FilterDestroy_InvalidData_HandlesGracefully) {
    // RED: Test should fail if filter_destroy doesn't handle invalid data
    
    // Test null pointer
    EXPECT_NO_THROW(OBSIntegration::filter_destroy(nullptr))
        << "filter_destroy should handle null data gracefully";
    
    // Test invalid pointer
    void* invalid_ptr = reinterpret_cast<void*>(0x1234);
    EXPECT_NO_THROW(OBSIntegration::filter_destroy(invalid_ptr))
        << "filter_destroy should handle invalid pointers gracefully";
}

TEST_F(ExceptionSafetyTest, FilterDestroy_ValidFilter_CleansUpProperly) {
    // RED: Test should fail if filter destruction doesn't work properly
    
    // Create a filter for destruction testing
    auto test_filter = std::make_unique<StabilizerFilter>();
    void* filter_ptr = test_filter.release(); // Release ownership
    
    // This should properly clean up without crashes
    EXPECT_NO_THROW(OBSIntegration::filter_destroy(filter_ptr))
        << "filter_destroy should clean up valid filters without exceptions";
}

// ====================
// Exception-specific validation tests
// ====================

TEST_F(ExceptionSafetyTest, ErrorHandler_SafeExecute_CatchesStandardExceptions) {
    // RED: Test should fail if ErrorHandler doesn't catch standard exceptions properly
    
    bool caught_exception = false;
    
    auto throw_function = []() {
        throw std::runtime_error("Test exception");
    };
    
    bool result = ErrorHandler::safe_execute(
        throw_function, ErrorCategory::VALIDATION, "test_operation");
    
    EXPECT_FALSE(result) << "safe_execute should return false when exception is caught";
}

TEST_F(ExceptionSafetyTest, ErrorHandler_SafeExecute_CatchesUnknownExceptions) {
    // RED: Test should fail if ErrorHandler doesn't catch unknown exceptions properly
    
    auto throw_function = []() {
        throw 42; // Non-standard exception
    };
    
    bool result = ErrorHandler::safe_execute(
        throw_function, ErrorCategory::VALIDATION, "test_operation");
    
    EXPECT_FALSE(result) << "safe_execute should return false when unknown exception is caught";
}

#ifdef ENABLE_STABILIZATION
TEST_F(ExceptionSafetyTest, ErrorHandler_SafeExecuteCV_CatchesOpenCVExceptions) {
    // RED: Test should fail if ErrorHandler doesn't catch OpenCV exceptions properly
    
    cv::Mat result_mat;
    
    auto throw_opencv_function = []() -> cv::Mat {
        // This should trigger a cv::Exception
        cv::Mat invalid_mat;
        cv::Mat result;
        cv::warpAffine(invalid_mat, result, cv::Mat(), cv::Size(0, 0)); // Invalid operation
        return result;
    };
    
    bool success = ErrorHandler::safe_execute_cv(
        throw_opencv_function, result_mat, ErrorCategory::OPENCV_INTERNAL, "test_opencv");
    
    EXPECT_FALSE(success) << "safe_execute_cv should return false when OpenCV exception is caught";
    EXPECT_TRUE(result_mat.empty()) << "result should remain empty when exception occurs";
}
#endif

// ====================
// Memory safety tests
// ====================

TEST_F(ExceptionSafetyTest, ParameterValidator_ValidatePointerNotNull_RejectsNull) {
    // RED: Test should fail if ParameterValidator doesn't properly validate null pointers
    
    ValidationResult result = ParameterValidator::validate_pointer_not_null(nullptr, "test_ptr");
    EXPECT_FALSE(result.is_valid) << "validate_pointer_not_null should reject null pointers";
    EXPECT_NE(result.error_message, nullptr) << "Error message should be provided";
}

TEST_F(ExceptionSafetyTest, ParameterValidator_ValidateArrayAccess_RejectsOutOfBounds) {
    // RED: Test should fail if array bounds checking isn't implemented
    
    int test_array[10];
    
    ValidationResult result1 = ParameterValidator::validate_array_access(
        test_array, 15, 10, "test_array"); // Index 15 > max_size 10
    EXPECT_FALSE(result1.is_valid) << "validate_array_access should reject out-of-bounds access";
    
    ValidationResult result2 = ParameterValidator::validate_array_access(
        test_array, 5, 10, "test_array"); // Valid access
    EXPECT_TRUE(result2.is_valid) << "validate_array_access should accept valid access";
}

TEST_F(ExceptionSafetyTest, ParameterValidator_ValidateBufferSize_RejectsInsufficientSize) {
    // RED: Test should fail if buffer size validation isn't implemented
    
    ValidationResult result1 = ParameterValidator::validate_buffer_size(
        100, 200, "test_buffer"); // actual_size < required_size
    EXPECT_FALSE(result1.is_valid) << "validate_buffer_size should reject insufficient buffer";
    
    ValidationResult result2 = ParameterValidator::validate_buffer_size(
        200, 200, "test_buffer"); // Equal sizes should be valid
    EXPECT_TRUE(result2.is_valid) << "validate_buffer_size should accept sufficient buffer";
}

// ====================
// Integration safety tests
// ====================

TEST_F(ExceptionSafetyTest, StabilizerCore_ProcessFrame_HandlesInvalidFrames) {
    // RED: Test should fail if StabilizerCore doesn't handle invalid frames safely
    
    StabilizerConfig config;
    config.enable_stabilization = true;
    
    auto core = std::make_unique<StabilizerCore>();
    core->initialize(config);
    
    // Test with null frame
    TransformResult result1 = core->process_frame(nullptr);
    EXPECT_FALSE(result1.success) << "process_frame should handle null frames safely";
    
    // Test with invalid frame dimensions
    MockOBSFrame invalid_frame;
    invalid_frame.width = 0;
    invalid_frame.height = 0;
    
    TransformResult result2 = core->process_frame(
        reinterpret_cast<frame_t*>(&invalid_frame));
    EXPECT_FALSE(result2.success) << "process_frame should handle invalid dimensions safely";
}

// Note: Following t-wada TDD methodology:
// 1. These tests should initially FAIL (Red phase)
// 2. Implementation should be added to make tests pass (Green phase)
// 3. Code should be refactored while keeping tests green (Refactor phase)