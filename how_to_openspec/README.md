# OpenSpec 工作流总结

## 目录结构
- `openspec/changes/create-basic-calculator/`：当前 change 的工作目录
- `openspec/changes/create-basic-calculator/proposal.md`：proposal 文档
- `openspec/changes/create-basic-calculator/design.md`：design 文档
- `openspec/changes/create-basic-calculator/specs/basic-calculator/spec.md`：spec 文档
- `openspec/changes/create-basic-calculator/tasks.md`：tasks 任务清单
- `src/calculator.cpp`：Calculator 实现
- `include/myproject/calculator.h`：Calculator 头文件
- `tests/unit/test_calculator.cpp`：单元测试文件
- `tests/CMakeLists.txt`：测试构建配置
- `CMakeLists.txt`：项目主构建配置

## 1. 环境准备

### 1.1 安装 OpenSpec CLI
命令：
```bash
./prepare/install_openspec.sh
```

原因：
- 安装 OpenSpec CLI，后续才能使用 `openspec new change`、`openspec status`、`openspec instructions` 等命令。
- 该脚本会把 OpenSpec 依赖安装到当前环境中。

效果：
- `openspec` 命令可用。

## 2. 创建 Change

### 2.1 新建变更
命令：
```bash
openspec new change "create-basic-calculator"
```

原因：
- 这个命令初始化一个新的 OpenSpec change，生成 `openspec/changes/create-basic-calculator/` 目录。
- OpenSpec 会根据默认 schema 创建工作流骨架。

效果：
- 生成 `openspec/changes/create-basic-calculator/.openspec.yaml`
- 初始化 artifact 结构：`proposal`、`design`、`specs`、`tasks`

## 3. 查看 Change 状态

### 3.1 检查当前 artifact 状态
命令：
```bash
openspec status --change "create-basic-calculator" --json
```

原因：
- 获取当前 change 的进度和依赖关系。
- 确认哪些 artifact 已准备好、哪些还被阻塞。

效果：
- 了解到 `proposal` 先写，之后才能生成 `design` 和 `specs`。

## 4. 生成 Proposal

### 4.1 获取 Proposal 指令
命令：
```bash
openspec instructions proposal --change "create-basic-calculator" --json
```

原因：
- 查看 OpenSpec 对 proposal 的模板和字段要求。
- 理解 proposal 需要包含的部分：`Why`、`What Changes`、`Capabilities`、`Impact`。

### 4.2 创建 `proposal.md`
文件内容说明：
- `Why`：说明为什么需要这次改动
- `What Changes`：列出代码和功能变化
- `Capabilities`：定义新能力 `basic-calculator`
- `Impact`：说明受影响代码和测试

效果：
- proposal 成为后续 `design`、`specs`、`tasks` 的基础。

## 5. 生成 Design

### 5.1 获取 Design 指令
命令：
```bash
openspec instructions design --change "create-basic-calculator" --json
```

原因：
- 查看 design 文档需要的结构和内容。
- 确保实现方案清晰且与 proposal 保持一致。

### 5.2 创建 `design.md`
文件内容说明：
- `Context`：当前项目结构和目标
- `Goals / Non-Goals`：实现范围与非范围
- `Decisions`：设计决策，例如静态方法、异常处理、double 类型、文件组织
- `Risks / Trade-offs`：风险与缓解措施

效果：
- 提前固化实现思路，减少编码阶段的歧义。

## 6. 生成 Specs

### 6.1 获取 Specs 指令
命令：
```bash
openspec instructions specs --change "create-basic-calculator" --json
```

原因：
- 确认 spec 文件格式要求和规范。
- 明确需求应该使用 `SHALL/MUST` 语义，并包含可测试场景。

### 6.2 创建 `specs/basic-calculator/spec.md`
文件内容说明：
- `ADDED Requirements`：新增功能需求
- 每个 `Requirement` 有至少一个 `Scenario`
- 使用 `#### Scenario:` 精确格式

效果：
- 为实现和测试提供明确的行为契约。

## 7. 生成 Tasks

### 7.1 获取 Tasks 指令
命令：
```bash
openspec instructions tasks --change "create-basic-calculator" --json
```

原因：
- 确认任务文件格式，OpenSpec 需要 `- [ ]` 格式以便跟踪。
- 将实现工作拆分为可执行的小任务。

### 7.2 创建 `tasks.md`
文件内容说明：
- 1. Create Calculator Module
- 2. Implement Arithmetic Operations
- 3. Integrate with Main Application
- 4. Add Unit Tests

效果：
- 形成明确的实现步骤，便于逐个完成并记录进度。

## 8. 实现与应用 Change

### 8.1 获取 apply 指令
命令：
```bash
openspec instructions apply --change "create-basic-calculator" --json
```

原因：
- 读取 apply 阶段的任务清单和当前状态。
- 获取当前 pending task 列表和动态指令。

### 8.2 读取上下文文件
需要读取的文件：
- `proposal.md`
- `design.md`
- `specs/**/*.md`
- `tasks.md`

原因：
- 通过上下文文件理解实现目标与需求。
- 避免直接编码时偏离 spec。

### 8.3 按任务逐项实现
已完成工作：
- `include/myproject/calculator.h`
- `src/calculator.cpp`
- 修改 `src/main.cpp` 引入 calculator 示例
- `tests/unit/test_calculator.cpp` 编写单元测试

任务状态更新：
- 每完成一项即修改 `tasks.md` 中对应 `- [ ]` 为 `- [x]`

原因：
- 确保 implementation 与 tasks 进度同步。
- OpenSpec apply 阶段依赖任务文件来追踪进度。

## 9. 编译与测试

### 9.1 修正 CMake 配置
修改内容：
- `CMakeLists.txt` 添加 `src/calculator.cpp` 到 `main` 可执行目标
- `tests/CMakeLists.txt` 将 `src/calculator.cpp` 也加入 `test_calculator` 可执行目标

原因：
- `test_calculator` 需要 calculator 的定义，否则会出现链接错误。
- 头文件声明和实现文件都必须被链接到目标。

### 9.2 编译命令
```bash
cmake -S . -B build
cmake --build /workspaces/cpp_on_code_space/build
```

### 9.3 运行主程序
```bash
./build/main
```

### 9.4 运行测试
```bash
./build/tests/test_calculator
```

效果：
- `main` 输出 calculator 演示结果
- 单元测试全部通过

## 10. 关键问题与原因说明

### 10.1 链接错误原因
问题：测试可执行文件能够编译，但链接时出现 `undefined reference`。
原因：
- `tests/CMakeLists.txt` 初始只编译了测试源码 `unit/test_calculator.cpp`
- 但 `Calculator` 的实现位于 `src/calculator.cpp`
- 结果测试目标没有链接到实现文件，导致链接失败

解决方案：
- 在 `tests/CMakeLists.txt` 中添加 `../src/calculator.cpp`
- 或者将 `calculator.cpp` 提取为库 target，主程序和测试都链接该库

## 11. 结果文件清单

- `openspec/changes/create-basic-calculator/proposal.md`
- `openspec/changes/create-basic-calculator/design.md`
- `openspec/changes/create-basic-calculator/specs/basic-calculator/spec.md`
- `openspec/changes/create-basic-calculator/tasks.md`
- `include/myproject/calculator.h`
- `src/calculator.cpp`
- `src/main.cpp`
- `tests/unit/test_calculator.cpp`
- `tests/CMakeLists.txt`
- `CMakeLists.txt`

## 12. 归档 Change

### 12.1 归档命令
命令：
```bash
openspec archive create-basic-calculator
```

原因：
- `openspec archive` 的命令格式是 `openspec archive [change-name]`，不支持 `--change` 选项。
- 直接给出 change 名称，OpenSpec 会把已完成 change 移到归档目录。

### 12.2 常用选项
- `-y` / `--yes`：跳过确认提示
- `--skip-specs`：跳过 main specs 的更新
- `--no-validate`：跳过验证（不推荐）

### 12.3 归档的作用和影响
- 把 `openspec/changes/create-basic-calculator/` 移动到 `openspec/changes/archive/YYYY-MM-DD-create-basic-calculator/`
- 该 change 不再作为活动 change 跟踪
- 保留变更历史和 `.openspec.yaml`
- 不会直接修改代码逻辑，只是工作流状态整理

### 12.4 归档前检查
- `openspec status --change "create-basic-calculator" --json`，确认 artifacts 已完成
- `openspec/changes/create-basic-calculator/tasks.md` 中任务已全部打勾
- 如果有 delta specs，先决定是否同步到主 spec

## 13. 延伸：在每个步骤中你还可以做什么

### 13.1 Proposal 阶段
- 修正需求范围：如果你发现目标不够清晰，可以重新定义 `Why` 和 `What Changes`，让后续设计更准确。
- 补充 Capability：如果发现需要更多用例，可以增加 `Capabilities`，并明确对应 `specs/<name>/spec.md`。
- 明确影响范围：把受影响文件、API、测试、依赖写清楚，避免实现时漏改。

### 13.2 Design 阶段
- 深化技术方案：把每个方案选项写出来，说明为什么选这个而不是另一个。
- 明确边界条件：列出除零、输入类型、错误处理等潜在问题。
- 记录开放问题：如果有不确定的实现细节，写进 `Open Questions`，后续再补。

### 13.3 Specs 阶段
- 补全场景：每条需求至少写一个 `Scenario`，最好覆盖正常路径和异常路径。
- 细化语义：使用 `SHALL/MUST` 来定义行为，避免 `should/may`。
- 对齐测试：把场景直接映射成测试用例，方便从 spec 到代码的转换。

### 13.4 Tasks 阶段
- 细分任务：如果任务太大，可以把它拆成更小的子任务，降低实现风险。
- 逐项完成：每做一项就打勾，保证工作进度可跟踪。
- 添加验证任务：把“运行测试”“检查输出”“修复构建问题”也写进任务列表。

### 13.5 Apply 阶段
- 读取上下文：仔细阅读 proposal/design/specs/tasks，避免偏离目标。
- 发现问题及时回写：如果实现过程中发现设计或需求不对，可以回到 `proposal`/`design`/`specs` 修改。
- 保持最小变更：每次改动都只做当前任务所需内容，降低回滚成本。

### 13.6 编译与测试阶段
- 检查构建依赖：确保新增文件被 CMake 或构建系统正确包含。
- 链接验证：如果出现 `undefined reference`，说明实现文件未链接到目标。
- 运行测试：单元测试不仅验证正确性，还能覆盖异常情况。

### 13.7 迭代改进
- 复盘流程：每完成一个 change 后，总结哪些步骤效率高、哪些步骤出错多。
- 规范文档：把常见问题和解决办法写入 `how_to_openspec/README.md`，积累团队经验。
- 扩展 schema：如果当前 change 需要更复杂流程，可以考虑定制 OpenSpec schema 或 artifact 结构。

---

此文档用于记录从 OpenSpec 变更初始化到代码实现、构建验证的完整工作流，包含每一步的原因与作用，以及每个步骤中可以进一步补充和改进的内容。