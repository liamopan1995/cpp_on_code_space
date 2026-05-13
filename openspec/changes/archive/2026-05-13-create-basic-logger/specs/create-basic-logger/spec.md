# basic-logger Specification

## Purpose
Provide a baseline logging facility that replaces ad hoc `std::cout` and `printf` debugging with a consistent, testable logger for console and file output.

## Requirements
### Requirement: Logger shall support four severity levels
The logger SHALL support `DEBUG`, `INFO`, `WARN`, and `ERROR` severities.

#### Scenario: Severity label is emitted
- **WHEN** `LOG_INFO("message")` is called after the logger is initialized
- **THEN** the emitted entry contains `[INFO]`
- **AND** the emitted entry contains the message text.

#### Scenario: Lower severity entries are filtered
- **WHEN** the effective minimum level is `WARN`
- **AND** `LOG_INFO("message")` is called
- **THEN** no entry is emitted to any enabled destination.

### Requirement: Logger configuration shall support environment-driven defaults
The logger SHALL derive its default behavior from environment variables during initialization or first-use auto-initialization.

- `LOG_LEVEL` SHALL control the minimum emitted severity.
- `LOG_OUTPUT` SHALL control destination mode.
- `LOG_FILE` SHALL control the file path when file output is enabled.
- `LOG_DAEMON_ADDR` SHALL control the daemon address when daemon output is enabled.

#### Scenario: Valid log level from environment
- **WHEN** `LOG_LEVEL=debug`
- **AND** the logger initializes
- **THEN** `DEBUG` entries are eligible for emission.

#### Scenario: Invalid log level falls back to warning
- **WHEN** `LOG_LEVEL` is set to an unsupported value
- **AND** the logger initializes
- **THEN** the effective minimum level is `WARN`.

#### Scenario: Output mode defaults to file
- **WHEN** `LOG_OUTPUT` is unset or invalid
- **AND** the logger initializes
- **THEN** the logger enables file output by default.

### Requirement: Logger shall support console and file destinations
The logger SHALL support console-only, file-only, and combined console-plus-file output modes.

#### Scenario: Console-only output
- **WHEN** `LOG_OUTPUT=console`
- **AND** an eligible entry is logged
- **THEN** the entry is written to the console
- **AND** no log file is opened for that entry.

#### Scenario: Combined output
- **WHEN** `LOG_OUTPUT=both`
- **AND** an eligible entry is logged
- **THEN** the entry is written to the console
- **AND** the same entry is written to the configured log file.

#### Scenario: File open failure falls back to console
- **WHEN** file output is enabled
- **AND** the configured file path cannot be opened
- **THEN** file output is disabled
- **AND** console output remains enabled so the entry is still observable.

#### Scenario: Runtime file logging can be enabled and disabled
- **WHEN** `enableFileLogging(path)` is called successfully
- **THEN** subsequent eligible entries are appended to that file.
- **WHEN** `disableFileLogging()` is called
- **THEN** subsequent entries are no longer written to the file.

### Requirement: Logger shall support logdaemon destination for multi-process collection
The logger SHALL support sending eligible log entries to a local `logdaemon` collector process so multiple processes can contribute to a single serialized log stream without directly sharing a file.

- `LOG_OUTPUT` SHALL support a daemon mode (e.g., `daemon`) and MAY support combinations (e.g., `both+daemon`).
- `LOG_DAEMON_ADDR` SHALL control the daemon address (UDS path or `host:port`).

#### Scenario: Daemon output emits entries via IPC
- **WHEN** `LOG_OUTPUT=daemon`
- **AND** the logger is initialized with a reachable daemon at `LOG_DAEMON_ADDR`
- **AND** an eligible entry is logged
- **THEN** the entry is sent to `logdaemon`
- **AND** the entry is observable in the daemon’s configured destinations (file and/or console).

#### Scenario: Daemon unreachable falls back safely
- **WHEN** `LOG_OUTPUT=daemon`
- **AND** the daemon is unreachable or disconnects
- **THEN** the logger applies a documented fallback policy (e.g., drop with counters, or fall back to console-only)
- **AND** the application does not deadlock or block indefinitely on logging.

#### Scenario: Cross-process line interleaving is prevented by daemon serialization
- **WHEN** multiple processes send entries to the same `logdaemon`
- **THEN** each persisted entry is one complete line
- **AND** fragments from different processes are not interleaved in the daemon-managed log file.

### Requirement: Logger shall emit structured textual context
Each emitted entry SHALL be formatted as a single line containing timestamp, severity, process context, thread context, and message text.

#### Scenario: Standard entry format
- **WHEN** an eligible entry is emitted
- **THEN** the line contains a timestamp
- **AND** the line contains a severity label
- **AND** the line contains `[pid=<number>]`
- **AND** the line contains `[thread=<number>]`
- **AND** the line contains the message text.

#### Scenario: Optional source location and module context
- **WHEN** a logging macro captures file and line information
- **THEN** the emitted entry contains `[file:line]`.
- **WHEN** a module name is provided
- **THEN** the emitted entry contains `[module-name]`.

### Requirement: Logger shall provide stable per-thread numbering
The logger SHALL assign a stable positive integer thread number to each participating thread within a process.

#### Scenario: Thread number assignment
- **WHEN** a thread emits its first log entry
- **THEN** the logger assigns that thread a positive integer identifier
- **AND** later entries from the same thread reuse the same identifier.

#### Scenario: Thread number format remains parseable
- **WHEN** any thread emits an entry
- **THEN** thread context is rendered as `thread=<n>`
- **AND** the format remains consistent across console and file destinations.

### Requirement: Logger shall provide initialization and shutdown lifecycle
The logger SHALL support explicit initialization and shutdown, and SHALL prevent output after shutdown until it is initialized again.

#### Scenario: First-use auto initialization
- **WHEN** a log macro is called before explicit initialization
- **THEN** the logger initializes itself once using environment-driven defaults
- **AND** the entry is emitted if it passes filtering.

#### Scenario: Shutdown suppresses further output
- **WHEN** `shutdown()` is called
- **AND** a log macro is called afterward without reinitialization
- **THEN** no entry is emitted.

#### Scenario: Reinitialization resets destinations and thresholds
- **WHEN** the logger is initialized again after prior use
- **THEN** destination state and minimum severity are rebuilt from the new initialization inputs.

### Requirement: Logger shall serialize concurrent writes within a process
The logger SHALL preserve each entry as one complete line when multiple threads log concurrently in the same process.

#### Scenario: No line interleaving under multithreaded logging
- **WHEN** multiple threads emit entries at nearly the same time
- **THEN** each emitted entry appears as one complete line
- **AND** fragments from different entries are not interleaved on console output or in the log file.

#### Scenario: Ordering is logger serialization order
- **WHEN** multiple threads emit entries concurrently
- **THEN** the observed order is the order in which the logger serializes writes
- **AND** the specification does not require that order to reflect cross-thread causal order.

## Potential Improvements
- Add sub-second timestamps so near-simultaneous events from different threads or processes can be correlated more precisely.
- Add a structured correlation field such as request ID, object ID, or session ID to make interaction traces easier to group during debugging.
