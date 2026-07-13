#pragma once

#include <string_view>

namespace STABILIZER_GPU {

enum class Backend {
    Cpu,
    OpenCL,
    Cuda,
    Metal,
};

struct Capabilities {
    bool opencl_available{false};
    bool cuda_available{false};
    bool metal_available{false};
};

struct Selection {
    Backend backend{Backend::Cpu};
    bool fell_back_to_cpu{false};
};

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

constexpr Selection select_backend(Backend requested, const Capabilities& capabilities) noexcept
{
    if (is_available(requested, capabilities)) {
        return {requested, false};
    }
    return {Backend::Cpu, requested != Backend::Cpu};
}

} // namespace STABILIZER_GPU
