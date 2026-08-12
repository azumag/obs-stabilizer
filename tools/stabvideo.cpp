// Offline video runner for stabilization quality measurement.
//
// Feeds a video file through StabilizerCore exactly like the OBS filter does
// (BGRA frames, streaming preset, padding edge mode by default) and writes
// the stabilized frames plus per-frame diagnostics, so algorithm changes can
// be evaluated on identical input without a live OBS session.

#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgcodecs.hpp>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

#include "core/stabilizer_core.hpp"

namespace {

void print_usage() {
    std::printf(
        "usage: stabvideo --input FILE [options]\n"
        "  --output FILE          write stabilized video (mp4)\n"
        "  --frames-dir DIR       write stabilized frames as PNG (frame-%%05d.png)\n"
        "  --log-csv FILE         per-frame processing-time CSV\n"
        "  --preset NAME          streaming (default) | gaming | recording\n"
        "  --passthrough          disable stabilization (baseline copy)\n");
}

} // namespace

int main(int argc, char** argv) {
    std::string input_path;
    std::string output_path;
    std::string frames_dir;
    std::string log_csv;
    std::string preset = "streaming";
    bool passthrough = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto next = [&](const char* name) -> std::string {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "missing value for %s\n", name);
                std::exit(2);
            }
            return argv[++i];
        };
        if (arg == "--input") input_path = next("--input");
        else if (arg == "--output") output_path = next("--output");
        else if (arg == "--frames-dir") frames_dir = next("--frames-dir");
        else if (arg == "--log-csv") log_csv = next("--log-csv");
        else if (arg == "--preset") preset = next("--preset");
        else if (arg == "--passthrough") passthrough = true;
        else if (arg == "--help") { print_usage(); return 0; }
        else {
            std::fprintf(stderr, "unknown argument: %s\n", arg.c_str());
            print_usage();
            return 2;
        }
    }

    if (input_path.empty()) {
        print_usage();
        return 2;
    }

    cv::VideoCapture capture(input_path);
    if (!capture.isOpened()) {
        std::fprintf(stderr, "cannot open input: %s\n", input_path.c_str());
        return 1;
    }
    const int width = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH));
    const int height = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT));
    const double fps = capture.get(cv::CAP_PROP_FPS) > 0.0
                           ? capture.get(cv::CAP_PROP_FPS)
                           : 30.0;

    StabilizerCore::StabilizerParams params;
    if (preset == "gaming") params = StabilizerCore::get_preset_gaming();
    else if (preset == "recording") params = StabilizerCore::get_preset_recording();
    else params = StabilizerCore::get_preset_streaming();
    params.edge_mode = StabilizerCore::EdgeMode::Padding;

    StabilizerCore core;
    if (!passthrough && !core.initialize(static_cast<uint32_t>(width),
                                         static_cast<uint32_t>(height), params)) {
        std::fprintf(stderr, "initialize failed: %s\n", core.get_last_error().c_str());
        return 1;
    }

    cv::VideoWriter writer;
    if (!output_path.empty()) {
        writer.open(output_path, cv::VideoWriter::fourcc('a', 'v', 'c', '1'),
                    fps, cv::Size(width, height));
        if (!writer.isOpened()) {
            std::fprintf(stderr, "cannot open output: %s\n", output_path.c_str());
            return 1;
        }
    }
    if (!frames_dir.empty()) {
        std::filesystem::create_directories(frames_dir);
    }
    std::ofstream csv;
    if (!log_csv.empty()) {
        csv.open(log_csv);
        csv << "frame,ms\n";
    }

    cv::Mat frame_bgr;
    cv::Mat frame_bgra;
    int frame_index = 0;
    while (capture.read(frame_bgr)) {
        cv::Mat out_bgr;
        double elapsed_ms = 0.0;
        if (passthrough) {
            out_bgr = frame_bgr;
        } else {
            cv::cvtColor(frame_bgr, frame_bgra, cv::COLOR_BGR2BGRA);
            const auto start = std::chrono::high_resolution_clock::now();
            cv::Mat out_bgra = core.process_frame(frame_bgra);
            elapsed_ms = std::chrono::duration<double, std::milli>(
                             std::chrono::high_resolution_clock::now() - start)
                             .count();
            if (out_bgra.empty()) {
                std::fprintf(stderr, "frame %d: process_frame failed: %s\n",
                             frame_index, core.get_last_error().c_str());
                out_bgra = frame_bgra;
            }
            cv::cvtColor(out_bgra, out_bgr, cv::COLOR_BGRA2BGR);
        }

        if (writer.isOpened()) writer.write(out_bgr);
        if (!frames_dir.empty()) {
            char name[64];
            std::snprintf(name, sizeof(name), "frame-%05d.png", frame_index);
            cv::imwrite((std::filesystem::path(frames_dir) / name).string(), out_bgr);
        }
        if (csv.is_open()) {
            csv << frame_index << "," << elapsed_ms << "\n";
        }
        frame_index++;
    }
    if (writer.isOpened()) writer.release();

    if (passthrough) {
        std::printf("{\"frames\": %d, \"passthrough\": true}\n", frame_index);
        return 0;
    }
    const StabilizerCore::PerformanceMetrics metrics = core.get_performance_metrics();
    std::printf(
        "{\"frames\": %d, \"total_frames\": %llu, \"successful_frames\": %llu, "
        "\"tracking_failures\": %llu, \"avg_processing_ms\": %.3f}\n",
        frame_index,
        static_cast<unsigned long long>(metrics.total_frames),
        static_cast<unsigned long long>(metrics.successful_frames),
        static_cast<unsigned long long>(metrics.tracking_failures),
        metrics.avg_processing_time * 1000.0);
    return 0;
}
