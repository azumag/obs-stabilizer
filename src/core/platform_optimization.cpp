#ifndef BUILD_STANDALONE

#include "core/platform_optimization.hpp"
#include <cstring>

namespace PlatformOptimization {

    // Additional platform-specific implementations can be added here
    // For now, this file provides the foundation for platform optimizations

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
            flags += "APPLE_ACCELERATE ";
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

    // SIMD-optimized utilities for common operations
    namespace SIMD {

        // Optimized sum of array elements using NEON when available
        inline float sum_array(const float* data, size_t count) {
            if (count == 0) return 0.0f;
            
            #ifdef __aarch64__
            if (has_neon_support()) {
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
            if (has_neon_support()) {
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
            if (has_neon_support()) {
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
            if (has_neon_support()) {
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

        // Cache-aligned memory pool for frequently allocated objects
        template<typename T, size_t PoolSize = 64>
        class CacheAlignedPool {
        private:
            struct alignas(64) PoolEntry {
                T data;
                bool used;
            };
            
            PoolEntry pool_[PoolSize];
            std::vector<void*> allocated_;
            std::mutex mutex_;

        public:
            CacheAlignedPool() {
                // Initialize all entries as free
                for (size_t i = 0; i < PoolSize; ++i) {
                    pool_[i].used = false;
                }
            }
            
            ~CacheAlignedPool() {
                std::lock_guard<std::mutex> lock(mutex_);
                for (void* ptr : allocated_) {
                    free_aligned(ptr);
                }
            }
            
            T* allocate() {
                std::lock_guard<std::mutex> lock(mutex_);
                
                // Try to find a free entry in the pool
                for (size_t i = 0; i < PoolSize; ++i) {
                    if (!pool_[i].used) {
                        pool_[i].used = true;
                        return &pool_[i].data;
                    }
                }
                
                // Pool is full, allocate new aligned memory
                T* ptr = static_cast<T*>(allocate_aligned(sizeof(T)));
                if (ptr) {
                    new (ptr) T(); // Construct in-place
                    allocated_.push_back(ptr);
                }
                return ptr;
            }
            
            void deallocate(T* ptr) {
                std::lock_guard<std::mutex> lock(mutex_);
                
                // Check if it's in the pool
                for (size_t i = 0; i < PoolSize; ++i) {
                    if (&pool_[i].data == ptr) {
                        pool_[i].used = false;
                        ptr->~T(); // Destruct
                        return;
                    }
                }
                
                // Not in pool, find in allocated list and free
                auto it = std::find(allocated_.begin(), allocated_.end(), ptr);
                if (it != allocated_.end()) {
                    ptr->~T(); // Destruct
                    free_aligned(*it);
                    allocated_.erase(it);
                }
            }
            
            size_t pool_size() const { return PoolSize; }
            size_t allocated_count() const { return allocated_.size(); }
            
        };

        // Specialized pool for cv::Mat objects (common in video processing)
        class MatPool {
        private:
            struct alignas(64) MatEntry {
                cv::Mat mat;
                bool used;
                int width, height;
            };
            
            std::vector<MatEntry> pool_;
            std::mutex mutex_;

        public:
            explicit MatPool(size_t size = 16) : pool_(size) {
                for (auto& entry : pool_) {
                    entry.used = false;
                }
            }
            
            cv::Mat* get(int width, int height, int type) {
                std::lock_guard<std::mutex> lock(mutex_);
                
                // Try to find a matching free entry
                for (auto& entry : pool_) {
                    if (!entry.used && entry.width == width && entry.height == height && 
                        entry.mat.type() == type) {
                        entry.used = true;
                        return &entry.mat;
                    }
                }
                
                // Try any free entry
                for (auto& entry : pool_) {
                    if (!entry.used) {
                        entry.used = true;
                        entry.width = width;
                        entry.height = height;
                        entry.mat.create(height, width, type);
                        return &entry.mat;
                    }
                }
                
                return nullptr; // Pool exhausted
            }
            
            void release(cv::Mat* mat) {
                std::lock_guard<std::mutex> lock(mutex_);
                
                for (auto& entry : pool_) {
                    if (&entry.mat == mat) {
                        entry.used = false;
                        entry.mat.release();
                        return;
                    }
                }
            }
            
            size_t size() const { return pool_.size(); }
            size_t used_count() const {
                size_t count = 0;
                for (const auto& entry : pool_) {
                    if (entry.used) count++;
                }
                return count;
            }
            
        };

    } // namespace Memory

} // namespace PlatformOptimization

#endif // BUILD_STANDALONE