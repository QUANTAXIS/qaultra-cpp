# QAULTRA C++ 依赖项说明

**日期**: 2025-10-01

---

## 必需依赖

### 1. C++ 编译器
- **要求**: 支持 C++17 标准
- **推荐版本**:
  - GCC >= 9.0
  - Clang >= 10.0
  - MSVC >= 2019

### 2. CMake
- **最低版本**: 3.16
- **安装**:
  ```bash
  # Ubuntu/Debian
  sudo apt-get install cmake

  # macOS
  brew install cmake

  # Windows
  # 从 https://cmake.org/download/ 下载安装
  ```

### 3. nlohmann_json
- **用途**: JSON 序列化/反序列化
- **版本**: >= 3.0
- **安装**:
  ```bash
  # Ubuntu/Debian
  sudo apt-get install nlohmann-json3-dev

  # macOS
  brew install nlohmann-json

  # Windows (vcpkg)
  vcpkg install nlohmann-json:x64-windows
  ```

### 4. pthread (Linux/macOS)
- **用途**: 多线程支持
- **安装**: 通常系统自带

---

## 可选依赖

### 5. Google Test (可选)
- **用途**: 单元测试
- **版本**: >= 1.10
- **CMake 选项**: `QAULTRA_BUILD_TESTS=ON`
- **安装**:
  ```bash
  # Ubuntu/Debian
  sudo apt-get install libgtest-dev

  # macOS
  brew install googletest

  # Windows (vcpkg)
  vcpkg install gtest:x64-windows
  ```

### 6. Google Benchmark (可选)
- **用途**: 性能基准测试
- **CMake 选项**: `QAULTRA_BUILD_BENCHMARKS=ON`
- **安装**:
  ```bash
  # Ubuntu/Debian
  sudo apt-get install libbenchmark-dev

  # macOS
  brew install google-benchmark

  # Windows (vcpkg)
  vcpkg install benchmark:x64-windows
  ```

### 7. Apache Arrow (可选)
- **用途**: 高性能数据交换
- **CMake 选项**: `QAULTRA_USE_ARROW=ON`
- **安装**:
  ```bash
  # Ubuntu/Debian
  sudo apt-get install libarrow-dev

  # macOS
  brew install apache-arrow

  # Windows (vcpkg)
  vcpkg install arrow:x64-windows
  ```

### 8. MongoDB C++ Driver (可选)
- **用途**: MongoDB 数据库连接
- **CMake 选项**: `QAULTRA_USE_MONGODB=ON`
- **安装**:
  ```bash
  # Ubuntu/Debian
  # 需要从源码编译，见官方文档
  # https://www.mongodb.com/docs/languages/cpp/drivers/current/

  # macOS
  brew tap mongodb/brew
  brew install mongo-cxx-driver
  ```

### 9. IceOryx (可选)
- **用途**: 零拷贝进程间通信 (IPC)
- **版本**: v2.0+
- **CMake 选项**: `QAULTRA_USE_ICEORYX=ON`
- **安装**: 需要从源码编译
  ```bash
  git clone https://github.com/eclipse-iceoryx/iceoryx.git
  cd iceoryx
  mkdir build && cd build
  cmake .. -DCMAKE_BUILD_TYPE=Release
  make -j$(nproc)
  sudo make install
  ```
- **注意**: GitHub Actions 中默认禁用

### 10. iceoryx2 (可选)
- **用途**: 新一代零拷贝 IPC (支持 Rust/C++/Python)
- **CMake 选项**: `QAULTRA_USE_ICEORYX2=ON`
- **安装**: 需要从源码编译 (需要 Rust 工具链)
  ```bash
  # 安装 Rust
  curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

  # 编译 iceoryx2
  git clone https://github.com/eclipse-iceoryx/iceoryx2.git
  cd iceoryx2
  cargo build --release
  ```
- **注意**: GitHub Actions 中默认禁用

---

## CMake 配置选项

### 基本选项

| 选项 | 默认值 | 说明 |
|------|-------|------|
| `QAULTRA_BUILD_TESTS` | ON | 构建测试 |
| `QAULTRA_BUILD_EXAMPLES` | OFF | 构建示例 |
| `QAULTRA_BUILD_BENCHMARKS` | OFF | 构建性能测试 |

### 功能选项

| 选项 | 默认值 | 说明 |
|------|-------|------|
| `QAULTRA_USE_ARROW` | OFF | 启用 Apache Arrow 支持 |
| `QAULTRA_USE_MONGODB` | OFF | 启用 MongoDB 连接器 |
| `QAULTRA_USE_ICEORYX` | ON | 启用 IceOryx v1 |
| `QAULTRA_USE_ICEORYX2` | ON | 启用 iceoryx2 |
| `QAULTRA_USE_FULL_FEATURES` | OFF | 启用所有功能 |

---

## GitHub Actions 配置

在 GitHub Actions 中，为了保证 CI 稳定性，使用以下配置：

```yaml
cmake -B build \
  -DQAULTRA_BUILD_TESTS=OFF \
  -DQAULTRA_BUILD_EXAMPLES=OFF \
  -DQAULTRA_BUILD_BENCHMARKS=OFF \
  -DQAULTRA_USE_ARROW=OFF \
  -DQAULTRA_USE_MONGODB=OFF \
  -DQAULTRA_USE_ICEORYX=OFF \
  -DQAULTRA_USE_ICEORYX2=OFF \
  -DQAULTRA_USE_FULL_FEATURES=OFF
```

**原因**:
- IceOryx/iceoryx2 安装复杂，在 CI 中禁用
- MongoDB 需要额外配置，在 CI 中禁用
- 测试当前还在修复中，暂时禁用
- 只构建核心库 `libqaultra.a`

---

## 本地开发环境配置

### 最小配置 (核心库)

```bash
sudo apt-get install -y \
  build-essential \
  cmake \
  nlohmann-json3-dev

cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DQAULTRA_USE_ICEORYX=OFF \
  -DQAULTRA_USE_ICEORYX2=OFF
make -j$(nproc)
```

### 完整配置 (所有功能)

```bash
# 安装基础依赖
sudo apt-get install -y \
  build-essential \
  cmake \
  nlohmann-json3-dev \
  libgtest-dev \
  libbenchmark-dev \
  libarrow-dev

# 编译 IceOryx (可选)
# 编译 iceoryx2 (可选)
# 安装 MongoDB C++ driver (可选)

cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DQAULTRA_BUILD_TESTS=ON \
  -DQAULTRA_BUILD_EXAMPLES=ON \
  -DQAULTRA_USE_ARROW=ON \
  -DQAULTRA_USE_FULL_FEATURES=ON
make -j$(nproc)
```

---

## 依赖项版本兼容性

| 依赖项 | 最低版本 | 推荐版本 | 最新测试版本 |
|--------|---------|---------|------------|
| GCC | 9.0 | 11.0 | 13.0 |
| Clang | 10.0 | 14.0 | 17.0 |
| CMake | 3.16 | 3.22 | 3.28 |
| nlohmann_json | 3.0 | 3.10 | 3.11 |
| Google Test | 1.10 | 1.12 | 1.14 |
| Apache Arrow | 8.0 | 12.0 | 14.0 |
| IceOryx | 2.0 | 2.0 | 2.0 |
| iceoryx2 | 0.1 | 0.3 | 0.3 |

---

## 故障排查

### 问题 1: 找不到 nlohmann_json

**错误**:
```
Could NOT find nlohmann_json
```

**解决**:
```bash
# Ubuntu/Debian
sudo apt-get install nlohmann-json3-dev

# 或者从源码安装
git clone https://github.com/nlohmann/json.git
cd json
mkdir build && cd build
cmake .. -DJSON_BuildTests=OFF
sudo make install
```

### 问题 2: IceOryx 路径错误

**错误**:
```
IceOryx not found. Zero-copy IPC features will be disabled.
```

**解决**:
- 方案 1: 禁用 IceOryx
  ```bash
  cmake .. -DQAULTRA_USE_ICEORYX=OFF
  ```

- 方案 2: 编译并安装 IceOryx
  ```bash
  # 见上文 IceOryx 安装说明
  ```

### 问题 3: Windows 上 vcpkg 安装失败

**解决**:
```powershell
# 设置 vcpkg 工具链文件
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

---

## 总结

### GitHub Actions 所需依赖

**Ubuntu**:
- nlohmann-json3-dev (必需)
- libgtest-dev (可选，测试用)
- cmake, build-essential (基础)

**Windows**:
- nlohmann-json (通过 vcpkg)
- gtest (通过 vcpkg，可选)

### 本地开发所需依赖

**核心开发** (最小集):
- C++ 编译器 (C++17)
- CMake >= 3.16
- nlohmann_json

**完整开发**:
- 核心依赖 +
- Google Test (测试)
- Google Benchmark (性能测试)
- Apache Arrow (高性能数据)
- IceOryx/iceoryx2 (零拷贝 IPC)
- MongoDB C++ driver (数据库)

---

**参考文档**:
- nlohmann/json: https://github.com/nlohmann/json
- Google Test: https://github.com/google/googletest
- Apache Arrow: https://arrow.apache.org/
- IceOryx: https://github.com/eclipse-iceoryx/iceoryx
- iceoryx2: https://github.com/eclipse-iceoryx/iceoryx2
- MongoDB C++ Driver: https://www.mongodb.com/docs/languages/cpp/drivers/
