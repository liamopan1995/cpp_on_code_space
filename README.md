# 项目目录结构说明

## 环境准备

```text
prepare/install_openspec.sh
```

---

# 推荐目录结构

```text
.
├── README.md
├── openspec/                  # ← 不要动这里，放规格用
│   ├── config.yaml
│   ├── changes/
│   ├── specs/                 # 当前系统的「单一事实来源」
│   └── ... (archive 等)
│
├── prepare/
│   └── install_openspec.sh
│
├── src/                       # ← 你的主要源码
│   ├── main.cpp
│   ├── module1/
│   │   ├── foo.cpp
│   │   └── foo.h
│   └── ...
│
├── include/                   # ← 公开的头文件（可选，按需）
│   └── myproject/
│
├── tests/                     # ← 测试用例（强烈推荐）
│   ├── unit/
│   ├── integration/
│   └── CMakeLists.txt
│
├── CMakeLists.txt             # 构建系统
├── CMakePresets.json
├── .gitignore
└── ... (其他配置文件)
```

---

# 各目录作用说明

| 目录 | 作用 |
|---|---|
| `openspec/` | OpenSpec 规格管理目录，用于需求、变更、设计文档 |
| `prepare/` | 环境初始化脚本 |
| `src/` | 核心源码目录 |
| `include/` | 对外公开头文件 |
| `tests/` | 单元测试 / 集成测试 |
| `CMakeLists.txt` | CMake 构建入口 |
| `CMakePresets.json` | CMake 预设配置 |
| `README.md` | 项目说明文档 |

---

# openspec 目录说明

```text
openspec/
```

属于：

```text
规格（Specification）层
```

而不是业务代码层。

---

## 推荐原则

### ✅ 放：

- 需求文档
- 设计文档
- change proposal
- spec
- architecture

---

### ❌ 不放：

- cpp 源码
- build 目录
- 临时代码
- 测试产物

---

# src 目录说明

```text
src/
```

用于：

```text
实际业务逻辑实现
```

例如：

- daemon
- logger
- socket
- parser
- storage

等模块。

---

# include 目录说明

```text
include/myproject/
```

通常用于：

```text
对外暴露 API
```

例如：

```cpp
#include <myproject/logger.h>
```

---

# tests 目录说明

推荐拆分：

```text
tests/
├── unit/
└── integration/
```

---

## unit

单元测试：

```text
测试单个类/函数
```

例如：

- splitLines
- parser
- formatter

---

## integration

集成测试：

```text
测试完整链路
```

例如：

```text
client → daemon → file
```

---

# 推荐开发流程

```text
OpenSpec
    ↓
设计 spec/change
    ↓
src 实现
    ↓
tests 验证
    ↓
CMake 构建
```

---

# 快速验证：Logger → logdaemon

下面用最少的命令验证“应用进程发出的日志条目会被转发到 `logdaemon`”。

## 1) 构建

```bash
cmake -S . -B build
cmake --build build -j
```

## 2) 终端 A：启动 daemon

TCP 示例：

```bash
./build/logdaemon --listen 127.0.0.1:7777
```

Unix Domain Socket 示例：

```bash
./build/logdaemon --listen unix:/tmp/log.sock
```

（可选）让 daemon 也写文件：

```bash
./build/logdaemon --listen 127.0.0.1:7777 --file /tmp/daemon.log
```

## 3) 终端 B：启动应用并把 logger 指向 daemon

仅发送到 daemon：

```bash
export LOG_OUTPUT=daemon
export LOG_DAEMON_ADDR=127.0.0.1:7777
export LOG_LEVEL=debug
./build/main
```

同时输出到 console/file + daemon：

```bash
export LOG_OUTPUT=both+daemon
export LOG_DAEMON_ADDR=127.0.0.1:7777
export LOG_FILE=demo_log.txt
./build/logger_demo
```

预期现象：
- 终端 A 会看到应用输出的日志行（每条日志为一行，以 `\n` 结尾）。
- 如果加了 `--file /tmp/daemon.log`，可以用 `tail -f /tmp/daemon.log` 观察落盘内容。

注意：如果你在受限的 sandbox/容器环境里运行，可能会因为系统策略禁止创建/绑定 socket 而看到 `Operation not permitted`，这不是 logger 逻辑问题。

## 4) 用测试证明（自动化）

```bash
ctest --test-dir build -R test_logger --output-on-failure
```

# 推荐原则总结

## OpenSpec

负责：

```text
“系统应该是什么”
```

---

## src

负责：

```text
“系统具体怎么实现”
```

---

## tests

负责：

```text
“系统是否正确”
```

---

## CMake

负责：

```text
“系统如何构建”
```

# Log Daemon 通信架构总结

## 1. 项目定位

该 `logdaemon` 是一个基于 Linux Socket API 的日志收集服务。

作用：

```text
多个客户端进程
        ↓
    socket 通信
        ↓
    logdaemon
        ↓
 console / file 落盘
```

本质上属于：

- IPC（Inter Process Communication，进程间通信）
- Socket Programming
- Linux System Programming

领域。

---

# 2. 当前支持的通信方式

当前 daemon 支持两种 socket family：

| 类型 | Address Family | 地址形式 | 使用场景 |
|---|---|---|---|
| TCP Socket | `AF_INET` / `AF_INET6` | `127.0.0.1:7777` | 网络通信 |
| Unix Domain Socket | `AF_UNIX` | `/tmp/log.sock` | 本机 IPC |

---

# 3. TCP Socket（网络通信）

## 使用方式

```bash
./logdaemon --listen 127.0.0.1:7777
```

客户端：

```text
connect("127.0.0.1:7777")
```

---

## 底层协议栈

```text
Application
    ↓
Socket API
    ↓
TCP
    ↓
IP
    ↓
Network Interface
```

---

## 特点

### 优点

- 可跨机器
- 可远程调试
- 通用性强

### 缺点

- 经过 TCP/IP 协议栈
- 性能低于 Unix Socket
- 需要端口管理

---

# 4. Unix Domain Socket（本地 IPC）

## 使用方式

```bash
./logdaemon --listen unix:/tmp/log.sock
```

客户端：

```text
connect("/tmp/log.sock")
```

---

## 底层机制

```text
Application
    ↓
Socket API
    ↓
Unix IPC
    ↓
Kernel Memory
```

不经过：

- TCP
- IP
- 网卡

属于纯本地内核通信。

---

## 特点

### 优点

- 更低延迟
- 更高吞吐
- 更低 CPU 开销
- 更安全（仅本机）

### 缺点

- 无法跨机器

---

# 5. 为什么 Socket API 不等于“网络”

本项目同时支持：

```text
TCP/IP
Unix IPC
```

但两者使用的 API 基本一致：

```cpp
socket()
bind()
listen()
accept()
read()
close()
```

说明：

> socket API 是“统一通信接口”，而不仅仅是“网络接口”。

Linux 中很多通信机制都基于 socket abstraction：

| Family | 用途 |
|---|---|
| AF_INET | IPv4 |
| AF_INET6 | IPv6 |
| AF_UNIX | 本地 IPC |
| AF_NETLINK | 用户态 ↔ 内核 |
| AF_CAN | CAN 总线 |
| AF_BLUETOOTH | 蓝牙 |

---

# 6. 当前并发模型

当前采用：

```text
thread-per-connection
```

模型。

即：

```text
accept()
    ↓
每个 client 创建一个 thread
    ↓
thread 内 blocking read()
```

---

## 当前结构

```text
Main Thread:
    accept loop

Client Thread A:
    read()
    splitLines()
    writeLine()

Client Thread B:
    read()
    splitLines()
    writeLine()
```

---

## 特点

### 优点

- 实现简单
- 调试直观
- 易于理解

### 缺点

- 高并发下线程数量过多
- blocking read 存在 hang 风险
- 上下文切换开销较高

适合：

- 内部工具
- 调试 daemon
- 中低并发日志系统

不适合：

- 超高并发 server
- gateway/proxy 类系统

---

# 7. 日志协议（当前实现）

当前日志协议非常简单：

```text
一行日志 = 一个 '\n' 结尾的字符串
```

例如：

```text
[INFO] hello world\n
```

daemon 通过：

```cpp
splitLines()
```

按换行符拆分日志。

---

## 为什么需要 carry buffer

socket stream 不保证：

```text
一次 send == 一次 read
```

例如：

客户端：

```text
send("hello\n")
```

daemon 可能：

```text
第一次 read:
    "hel"

第二次 read:
    "lo\n"
```

因此需要：

```cpp
carry + 当前 buffer
```

重新拼接后再 split。

这是 stream protocol 的典型处理方式。

---

# 8. 为什么当前没有支持 UDP

当前 daemon 使用：

```cpp
SOCK_STREAM
```

即：

```text
TCP stream socket
```

而不是：

```cpp
SOCK_DGRAM
```

（UDP datagram socket）。

---

# 9. 为什么日志系统通常不优先使用 UDP

UDP 的特点：

```text
无连接
不可靠
不保证顺序
不保证到达
```

---

## UDP 的问题

### 1. 日志可能丢失

UDP 不保证送达：

```text
send() 成功
≠
对方收到
```

日志系统通常更关注：

```text
可靠性
```

因此 TCP 更适合。

---

### 2. 不保证顺序

可能：

```text
log3 先于 log2 到达
```

导致日志时间线错乱。

---

### 3. 大日志可能被截断

UDP 有 MTU 限制：

```text
~1500 bytes
```

超长日志可能：

- 分片
- 丢包
- 截断

---

### 4. 不适合 stream framing

当前 daemon 使用：

```text
read stream
split '\n'
```

属于 stream protocol。

UDP 是 message-oriented：

```text
一次 recvfrom()
对应一个 datagram
```

协议模型不同。

---

# 10. 为什么工业日志系统更常使用 TCP / Unix Socket

日志系统通常更关注：

| 指标 | 重要性 |
|---|---|
| 不丢日志 | 高 |
| 顺序正确 | 高 |
| 本地性能 | 高 |
| 极限低延迟 | 中 |

因此：

| 场景 | 常见方案 |
|---|---|
| 本机 daemon | Unix Socket |
| 跨机器日志 | TCP |
| 超高吞吐 telemetry | UDP（部分场景） |

---

# 11. 当前系统的生命周期

```text
logdaemon:
    socket()
    bind()
    listen()
    accept()

client:
    connect()
    send(log + '\n')

daemon thread:
    read()
    splitLines()
    writeLine()

client:
    close()
```

---

# 12. 后续可演进方向

当前实现属于：

```text
blocking IO + thread-per-connection
```

后续可升级：

---

## IO 模型

- non-blocking socket
- select/poll/epoll
- reactor model

---

## 协议增强

支持：

```text
timestamp
log level
binary frame
structured logging
```

---

## 稳定性增强

- signal handling
- graceful shutdown
- reconnect
- heartbeat
- backpressure

---

# 13. 总结

本项目实现了一个：

```text
基于 Socket API 的多客户端日志 daemon
```

特点：

- 同时支持 TCP 与 Unix Domain Socket
- 使用 blocking IO
- thread-per-connection 并发模型
- 基于 '\n' 的 stream framing
- 支持 console/file 输出

该实现适合作为：

- Linux socket 编程学习项目
- IPC 学习项目
- daemon 架构原型
- 小型内部日志系统

同时也为后续：

- epoll
- reactor
- 高性能日志系统

等方向提供了良好的基础。
