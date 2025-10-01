# QAULTRA C++ 贡献指南

欢迎为 QAULTRA C++ 贡献代码！本文档将指导您如何参与项目开发。

## 目录

- [行为准则](#行为准则)
- [开始之前](#开始之前)
- [贡献流程](#贡献流程)
- [代码规范](#代码规范)
- [提交规范](#提交规范)
- [测试要求](#测试要求)
- [文档编写](#文档编写)
- [问题反馈](#问题反馈)

---

## 行为准则

### 我们的承诺

为了营造开放和友好的环境，我们作为贡献者和维护者承诺：

- ✅ 尊重不同观点和经验
- ✅ 接受建设性批评
- ✅ 关注对社区最有利的事情
- ✅ 对其他社区成员表示同理心

### 不可接受的行为

- ❌ 使用性化的语言或图像，以及不受欢迎的性关注
- ❌ 挑衅、侮辱或贬损性评论，人身或政治攻击
- ❌ 公开或私下骚扰
- ❌ 未经明确许可发布他人的私人信息

---

## 开始之前

### 1. 了解项目架构

在贡献代码前，请：

1. **阅读核心文档**:
   - [README.md](README.md) - 项目概览
   - [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) - 架构设计
   - [docs/API_REFERENCE.md](docs/API_REFERENCE.md) - API 文档

2. **理解设计原则**:
   - **Rust 为核心**: C++ 实现完全对标 Rust 版本
   - **零冗余**: 避免创建简化版或重复功能
   - **高性能**: 零拷贝、SIMD 优化、无锁并发

3. **熟悉代码结构**:
```
qaultra-cpp/
├── include/qaultra/      # 头文件
│   ├── account/          # 账户系统
│   ├── market/           # 市场系统
│   ├── data/             # 数据类型
│   ├── protocol/         # 协议
│   └── ipc/              # IPC 通信
├── src/                  # 实现文件
├── tests/                # 测试代码
└── examples/             # 示例代码
```

### 2. 设置开发环境

```bash
# 1. Fork 项目
# 在 GitHub 上点击 "Fork" 按钮

# 2. 克隆你的 Fork
git clone https://github.com/YOUR_USERNAME/qaultra-cpp.git
cd qaultra-cpp

# 3. 添加上游仓库
git remote add upstream https://github.com/quantaxis/qaultra-cpp.git

# 4. 安装依赖
sudo apt-get install -y \
    build-essential cmake ninja-build \
    nlohmann-json3-dev libgtest-dev

# 5. 编译项目
mkdir build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Debug -DQAULTRA_BUILD_TESTS=ON
ninja

# 6. 运行测试
./progressive_test
./protocol_test
```

---

## 贡献流程

### 1. 创建 Issue

在开始编码前，先创建一个 Issue 描述你的想法：

**Bug 报告模板**:
```markdown
**描述 Bug**
简明扼要地描述 Bug

**复现步骤**
1. 执行 '...'
2. 点击 '...'
3. 看到错误 '...'

**预期行为**
描述你期望发生什么

**实际行为**
描述实际发生了什么

**环境信息**
- OS: [例如 Ubuntu 22.04]
- 编译器: [例如 GCC 11.2]
- 版本: [例如 1.0.0]

**额外信息**
添加任何其他相关信息
```

**功能请求模板**:
```markdown
**功能描述**
简明扼要地描述新功能

**动机**
为什么需要这个功能？解决什么问题？

**实现思路**
你认为应该如何实现？

**替代方案**
是否考虑过其他方案？

**Rust 对齐**
Rust 版本是否有对应功能？如何对齐？
```

### 2. 创建分支

```bash
# 更新主分支
git checkout master
git pull upstream master

# 创建功能分支
git checkout -b feature/your-feature-name

# 或修复分支
git checkout -b fix/bug-description
```

**分支命名规范**:
- `feature/xxx`: 新功能
- `fix/xxx`: Bug 修复
- `docs/xxx`: 文档更新
- `refactor/xxx`: 代码重构
- `perf/xxx`: 性能优化

### 3. 编写代码

#### 代码风格

**C++ 代码规范** (遵循 Google C++ Style Guide):

```cpp
// ✅ 正确示例

// 命名空间
namespace qaultra::market {

// 类名使用 PascalCase
class QAMarketSystem {
private:
    // 私有成员使用 snake_case + 下划线后缀
    std::string account_cookie_;
    double init_cash_;

public:
    // 方法使用 snake_case
    void register_account(const std::string& name, double cash);

    // 常量引用传递大对象
    const std::unordered_map<std::string, QA_Position>& get_positions() const;
};

// 自由函数使用 snake_case
double calculate_sharpe_ratio(const std::vector<double>& returns);

}  // namespace qaultra::market
```

**❌ 避免的做法**:
```cpp
// 不要使用驼峰命名成员变量
class BadExample {
    std::string accountCookie;  // ❌
    double InitCash;            // ❌
};

// 不要按值传递大对象
void bad_function(std::vector<StockCnDay> bars);  // ❌
void good_function(const std::vector<StockCnDay>& bars);  // ✅

// 不要省略命名空间
using namespace std;  // ❌
```

#### 头文件组织

```cpp
// my_class.hpp
#pragma once  // 使用 pragma once 而非 include guard

#include <string>
#include <vector>
#include <memory>

#include <nlohmann/json.hpp>  // 第三方库

#include "qaultra/data/datatype.hpp"  // 项目头文件

namespace qaultra::account {

class MyClass {
    // ... 声明
};

}  // namespace qaultra::account
```

#### 错误处理

```cpp
// ✅ 使用标准异常
void register_account(const std::string& name) {
    if (accounts_.find(name) != accounts_.end()) {
        throw std::runtime_error("Account already exists: " + name);
    }
}

// ✅ 使用 std::optional 表示可能不存在的值
std::optional<QA_Position> get_position(const std::string& code) const {
    auto it = positions_.find(code);
    if (it == positions_.end()) {
        return std::nullopt;
    }
    return it->second;
}
```

#### 性能优化

```cpp
// ✅ 预分配容器
std::vector<StockCnDay> bars;
bars.reserve(10000);  // 如果知道大小

// ✅ 使用移动语义
std::vector<Order> create_orders() {
    std::vector<Order> orders;
    // ...
    return orders;  // 自动移动，无拷贝
}

// ✅ 避免不必要的拷贝
for (const auto& [code, position] : positions_) {  // ✅ 引用
    // ...
}

for (auto [code, position] : positions_) {  // ❌ 拷贝
    // ...
}
```

### 4. 编写测试

**每个新功能必须包含测试**。

**单元测试示例**:
```cpp
// tests/test_my_feature.cpp
#include <gtest/gtest.h>
#include "qaultra/market/market_system.hpp"

namespace qaultra::market::test {

class MarketSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        market_ = std::make_shared<QAMarketSystem>();
    }

    std::shared_ptr<QAMarketSystem> market_;
};

TEST_F(MarketSystemTest, RegisterAccount) {
    market_->register_account("test_acc", 1000000.0);

    auto account = market_->get_account("test_acc");
    ASSERT_NE(account, nullptr);
    EXPECT_EQ(account->get_cash(), 1000000.0);
}

TEST_F(MarketSystemTest, DuplicateAccountThrows) {
    market_->register_account("test_acc", 1000000.0);

    EXPECT_THROW(
        market_->register_account("test_acc", 500000.0),
        std::runtime_error
    );
}

}  // namespace qaultra::market::test
```

**集成测试示例**:
```cpp
// tests/test_integration.cpp
TEST(IntegrationTest, FullTradingWorkflow) {
    auto market = std::make_shared<market::QAMarketSystem>();
    market->register_account("acc_001", 1000000.0);

    auto account = market->get_account("acc_001");

    // 1. 买入
    account->buy("000001.XSHE", 100, 10.0);

    // 2. 验证持仓
    auto positions = account->get_positions();
    ASSERT_EQ(positions.size(), 1);
    EXPECT_EQ(positions["000001.XSHE"].volume, 100);

    // 3. 卖出
    account->sell("000001.XSHE", 50, 10.5);

    // 4. 验证剩余持仓
    positions = account->get_positions();
    EXPECT_EQ(positions["000001.XSHE"].volume, 50);

    // 5. 验证 QIFI
    auto qifi = account->get_qifi();
    EXPECT_GT(qifi.account.balance, 1000000.0);  // 有盈利
}
```

**运行测试**:
```bash
cd build
ninja

# 运行所有测试
./progressive_test
./protocol_test
./unified_account_test

# 使用 GTest 过滤
./qaultra_tests --gtest_filter=MarketSystemTest.*
```

### 5. 提交更改

#### Commit 规范

遵循 [Conventional Commits](https://www.conventionalcommits.org/zh-hans/) 规范：

**格式**:
```
<type>(<scope>): <subject>

<body>

<footer>
```

**类型 (type)**:
- `feat`: 新功能
- `fix`: Bug 修复
- `docs`: 文档更新
- `style`: 代码格式（不影响功能）
- `refactor`: 重构
- `perf`: 性能优化
- `test`: 测试相关
- `chore`: 构建/工具更改

**示例**:
```bash
# 新功能
git commit -m "feat(market): add QAMarketSystem aligned with Rust QAMarket

- Implement account registration and management
- Add order scheduling queue
- Support QIFI snapshot management

Closes #123"

# Bug 修复
git commit -m "fix(data): resolve C++17 compatibility issue in datatype.cpp

Replace std::chrono::year_month_day with custom Date struct
for C++17 compatibility.

Fixes #456"

# 文档更新
git commit -m "docs: update README with latest architecture changes"
```

### 6. 推送并创建 PR

```bash
# 推送分支
git push origin feature/your-feature-name

# 在 GitHub 上创建 Pull Request
```

**PR 描述模板**:
```markdown
## 描述
简要描述本次 PR 的目的

## 变更类型
- [ ] Bug 修复
- [ ] 新功能
- [ ] 重构
- [ ] 性能优化
- [ ] 文档更新

## 变更内容
- 添加了 XXX 功能
- 修复了 YYY 问题
- 重构了 ZZZ 模块

## Rust 对齐验证
- [ ] 已验证与 Rust 版本对齐
- [ ] 已更新 ARCHITECTURE.md 对齐表
- [ ] API 命名与 Rust 保持一致

## 测试
- [ ] 添加了单元测试
- [ ] 添加了集成测试
- [ ] 所有测试通过
- [ ] 性能测试通过（如适用）

## 文档
- [ ] 更新了 API 文档
- [ ] 更新了示例代码
- [ ] 更新了 CHANGELOG.md

## 检查清单
- [ ] 代码遵循项目规范
- [ ] 通过了所有 CI 检查
- [ ] 无编译警告
- [ ] 代码覆盖率未降低

## 相关 Issue
Closes #XXX
Fixes #YYY

## 截图（如适用）

## 额外说明
```

---

## 代码审查

### 审查者指南

作为审查者，请关注：

1. **架构对齐**: 是否与 Rust 版本保持一致？
2. **代码质量**: 是否遵循编码规范？
3. **性能**: 是否有性能问题？
4. **测试覆盖**: 测试是否充分？
5. **文档完整性**: 文档是否更新？

**审查评论示例**:
```markdown
# ✅ 批准
LGTM! 代码质量很高，测试覆盖充分。

建议：可以考虑在 `get_positions()` 中使用 `const&` 返回以避免拷贝。

# 🔄 请求修改
1. 请添加单元测试覆盖新增的 `calculate_sharpe_ratio()` 方法
2. `market_system.hpp:123` 处的命名与 Rust 不一致，建议改为 `schedule_order`
3. 请更新 `docs/API_REFERENCE.md` 添加新 API 说明

# 💬 评论
这个实现很有创意！不过请确认一下 Rust 版本是否也采用了类似的方法？
我们需要保持架构对齐。
```

### 被审查者响应

```markdown
感谢审查！已根据反馈进行修改：

✅ 1. 已添加 `test_calculate_sharpe_ratio` 单元测试
✅ 2. 已将 `scheduleOrder` 改名为 `schedule_order`，与 Rust 保持一致
✅ 3. 已更新 API 文档

关于性能问题，我做了基准测试，耗时从 500ms 降至 50ms。

请再次审查。
```

---

## 测试要求

### 测试覆盖率

- **最低覆盖率**: 80%
- **核心模块覆盖率**: 90%+

**检查覆盖率**:
```bash
# 使用 gcov/lcov
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCOVERAGE=ON
ninja
ninja coverage

# 查看报告
open coverage/index.html
```

### 性能测试

对于性能相关的 PR，需要提供基准测试结果：

```cpp
// benchmark/benchmark_my_feature.cpp
#include <benchmark/benchmark.h>
#include "qaultra/market/market_system.hpp"

static void BM_RegisterAccount(benchmark::State& state) {
    auto market = std::make_shared<qaultra::market::QAMarketSystem>();

    for (auto _ : state) {
        market->register_account("test_" + std::to_string(state.iterations()),
                                1000000.0);
    }
}
BENCHMARK(BM_RegisterAccount);

BENCHMARK_MAIN();
```

**运行基准测试**:
```bash
./benchmark_my_feature --benchmark_format=json > results.json
```

---

## 文档编写

### 更新 API 文档

新增 API 必须更新 `docs/API_REFERENCE.md`：

```markdown
### my_new_function

```cpp
void my_new_function(const std::string& param);
```

描述函数功能。

**参数**:
- `param`: 参数说明

**返回值**:
- 返回值说明

**异常**:
- `std::runtime_error`: 异常说明

**示例**:
```cpp
my_new_function("test");
```
```

### 更新 CHANGELOG

在 `CHANGELOG.md` 中添加条目：

```markdown
## [Unreleased]

### Added
- feat(market): add `my_new_function` for XXX (#PR_NUMBER)
```

---

## 问题反馈

### 报告 Bug

1. 搜索现有 Issue，避免重复
2. 使用 Bug 报告模板
3. 提供完整的复现步骤
4. 附上环境信息

### 功能请求

1. 描述清楚功能需求
2. 说明使用场景
3. 确认 Rust 版本是否有对应功能
4. 讨论实现方案

### 提问

- **GitHub Discussions**: 一般性问题、讨论
- **GitHub Issues**: Bug 报告、功能请求
- **邮件**: quantaxis@qq.com

---

## 许可证

通过贡献代码，您同意您的贡献将在 [MIT License](LICENSE) 下授权。

---

## 致谢

感谢所有贡献者！您的贡献让 QAULTRA C++ 变得更好。

### 核心贡献者
- @yutiansut - 项目创始人
- @quantaxis-team - 核心开发团队

### 所有贡献者
查看 [贡献者列表](https://github.com/quantaxis/qaultra-cpp/graphs/contributors)

---

**问题？** 访问 [GitHub Discussions](https://github.com/quantaxis/qaultra-cpp/discussions)
**反馈？** 创建 [Issue](https://github.com/quantaxis/qaultra-cpp/issues)

感谢您的贡献！🎉
