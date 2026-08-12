#pragma once

#include <cstddef>
#include <stdexcept>

namespace STABILIZER_METRICS {

/**
 * Incremental arithmetic mean accumulator.
 *
 * Keeps only the current sample count and mean, avoiding an ever-growing
 * history buffer. Negative samples are rejected because processing durations
 * and similar stabilizer metrics cannot be negative.
 */
class RollingAverage {
public:
    /** Add a non-negative sample to the running mean. */
    void add(double sample)
    {
        if (sample < 0.0) {
            throw std::invalid_argument("rolling average sample must be non-negative");
        }

        ++sample_count_;
        average_ += (sample - average_) / static_cast<double>(sample_count_);
    }

    /** Clear all accumulated samples and reset the mean to zero. */
    void reset() noexcept
    {
        sample_count_ = 0;
        average_ = 0.0;
    }

    /** Return the number of samples included in the mean. */
    [[nodiscard]] std::size_t sample_count() const noexcept
    {
        return sample_count_;
    }

    /** Return the current arithmetic mean, or zero when empty. */
    [[nodiscard]] double value() const noexcept
    {
        return average_;
    }

    /** Return true when no samples have been recorded. */
    [[nodiscard]] bool empty() const noexcept
    {
        return sample_count_ == 0;
    }

private:
    std::size_t sample_count_ = 0;
    double average_ = 0.0;
};

} // namespace STABILIZER_METRICS
