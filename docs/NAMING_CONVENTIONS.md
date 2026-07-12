# Naming conventions

This document defines naming rules for production code, tests, and OBS integration code. The goal is to make purpose, ownership, lifecycle, and state transitions clear without requiring readers to inspect implementation details.

## General principles

- Prefer names that describe domain meaning rather than container type or abbreviation.
- Encode lifecycle or frame ownership when values represent temporal state.
- Avoid shortened names unless they are standard library or domain terms (`cv`, `obs`, `fps`).
- Keep names consistent between declarations, definitions, tests, and log messages.
- Renaming must not change behavior; run the complete test suite after refactoring.

## Variables

Use descriptive lower snake case for local variables and parameters.

```cpp
// Avoid
std::vector<cv::Point2f> curr_pts;
std::vector<cv::Point2f> prev_pts;

// Prefer
std::vector<cv::Point2f> current_frame_features;
std::vector<cv::Point2f> previous_frame_features;
```

Member variables use a trailing underscore and should describe ownership or lifecycle when relevant.

```cpp
cv::Mat previous_grayscale_frame_;
std::vector<cv::Point2f> previous_frame_features_;
```

## Boolean values

Boolean names must read as a question and start with an action or state prefix such as `is_`, `has_`, `can_`, `should_`, or `was_`.

```cpp
bool is_video_pipeline_initialized_;
bool has_valid_settings_;
bool should_reinitialize_tracking_;
```

Avoid generic names such as `initialized`, `valid`, `ready`, or `enabled` when the subject is not obvious from the containing type.

## Functions

Functions use verb-noun names that describe their observable action.

```cpp
detect_frame_features();
validate_stabilizer_settings();
register_obs_callbacks();
```

Predicates use `is_`, `has_`, `can_`, or `should_`.

```cpp
is_frame_supported();
has_enough_tracking_features();
should_refresh_features();
```

## Types and namespaces

Types use PascalCase. Namespaces must be specific enough to avoid collisions with dependencies and consuming applications.

```cpp
namespace ObsStabilizer {
namespace FeatureDetection {
// ...
}
}
```

Do not introduce a new top-level namespace with a generic name such as `Utils`, `Common`, or `FeatureDetection`.

## Constants

Compile-time constants use `SCREAMING_SNAKE_CASE` when they are macros or existing project-level constants. Strongly typed scoped constants may use the surrounding type's established style.

```cpp
static constexpr int MAX_TRACKING_FAILURES = 5;
```

## Refactoring checklist

Before merging a naming-only change:

1. Search for every declaration and reference, including tests and documentation.
2. Keep public API compatibility unless the PR explicitly documents a breaking change.
3. Update log messages and comments that mention old names.
4. Run formatting, build, unit tests, and static analysis.
5. Keep naming-only changes separate from behavioral changes whenever possible.
