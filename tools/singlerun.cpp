/*
 * OBS Stabilizer Plugin - Single Run Performance Benchmark
 * Lightweight, quick validation tool for developers
 */

#include "benchmark.hpp"
#include <iostream>
#include <cstring>
#include <iomanip>

using namespace PERF;

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n\n";
    std::cout << "Quick validation tool for performance testing.\n\n";
    std::cout << "Options:\n";
    std::cout << "  --scenario <name>    Run specific scenario (480p, 720p, 1080p, 1440p, 4k)\n";
    std::cout << "  --frames <num>       Number of frames to process (default: 500)\n";
    std::cout << "  --warmup <num>       Warmup frames to skip (default: 5)\n";
    std::cout << "  --output <file>      Output file path (default: singlerun_results.csv)\n";
    std::cout << "  --format <fmt>       Output format: csv or json (default: csv)\n";
    std::cout << "  --verbose            Show detailed progress and metrics\n";
    std::cout << "  --help               Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program_name << " --scenario 1080p --frames 1000\n";
    std::cout << "  " << program_name << " --output results.json --format json --verbose\n";
    std::cout << "  " << program_name << " --scenario 720p\n\n";
    std::cout << "Note: No baseline comparison is performed in singlerun mode.\n";
    std::cout << "     Results are saved directly without comparison.\n";
}

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   OBS Stabilizer - Single Run Performance Test                 ║\n";
    std::cout << "║   Quick Validation Tool for Developers                         ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    
    BenchmarkRunner runner;
    BenchmarkConfig config = runner.get_config();
    
    std::string scenario_arg;
    bool verbose = false;
    
    if (argc == 1) {
        print_usage(argv[0]);
        return 0;
    }
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--scenario") == 0) {
            if (i + 1 < argc) {
                scenario_arg = argv[++i];
            }
        } else if (strcmp(argv[i], "--frames") == 0) {
            if (i + 1 < argc) {
                int frames = std::atoi(argv[++i]);
                if (frames > 0) {
                    config.num_frames = frames;
                }
            }
        } else if (strcmp(argv[i], "--warmup") == 0) {
            if (i + 1 < argc) {
                int warmup = std::atoi(argv[++i]);
                if (warmup >= 0) {
                    config.warmup_frames = warmup;
                }
            }
        } else if (strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) {
                config.output_file = argv[++i];
            }
        } else if (strcmp(argv[i], "--format") == 0) {
            if (i + 1 < argc) {
                config.output_format = argv[++i];
            }
        } else if (strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        }
    }
    
    runner.set_config(config);
    
    if (!scenario_arg.empty()) {
        TestScenario scenario;
        if (scenario_arg == "480p") {
            scenario = TestScenario::RESOLUTION_480P;
        } else if (scenario_arg == "720p") {
            scenario = TestScenario::RESOLUTION_720P;
        } else if (scenario_arg == "1080p") {
            scenario = TestScenario::RESOLUTION_1080P;
        } else if (scenario_arg == "1440p") {
            scenario = TestScenario::RESOLUTION_1440P;
        } else if (scenario_arg == "4k") {
            scenario = TestScenario::RESOLUTION_4K;
        } else {
            std::cerr << "Error: Unknown scenario '" << scenario_arg << "'\n";
            print_usage(argv[0]);
            return 1;
        }
        
        if (verbose) {
            std::cout << "Running scenario: " << scenario_arg << "\n";
            std::cout << "Frames: " << config.num_frames << " (warmup: " << config.warmup_frames << ")\n";
            std::cout << "Output format: " << config.output_format << "\n";
            std::cout << "Output file: " << config.output_file << "\n\n";
        }
        
        runner.run_scenario(scenario);
    } else {
        std::cerr << "Error: No scenario specified. Use --scenario <name> or --help\n";
        print_usage(argv[0]);
        return 1;
    }
    
    auto results = runner.get_results();
    if (!results.empty()) {
        const auto& metrics = results.back();
        
        std::cout << "\n";
        std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║   Test Results                                                 ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
        std::cout << "\n";
        
        std::cout << "Scenario: " << metrics.scenario_name << "\n";
        std::cout << "Resolution: " << metrics.resolution_width << "x" << metrics.resolution_height << "\n";
        std::cout << "Frame Rate: " << metrics.frame_rate << " fps\n";
        std::cout << "\n";
        
        std::cout << "Processing Time:\n";
        std::cout << "  Average: " << std::fixed << std::setprecision(2) 
                  << metrics.avg_processing_time_ms << " ms\n";
        std::cout << "  Minimum: " << metrics.min_processing_time_ms << " ms\n";
        std::cout << "  Maximum: " << metrics.max_processing_time_ms << " ms\n";
        std::cout << "  Std Dev: " << metrics.std_deviation_ms << " ms\n";
        std::cout << "\n";
        
        std::cout << "Memory Usage:\n";
        std::cout << "  Peak: " << metrics.peak_memory_bytes << " bytes\n";
        std::cout << "  Average: " << metrics.avg_memory_bytes << " bytes\n";
        std::cout << "\n";
        
        std::cout << "Performance Target: <" << metrics.target_processing_time_ms << " ms/frame\n";
        std::cout << "Real-time Requirement: " << (1000.0 / metrics.target_processing_time_ms) << " fps\n";
        std::cout << "\n";
        
        std::cout << "Status: ";
        if (metrics.meets_realtime_requirement) {
            std::cout << "✅ PASS\n";
        } else {
            std::cout << "❌ FAIL\n";
        }
        
        if (!metrics.failure_reason.empty() && !metrics.meets_realtime_requirement) {
            std::cout << "Reason: " << metrics.failure_reason << "\n";
        }
        
        std::cout << "\n";
        runner.save_results(config.output_file);
        std::cout << "\n";
    }
    
    return 0;
}
