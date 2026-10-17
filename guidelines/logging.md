---
name: Logging
description: "Logging levels, message style, and runtime logging guidance"
---

# Logging

- PROTOCOL level is used for all protocol-related messages.
- DEBUG level is used for all kind of debugging information. Proper debug level logging as important, as warning or error level - it helps to understand the flow and state of the application during development and troubleshooting.
- INFO level is used for messages that highlight the progress of the application, like successful start, stop, or other important events.
- WARNING level is used for messages that indicate a potential problem or unexpected situation, but the application can continue running.
- ERROR level is used for messages that indicate a serious problem that may prevent the application from continuing to run or may cause data loss.

- Each execution path can have at most one log message, regardless of logging level.
- Use logging as the commentary for the code, providing context and explanations for the events being logged.
- Message should be clear and concise, describing the event or state being logged.
- Float point variables should be logged with next precision parameters: `{:.9f}` for float, `{:.17f}` for double, and `{:.17Lf}` for long double.
- Use LOG_*LEVEL*_NEW for all logging, where *LEVEL* is one of the following: PROTOCOL, DEBUG, INFO, WARNING, ERROR.
- For values listing use `key: value` format, add `,` to separate multiple `key: value` pairs. Example:

```cpp
LOG_DEBUG_NEW("Connection established, id: {}, ip: {}, port: {}", connectionId, ipStr, port);
```

- For values in message in native style use `key description value` format. Example:

```cpp
LOG_DEBUG_NEW("Max connections per IP is already set to {}, no change needed", value);
```

- Wrap values in `""` if they are strings.

Example:

```cpp
LOG_DEBUG_NEW("Access is not granted for connection: {}, error: \"{}\"", connectionId, errorMessage);
```

- Log a relevant `errno` value and description after a failed POSIX operation. //! Sample
- Avoid logging or expensive message construction in hot loops or under locking unless an existing runtime guard makes the cost acceptable.
