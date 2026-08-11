#include "core/performance_metrics.hpp"

#include <cassert>
#include <cmath>
#include <string>

using namespace STABILIZER_PERFORMANCE;

static bool nearly_equal(double lhs, double rhs, double epsilon = 1e-9)
{
    return std::fabs(lhs - rhs) <= epsilon;
}

int main()
{
    Accumulator metrics;

    const Snapshot empty = metrics.snapshot();
    assert(empty.frame_count == 0);
    assert(nearly_equal(empty.average_processing_ms, 0.0));
    assert(nearly_equal(empty.feature_success_rate, 0.0));

    metrics.record_frame(5.0, true);
    metrics.record_frame(15.0, false);
    metrics.record_frame(-4.0, true);

    const Snapshot snapshot = metrics.snapshot();
    assert(snapshot.frame_count == 3);
    assert(nearly_equal(snapshot.average_processing_ms, 20.0 / 3.0));
    assert(nearly_equal(snapshot.estimated_fps, 150.0));
    assert(nearly_equal(snapshot.feature_success_rate, 200.0 / 3.0));
    assert(snapshot.status == Status::Good);
    assert(std::string(status_label(Status::Warning)) == "Warning");
    assert(!recommendation_for(Status::Poor).empty());

    assert(classify_processing_time(9.99) == Status::Good);
    assert(classify_processing_time(10.0) == Status::Warning);
    assert(classify_processing_time(20.0) == Status::Warning);
    assert(classify_processing_time(20.01) == Status::Poor);

    metrics.reset();
    assert(metrics.snapshot().frame_count == 0);

    return 0;
}
