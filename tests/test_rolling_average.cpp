#include "core/rolling_average.hpp"

#include <cassert>
#include <cmath>
#include <stdexcept>

namespace {

bool approximately_equal(double lhs, double rhs)
{
    return std::fabs(lhs - rhs) < 1e-9;
}

} // namespace

int main()
{
    STABILIZER_METRICS::RollingAverage average;
    assert(average.empty());
    assert(average.sample_count() == 0);
    assert(approximately_equal(average.value(), 0.0));

    average.add(10.0);
    average.add(20.0);
    average.add(30.0);
    assert(!average.empty());
    assert(average.sample_count() == 3);
    assert(approximately_equal(average.value(), 20.0));

    average.add(0.0);
    assert(average.sample_count() == 4);
    assert(approximately_equal(average.value(), 15.0));

    bool rejected_negative_sample = false;
    try {
        average.add(-0.1);
    } catch (const std::invalid_argument&) {
        rejected_negative_sample = true;
    }
    assert(rejected_negative_sample);
    assert(average.sample_count() == 4);
    assert(approximately_equal(average.value(), 15.0));

    average.reset();
    assert(average.empty());
    assert(average.sample_count() == 0);
    assert(approximately_equal(average.value(), 0.0));

    return 0;
}
