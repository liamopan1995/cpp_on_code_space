## Why
当前代码库缺少统一的日志记录能力，调试与运行时异常排查依赖手工打印信息，管理不便。通过引入基础 logger，可让项目日志输出更规范、可控，并为后续性能、格式、过滤和持久化增强提供基础。

## What Changes
- 添加 `include/myproject/logger.h`，声明基础 logger 接口和配置选项
- 添加 `src/logger.cpp`，实现 logger 基础功能
- 添加 `logdaemon` 守护进程与对应构建目标（daemon 进程负责汇聚多进程日志并串行写入）
- 为 logger 增加 daemon 输出目的地（IPC client sink），支持将日志发送到 `logdaemon`
- 修改 `src/main.cpp` 或示例入口，以演示 logger 输出和落盘能力
- 添加或更新单元测试 `tests/unit/test_logger.cpp`，验证日志级别、控制台输出和文件落盘行为
- 更新 logger 规格，明确多线程近同时刻写日志时的行为约束
- 为后续增强补充进程 ID 与线程编号输出要求
- 更新 `CMakeLists.txt` 和 `tests/CMakeLists.txt`，确保 logger 实现与测试正确构建

## Capabilities
- 提供基础日志记录能力 `basic-logger`
- 支持不同日志级别：`debug`、`info`、`warning`、`error`
- 支持运行时实时落盘，将日志输出到指定文件
- 支持控制台输出，包含时间戳和日志级别信息
- 支持可配置日志前缀或模块名
- 明确并发日志输出时单条日志不可交叉污染
- 计划支持在日志中输出进程 ID 与线程编号
- 支持 `logdaemon` 多进程日志汇聚：通过独立守护进程收集日志并串行写入文件/控制台，提供明确的排序、背压与降级策略

## Impact
- 影响文件：`include/myproject/logger.h`、`src/logger.cpp`、`src/main.cpp`
- 影响测试：`tests/unit/test_logger.cpp`
- 影响规格文档：`openspec/changes/create-basic-logger/specs/create-basic-logger/spec.md`、`openspec/changes/create-basic-logger/design.md`
- 需要修改构建配置：`CMakeLists.txt`、`tests/CMakeLists.txt`
- 为后续日志写入文件、日志格式扩展、日志级别过滤、线程上下文输出以及异步写盘提供基础接口和实现
- 增加 `logdaemon` 汇聚路径，避免多进程直接共享同一日志文件带来的并发写入与顺序不确定问题
