#include <cassert>
#include <cstdint>
#include <cstring>

#include <opencv2/opencv.hpp>

#include "core/frame_utils.hpp"

int main() {
    obs_source_frame reference{};
    reference.width = 2;
    reference.height = 2;
    reference.format = VIDEO_FORMAT_BGRA;
    reference.timestamp = 42;

    const cv::Mat first_mat(2, 2, CV_8UC4, cv::Scalar(1, 2, 3, 4));
    const cv::Mat second_mat(2, 2, CV_8UC4, cv::Scalar(9, 8, 7, 6));

    obs_source_frame* first = FRAME_UTILS::FrameBuffer::create(first_mat, &reference);
    assert(first != nullptr);
    assert(first->data[0] != nullptr);

    uint8_t first_snapshot[16]{};
    std::memcpy(first_snapshot, first->data[0], sizeof(first_snapshot));

    obs_source_frame* second = FRAME_UTILS::FrameBuffer::create(second_mat, &reference);
    assert(second != nullptr);
    assert(second->data[0] != nullptr);

    // Every conversion must own an independent frame structure and data buffer.
    assert(first != second);
    assert(first->data[0] != second->data[0]);

    // Creating a later frame must not overwrite data still referenced by an earlier frame.
    assert(std::memcmp(first_snapshot, first->data[0], sizeof(first_snapshot)) == 0);
    assert(std::memcmp(first->data[0], second->data[0], sizeof(first_snapshot)) != 0);

    FRAME_UTILS::FrameBuffer::release(first);
    FRAME_UTILS::FrameBuffer::release(second);
    return 0;
}
