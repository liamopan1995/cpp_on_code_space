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
