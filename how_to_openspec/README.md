# OpenSpec 工作流总结

## 目录结构
- `openspec/changes/<change-name>/`：当前 change 的工作目录（例如 `create-basic-logger`）
- `openspec/changes/<change-name>/proposal.md`：proposal 文档
- `openspec/changes/<change-name>/design.md`：design 文档
- `openspec/changes/<change-name>/tasks.md`：tasks 任务清单
- `openspec/changes/<change-name>/specs/<capability>/spec.md`：变更内规格（delta specs，用于本次 change）
- `openspec/specs/<capability>/spec.md`：主规格（main specs，系统单一事实来源）
- `openspec/changes/archive/YYYY-MM-DD-<change-name>/`：已归档的 change（只读记录）
- `src/`、`include/`：实现代码
- `tests/`：单元测试 / 集成测试
- `CMakeLists.txt`、`tests/CMakeLists.txt`：构建配置

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
openspec new change "<change-name>"
```

原因：
- 这个命令初始化一个新的 OpenSpec change，生成 `openspec/changes/<change-name>/` 目录。
- OpenSpec 会根据默认 schema 创建工作流骨架。

效果：
- 生成 `openspec/changes/<change-name>/.openspec.yaml`
- 初始化 artifact 结构：`proposal`、`design`、`specs`、`tasks`

`.openspec.yaml`（参考）：
```yaml
schema: spec-driven
created: YYYY-MM-DD
```

书写建议：
- `<change-name>` 用动词短语（例如 `add-xxx` / `fix-yyy`），避免含糊名称。
- `<capability>` 用稳定、可复用的名词短语（避免带版本号或临时前缀）。

## 3. 查看 Change 状态

### 3.1 检查当前 artifact 状态
命令：
```bash
openspec status --change "<change-name>" --json
```

原因：
- 获取当前 change 的进度和依赖关系。
- 确认哪些 artifact 已准备好、哪些还被阻塞。

效果：
- 了解到 `proposal` 先写，之后依次完成 `specs` → `design` → `tasks`。

## 4. 生成 Proposal

### 4.1 获取 Proposal 指令
命令：
```bash
openspec instructions proposal --change "<change-name>" --json
```

原因：
- 查看 OpenSpec 对 proposal 的模板和字段要求。
- 理解 proposal 需要包含的部分：`Why`、`What Changes`、`Capabilities`、`Impact`。

### 4.2 创建 `proposal.md`
文件内容说明：
- `Why`：说明为什么需要这次改动
- `What Changes`：列出代码和功能变化
- `Capabilities`：定义本次 change 涉及的能力（capability）
- `Impact`：说明受影响代码和测试

效果：
- proposal 成为后续 `design`、`specs`、`tasks` 的基础。

模板（精简）：
```md
# Proposal: <change-name>

## Why
一句话说明动机 + 业务/技术背景。

## What Changes
- 新增/修改的关键行为（面向用户/调用方）
- 主要代码改动点（面向实现）

## Capabilities
- <capability-1>
- <capability-2>

## Impact
- 影响的模块/接口/兼容性/迁移
- 测试与发布注意事项
```

书写建议：
- `Why` 先讲问题与价值，再讲方案。
- `What Changes` 写“会发生什么变化”，避免写成实现细节堆砌。
- `Impact` 明确破坏性变更、数据迁移、回滚策略（如有）。

> 说明：本文档只描述工作流。为避免示例与实际项目偏离，这里仅提供“可复用模板片段”；后续会用我们马上要做的新项目补充一个完整示例（当前先占位，不展开）。

## 5. 生成 Specs（先于 Design）

### 5.1 获取 Specs 指令
命令：
```bash
openspec instructions specs --change "<change-name>" --json
```

原因：
- 确认 spec 文件格式要求和规范。
- 明确需求应使用可验证的规范语言（例如 MUST/SHALL），并包含可测试场景。

### 5.2 在 change 内创建/更新规格（delta specs）
目标路径：
- `openspec/changes/<change-name>/specs/<capability>/spec.md`

效果：
- 规格先在 change 内沉淀，便于与 proposal / design / tasks 一起迭代。

模板（精简）：
```md
# Spec: <capability>

## Summary
一句话描述这个 capability 是什么。

## Requirements
### R1: <short-name>
The system MUST ...

#### Scenario: <happy-path>
Given ...
When ...
Then ...

#### Scenario: <edge-case>
Given ...
When ...
Then ...
```

书写建议：
- Requirement 用 MUST/SHALL + 可测的动词（返回/拒绝/记录/限制），避免“提升/优化/支持”等不可验证表述。
- Scenario 尽量让测试能直接映射：输入 → 行为 → 可观察输出（返回值/副作用/日志/错误码）。

## 6. 生成 Design

### 6.1 获取 Design 指令
命令：
```bash
openspec instructions design --change "<change-name>" --json
```

原因：
- 查看 design 文档需要的结构和内容。
- 确保实现方案清晰且与 proposal 保持一致。

### 6.2 创建 `design.md`
文件内容说明：
- `Context`：当前项目结构和目标
- `Goals / Non-Goals`：实现范围与非范围
- `Decisions`：设计决策，例如静态方法、异常处理、double 类型、文件组织
- `Risks / Trade-offs`：风险与缓解措施

效果：
- 提前固化实现思路，减少编码阶段的歧义。

模板（精简）：
```md
# Design: <change-name>

## Context
现状/约束/相关系统与依赖。

## Goals
- ...

## Non-Goals
- ...

## Approach
总体方案与模块划分。

## Decisions
- Decision: ...
  - Rationale: ...
  - Alternatives: ...

## Risks & Trade-offs
- Risk: ...
  - Mitigation: ...
```

书写建议：
- 把“为什么这么做”写清楚（rationale），比写“怎么做”更重要。
- 对外接口（API/CLI/配置）与错误处理策略要明确，便于 spec 与测试对齐。

## 7. 生成 Tasks

### 7.1 获取 Tasks 指令
命令：
```bash
openspec instructions tasks --change "<change-name>" --json
```

原因：
- 确认任务文件格式，OpenSpec 需要 `- [ ]` 格式以便跟踪。
- 将实现工作拆分为可执行的小任务。

### 7.2 创建 `tasks.md`
文件内容说明：
- 用 `- [ ]` 列出可独立交付的任务条目（实现、测试、文档、构建等）
- 每个任务尽量可在一次提交内完成

效果：
- 形成明确的实现步骤，便于逐个完成并记录进度。

模板（精简）：
```md
# Tasks: <change-name>

- [ ] Define public API / interfaces
- [ ] Implement core behavior
- [ ] Add tests for scenarios
- [ ] Wire build / CI if needed
- [ ] Update docs / examples
```

书写建议：
- 任务写成“动词开头 + 可验收结果”，例如 “Add tests for <scenario>”。
- 把“验证任务”也写进去（如 `ctest`、静态检查、手动验证步骤）。

## 8. 实现与应用 Change

### 8.1 获取 apply 指令
命令：
```bash
openspec instructions apply --change "<change-name>" --json
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
任务状态更新：
- 每完成一项即修改 `tasks.md` 中对应 `- [ ]` 为 `- [x]`

原因：
- 确保 implementation 与 tasks 进度同步。
- OpenSpec apply 阶段依赖任务文件来追踪进度。

## 9. 编译与测试

### 9.1 修正 CMake 配置
修改内容：
- 把本次 change 新增/修改的实现文件加入对应的 target（例如主程序、库、测试可执行文件）
- 如果测试需要复用实现代码，建议抽取为 library target，然后让主程序与测试都链接该库

原因：
- 仅包含头文件声明不会参与链接；参与链接的是实现文件/库 target。
- 测试编译通过但链接失败，通常意味着“测试 target 没有链接到实现”。

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
ctest --test-dir build --output-on-failure
```

效果：
- 构建成功
- 测试通过

## 10. 关键问题与原因说明

### 10.1 链接错误原因
问题：测试可执行文件能够编译，但链接时出现 `undefined reference`。
原因：
- 测试 target 只编译了测试源码，但没有链接到实现代码（实现文件未加入 target，或未链接到对应库）。

解决方案：
- 方案 A：把实现源文件加入测试 target 的 sources
- 方案 B（推荐）：抽取为 library target，并在主程序与测试中统一链接该库

## 11. 结果文件清单

- `openspec/changes/<change-name>/proposal.md`
- `openspec/changes/<change-name>/design.md`
- `openspec/changes/<change-name>/tasks.md`
- `openspec/changes/<change-name>/specs/<capability>/spec.md`
- `openspec/specs/<capability>/spec.md`（归档时由 delta specs 更新/合并而来）
- `src/`、`include/` 下与本次 change 对应的实现代码
- `tests/` 下与本次 change 对应的测试
- 构建系统文件（如 `CMakeLists.txt`、`tests/CMakeLists.txt`）的必要更新

## 12. 归档 Change

### 12.1 归档命令
命令：
```bash
openspec archive <change-name>
```

原因：
- `openspec archive` 的命令格式是 `openspec archive [change-name]`，不支持 `--change` 选项。
- 直接给出 change 名称，OpenSpec 会把已完成 change 移到归档目录。

### 12.2 常用选项
- `-y` / `--yes`：跳过确认提示
- `--skip-specs`：跳过 main specs 的更新
- `--no-validate`：跳过验证（不推荐）

### 12.3 归档的作用和影响
- 把 `openspec/changes/<change-name>/` 移动到 `openspec/changes/archive/YYYY-MM-DD-<change-name>/`
- 该 change 不再作为活动 change 跟踪
- 保留变更历史和 `.openspec.yaml`
- 默认会尝试把 change 内的 delta specs 同步/合并到 `openspec/specs/`（可用 `--skip-specs` 跳过）

### 12.4 归档前检查
- `openspec status --change "<change-name>" --json`，确认 artifacts 已完成
- `openspec/changes/<change-name>/tasks.md` 中任务已全部打勾
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

## 14. 示例（占位）

> TODO：后续用“我们马上要做的新项目”补充一个完整示例（从 `openspec new change` 到 `openspec archive`），并确保示例与仓库结构、构建系统、测试实际一致。

---

此文档用于记录从 OpenSpec 变更初始化到代码实现、构建验证的完整工作流，包含每一步的原因与作用，以及每个步骤中可以进一步补充和改进的内容。
