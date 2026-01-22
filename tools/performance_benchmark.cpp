/*
 * OBS Stabilizer Plugin - Performance Benchmark Main Entry Point
 * Comprehensive performance testing for stabilization algorithms
 */

#include "src/core/benchmark.hpp"
#include <iostream>
#include <cstring>
#include <thread>

using namespace PERF;

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --scenario <name>      Run specific scenario (480p, 720p, 1080p, 1440p, 4k)\n";
    std::cout << "  --frames <num>         Number of frames to process (default: 1000)\n";
    std::cout << "  --warmup <num>         Warmup frames to skip (default: 10)\n";
    std::cout << "  --output <file>        Output file path (default: performance_results.csv)\n";
    std::cout << "  --format <fmt>         Output format: csv or json (default: csv)\n";
    std::cout << "  --baseline <file>      Compare against baseline file\n";
    std::cout << "  --save-baseline <file> Save results as baseline\n";
    std::cout << "  --no-memory           Disable memory tracking\n";
    std::cout << "  --help                 Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program_name << " --scenario 1080p --frames 500\n";
    std::cout << "  " << program_name << " --output results.json --format json\n";
    std::cout << "  " << program_name << " --baseline baseline.json --output results.csv\n";
}

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   OBS Stabilizer Performance Benchmark Suite                   ║\n";
    std::cout << "║   Comprehensive Performance Testing Infrastructure             ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    
    BenchmarkRunner runner;
    BenchmarkConfig config = runner.get_config();
    
    std::string scenario_arg;
    std::string baseline_file;
    std::string save_baseline_file;
    bool run_all = true;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--scenario") == 0) {
            if (i + 1 < argc) {
                scenario_arg = argv[++i];
                run_all = false;
            }
        } else if (strcmp(argv[i], "--frames") == 0) {
            if (i + 1 < argc) {
                config.num_frames = std::atoi(argv[++i]);
            }
        } else if (strcmp(argv[i], "--warmup") == 0) {
            if (i + 1 < argc) {
                config.warmup_frames = std::atoi(argv[++i]);
            }
        } else if (strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) {
                config.output_file = argv[++i];
            }
        } else if (strcmp(argv[i], "--format") == 0) {
            if (i + 1 < argc) {
                config.output_format = argv[++i];
            }
        } else if (strcmp(argv[i], "--baseline") == 0) {
            if (i + 1 < argc) {
                baseline_file = argv[++i];
            }
        } else if (strcmp(argv[i], "--save-baseline") == 0) {
            if (i + 1 < argc) {
                save_baseline_file = argv[++i];
            }
        } else if (strcmp(argv[i], "--no-memory") == 0) {
            config.enable_memory_tracking = false;
        }
    }
    
    runner.set_config(config);
    
    // Load baseline if specified
    if (!baseline_file.empty()) {
        runner.load_baseline(baseline_file);
    }
    
    // Run benchmarks
    if (run_all) {
        runner.run_all_scenarios();
    } else {
        // Run specific scenario
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
        
        runner.run_scenario(scenario);
        runner.print_summary();
    }
    
    // Compare against baseline
    if (!baseline_file.empty()) {
        bool regression_detected = !runner.compare_against_baseline();
        if (regression_detected) {
            std::cout << "\n⚠️  PERFORMANCE REGRESSION DETECTED!\n";
            std::cout << "Current performance is worse than baseline.\n";
            return 1;
        } else {
            std::cout << "\n✅ No performance regressions detected.\n";
        }
    }
    
    // Save baseline if specified
    if (!save_baseline_file.empty()) {
        runner.save_baseline(save_baseline_file);
    }
    
    // Check overall success
    auto results = runner.get_results();
    int failed = 0;
    for (const auto& result : results) {
        if (!result.passed) {
            failed++;
        }
    }
    
    std::cout << "\n";
    if (failed == 0) {
        std::cout << "✅ ALL BENCHMARKS PASSED\n";
    } else {
        std::cout << "❌ " << failed << " BENCHMARKS FAILED\n";
    }
    std::cout << "\n";
    
    return failed > 0 ? 1 : 0;
}
