#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include "qaultra/simd/simd_math.hpp"
#include "qaultra/memory/object_pool.hpp"
#include "qaultra/threading/lockfree_queue.hpp"

namespace py = pybind11;
using namespace qaultra;

void bind_simd_types(py::module& m) {
    auto simd = m.def_submodule("simd", "SIMD-optimized mathematical operations");

    // SIMD Math Functions
    simd.def("vectorized_add",
        [](py::array_t<double> a, py::array_t<double> b) -> py::array_t<double> {
            if (a.size() != b.size()) {
                throw std::invalid_argument("Arrays must have the same size");
            }

            auto result = py::array_t<double>(a.size());
            auto result_ptr = static_cast<double*>(result.mutable_unchecked<1>().mutable_data(0));
            auto a_ptr = static_cast<const double*>(a.unchecked<1>().data(0));
            auto b_ptr = static_cast<const double*>(b.unchecked<1>().data(0));

            auto result_vec = simd::vectorized_add(a_ptr, b_ptr, a.size());
            std::copy(result_vec.begin(), result_vec.end(), result_ptr);

            return result;
        },
        "Vectorized addition of two arrays",
        py::arg("a"), py::arg("b"));

    simd.def("vectorized_multiply",
        [](py::array_t<double> a, py::array_t<double> b) -> py::array_t<double> {
            if (a.size() != b.size()) {
                throw std::invalid_argument("Arrays must have the same size");
            }

            auto result = py::array_t<double>(a.size());
            auto result_ptr = static_cast<double*>(result.mutable_unchecked<1>().mutable_data(0));
            auto a_ptr = static_cast<const double*>(a.unchecked<1>().data(0));
            auto b_ptr = static_cast<const double*>(b.unchecked<1>().data(0));

            auto result_vec = simd::vectorized_multiply(a_ptr, b_ptr, a.size());
            std::copy(result_vec.begin(), result_vec.end(), result_ptr);

            return result;
        },
        "Vectorized multiplication of two arrays",
        py::arg("a"), py::arg("b"));

    simd.def("vectorized_multiply_scalar",
        [](py::array_t<double> array, double scalar) -> py::array_t<double> {
            auto result = py::array_t<double>(array.size());
            auto result_ptr = static_cast<double*>(result.mutable_unchecked<1>().mutable_data(0));
            auto array_ptr = static_cast<const double*>(array.unchecked<1>().data(0));

            auto result_vec = simd::vectorized_multiply_scalar(array_ptr, &scalar, array.size());
            std::copy(result_vec.begin(), result_vec.end(), result_ptr);

            return result;
        },
        "Vectorized scalar multiplication",
        py::arg("array"), py::arg("scalar"));

    simd.def("calculate_sma",
        [](py::array_t<double> prices, int window) -> py::array_t<double> {
            auto prices_ptr = static_cast<const double*>(prices.unchecked<1>().data(0));
            auto result_vec = simd::calculate_sma(prices_ptr, prices.size(), window);

            auto result = py::array_t<double>(result_vec.size());
            auto result_ptr = static_cast<double*>(result.mutable_unchecked<1>().mutable_data(0));
            std::copy(result_vec.begin(), result_vec.end(), result_ptr);

            return result;
        },
        "Calculate Simple Moving Average with SIMD optimization",
        py::arg("prices"), py::arg("window"));

    simd.def("calculate_ema",
        [](py::array_t<double> prices, double alpha) -> py::array_t<double> {
            auto prices_ptr = static_cast<const double*>(prices.unchecked<1>().data(0));
            auto result_vec = simd::calculate_ema(prices_ptr, prices.size(), alpha);

            auto result = py::array_t<double>(result_vec.size());
            auto result_ptr = static_cast<double*>(result.mutable_unchecked<1>().mutable_data(0));
            std::copy(result_vec.begin(), result_vec.end(), result_ptr);

            return result;
        },
        "Calculate Exponential Moving Average with SIMD optimization",
        py::arg("prices"), py::arg("alpha"));

    simd.def("calculate_returns",
        [](py::array_t<double> prices) -> py::array_t<double> {
            auto prices_ptr = static_cast<const double*>(prices.unchecked<1>().data(0));
            auto result_vec = simd::calculate_returns(prices_ptr, prices.size());

            auto result = py::array_t<double>(result_vec.size());
            auto result_ptr = static_cast<double*>(result.mutable_unchecked<1>().mutable_data(0));
            std::copy(result_vec.begin(), result_vec.end(), result_ptr);

            return result;
        },
        "Calculate returns with SIMD optimization",
        py::arg("prices"));

    simd.def("calculate_volatility",
        [](py::array_t<double> returns, int window) -> py::array_t<double> {
            auto returns_ptr = static_cast<const double*>(returns.unchecked<1>().data(0));
            auto result_vec = simd::calculate_volatility(returns_ptr, returns.size(), window);

            auto result = py::array_t<double>(result_vec.size());
            auto result_ptr = static_cast<double*>(result.mutable_unchecked<1>().mutable_data(0));
            std::copy(result_vec.begin(), result_vec.end(), result_ptr);

            return result;
        },
        "Calculate volatility with SIMD optimization",
        py::arg("returns"), py::arg("window"));

    // Financial calculations with SIMD
    auto financial = simd.def_submodule("financial", "SIMD-optimized financial calculations");

    financial.def("calculate_sharpe_ratio_simd",
        [](py::array_t<double> returns, double risk_free_rate) -> double {
            auto returns_ptr = static_cast<const double*>(returns.unchecked<1>().data(0));
            return simd::calculate_sharpe_ratio_simd(returns_ptr, returns.size(), risk_free_rate);
        },
        "Calculate Sharpe ratio with SIMD optimization",
        py::arg("returns"), py::arg("risk_free_rate") = 0.0);

    financial.def("calculate_portfolio_variance_simd",
        [](py::array_t<double> weights, py::array_t<double> covariance_matrix) -> double {
            if (weights.ndim() != 1) {
                throw std::invalid_argument("Weights must be 1D array");
            }
            if (covariance_matrix.ndim() != 2) {
                throw std::invalid_argument("Covariance matrix must be 2D array");
            }

            size_t n = weights.size();
            if (covariance_matrix.shape(0) != n || covariance_matrix.shape(1) != n) {
                throw std::invalid_argument("Covariance matrix dimensions must match weights size");
            }

            auto weights_ptr = static_cast<const double*>(weights.unchecked<1>().data(0));
            auto cov_ptr = static_cast<const double*>(covariance_matrix.unchecked<2>().data(0, 0));

            return simd::calculate_portfolio_variance_simd(weights_ptr, cov_ptr, n);
        },
        "Calculate portfolio variance with SIMD optimization",
        py::arg("weights"), py::arg("covariance_matrix"));

    financial.def("calculate_var_simd",
        [](py::array_t<double> returns, double confidence_level) -> double {
            auto returns_ptr = static_cast<const double*>(returns.unchecked<1>().data(0));
            return simd::calculate_var_simd(returns_ptr, returns.size(), confidence_level);
        },
        "Calculate Value at Risk with SIMD optimization",
        py::arg("returns"), py::arg("confidence_level") = 0.05);

    // Memory management utilities
    auto memory = simd.def_submodule("memory", "High-performance memory management");

    // Aligned allocator
    py::class_<memory::AlignedAllocator<double>>(memory, "AlignedDoubleAllocator")
        .def(py::init<size_t>(),
            "Create aligned allocator",
            py::arg("alignment") = 32)
        .def("allocate", [](memory::AlignedAllocator<double>& self, size_t n) {
            auto ptr = self.allocate(n);
            return py::array_t<double>(
                n,
                ptr,
                py::handle() // No base object - memory managed by allocator
            );
        }, "Allocate aligned memory",
           py::arg("size"),
           py::return_value_policy::reference_internal);

    // Object pool for frequent allocations
    py::class_<memory::ObjectPool<double>>(memory, "DoubleObjectPool")
        .def(py::init<size_t>(),
            "Create object pool",
            py::arg("initial_size"))
        .def("acquire", [](memory::ObjectPool<double>& self) {
            return self.acquire();
        }, "Acquire object from pool")
        .def("release", &memory::ObjectPool<double>::release,
            "Release object back to pool",
            py::arg("obj"))
        .def("size", &memory::ObjectPool<double>::size,
            "Get current pool size")
        .def("capacity", &memory::ObjectPool<double>::capacity,
            "Get pool capacity");

    // Memory-mapped array for zero-copy operations
    py::class_<memory::MemoryMappedArray<double>>(memory, "MemoryMappedDoubleArray")
        .def(py::init<const std::string&, size_t>(),
            "Create memory-mapped array",
            py::arg("filename"), py::arg("size"))
        .def("__getitem__", [](const memory::MemoryMappedArray<double>& self, size_t index) {
            if (index >= self.size()) {
                throw py::index_error("Index out of range");
            }
            return self[index];
        })
        .def("__setitem__", [](memory::MemoryMappedArray<double>& self, size_t index, double value) {
            if (index >= self.size()) {
                throw py::index_error("Index out of range");
            }
            self[index] = value;
        })
        .def("size", &memory::MemoryMappedArray<double>::size)
        .def("as_array", [](memory::MemoryMappedArray<double>& self) {
            return py::array_t<double>(
                self.size(),
                self.data(),
                py::handle() // Memory-mapped, no ownership transfer
            );
        }, "Get as NumPy array (zero-copy)",
           py::return_value_policy::reference_internal)
        .def("sync", &memory::MemoryMappedArray<double>::sync,
            "Synchronize changes to disk");

    // Threading utilities
    auto threading = simd.def_submodule("threading", "Lock-free threading utilities");

    // Lock-free queue
    py::class_<threading::LockFreeQueue<double>>(threading, "LockFreeDoubleQueue")
        .def(py::init<size_t>(),
            "Create lock-free queue",
            py::arg("capacity"))
        .def("enqueue", &threading::LockFreeQueue<double>::enqueue,
            "Add element to queue",
            py::arg("item"))
        .def("dequeue", [](threading::LockFreeQueue<double>& self) -> py::object {
            double item;
            if (self.dequeue(item)) {
                return py::cast(item);
            }
            return py::none();
        }, "Remove element from queue, returns None if empty")
        .def("size", &threading::LockFreeQueue<double>::size,
            "Get current queue size")
        .def("is_empty", &threading::LockFreeQueue<double>::is_empty,
            "Check if queue is empty")
        .def("capacity", &threading::LockFreeQueue<double>::capacity,
            "Get queue capacity");

    // Lock-free ring buffer
    py::class_<threading::LockFreeRingBuffer<double>>(threading, "LockFreeDoubleRingBuffer")
        .def(py::init<size_t>(),
            "Create lock-free ring buffer",
            py::arg("capacity"))
        .def("push", &threading::LockFreeRingBuffer<double>::push,
            "Push element to buffer",
            py::arg("item"))
        .def("pop", [](threading::LockFreeRingBuffer<double>& self) -> py::object {
            double item;
            if (self.pop(item)) {
                return py::cast(item);
            }
            return py::none();
        }, "Pop element from buffer, returns None if empty")
        .def("size", &threading::LockFreeRingBuffer<double>::size,
            "Get current buffer size")
        .def("capacity", &threading::LockFreeRingBuffer<double>::capacity,
            "Get buffer capacity")
        .def("is_empty", &threading::LockFreeRingBuffer<double>::is_empty,
            "Check if buffer is empty")
        .def("is_full", &threading::LockFreeRingBuffer<double>::is_full,
            "Check if buffer is full");

    // Performance monitoring
    auto perf = simd.def_submodule("perf", "Performance monitoring utilities");

    perf.def("benchmark_function", [](py::function func, py::args args, py::kwargs kwargs, int iterations) {
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            func(*args, **kwargs);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

        py::dict result;
        result["total_time_ns"] = duration.count();
        result["avg_time_ns"] = duration.count() / iterations;
        result["iterations"] = iterations;
        result["ops_per_second"] = iterations / (duration.count() / 1e9);

        return result;
    }, "Benchmark function performance",
       py::arg("func"), py::arg("args"), py::arg("kwargs"), py::arg("iterations") = 1000);

    perf.def("compare_implementations", [](py::list functions, py::args args, py::kwargs kwargs, int iterations) {
        py::list results;

        for (auto func_item : functions) {
            py::function func = func_item.cast<py::function>();
            auto start = std::chrono::high_resolution_clock::now();

            for (int i = 0; i < iterations; ++i) {
                func(*args, **kwargs);
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

            py::dict result;
            result["function_name"] = py::str(func.attr("__name__"));
            result["total_time_ns"] = duration.count();
            result["avg_time_ns"] = duration.count() / iterations;
            result["ops_per_second"] = iterations / (duration.count() / 1e9);

            results.append(result);
        }

        return results;
    }, "Compare performance of multiple implementations",
       py::arg("functions"), py::arg("args"), py::arg("kwargs"), py::arg("iterations") = 1000);

    // SIMD capability detection
    simd.def("get_simd_capabilities", []() {
        py::dict caps;
        caps["sse"] = simd::has_sse();
        caps["sse2"] = simd::has_sse2();
        caps["sse3"] = simd::has_sse3();
        caps["sse4_1"] = simd::has_sse4_1();
        caps["sse4_2"] = simd::has_sse4_2();
        caps["avx"] = simd::has_avx();
        caps["avx2"] = simd::has_avx2();
        caps["avx512"] = simd::has_avx512();
        return caps;
    }, "Get available SIMD capabilities");

    simd.def("get_optimal_batch_size", []() {
        return simd::get_optimal_batch_size();
    }, "Get optimal batch size for SIMD operations");

    // Utility functions for array operations
    simd.def("ensure_aligned", [](py::array_t<double> array, size_t alignment) {
        if (reinterpret_cast<uintptr_t>(array.data()) % alignment == 0) {
            return array; // Already aligned
        }

        // Create aligned copy
        auto aligned = py::array_t<double>(
            array.size(),
            memory::AlignedAllocator<double>(alignment).allocate(array.size())
        );

        auto src = static_cast<const double*>(array.unchecked<1>().data(0));
        auto dst = static_cast<double*>(aligned.mutable_unchecked<1>().mutable_data(0));
        std::copy(src, src + array.size(), dst);

        return aligned;
    }, "Ensure array is SIMD-aligned",
       py::arg("array"), py::arg("alignment") = 32);

    simd.def("prefetch_data", [](py::array_t<double> array, int hint) {
        auto ptr = static_cast<const double*>(array.unchecked<1>().data(0));
        simd::prefetch_data(ptr, array.size() * sizeof(double), hint);
    }, "Prefetch array data into cache",
       py::arg("array"), py::arg("hint") = 3); // 3 = prefetch to all cache levels
}