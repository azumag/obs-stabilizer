#pragma once

#include <string_view>

namespace STABILIZER_GPU {

/** Compute backend requested or selected for stabilization work. */
enum class Backend {
    Cpu,
    OpenCL,
    Cuda,
    Metal,
};

/** Runtime availability of optional accelerated backends. */
struct Capabilities {
    /** OpenCL is available for use. */
    bool opencl_available{false};
    /** CUDA is available for use. */
    bool cuda_available{false};
    /** Metal is available for use. */
    bool metal_available{false};
};

/** Result of resolving a requested backend against detected capabilities. */
struct Selection {
    /** Backend that should actually be used. */
    Backend backend{Backend::Cpu};
    /** True when an unavailable accelerator forced CPU fallback. */
    bool fell_back_to_cpu{false};
};

/** Return the stable configuration label for a backend. */
constexpr std::string_view to_string(Backend backend) noexcept
{
    switch (backend) {
    case Backend::Cpu:
        return "cpu";
    case Backend::OpenCL:
        return "opencl";
    case Backend::Cuda:
        return "cuda";
    case Backend::Metal:
        return "metal";
    }
    return "cpu";
}

/** Return whether the requested backend is present in the supplied capabilities. */
constexpr bool is_available(Backend backend, const Capabilities& capabilities) noexcept
{
    switch (backend) {
    case Backend::Cpu:
        return true;
    case Backend::OpenCL:
        return capabilities.opencl_available;
    case Backend::Cuda:
        return capabilities.cuda_available;
    case Backend::Metal:
        return capabilities.metal_available;
    }
    return false;
}

/** Select the requested backend when available, otherwise fall back to CPU. */
constexpr Selection select_backend(Backend requested, const Capabilities& capabilities) noexcept
{
    if (is_available(requested, capabilities)) {
        return {requested, false};
    }
    return {Backend::Cpu, requested != Backend::Cpu};
}

} // namespace STABILIZER_GPU
