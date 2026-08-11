#include "core/gpu_backend_policy.hpp"

#include <cassert>

using STABILIZER_GPU::Backend;
using STABILIZER_GPU::Capabilities;
using STABILIZER_GPU::select_backend;
using STABILIZER_GPU::to_string;

int main()
{
    const Capabilities none{};
    const auto cpu = select_backend(Backend::Cpu, none);
    assert(cpu.backend == Backend::Cpu);
    assert(!cpu.fell_back_to_cpu);

    const auto opencl_fallback = select_backend(Backend::OpenCL, none);
    assert(opencl_fallback.backend == Backend::Cpu);
    assert(opencl_fallback.fell_back_to_cpu);

    const Capabilities all{true, true, true};
    assert(select_backend(Backend::OpenCL, all).backend == Backend::OpenCL);
    assert(select_backend(Backend::Cuda, all).backend == Backend::Cuda);
    assert(select_backend(Backend::Metal, all).backend == Backend::Metal);

    const Capabilities opencl_only{true, false, false};
    assert(select_backend(Backend::OpenCL, opencl_only).backend == Backend::OpenCL);
    assert(select_backend(Backend::Cuda, opencl_only).backend == Backend::Cpu);
    assert(select_backend(Backend::Metal, opencl_only).backend == Backend::Cpu);

    assert(to_string(Backend::Cpu) == "cpu");
    assert(to_string(Backend::OpenCL) == "opencl");
    assert(to_string(Backend::Cuda) == "cuda");
    assert(to_string(Backend::Metal) == "metal");

    return 0;
}
