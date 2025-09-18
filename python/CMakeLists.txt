# Python bindings CMakeLists.txt

# Find Python components
find_package(Python COMPONENTS Interpreter Development REQUIRED)

# Create Python module
pybind11_add_module(qaultra_cpp
    qaultra_module.cpp
    data_bindings.cpp
    account_bindings.cpp
    market_bindings.cpp
    protocol_bindings.cpp
    engine_bindings.cpp
    simd_bindings.cpp
)

# Link against main library
target_link_libraries(qaultra_cpp PRIVATE qaultra)

# Set target properties
set_target_properties(qaultra_cpp PROPERTIES
    CXX_VISIBILITY_PRESET "hidden"
    INTERPROCEDURAL_OPTIMIZATION ON
)

# Compiler-specific optimizations
target_compile_definitions(qaultra_cpp PRIVATE VERSION_INFO=${PROJECT_VERSION})

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(qaultra_cpp PRIVATE
        -ffast-math
        -funroll-loops
        -fno-exceptions  # pybind11 handles exceptions
    )
endif()

# Install Python module
install(TARGETS qaultra_cpp
    DESTINATION ${Python_SITEARCH}/qaultra
)