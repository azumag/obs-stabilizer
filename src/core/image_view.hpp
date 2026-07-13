#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

namespace stabilizer::image {

enum class PixelFormat {
    Gray8,
    Bgr8,
    Bgra8,
};

constexpr std::size_t channels_for(PixelFormat format) noexcept
{
    switch (format) {
    case PixelFormat::Gray8:
        return 1;
    case PixelFormat::Bgr8:
        return 3;
    case PixelFormat::Bgra8:
        return 4;
    }

    return 0;
}

struct ImageView {
    std::uint8_t* data = nullptr;
    std::size_t width = 0;
    std::size_t height = 0;
    std::size_t stride = 0;
    PixelFormat format = PixelFormat::Bgra8;

    [[nodiscard]] constexpr std::size_t channels() const noexcept
    {
        return channels_for(format);
    }

    [[nodiscard]] constexpr std::size_t minimum_stride() const noexcept
    {
        return width * channels();
    }

    [[nodiscard]] constexpr bool is_valid() const noexcept
    {
        return data != nullptr && width > 0 && height > 0 &&
               channels() > 0 && stride >= minimum_stride();
    }

    [[nodiscard]] constexpr std::size_t byte_size() const noexcept
    {
        return is_valid() ? stride * height : 0;
    }

    [[nodiscard]] constexpr std::optional<ImageView> row(std::size_t index) const noexcept
    {
        if (!is_valid() || index >= height) {
            return std::nullopt;
        }

        return ImageView{
            data + (index * stride),
            width,
            1,
            stride,
            format,
        };
    }
};

struct ConstImageView {
    const std::uint8_t* data = nullptr;
    std::size_t width = 0;
    std::size_t height = 0;
    std::size_t stride = 0;
    PixelFormat format = PixelFormat::Bgra8;

    constexpr ConstImageView() noexcept = default;

    constexpr ConstImageView(const ImageView& view) noexcept
        : data(view.data),
          width(view.width),
          height(view.height),
          stride(view.stride),
          format(view.format)
    {
    }

    [[nodiscard]] constexpr std::size_t channels() const noexcept
    {
        return channels_for(format);
    }

    [[nodiscard]] constexpr std::size_t minimum_stride() const noexcept
    {
        return width * channels();
    }

    [[nodiscard]] constexpr bool is_valid() const noexcept
    {
        return data != nullptr && width > 0 && height > 0 &&
               channels() > 0 && stride >= minimum_stride();
    }

    [[nodiscard]] constexpr std::size_t byte_size() const noexcept
    {
        return is_valid() ? stride * height : 0;
    }
};

} // namespace stabilizer::image
