## 1. Foundations

- [x] 1.1 Add `include/myproject/logger.h` with the logger type, log levels, and convenience macros.
- [x] 1.2 Add `src/logger.cpp` with the initial logger implementation.
- [x] 1.3 Update `CMakeLists.txt` to build logger-enabled application and demo targets.
- [x] 1.4 Update `tests/CMakeLists.txt` to build the logger unit tests.

## 2. Implemented Core Behavior

- [x] 2.1 Support log levels `debug`, `info`, `warn`, and `error`.
- [x] 2.2 Format log entries with timestamps and severity labels.
- [x] 2.3 Support optional module names in log output.
- [x] 2.4 Support console output for emitted log messages.
- [x] 2.5 Support file output through runtime file configuration.
- [x] 2.6 Filter messages below the configured minimum severity.
- [x] 2.7 Read `LOG_LEVEL`, `LOG_OUTPUT`, and `LOG_FILE` environment variables during initialization.

## 3. Remaining Core Work

- [x] 3.1 Add an explicit `shutdown()` API and ensure no further messages are written after shutdown.
- [x] 3.2 Make `LOG_OUTPUT` behavior match the change requirements exactly for `console`, `file`, and default file fallback.
- [x] 3.3 Remove unconditional initialization and status messages that pollute normal logger output.
- [x] 3.4 Ensure repeated initialization cleanly resets output destinations and log level across tests and reruns.
- [x] 3.5 Retain file and line capture as optional macro-based context and keep formatting consistent when it is present.

## 4. Demo And Integration

- [x] 4.1 Add logger usage examples in `src/main.cpp` and `src/logger_demo.cpp`.
- [x] 4.2 Clean up demo code so logger examples are focused and remove unrelated debug `stderr` output.
- [x] 4.3 Fix the duplicated `src/logger_demo.cpp` entry in `CMakeLists.txt`.

## 5. Validation

- [x] 5.1 Add baseline unit tests for level filtering and basic output routing.
- [x] 5.2 Add tests for `debug` messages and `LOG_LEVEL=debug`.
- [x] 5.3 Add tests for timestamp and module formatting.
- [x] 5.4 Add tests that verify true console-only and file-only behavior from `LOG_OUTPUT`.
- [x] 5.5 Add tests for invalid environment variable fallbacks.
- [x] 5.6 Add tests for `shutdown()` and repeated initialization behavior.
- [x] 5.7 Add tests for runtime file-path configuration and file creation failure handling.

## 6. Next Revision: Concurrency And Runtime Context

- [x] 6.1 Extend the logger format to include process ID in every emitted entry.
- [x] 6.2 Add thread context output to every emitted entry using a stable thread number format.
- [x] 6.3 Define and implement the source and formatting rule for thread numbers.
- [x] 6.4 Tighten the logger initialization path so concurrent first-use logging does not race through redundant initialization.
- [x] 6.5 Add multithreaded tests that verify entries are emitted as complete, non-interleaved lines under concurrent logging.
- [x] 6.6 Add tests that verify process ID and thread number appear in both console and file output.
- [x] 6.7 Add tests or documentation that clarify concurrent output ordering is serialization order, not cross-thread causal order.

## 7. Logdaemon (Multi-Process Collection)

- [ ] 7.1 Define a minimal message schema and framing for log entries (timestamp, severity, pid, thread, message).
- [ ] 7.2 Choose an IPC transport (e.g., localhost TCP / Unix domain socket) and document the trade-offs.
- [ ] 7.3 Add a `LOG_OUTPUT=daemon` (and/or `both+daemon`) mode plus `LOG_DAEMON_ADDR` configuration.
- [ ] 7.4 Implement a client sink with reconnect, bounded buffering, and explicit backpressure/drop policy.
- [ ] 7.5 Implement a `logdaemon` process that serializes writes to file/console and prevents line interleaving across senders.
- [ ] 7.6 Add rotation/retention policy and security defaults (local-only by default; document auth if remote is enabled).
