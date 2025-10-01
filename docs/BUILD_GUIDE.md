# QAULTRA C++ 编译指南

**版本**: 1.0.0
**最后更新**: 2025-10-01

## 目录

- [系统要求](#系统要求)
- [依赖安装](#依赖安装)
- [编译步骤](#编译步骤)
- [编译选项](#编译选项)
- [常见问题](#常见问题)
- [高级配置](#高级配置)

---

## 系统要求

### 操作系统

- **Linux**: Ubuntu 20.04+ / CentOS 8+ / Debian 11+
- **macOS**: 12.0+ (Monterey)
- **Windows**: Windows 10/11 (使用 MSVC 2019+ 或 WSL2)

### 编译器

| 编译器 | 最低版本 | 推荐版本 |
|--------|---------|---------|
| GCC | 9.0 | 11.0+ |
| Clang | 10.0 | 14.0+ |
| MSVC | 2019 (19.20) | 2022 |

**检查编译器版本**:
```bash
# GCC
g++ --version

# Clang
clang++ --version

# MSVC (Windows)
cl.exe
```

### 构建工具

- **CMake**: 3.16 或更高
- **Make** / **Ninja** (推荐)
- **Git**: 2.20+

---

## 依赖安装

### Ubuntu/Debian

```bash
# 基础依赖
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    ninja-build \
    pkg-config

# nlohmann_json (必需)
sudo apt-get install -y nlohmann-json3-dev

# Google Test (可选，用于测试)
sudo apt-get install -y libgtest-dev

# MongoDB C++ Driver (可选)
sudo apt-get install -y \
    libmongocxx-dev \
    libbsoncxx-dev

# Apache Arrow (可选)
sudo apt-get install -y \
    libarrow-dev \
    libparquet-dev
```

### CentOS/RHEL

```bash
# 启用 EPEL
sudo yum install -y epel-release

# 基础依赖
sudo yum install -y \
    gcc-c++ \
    cmake3 \
    git \
    ninja-build

# nlohmann_json
sudo yum install -y json-devel

# Google Test
sudo yum install -y gtest-devel
```

### macOS

```bash
# 使用 Homebrew
brew install cmake ninja nlohmann-json googletest

# 可选依赖
brew install mongo-cxx-driver apache-arrow
```

### IceOryx/iceoryx2 (可选 IPC 支持)

#### 安装 IceOryx (v1)

```bash
# 克隆仓库
git clone https://github.com/eclipse-iceoryx/iceoryx.git
cd iceoryx
git checkout v2.0.5

# 构建
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/home/quantaxis/iceoryx/build/install
make -j$(nproc)
make install
```

#### 安装 iceoryx2

```bash
# 克隆仓库
git clone https://github.com/eclipse-iceoryx/iceoryx2.git
cd iceoryx2

# 构建 Rust 核心
cargo build --release

# 构建 C++ 绑定
cd iceoryx2-cxx
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

---

## 编译步骤

### 基础编译（最小依赖）

```bash
# 1. 克隆项目
git clone https://github.com/quantaxis/qaultra-cpp.git
cd qaultra-cpp

# 2. 创建构建目录
mkdir build && cd build

# 3. 配置 CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DQAULTRA_BUILD_TESTS=ON

# 4. 编译
make -j$(nproc)

# 5. 运行测试
./progressive_test
./protocol_test
./unified_account_test
```

### 完整编译（所有功能）

```bash
# 配置 CMake（启用所有功能）
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DQAULTRA_BUILD_TESTS=ON \
    -DQAULTRA_BUILD_EXAMPLES=ON \
    -DQAULTRA_USE_MONGODB=ON \
    -DQAULTRA_USE_ARROW=ON \
    -DQAULTRA_USE_ICEORYX=ON \
    -DQAULTRA_USE_ICEORYX2=ON \
    -DQAULTRA_USE_FULL_FEATURES=ON \
    -GNinja

# 使用 Ninja 编译（更快）
ninja

# 运行所有测试
ninja test
```

### 使用 Ninja（推荐，速度更快）

```bash
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
ninja -j$(nproc)
```

---

## 编译选项

### CMake 选项详解

| 选项 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `CMAKE_BUILD_TYPE` | STRING | `Debug` | 构建类型 (Debug/Release/RelWithDebInfo) |
| `QAULTRA_BUILD_TESTS` | BOOL | `ON` | 构建测试程序 |
| `QAULTRA_BUILD_EXAMPLES` | BOOL | `OFF` | 构建示例程序 |
| `QAULTRA_BUILD_BENCHMARKS` | BOOL | `OFF` | 构建性能基准测试 |
| `QAULTRA_USE_ARROW` | BOOL | `OFF` | 启用 Apache Arrow 支持 |
| `QAULTRA_USE_MONGODB` | BOOL | `OFF` | 启用 MongoDB 连接器 |
| `QAULTRA_USE_ICEORYX` | BOOL | `ON` | 启用 IceOryx (v1) IPC |
| `QAULTRA_USE_ICEORYX2` | BOOL | `ON` | 启用 iceoryx2 IPC |
| `QAULTRA_USE_FULL_FEATURES` | BOOL | `OFF` | 启用所有完整功能 |

### 构建类型对比

| 构建类型 | 优化级别 | 调试信息 | 断言 | 适用场景 |
|---------|---------|---------|------|---------|
| `Debug` | `-O0` | 完整 | 开启 | 开发调试 |
| `Release` | `-O3` | 无 | 关闭 | 生产部署 |
| `RelWithDebInfo` | `-O2` | 部分 | 关闭 | 性能分析 |
| `MinSizeRel` | `-Os` | 无 | 关闭 | 嵌入式 |

### 自定义编译标志

```bash
# 启用 AVX2 指令集
cmake .. -DCMAKE_CXX_FLAGS="-mavx2"

# 启用原生 CPU 优化
cmake .. -DCMAKE_CXX_FLAGS="-march=native"

# 最大优化 + 原生 CPU
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-O3 -march=native -ffast-math"
```

---

## 常见问题

### 问题 1: 找不到 nlohmann_json

**错误信息**:
```
Could NOT find nlohmann_json (missing: nlohmann_json_DIR)
```

**解决方案**:
```bash
# Ubuntu/Debian
sudo apt-get install -y nlohmann-json3-dev

# 或者从源码安装
git clone https://github.com/nlohmann/json.git
cd json
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
sudo make install
```

---

### 问题 2: IceOryx 路径未找到

**错误信息**:
```
Could NOT find iceoryx_posh (missing: iceoryx_posh_DIR)
```

**解决方案**:

在 `CMakeLists.txt` 中指定正确路径：
```cmake
set(iceoryx_posh_DIR "/home/quantaxis/iceoryx/build/install/lib/cmake/iceoryx_posh")
set(iceoryx_hoofs_DIR "/home/quantaxis/iceoryx/build/install/lib/cmake/iceoryx_hoofs")
```

或使用 CMake 参数：
```bash
cmake .. \
    -Diceoryx_posh_DIR=/path/to/iceoryx/lib/cmake/iceoryx_posh \
    -Diceoryx_hoofs_DIR=/path/to/iceoryx/lib/cmake/iceoryx_hoofs
```

---

### 问题 3: MongoDB C++ Driver 未找到

**错误信息**:
```
Could NOT find mongocxx
```

**解决方案**:

手动安装 MongoDB C++ Driver：
```bash
# 安装 MongoDB C 驱动
sudo apt-get install -y libmongoc-dev

# 编译 C++ 驱动
git clone https://github.com/mongodb/mongo-cxx-driver.git \
    --branch releases/stable --depth 1
cd mongo-cxx-driver/build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local
sudo cmake --build . --target install
```

---

### 问题 4: C++17 编译错误

**错误信息**:
```
error: 'std::optional' has not been declared
```

**解决方案**:

确保使用 C++17 标准：
```bash
cmake .. -DCMAKE_CXX_STANDARD=17
```

或升级编译器到支持 C++17 的版本（GCC 9+）。

---

### 问题 5: 链接错误 (libiceoryx2_cxx.a)

**错误信息**:
```
cannot find -liceoryx2_cxx
```

**解决方案**:

确保 iceoryx2 已正确编译：
```bash
cd /home/quantaxis/iceoryx2/build
ls -la iceoryx2-cxx/libiceoryx2_cxx.a  # 确认文件存在
```

更新 `CMakeLists.txt` 中的路径：
```cmake
set(ICEORYX2_LIB_DIR "/path/to/iceoryx2/build/iceoryx2-cxx")
```

---

## 高级配置

### 交叉编译

#### ARM64 交叉编译

```bash
# 安装交叉编译工具链
sudo apt-get install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# 创建工具链文件 arm64-toolchain.cmake
cat > arm64-toolchain.cmake << 'EOF'
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
EOF

# 使用工具链编译
cmake .. -DCMAKE_TOOLCHAIN_FILE=../arm64-toolchain.cmake
make -j$(nproc)
```

---

### 静态链接

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++"
```

---

### 编译器缓存 (ccache)

```bash
# 安装 ccache
sudo apt-get install -y ccache

# 配置 CMake 使用 ccache
cmake .. \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DCMAKE_BUILD_TYPE=Release

# 查看缓存统计
ccache -s
```

---

### 分布式编译 (distcc)

```bash
# 安装 distcc
sudo apt-get install -y distcc

# 配置编译服务器
export DISTCC_HOSTS="localhost server1 server2"

# 使用 distcc 编译
cmake .. -DCMAKE_CXX_COMPILER=distcc
make -j$(distcc -j)
```

---

### 性能分析编译

```bash
# 启用性能分析信息
cmake .. \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_CXX_FLAGS="-pg"  # gprof 分析

# 或使用 perf
cmake .. \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_CXX_FLAGS="-fno-omit-frame-pointer"
```

---

### Docker 编译

```dockerfile
# Dockerfile
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    nlohmann-json3-dev \
    libgtest-dev

WORKDIR /workspace
COPY . .

RUN mkdir build && cd build && \
    cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release && \
    ninja

CMD ["./build/progressive_test"]
```

**构建 Docker 镜像**:
```bash
docker build -t qaultra-cpp:latest .
docker run --rm qaultra-cpp:latest
```

---

## 构建输出

### 编译产物

成功编译后，`build/` 目录包含：

```
build/
├── libqaultra.a                    # 静态库
├── progressive_test                 # 基础测试
├── protocol_test                    # 协议测试
├── unified_account_test             # 账户测试
├── batch_operations_test            # 批量操作测试 (需 GTest)
├── performance_analysis_test        # 性能测试
├── broadcast_basic_test             # IPC 基础测试 (需 iceoryx2)
└── cross_lang_cpp_publisher         # 跨语言示例 (需 Arrow)
```

### 运行测试

```bash
cd build

# 基础测试
./progressive_test
./protocol_test

# 账户测试
./unified_account_test

# 性能测试
./performance_analysis_test

# IPC 测试 (需要先启动 RouDi)
# 终端1: iox-roudi
# 终端2:
./broadcast_basic_test
```

---

## 安装

### 系统级安装

```bash
cd build
sudo make install

# 默认安装路径
# 头文件: /usr/local/include/qaultra/
# 库文件: /usr/local/lib/libqaultra.a
# CMake 配置: /usr/local/lib/cmake/qaultra/
```

### 自定义安装路径

```bash
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/qaultra
make install

# 安装到用户目录
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
make install
```

### 卸载

```bash
cd build
sudo make uninstall

# 或手动删除
sudo rm -rf /usr/local/include/qaultra
sudo rm -f /usr/local/lib/libqaultra.a
```

---

## 验证安装

### 编写测试程序

```cpp
// test_qaultra.cpp
#include <qaultra/market/market_system.hpp>
#include <iostream>

int main() {
    using namespace qaultra;

    auto market = std::make_shared<market::QAMarketSystem>();
    market->register_account("test_account", 1000000.0);

    std::cout << "QAULTRA C++ 安装成功！" << std::endl;
    return 0;
}
```

### 编译测试

```bash
# 使用 pkg-config
g++ -std=c++17 test_qaultra.cpp -o test_qaultra \
    $(pkg-config --cflags --libs qaultra)

# 手动指定路径
g++ -std=c++17 test_qaultra.cpp -o test_qaultra \
    -I/usr/local/include \
    -L/usr/local/lib \
    -lqaultra -lpthread

# 运行
./test_qaultra
```

---

## 持续集成 (CI)

### GitHub Actions 示例

```yaml
# .github/workflows/build.yml
name: Build and Test

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-22.04

    steps:
    - uses: actions/checkout@v3

    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y \
          build-essential cmake ninja-build \
          nlohmann-json3-dev libgtest-dev

    - name: Configure
      run: |
        mkdir build && cd build
        cmake .. -GNinja \
          -DCMAKE_BUILD_TYPE=Release \
          -DQAULTRA_BUILD_TESTS=ON

    - name: Build
      run: ninja -C build

    - name: Test
      run: |
        cd build
        ./progressive_test
        ./protocol_test
```

---

**维护者**: QUANTAXIS Team
**问题反馈**: [GitHub Issues](https://github.com/quantaxis/qaultra-cpp/issues)
