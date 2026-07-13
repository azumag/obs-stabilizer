#include "../src/core/image_view.hpp"

#include <array>
#include <cassert>
#include <cstdint>

using stabilizer::image::ConstImageView;
using stabilizer::image::ImageView;
using stabilizer::image::PixelFormat;
using stabilizer::image::channels_for;

int main()
{
    static_assert(channels_for(PixelFormat::Gray8) == 1);
    static_assert(channels_for(PixelFormat::Bgr8) == 3);
    static_assert(channels_for(PixelFormat::Bgra8) == 4);

    std::array<std::uint8_t, 32> pixels{};
    ImageView view{pixels.data(), 3, 2, 16, PixelFormat::Bgra8};

    assert(view.is_valid());
    assert(view.channels() == 4);
    assert(view.minimum_stride() == 12);
    assert(view.byte_size() == 32);

    const auto second_row = view.row(1);
    assert(second_row.has_value());
    assert(second_row->data == pixels.data() + 16);
    assert(second_row->height == 1);
    assert(!view.row(2).has_value());

    ConstImageView const_view{view};
    assert(const_view.is_valid());
    assert(const_view.data == pixels.data());
    assert(const_view.byte_size() == 32);

    ImageView tight_bgr{pixels.data(), 4, 2, 12, PixelFormat::Bgr8};
    assert(tight_bgr.is_valid());
    assert(tight_bgr.byte_size() == 24);

    ImageView null_data{nullptr, 3, 2, 12, PixelFormat::Bgra8};
    assert(!null_data.is_valid());
    assert(null_data.byte_size() == 0);

    ImageView zero_width{pixels.data(), 0, 2, 16, PixelFormat::Bgra8};
    assert(!zero_width.is_valid());

    ImageView short_stride{pixels.data(), 4, 2, 15, PixelFormat::Bgra8};
    assert(!short_stride.is_valid());
    assert(!short_stride.row(0).has_value());

    return 0;
}
