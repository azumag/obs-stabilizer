/*
Core-only compilation test for StabilizerCore
Tests the core stabilization module without OBS dependencies
*/

#include <iostream>
#include <memory>

// Mock OBS definitions for compilation test
#define LOG_INFO 300

inline void obs_log(int, const char*, ...) { 
    // Mock implementation for testing
}

// Always include the core module (supports both OpenCV and stub modes)
#include "src/core/stabilizer_core.hpp"

// Test compilation of core module
#ifdef ENABLE_STABILIZATION

void test_core_compilation() {
    using namespace obs_stabilizer;
    
    std::cout << "Testing StabilizerCore compilation..." << std::endl;
    
    // Test basic instantiation
    auto core = std::make_unique<StabilizerCore>();
    
    // Test configuration
    StabilizerConfig config;
    config.smoothing_radius = 30;
    config.max_features = 200;
    config.enable_stabilization = true;
    config.error_threshold = 30.0f;
    config.min_feature_quality = 0.01f;
    
    // Test initialization
    bool init_result = core->initialize(config);
    
    // Test status query
    StabilizerStatus status = core->get_status();
    
    // Test metrics query
    StabilizerMetrics metrics = core->get_metrics();
    
    // Test configuration update
    config.smoothing_radius = 50;
    core->update_configuration(config);
    
    // Test reset
    core->reset();
    
    std::cout << "✅ StabilizerCore compilation and API test PASSED" << std::endl;
    std::cout << "   - Configuration: smoothing_radius=" << config.smoothing_radius << std::endl;
    std::cout << "   - Initialization result: " << (init_result ? "SUCCESS" : "FAILED") << std::endl;
    std::cout << "   - Status query works: " << static_cast<int>(status) << std::endl;
    std::cout << "   - Metrics query works: " << metrics.tracked_features << " tracked features" << std::endl;
}

void test_core_enum_definitions() {
    using namespace obs_stabilizer;
    
    std::cout << "Testing enum definitions..." << std::endl;
    
    // Test StabilizerStatus enum
    StabilizerStatus status = StabilizerStatus::INACTIVE;
    status = StabilizerStatus::INITIALIZING;
    status = StabilizerStatus::ACTIVE;
    status = StabilizerStatus::DEGRADED;
    status = StabilizerStatus::ERROR_RECOVERY;
    status = StabilizerStatus::FAILED;
    
    std::cout << "✅ StabilizerStatus enum compilation PASSED" << std::endl;
    std::cout << "   - All status values accessible: " << static_cast<int>(status) << std::endl;
}
#endif

int main() {
    std::cout << "=== StabilizerCore Compilation Test ===" << std::endl;
    
    try {
        #ifdef ENABLE_STABILIZATION
        test_core_compilation();
        test_core_enum_definitions();
        
        std::cout << std::endl;
        std::cout << "🎉 STABILIZERCORE COMPILATION TESTS PASSED" << std::endl;
        std::cout << "   - Core module compiles successfully with OpenCV" << std::endl;
        std::cout << "   - All API methods are properly defined" << std::endl;
        std::cout << "   - Enum definitions work correctly" << std::endl;
        std::cout << "   - Ready for full integration testing" << std::endl;
        
        #else 
        // Test stub implementation
        using namespace obs_stabilizer;
        
        std::cout << "Testing StabilizerCore stub implementation..." << std::endl;
        
        auto core = std::make_unique<StabilizerCore>();
        StabilizerConfig config;
        config.smoothing_radius = 30;
        
        // These should work but return false/defaults in stub mode
        bool init_result = core->initialize(config);
        StabilizerStatus status = core->get_status();
        [[maybe_unused]] StabilizerMetrics metrics = core->get_metrics();
        
        std::cout << "✅ StabilizerCore stub compilation and API test PASSED" << std::endl;
        std::cout << "   - ENABLE_STABILIZATION not defined (no OpenCV)" << std::endl;
        std::cout << "   - Stub methods callable: init=" << (init_result ? "true" : "false") << std::endl;
        std::cout << "   - Status query works: " << static_cast<int>(status) << std::endl;
        std::cout << "   - Architecture supports graceful degradation" << std::endl;
        #endif
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ COMPILATION TEST FAILED: " << e.what() << std::endl;
        return 1;
    }
}