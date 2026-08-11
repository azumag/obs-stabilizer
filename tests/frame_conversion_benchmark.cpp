#include "core/frame_utils.hpp"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

namespace {
constexpr std::uint32_t kWidth = 1920;
constexpr std::uint32_t kHeight = 1080;
constexpr int kIterations = 30;
constexpr double kMaxAverageRoundTripMs = 20.0;
}

int main()
{
    std::vector<std::uint8_t> pixels(
        static_cast<std::size_t>(kWidth) * kHeight * 4U, 0x7f);

    obs_source_frame reference{};
    reference.width = kWidth;
    reference.height = kHeight;
    reference.format = VIDEO_FORMAT_BGRA;
    reference.timestamp = 123456;
    reference.data[0] = pixels.data();
    reference.linesize[0] = kWidth * 4U;

    double total_ms = 0.0;

    for (int iteration = 0; iteration < kIterations; ++iteration) {
        const auto start = std::chrono::steady_clock::now();

        cv::Mat converted = FRAME_UTILS::Conversion::obs_to_cv(&reference);
        if (converted.empty() || converted.cols != static_cast<int>(kWidth) ||
            converted.rows != static_cast<int>(kHeight) || converted.channels() != 4) {
            std::cerr << "OBS to OpenCV conversion produced an invalid frame\n";
            return 1;
        }

        obs_source_frame* round_trip =
            FRAME_UTILS::Conversion::cv_to_obs(converted, &reference);
        if (!round_trip || round_trip->width != kWidth ||
            round_trip->height != kHeight ||
            round_trip->format != VIDEO_FORMAT_BGRA ||
            round_trip->timestamp != reference.timestamp) {
            std::cerr << "OpenCV to OBS conversion produced invalid metadata\n";
            FRAME_UTILS::FrameBuffer::release(round_trip);
            return 1;
        }

        const std::size_t byte_count =
            static_cast<std::size_t>(kWidth) * kHeight * 4U;
        if (std::memcmp(round_trip->data[0], pixels.data(), byte_count) != 0) {
            std::cerr << "Round-trip conversion changed pixel data\n";
            FRAME_UTILS::FrameBuffer::release(round_trip);
            return 1;
        }

        FRAME_UTILS::FrameBuffer::release(round_trip);

        const auto end = std::chrono::steady_clock::now();
        total_ms += std::chrono::duration<double, std::milli>(end - start).count();
    }

    const double average_ms = total_ms / kIterations;
    std::cout << "Average 1080p BGRA conversion round trip: " << average_ms
              << " ms over " << kIterations << " iterations\n";

    if (average_ms > kMaxAverageRoundTripMs) {
        std::cerr << "Average conversion latency exceeded "
                  << kMaxAverageRoundTripMs << " ms\n";
        return 1;
    }

    return 0;
}
