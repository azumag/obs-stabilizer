#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>

namespace stabilizer::image {

/** Pixel layouts supported by the generic image-view abstraction. */
enum class PixelFormat {
    Gray8,
    Bgr8,
    Bgra8,
};

/** Return the number of interleaved channels for a pixel format. */
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

/** Return whether a size multiplication can be performed without overflow. */
constexpr bool can_multiply(std::size_t left, std::size_t right) noexcept
{
    return right == 0 || left <= (std::numeric_limits<std::size_t>::max() / right);
}

/** Non-owning mutable view over a packed image buffer. */
struct ImageView {
    /** Pointer to the first byte of image data; never owned by the view. */
    std::uint8_t* data = nullptr;
    /** Image width in pixels. */
    std::size_t width = 0;
    /** Image height in pixels. */
    std::size_t height = 0;
    /** Number of bytes between adjacent rows. */
    std::size_t stride = 0;
    /** Pixel layout of the borrowed buffer. */
    PixelFormat format = PixelFormat::Bgra8;

    /** Return the number of channels represented by format. */
    [[nodiscard]] constexpr std::size_t channels() const noexcept
    {
        return channels_for(format);
    }

    /** Return the smallest valid stride for the current width and format. */
    [[nodiscard]] constexpr std::size_t minimum_stride() const noexcept
    {
        return can_multiply(width, channels()) ? width * channels() : 0;
    }

    /** Validate pointer, dimensions, stride, format, and byte-size arithmetic. */
    [[nodiscard]] constexpr bool is_valid() const noexcept
    {
        return data != nullptr && width > 0 && height > 0 &&
               channels() > 0 && minimum_stride() > 0 &&
               stride >= minimum_stride() && can_multiply(stride, height);
    }

    /** Return the addressable byte span, or zero when the view is invalid. */
    [[nodiscard]] constexpr std::size_t byte_size() const noexcept
    {
        return is_valid() ? stride * height : 0;
    }

    /** Return a one-row borrowed subview, or std::nullopt for an invalid index/view. */
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

/** Non-owning read-only view over a packed image buffer. */
struct ConstImageView {
    /** Pointer to the first byte of image data; never owned by the view. */
    const std::uint8_t* data = nullptr;
    /** Image width in pixels. */
    std::size_t width = 0;
    /** Image height in pixels. */
    std::size_t height = 0;
    /** Number of bytes between adjacent rows. */
    std::size_t stride = 0;
    /** Pixel layout of the borrowed buffer. */
    PixelFormat format = PixelFormat::Bgra8;

    /** Construct an empty invalid view. */
    constexpr ConstImageView() noexcept = default;

    /** Construct a read-only view that borrows the same storage as a mutable view. */
    constexpr ConstImageView(const ImageView& view) noexcept
        : data(view.data),
          width(view.width),
          height(view.height),
          stride(view.stride),
          format(view.format)
    {
    }

    /** Return the number of channels represented by format. */
    [[nodiscard]] constexpr std::size_t channels() const noexcept
    {
        return channels_for(format);
    }

    /** Return the smallest valid stride for the current width and format. */
    [[nodiscard]] constexpr std::size_t minimum_stride() const noexcept
    {
        return can_multiply(width, channels()) ? width * channels() : 0;
    }

    /** Validate pointer, dimensions, stride, format, and byte-size arithmetic. */
    [[nodiscard]] constexpr bool is_valid() const noexcept
    {
        return data != nullptr && width > 0 && height > 0 &&
               channels() > 0 && minimum_stride() > 0 &&
               stride >= minimum_stride() && can_multiply(stride, height);
    }

    /** Return the addressable byte span, or zero when the view is invalid. */
    [[nodiscard]] constexpr std::size_t byte_size() const noexcept
    {
        return is_valid() ? stride * height : 0;
    }
};

} // namespace stabilizer::image
