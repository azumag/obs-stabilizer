#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

#include "core/frame_utils.hpp"

namespace {

obs_source_frame make_frame(uint32_t format,
                            uint32_t width,
                            uint32_t height,
                            uint32_t linesize,
                            uint8_t* data) {
    obs_source_frame frame;
    std::memset(&frame, 0, sizeof(frame));
    frame.format = format;
    frame.width = width;
    frame.height = height;
    frame.linesize[0] = linesize;
    frame.data[0] = data;
    return frame;
}

void verifies_bgra_view_borrows_storage() {
    constexpr uint32_t width = 4;
    constexpr uint32_t height = 3;
    constexpr uint32_t linesize = width * 4 + 8;
    std::vector<uint8_t> storage(linesize * height, 0x11);
    auto frame = make_frame(VIDEO_FORMAT_BGRA, width, height, linesize, storage.data());

    cv::Mat view = FRAME_UTILS::Conversion::obs_to_cv_view(&frame);

    assert(!view.empty());
    assert(view.type() == CV_8UC4);
    assert(view.cols == static_cast<int>(width));
    assert(view.rows == static_cast<int>(height));
    assert(view.step == linesize);
    assert(view.data == storage.data());

    storage[0] = 0x7f;
    assert(view.data[0] == 0x7f);

    view.data[1] = 0x55;
    assert(storage[1] == 0x55);
}

void verifies_bgr3_view_uses_three_channels() {
    constexpr uint32_t width = 5;
    constexpr uint32_t height = 2;
    constexpr uint32_t linesize = width * 3;
    std::vector<uint8_t> storage(linesize * height, 0x22);
    auto frame = make_frame(VIDEO_FORMAT_BGR3, width, height, linesize, storage.data());

    cv::Mat view = FRAME_UTILS::Conversion::obs_to_cv_view(&frame);

    assert(!view.empty());
    assert(view.type() == CV_8UC3);
    assert(view.data == storage.data());
}

void rejects_planar_and_invalid_frames() {
    std::vector<uint8_t> storage(64, 0);
    auto planar = make_frame(VIDEO_FORMAT_NV12, 4, 4, 4, storage.data());
    assert(FRAME_UTILS::Conversion::obs_to_cv_view(&planar).empty());

    auto zero_width = make_frame(VIDEO_FORMAT_BGRA, 0, 4, 16, storage.data());
    assert(FRAME_UTILS::Conversion::obs_to_cv_view(&zero_width).empty());

    auto null_data = make_frame(VIDEO_FORMAT_BGRA, 4, 4, 16, nullptr);
    assert(FRAME_UTILS::Conversion::obs_to_cv_view(&null_data).empty());

    auto short_bgra_stride = make_frame(VIDEO_FORMAT_BGRA, 4, 2, 15, storage.data());
    assert(FRAME_UTILS::Conversion::obs_to_cv_view(&short_bgra_stride).empty());

    auto short_bgr3_stride = make_frame(VIDEO_FORMAT_BGR3, 4, 2, 11, storage.data());
    assert(FRAME_UTILS::Conversion::obs_to_cv_view(&short_bgr3_stride).empty());

    assert(FRAME_UTILS::Conversion::obs_to_cv_view(nullptr).empty());
}

} // namespace

int main() {
    verifies_bgra_view_borrows_storage();
    verifies_bgr3_view_uses_three_channels();
    rejects_planar_and_invalid_frames();
    return 0;
}
