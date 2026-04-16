## 1. Setup

- [x] 1.1 Add `include/myproject/logger.h` with the public logger API and configuration interface.
- [x] 1.2 Add `src/logger.cpp` and implement the logger module skeleton.
- [x] 1.3 Update `CMakeLists.txt` so the logger source and headers are built and installed with the project.
- [ ] 1.4 Update `tests/CMakeLists.txt` to add a unit test target for `tests/unit/test_logger.cpp`.
- [x] 1.4 Update `tests/CMakeLists.txt` to add a unit test target for `tests/unit/test_logger.cpp`.

## 2. Core implementation
- [x] 1.3 Update `CMakeLists.txt` so the logger source and headers are built and installed with the project.

- [x] 1.1 Add `include/myproject/logger.h` with the public logger API and configuration interface.
- [x] 1.2 Add `src/logger.cpp` and implement the logger module skeleton.
- [x] 1.3 Update `CMakeLists.txt` so the logger source and headers are built and installed with the project.
- [x] 1.4 Update `tests/CMakeLists.txt` to add a unit test target for `tests/unit/test_logger.cpp`.

## 2. Core implementation

- [x] 2.1 Implement log level parsing from `LOG_LEVEL` with valid values `info`, `warning`, and `error`, defaulting to `warning`.
- [x] 2.1 Implement log level parsing from `LOG_LEVEL` with valid values `info`, `warning`, and `error`, defaulting to `warning`.
- [x] 2.2 Implement output selection from `LOG_OUTPUT`, defaulting to file logging when unset or invalid.
- [x] 2.3 Implement timestamped log formatting with optional module names and a consistent line format.
- [x] 2.4 Implement logger initialization and shutdown, including safe file opening, file handle cleanup, and resource management.
- [x] 2.5 Implement main.cpp which is a demo for the usage of this logger.

## 3. Validation

- [x] 3.1 Add tests verifying `LOG_LEVEL` defaults to `warning` and filters out lower-severity messages.
- [x] 3.2 Add tests verifying `LOG_OUTPUT` controls console versus file output.
- [x] 3.3 Add tests verifying log entries include timestamp, level, optional module, and message text.
- [x] 3.4 Add or update `src/main.cpp` example usage to demonstrate logger initialization, file logging, and optional console output.

## 4. File Preservation Rules

- [x] 4.1 Do not delete or modify the existing test_xxx.cpp file. It must be preserved as-is.
## 4. File Preservation Rules
### 5.2 Functionality Verified
- [x] Logger initialization works correctly
- [x] Log levels (DEBUG, INFO, WARN, ERROR) function properly
- [x] Console output works with proper formatting
- [x] Thread safety implemented with mutex protection
### 5.4 Issues Resolved
- [x] Fixed recursive mutex deadlock in initialization
- [x] Fixed variable scope issues in logger.cpp
**Change "create-basic-logger" is COMPLETE and VERIFIED**
5. ✅ Proper resource management