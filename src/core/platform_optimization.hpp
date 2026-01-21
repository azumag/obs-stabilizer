/*
 * Platform-specific optimization utilities
 * ARM64 NEON optimizations with cross-platform support
 */

#pragma once

#include <cstdint>
#include <cstring>

// Platform detection
namespace PlatformOptimization {

    // Platform detection utilities
    inline bool is_arm64() {
        #if defined(__aarch64__)
        return true;
        #else
        return false;
        #endif
    }

    inline bool is_apple_silicon() {
        #if defined(__APPLE__) && defined(__aarch64__)
        return true;
        #else
        return false;
        #endif
    }

    // ARM64 NEON intrinsics wrappers
    namespace NEON {

        // 4-element vector structure for NEON operations
        struct alignas(16) float4 {
            float data[4];
            
            float4() = default;
            constexpr float4(float v0, float v1, float v2, float v3) : data{v0, v1, v2, v3} {}
            
            // Factory methods for common patterns
            static constexpr float4 zero() { return float4(0.0f, 0.0f, 0.0f, 0.0f); }
            static constexpr float4 one() { return float4(1.0f, 1.0f, 1.0f, 1.0f); }
            static constexpr float4 fill(float value) { return float4(value, value, value, value); }
        };

        // Convert from NEON register to float4 (ARM64 only)
        inline float4 from_neon(float32x4_t vec) {
            float4 result;
            vst1q_f32(result.data, vec);
            return result;
        }

        // Convert from float4 to NEON register (ARM64 only)
        inline float32x4_t to_neon(const float4& vec) {
            return vld1q_f32(vec.data);
        }

        // Generic fallback for non-ARM64 platforms
        inline float4 from_generic(const float* data) {
            return float4(data[0], data[1], data[2], data[3]);
        }

        inline void to_generic(float* data, const float4& vec) {
            data[0] = vec.data[0]; data[1] = vec.data[1]; 
            data[2] = vec.data[2]; data[3] = vec.data[3];
        }

        // Vector arithmetic operations
        inline float4 add(const float4& a, const float4& b) {
            #ifdef __aarch64__
            if (is_arm64()) {
                float32x4_t a_neon = to_neon(a);
                float32x4_t b_neon = to_neon(b);
                return from_neon(vaddq_f32(a_neon, b_neon));
            }
            #endif
            return float4(a.data[0] + b.data[0], a.data[1] + b.data[1], 
                         a.data[2] + b.data[2], a.data[3] + b.data[3]);
        }

        inline float4 sub(const float4& a, const float4& b) {
            #ifdef __aarch64__
            if (is_arm64()) {
                float32x4_t a_neon = to_neon(a);
                float32x4_t b_neon = to_neon(b);
                return from_neon(vsubq_f32(a_neon, b_neon));
            }
            #endif
            return float4(a.data[0] - b.data[0], a.data[1] - b.data[1], 
                         a.data[2] - b.data[2], a.data[3] - b.data[3]);
        }

        inline float4 mul(const float4& a, const float4& b) {
            #ifdef __aarch64__
            if (is_arm64()) {
                float32x4_t a_neon = to_neon(a);
                float32x4_t b_neon = to_neon(b);
                return from_neon(vmulq_f32(a_neon, b_neon));
            }
            #endif
            return float4(a.data[0] * b.data[0], a.data[1] * b.data[1], 
                         a.data[2] * b.data[2], a.data[3] * b.data[3]);
        }

        inline float4 div(const float4& a, const float4& b) {
            #ifdef __aarch64__
            if (is_arm64()) {
                float32x4_t a_neon = to_neon(a);
                float32x4_t b_neon = to_neon(b);
                return from_neon(vdivq_f32(a_neon, b_neon));
            }
            #endif
            return float4(a.data[0] / b.data[0], a.data[1] / b.data[1], 
                         a.data[2] / b.data[2], a.data[3] / b.data[3]);
        }

        // Scalar operations (broadcast)
        inline float4 mul_scalar(const float4& a, float scalar) {
            #ifdef __aarch64__
            if (is_arm64()) {
                float32x4_t a_neon = to_neon(a);
                float32x4_t scalar_vec = vdupq_n_f32(scalar);
                return from_neon(vmulq_f32(a_neon, scalar_vec));
            }
            #endif
            return float4(a.data[0] * scalar, a.data[1] * scalar, 
                         a.data[2] * scalar, a.data[3] * scalar);
        }

        // Dot product (returns sum of 4 elements)
        inline float dot(const float4& a, const float4& b) {
            #ifdef __aarch64__
            if (is_arm64()) {
                float32x4_t a_neon = to_neon(a);
                float32x4_t b_neon = to_neon(b);
                float32x4_t product = vmulq_f32(a_neon, b_neon);
                return vaddvq_f32(product);
            }
            #endif
            return a.data[0] * b.data[0] + a.data[1] * b.data[1] + 
                   a.data[2] * b.data[2] + a.data[3] * b.data[3];
        }

        // Matrix operations (2x3 transform matrices)
        struct alignas(16) TransformMatrix {
            float4 row0; // [a, b, tx, 0]
            float4 row1; // [c, d, ty, 0]
            
            TransformMatrix() = default;
            constexpr TransformMatrix(float a, float b, float tx, float c, float d, float ty) 
                : row0(a, b, tx, 0.0f), row1(c, d, ty, 0.0f) {}
            
            // Factory methods
            static constexpr TransformMatrix identity() {
                return TransformMatrix(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
            }
            
            static constexpr TransformMatrix zero() {
                return TransformMatrix(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            }
        };

        // Matrix addition
        inline TransformMatrix add(const TransformMatrix& a, const TransformMatrix& b) {
            TransformMatrix result;
            result.row0 = add(a.row0, b.row0);
            result.row1 = add(a.row1, b.row1);
            return result;
        }

        // Matrix multiplication by scalar
        inline TransformMatrix mul_scalar(const TransformMatrix& m, float scalar) {
            TransformMatrix result;
            result.row0 = mul_scalar(m.row0, scalar);
            result.row1 = mul_scalar(m.row1, scalar);
            return result;
        }

        // Apply transform to point
        inline float4 apply_transform(const TransformMatrix& m, const float4& point) {
            float4 x_mul = mul_scalar(m.row0, point.data[0]);
            float4 y_mul = mul_scalar(m.row1, point.data[1]);
            float4 result = add(x_mul, y_mul);
            return result;
        }

        // Sum all elements in a transform matrix (for averaging)
        inline float sum_all(const TransformMatrix& m) {
            return dot(m.row0, float4(1.0f, 1.0f, 1.0f, 0.0f)) + 
                   dot(m.row1, float4(1.0f, 1.0f, 1.0f, 0.0f));
        }

    } // namespace NEON

    // SIMD-optimized utilities for common operations
    namespace SIMD {

        // Optimized sum of array elements using NEON when available
        inline float sum_array(const float* data, size_t count) {
            if (count == 0) return 0.0f;
            
            #ifdef __aarch64__
            if (is_arm64()) {
                // Use NEON for aligned data and large counts
                if (count >= 4 && (reinterpret_cast<uintptr_t>(data) & 15) == 0) {
                    float32x4_t sum_vec = vdupq_n_f32(0.0f);
                    
                    size_t i = 0;
                    // Process 4 elements at a time
                    for (; i + 3 < count; i += 4) {
                        float32x4_t data_vec = vld1q_f32(&data[i]);
                        sum_vec = vaddq_f32(sum_vec, data_vec);
                    }
                    
                    // Sum the accumulated vector
                    float sum = vaddvq_f32(sum_vec);
                    
                    // Handle remaining elements
                    for (; i < count; ++i) {
                        sum += data[i];
                    }
                    
                    return sum;
                }
            }
            #endif
            
            // Fallback to simple summation
            float sum = 0.0f;
            for (size_t i = 0; i < count; ++i) {
                sum += data[i];
            }
            return sum;
        }

        // Optimized dot product using NEON when available
        inline float dot_product_array(const float* a, const float* b, size_t count) {
            if (count == 0) return 0.0f;
            
            #ifdef __aarch64__
            if (is_arm64()) {
                // Use NEON for aligned data and large counts
                if (count >= 4 && (reinterpret_cast<uintptr_t>(a) & 15) == 0 && 
                    (reinterpret_cast<uintptr_t>(b) & 15) == 0) {
                    float32x4_t sum_vec = vdupq_n_f32(0.0f);
                    
                    size_t i = 0;
                    // Process 4 elements at a time
                    for (; i + 3 < count; i += 4) {
                        float32x4_t a_vec = vld1q_f32(&a[i]);
                        float32x4_t b_vec = vld1q_f32(&b[i]);
                        float32x4_t product = vmulq_f32(a_vec, b_vec);
                        sum_vec = vaddq_f32(sum_vec, product);
                    }
                    
                    // Sum the accumulated vector
                    float sum = vaddvq_f32(sum_vec);
                    
                    // Handle remaining elements
                    for (; i < count; ++i) {
                        sum += a[i] * b[i];
                    }
                    
                    return sum;
                }
            }
            #endif
            
            // Fallback to simple dot product
            float sum = 0.0f;
            for (size_t i = 0; i < count; ++i) {
                sum += a[i] * b[i];
            }
            return sum;
        }

        // Optimized vector scaling using NEON when available
        inline void scale_array(float* data, float scale, size_t count) {
            if (count == 0) return;
            
            #ifdef __aarch64__
            if (is_arm64()) {
                // Use NEON for aligned data and large counts
                if (count >= 4 && (reinterpret_cast<uintptr_t>(data) & 15) == 0) {
                    float32x4_t scale_vec = vdupq_n_f32(scale);
                    
                    size_t i = 0;
                    // Process 4 elements at a time
                    for (; i + 3 < count; i += 4) {
                        float32x4_t data_vec = vld1q_f32(&data[i]);
                        float32x4_t result = vmulq_f32(data_vec, scale_vec);
                        vst1q_f32(&data[i], result);
                    }
                    
                    // Handle remaining elements
                    for (; i < count; ++i) {
                        data[i] *= scale;
                    }
                    return;
                }
            }
            #endif
            
            // Fallback to simple scaling
            for (size_t i = 0; i < count; ++i) {
                data[i] *= scale;
            }
        }

        // Optimized vector addition using NEON when available
        inline void add_arrays(float* result, const float* a, const float* b, size_t count) {
            if (count == 0) return;
            
            #ifdef __aarch64__
            if (is_arm64()) {
                // Use NEON for aligned data and large counts
                if (count >= 4 && (reinterpret_cast<uintptr_t>(result) & 15) == 0 &&
                    (reinterpret_cast<uintptr_t>(a) & 15) == 0 &&
                    (reinterpret_cast<uintptr_t>(b) & 15) == 0) {
                    
                    size_t i = 0;
                    // Process 4 elements at a time
                    for (; i + 3 < count; i += 4) {
                        float32x4_t a_vec = vld1q_f32(&a[i]);
                        float32x4_t b_vec = vld1q_f32(&b[i]);
                        float32x4_t result_vec = vaddq_f32(a_vec, b_vec);
                        vst1q_f32(&result[i], result_vec);
                    }
                    
                    // Handle remaining elements
                    for (; i < count; ++i) {
                        result[i] = a[i] + b[i];
                    }
                    return;
                }
            }
            #endif
            
            // Fallback to simple addition
            for (size_t i = 0; i < count; ++i) {
                result[i] = a[i] + b[i];
            }
        }

    } // namespace SIMD

    // Memory optimization utilities
    namespace Memory {

        // Cache-aligned memory allocator
        inline void* allocate_aligned(size_t size, size_t alignment = 64) {
            void* ptr = nullptr;
            
            #ifdef __APPLE__
            if (is_apple_silicon()) {
                // Use macOS aligned allocation
                posix_memalign(&ptr, alignment, size);
            } else {
            #endif
                // Fallback to regular allocation
                ptr = malloc(size);
            #ifdef __APPLE__
            }
            #endif
            
            return ptr;
        }

        // Free aligned memory
        inline void free_aligned(void* ptr) {
            if (ptr) {
                free(ptr);
            }
        }

        // Simple aligned vector template (no complex dependencies for standalone build)
        template<typename T>
        class AlignedVector {
        private:
            T* data_;
            size_t size_;
            size_t capacity_;

        public:
            AlignedVector() : data_(nullptr), size_(0), capacity_(0) {}
            
            explicit AlignedVector(size_t size) : size_(size), capacity_(size) {
                data_ = static_cast<T*>(allocate_aligned(size * sizeof(T)));
                if (data_) {
                    for (size_t i = 0; i < size; ++i) {
                        new (&data_[i]) T(); // Default construct
                    }
                }
            }
            
            ~AlignedVector() {
                if (data_) {
                    for (size_t i = 0; i < size_; ++i) {
                        data_[i].~T(); // Destruct
                    }
                    free_aligned(data_);
                }
            }
            
            // Disable copying
            AlignedVector(const AlignedVector&) = delete;
            AlignedVector& operator=(const AlignedVector&) = delete;
            
            // Enable moving
            AlignedVector(AlignedVector&& other) noexcept 
                : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
                other.data_ = nullptr;
                other.size_ = 0;
                other.capacity_ = 0;
            }
            
            AlignedVector& operator=(AlignedVector&& other) noexcept {
                if (this != &other) {
                    // Cleanup current
                    if (data_) {
                        for (size_t i = 0; i < size_; ++i) {
                            data_[i].~T();
                        }
                        free_aligned(data_);
                    }
                    
                    // Take ownership of other's data
                    data_ = other.data_;
                    size_ = other.size_;
                    capacity_ = other.capacity_;
                    
                    other.data_ = nullptr;
                    other.size_ = 0;
                    other.capacity_ = 0;
                }
                return *this;
            }
            
            T* data() { return data_; }
            const T* data() const { return data_; }
            size_t size() const { return size_; }
            size_t capacity() const { return capacity_; }
            
            T& operator[](size_t index) { return data_[index]; }
            const T& operator[](size_t index) const { return data_[index]; }
            
        };

    } // namespace Memory

    // Platform-specific performance monitoring
    namespace Performance {

        // Check if NEON instructions are available (ARM64)
        inline bool has_neon_support() {
            #ifdef __aarch64__
            return true;
            #else
            return false;
            #endif
        }

        // Check if Apple Accelerate framework is available
        inline bool has_accelerate_support() {
            #ifdef __APPLE__
            return true;
            #else
            return false;
            #endif
        }

        // Get platform-specific optimization flags
        inline std::string get_optimization_flags() {
            std::string flags;
            
            #ifdef __aarch64__
            flags += "ARM64_NEON ";
            #endif
            
            #ifdef __APPLE__
            flags += "APPLE_PLATFORM ";
            #endif
            
            #ifdef __clang__
            flags += "CLANG ";
            #endif
            
            #ifdef __GNUC__
            flags += "GCC ";
            #endif
            
            return flags.empty() ? "GENERIC" : flags;
        }

    } // namespace Performance

} // namespace PlatformOptimization