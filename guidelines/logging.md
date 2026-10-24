---
name: Logging
description: "Logging levels, message style, and runtime logging guidance"
---

# Logging

- Protocol implementations must use PROTOCOL level for messages that would otherwise use DEBUG or INFO. This keeps transport-level details out of application level.
- Debugging information outside protocol implementations must use DEBUG level. Debug logging helps explain application flow and state during development and troubleshooting.
- Messages highlighting application progress outside protocol implementations, such as successful start, stop, or other important events, must use INFO level.
- Messages indicating a potential problem or unexpected situation while the application can continue must use WARNING level.
- Messages indicating a serious problem that may prevent continued execution or cause data loss must use ERROR level.

The WARNING and ERROR rules also apply to failures within protocol implementations.

- Each execution path must contain at most one log message, regardless of logging level. An execution path is one logical branch or operation that an execution unit can follow.
- Logging should provide context and explanations for the events being logged, serving as commentary for the code.
- Messages should be clear and concise, describing the event or state being logged.
- Floating-point variables should use these precision parameters: `{:.9f}` for float, `{:.17f}` for double, and `{:.17Lf}` for long double.
- All logging must use LOG_*LEVEL*_NEW, where *LEVEL* is one of PROTOCOL, DEBUG, INFO, WARNING, or ERROR.
- Value lists must use `key: value` format, with commas separating multiple pairs. Example:

```cpp
LOG_DEBUG_NEW("Connection established, id: {}, ip: \"{}\", port: {}", connectionId, ipStr, port);
```

- Values embedded in prose must use `key description value` format. Example:

```cpp
LOG_DEBUG_NEW("Max connections per IP is already set to {}, no change needed", value);
```

- String values must be wrapped in `""`.

Example:

```cpp
LOG_DEBUG_NEW("Access is not granted for connection: {}, error: \"{}\"", connectionId, errorMessage);
```

- A failed POSIX operation must be logged with the relevant `errno` value and description.
- Logging or expensive message construction should be avoided in hot loops or while holding locks unless an existing runtime guard makes the cost acceptable.
