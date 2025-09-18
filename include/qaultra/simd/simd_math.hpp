#pragma once

#include <cstddef>
#include <vector>
#include <array>
#include <memory>

#ifdef QAULTRA_ENABLE_SIMD
#include <immintrin.h>
#ifdef __AVX512F__
#define QAULTRA_SIMD_WIDTH 512
#define QAULTRA_SIMD_ALIGNMENT 64
#elif __AVX2__
#define QAULTRA_SIMD_WIDTH 256
#define QAULTRA_SIMD_ALIGNMENT 32
#elif __SSE4_2__
#define QAULTRA_SIMD_WIDTH 128
#define QAULTRA_SIMD_ALIGNMENT 16
#else
#define QAULTRA_SIMD_WIDTH 64
#define QAULTRA_SIMD_ALIGNMENT 8
#endif
#else
#define QAULTRA_SIMD_WIDTH 64
#define QAULTRA_SIMD_ALIGNMENT 8
#endif

namespace qaultra::simd {

/// SIMD-aligned allocator
template<typename T, size_t Alignment = QAULTRA_SIMD_ALIGNMENT>
class aligned_allocator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = std::size_t;

    template<typename U>
    struct rebind {
        using other = aligned_allocator<U, Alignment>;
    };

    aligned_allocator() noexcept = default;

    template<typename U>
    aligned_allocator(const aligned_allocator<U, Alignment>&) noexcept {}

    pointer allocate(size_type n) {
        if (n == 0) return nullptr;

        void* ptr = nullptr;
#ifdef _WIN32
        ptr = _aligned_malloc(n * sizeof(T), Alignment);
        if (!ptr) throw std::bad_alloc();
#else
        if (posix_memalign(&ptr, Alignment, n * sizeof(T)) != 0) {
            throw std::bad_alloc();
        }
#endif
        return static_cast<pointer>(ptr);
    }

    void deallocate(pointer p, size_type) noexcept {
        if (p) {
#ifdef _WIN32
            _aligned_free(p);
#else
            free(p);
#endif
        }
    }

    template<typename U>
    bool operator==(const aligned_allocator<U, Alignment>&) const noexcept {
        return true;
    }

    template<typename U>
    bool operator!=(const aligned_allocator<U, Alignment>&) const noexcept {
        return false;
    }
};

/// SIMD-aligned vector types
template<typename T>
using aligned_vector = std::vector<T, aligned_allocator<T>>;

using f64_vector = aligned_vector<double>;
using f32_vector = aligned_vector<float>;
using i64_vector = aligned_vector<int64_t>;
using i32_vector = aligned_vector<int32_t>;

/// SIMD math operations
class SimdMath {
public:
    /// Vector addition with SIMD optimization
    static void add(const double* a, const double* b, double* result, size_t size);
    static void add(const float* a, const float* b, float* result, size_t size);

    /// Vector subtraction with SIMD optimization
    static void subtract(const double* a, const double* b, double* result, size_t size);
    static void subtract(const float* a, const float* b, float* result, size_t size);

    /// Vector multiplication with SIMD optimization
    static void multiply(const double* a, const double* b, double* result, size_t size);
    static void multiply(const float* a, const float* b, float* result, size_t size);

    /// Scalar multiplication with SIMD optimization
    static void multiply_scalar(const double* a, double scalar, double* result, size_t size);
    static void multiply_scalar(const float* a, float scalar, float* result, size_t size);

    /// Dot product with SIMD optimization
    static double dot_product(const double* a, const double* b, size_t size);
    static float dot_product(const float* a, const float* b, size_t size);

    /// Sum reduction with SIMD optimization
    static double sum(const double* data, size_t size);
    static float sum(const float* data, size_t size);

    /// Min/Max with SIMD optimization
    static double min(const double* data, size_t size);
    static double max(const double* data, size_t size);
    static float min(const float* data, size_t size);
    static float max(const float* data, size_t size);

    /// Moving average with SIMD optimization
    static void moving_average(const double* data, double* result, size_t size, size_t window);
    static void moving_average(const float* data, float* result, size_t size, size_t window);

    /// Standard deviation with SIMD optimization
    static double std_dev(const double* data, size_t size, double mean = 0.0);
    static float std_dev(const float* data, size_t size, float mean = 0.0f);

    /// Exponential moving average
    static void ema(const double* data, double* result, size_t size, double alpha);
    static void ema(const float* data, float* result, size_t size, float alpha);

    /// Returns calculation with SIMD optimization
    static void returns(const double* prices, double* returns, size_t size);
    static void log_returns(const double* prices, double* returns, size_t size);

    /// Price change calculation
    static void price_change(const double* prices, double* changes, size_t size);
    static void price_change_percent(const double* prices, double* changes, size_t size);

    /// Bollinger Bands calculation
    static void bollinger_bands(const double* prices, double* upper, double* lower,
                               size_t size, size_t period, double std_multiplier = 2.0);

    /// RSI calculation
    static void rsi(const double* prices, double* rsi_values, size_t size, size_t period = 14);

    /// MACD calculation
    static void macd(const double* prices, double* macd_line, double* signal_line,
                    double* histogram, size_t size, size_t fast_period = 12,
                    size_t slow_period = 26, size_t signal_period = 9);

private:
    /// Internal SIMD implementation helpers
    static constexpr size_t get_simd_stride() {
#ifdef QAULTRA_ENABLE_SIMD
#ifdef __AVX512F__
        return 8; // 512 bits / 64 bits = 8 doubles
#elif __AVX2__
        return 4; // 256 bits / 64 bits = 4 doubles
#elif __SSE4_2__
        return 2; // 128 bits / 64 bits = 2 doubles
#else
        return 1;
#endif
#else
        return 1;
#endif
    }

    static constexpr size_t get_simd_stride_f32() {
#ifdef QAULTRA_ENABLE_SIMD
#ifdef __AVX512F__
        return 16; // 512 bits / 32 bits = 16 floats
#elif __AVX2__
        return 8;  // 256 bits / 32 bits = 8 floats
#elif __SSE4_2__
        return 4;  // 128 bits / 32 bits = 4 floats
#else
        return 1;
#endif
#else
        return 1;
#endif
    }

    /// Fallback implementations for non-SIMD systems
    static void add_fallback(const double* a, const double* b, double* result, size_t size);
    static void multiply_fallback(const double* a, const double* b, double* result, size_t size);
    static double dot_product_fallback(const double* a, const double* b, size_t size);
    static double sum_fallback(const double* data, size_t size);
};

/// High-performance financial calculation utilities
class FinancialMath {
public:
    /// Calculate portfolio value with SIMD optimization
    static double portfolio_value(const double* prices, const double* quantities, size_t size);

    /// Calculate position values
    static void position_values(const double* prices, const double* quantities,
                               double* values, size_t size);

    /// Calculate portfolio weights
    static void portfolio_weights(const double* values, double* weights, size_t size);

    /// Calculate portfolio returns
    static double portfolio_return(const double* weights, const double* returns, size_t size);

    /// Calculate portfolio variance (risk)
    static double portfolio_variance(const double* weights, const double* covariance_matrix, size_t size);

    /// Calculate Sharpe ratio
    static double sharpe_ratio(const double* returns, size_t size, double risk_free_rate = 0.0);

    /// Calculate maximum drawdown
    static double max_drawdown(const double* cumulative_returns, size_t size);

    /// Calculate Value at Risk (VaR)
    static double value_at_risk(const double* returns, size_t size, double confidence = 0.95);

    /// Calculate Expected Shortfall (Conditional VaR)
    static double expected_shortfall(const double* returns, size_t size, double confidence = 0.95);

    /// Calculate beta coefficient
    static double beta(const double* asset_returns, const double* market_returns, size_t size);

    /// Calculate correlation coefficient
    static double correlation(const double* x, const double* y, size_t size);

    /// Calculate information ratio
    static double information_ratio(const double* portfolio_returns,
                                  const double* benchmark_returns, size_t size);

    /// Calculate Calmar ratio
    static double calmar_ratio(const double* returns, size_t size);

    /// Calculate Sortino ratio
    static double sortino_ratio(const double* returns, size_t size, double target_return = 0.0);
};

/// Memory-mapped file for zero-copy data access
template<typename T>
class MemoryMappedArray {
private:
    void* mapped_memory_ = nullptr;
    size_t size_ = 0;
    size_t capacity_ = 0;
    int fd_ = -1;

public:
    MemoryMappedArray() = default;

    /// Create memory-mapped array from file
    explicit MemoryMappedArray(const std::string& filename);

    /// Create memory-mapped array in memory
    explicit MemoryMappedArray(size_t size);

    ~MemoryMappedArray();

    // Non-copyable but movable
    MemoryMappedArray(const MemoryMappedArray&) = delete;
    MemoryMappedArray& operator=(const MemoryMappedArray&) = delete;

    MemoryMappedArray(MemoryMappedArray&& other) noexcept;
    MemoryMappedArray& operator=(MemoryMappedArray&& other) noexcept;

    /// Access elements
    T& operator[](size_t index) noexcept { return static_cast<T*>(mapped_memory_)[index]; }
    const T& operator[](size_t index) const noexcept { return static_cast<const T*>(mapped_memory_)[index]; }

    /// Get raw pointer
    T* data() noexcept { return static_cast<T*>(mapped_memory_); }
    const T* data() const noexcept { return static_cast<const T*>(mapped_memory_); }

    /// Size operations
    size_t size() const noexcept { return size_; }
    size_t capacity() const noexcept { return capacity_; }
    bool empty() const noexcept { return size_ == 0; }

    /// Resize (only works for in-memory arrays)
    void resize(size_t new_size);

    /// Flush changes to disk
    void flush();

    /// Iterator support
    T* begin() noexcept { return data(); }
    T* end() noexcept { return data() + size_; }
    const T* begin() const noexcept { return data(); }
    const T* end() const noexcept { return data() + size_; }

private:
    void unmap();
};

/// Lock-free ring buffer for high-frequency data
template<typename T, size_t Size>
class LockFreeRingBuffer {
    static_assert((Size & (Size - 1)) == 0, "Size must be power of 2");

private:
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};
    alignas(64) std::array<T, Size> buffer_;

public:
    bool push(const T& item) noexcept {
        const auto current_tail = tail_.load(std::memory_order_relaxed);
        const auto next_tail = (current_tail + 1) % Size;

        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false; // Buffer full
        }

        buffer_[current_tail] = item;
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    bool pop(T& item) noexcept {
        const auto current_head = head_.load(std::memory_order_relaxed);

        if (current_head == tail_.load(std::memory_order_acquire)) {
            return false; // Buffer empty
        }

        item = buffer_[current_head];
        head_.store((current_head + 1) % Size, std::memory_order_release);
        return true;
    }

    size_t size() const noexcept {
        return (tail_.load(std::memory_order_acquire) - head_.load(std::memory_order_acquire)) % Size;
    }

    bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }

    bool full() const noexcept {
        const auto current_tail = tail_.load(std::memory_order_acquire);
        const auto next_tail = (current_tail + 1) % Size;
        return next_tail == head_.load(std::memory_order_acquire);
    }
};

} // namespace qaultra::simd