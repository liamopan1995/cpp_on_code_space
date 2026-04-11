# basic-logger Specification

## Purpose
提供一个基础日志记录能力，统一替代项目中零散的 `std::cout` / `printf` 调试输出。Logger 应支持控制台和文件输出，并为后续日志格式扩展、级别过滤和持久化增强打下基础。

## Requirements
### Requirement: Logger shall support multiple log levels
Logger SHALL support logging messages at `info`, `warning`, and `error` levels.

#### Scenario: Logging an info message
- **WHEN** `Logger::info("module", "message")` is called
- **THEN** the logger writes a log entry containing `[INFO]` and the message.

### Requirement: Logger configuration shall use environment variables
The logger SHALL determine its behavior from environment variables.
- `LOG_OUTPUT` SHALL control whether logging is sent to console or file.
- `LOG_LEVEL` SHALL control the minimum severity to record.
- The default log level SHALL be `warning` when `LOG_LEVEL` is unset or invalid.
- The default output mode SHALL be file logging when `LOG_OUTPUT` is unset or invalid.

#### Scenario: Environment variable controls output destination
- **WHEN** `LOG_OUTPUT` is set to `console`
- **AND** `Logger::error("module", "message")` is called
- **THEN** the message is written to the console and not to a log file unless file output is also enabled.

#### Scenario: Environment variable controls log level
- **WHEN** `LOG_LEVEL` is set to `info`
- **AND** `Logger::info("module", "message")` is called
- **THEN** the logger records the message.
- **WHEN** `LOG_LEVEL` is unset
- **AND** `Logger::info("module", "message")` is called
- **THEN** the message is filtered out because the default level is `warning`.

### Requirement: Logger shall output to console
The logger SHALL write messages to the console when console logging is enabled.

#### Scenario: Console output enabled
- **WHEN** the logger is initialized for console output
- **AND** `Logger::warning("module", "message")` is called
- **THEN** the message is written to the console with a timestamp and log level.

### Requirement: Logger shall optionally write to a file
The logger SHALL support runtime configuration of a log file path and write messages to the file.

#### Scenario: File logging enabled
- **WHEN** the logger is initialized with a log file path
- **AND** `Logger::error("module", "message")` is called
- **THEN** the log file contains a line with a timestamp, log level, optional module name, and message.

### Requirement: Logger shall include timestamp and optional module
The logger SHALL prefix each entry with a timestamp and log level, and SHALL include a module name when provided.

#### Scenario: Module name included
- **WHEN** `Logger::info("network", "started")` is called
- **THEN** the log entry contains a timestamp, `[INFO]`, `[network]`, and the message.

### Requirement: Logger shall filter lower-level messages
The logger SHALL ignore messages below the configured minimum log level.

#### Scenario: Filtering lower-level messages
- **WHEN** the minimum log level is set to `warning`
- **AND** `Logger::info("module", "text")` is called
- **THEN** the message is not written to console or file.

### Requirement: Logger shall provide init and shutdown lifecycle
The logger SHALL provide explicit initialization and shutdown functions to manage resources and avoid file leaks.

#### Scenario: Logger lifecycle management
- **WHEN** the logger is initialized with a file path
- **AND** `Logger::shutdown()` is called
- **THEN** the log file is closed and further messages are not written.
