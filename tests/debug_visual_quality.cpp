#include <opencv2/opencv.hpp>
#include "../src/core/stabilizer_core.hpp"
#include "test_data_generator.hpp"

int main() {
    std::cout << "Debugging visual quality test failure...\n";

    // Create stabilizer
    StabilizerCore stabilizer;

    // Generate test frames with shake
    auto frames = TestDataGenerator::generate_test_sequence(
        50, 640, 480, "shake"
    );

    std::cout << "Generated " << frames.size() << " frames\n";

    // Initialize stabilizer
    StabilizerCore::StabilizerParams params;
    params.smoothing_radius = 30;
    params.max_correction = 50.0f;
    params.feature_count = 100;
    params.quality_level = 0.01f;
    params.min_distance = 7.0f;

    if (!stabilizer.initialize(640, 480, params)) {
        std::cout << "Failed to initialize stabilizer\n";
        return 1;
    }

    std::cout << "Stabilizer initialized\n";

    // Process frames
    for (size_t i = 0; i < frames.size(); i++) {
        cv::Mat result = stabilizer.process_frame(frames[i]);
        if (result.empty()) {
            std::cout << "Frame " << i << " returned empty result\n";
        }
    }

    std::cout << "All frames processed\n";

    return 0;
}
