# OBS Stabilizer User Guide

## Overview

OBS Stabilizer is a video filter for OBS Studio that reduces camera shake in real time. It tracks visual features between frames, estimates camera motion, smooths that motion, and applies a compensating transform.

Typical uses include handheld cameras, IRL streaming, webcams mounted on moving desks, and capture devices with minor vibration.

## Installation

### Windows

1. Download the Windows release artifact.
2. Copy the plugin binary and data files into the matching OBS plugin directories.
3. Restart OBS Studio.
4. Confirm that **Video Stabilizer** appears under source filters.

### macOS

1. Download the macOS release artifact for your architecture.
2. Copy the plugin into the OBS plugin directory.
3. If macOS blocks the plugin, review it in **System Settings > Privacy & Security**.
4. Restart OBS Studio.

### Linux

1. Install the required OpenCV and OBS runtime libraries.
2. Copy the plugin shared object into the OBS plugin directory.
3. Restart OBS Studio.

Exact paths vary by OBS packaging method. Keep the plugin architecture aligned with the installed OBS build.

## Quick start

1. Add or select a video source in OBS.
2. Open **Filters** for that source.
3. Add **Video Stabilizer** as an effect filter.
4. Choose a preset.
5. Enable stabilization.
6. Observe the preview and adjust smoothing and correction limits as needed.

Start with **Streaming** for general use. Use **Gaming** when low latency matters most, and **Recording** when stronger smoothing is acceptable.

## Parameters

### Enable Stabilization

Turns processing on or off without removing the filter.

### Smoothing Radius

Controls how many recent transforms contribute to motion smoothing.

- Lower values react faster and add less latency.
- Higher values produce steadier motion but may feel less responsive.

### Max Correction

Limits how far the image may be shifted or transformed. Lower values reduce aggressive corrections and edge exposure.

### Feature Count

Sets the target number of visual points used for tracking. Higher values may improve difficult scenes but increase CPU usage.

### Quality Level

Controls the minimum accepted feature quality. Lower values accept weaker corners; higher values keep only stronger features.

### Minimum Distance

Controls spacing between detected features. Increase it to distribute points more broadly across the frame.

### Block Size

Sets the neighborhood size used during feature detection. Odd values are recommended.

### Harris Detector and K

Enables Harris corner scoring and configures its sensitivity. Leave the preset defaults unless testing a scene with unusual textures.

### Edge Handling

- **Black Padding**: preserves scale but may expose empty borders.
- **Crop Borders**: crops unstable edges.
- **Scale to Fit**: enlarges the image to reduce visible borders.

### Smoothing Mode

- **Moving Average**: a windowed average of recent transforms; the default and the more predictable of the two.
- **Kalman**: a constant-velocity filter over the transform components; can track sustained motion with less lag at the cost of more tuning sensitivity.

### Debug Mode

Enables additional diagnostic logging in the OBS log. Leave off in normal use.

## Presets

### Gaming

Prioritizes responsiveness and lower processing overhead. Suitable for gameplay cameras and interactive content.

### Streaming

Balanced default for live production.

### Recording

Prioritizes smoothness over responsiveness. Suitable when minor latency is acceptable.

### Custom

Keeps manually selected values instead of applying a built-in preset. Custom parameter sets can be saved and reloaded by name from the filter's properties panel.

## Scene recommendations

- Ensure the frame contains visible corners or textured objects.
- Avoid pointing at large featureless surfaces.
- Use adequate lighting to reduce motion blur.
- Keep correction limits conservative for very wide camera movements.
- For 4K input, monitor CPU usage and reduce feature count if needed.

## Troubleshooting

### The filter does not appear

Confirm that the plugin was copied to the correct OBS plugin directory and matches the OBS architecture.

### The image is unchanged

Confirm that stabilization is enabled and the filter is attached to the intended source. A scene with too few detectable features may also remain unchanged.

### Borders are visible

Use **Crop Borders** or **Scale to Fit**, or lower the maximum correction.

### Motion looks delayed

Reduce the smoothing radius or switch to the Gaming preset.

### CPU usage is high

Reduce source resolution, feature count, or smoothing radius. Avoid stacking multiple expensive filters on the same source.

### The image jumps after a scene cut

The tracker may need several frames to reacquire features. Scene cuts and severe blur can temporarily reduce tracking quality.

## FAQ

### Does this replace optical image stabilization?

No. It is software stabilization applied after capture. Hardware or in-camera stabilization may still provide better results before compression.

### Can I use multiple instances?

Yes, but each instance consumes CPU and maintains separate tracking state.

### Is 4K supported?

The plugin can process high-resolution input, but performance depends on hardware and settings. Validate frame time before production use.

### Are settings applied immediately?

Filter settings are intended to apply during operation. Large changes may cause the tracker to reinitialize.

## Reporting problems

Include the operating system, OBS version, source resolution, selected preset, relevant settings, and a reproducible sequence. Attach OBS logs when possible — filter lines with `[obs-stabilizer]` in `%APPDATA%\obs-studio\logs\` (Windows), `~/Library/Application Support/obs-studio/logs/` (macOS), or `~/.config/obs-studio/logs/` (Linux).
