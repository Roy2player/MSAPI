---
name: logging
description: "Use when writing or fixing logging in files. Applies a compact, strict guideline for logging with exact tag ordering and wording rules."
---

# Logging

- Use LOG_*LEVEL*_NEW for all logging, where *LEVEL* is one of the following: PROTOCOL, DEBUG, INFO, WARNING, ERROR.
- For values listing use `key: value` format, add `,` to separate multiple `key: value` pairs. **Sample:**
```cpp
LOG_DEBUG_NEW("Connection established, id: {}, ip: {}, port: {}", connectionId, ipStr, port);
```
- For values in message in native style use `key description value` format. **Sample:**
```cpp
LOG_DEBUG_NEW("Max connections per IP is already set to {}, no change needed", value);
```
- Wrap values in `""` if they are strings.

**Sample:***
```cpp
LOG_DEBUG_NEW("Access is not granted for connection: {}, error: \"{}\"", connectionId, errorMessage);
```

- One path should not have multiple logging levels.
- Message should be clear and concise, describing the event or state being logged.
- Float point variables should be logged with next precision parameters: `{:.9f}` for float, `{:.17f}` for double, and `{:.17Lf}` for long double.

- PROTOCOL level is used for all protocol-related messages.
- DEBUG level is used for all kind of debugging information.
- INFO level is used for messages that highlight the progress of the application, like successful start, stop, or other important events.
- WARNING level is used for messages that indicate a potential problem or unexpected situation, but the application can continue running.
- ERROR level is used for messages that indicate a serious problem that may prevent the application from continuing to run or may cause data loss.