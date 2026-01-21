#ifndef BUILD_STANDALONE

#include "core/platform_optimization.hpp"
#include <cstring>

namespace PlatformOptimization {

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
