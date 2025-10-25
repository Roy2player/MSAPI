# MSAPI

MSAPI is a modular, high-performance C++ library for building Linux-based microservice and macrosystem architectures. It provides internal protocols, utilities, and server logic for efficient, scalable system development.

- ✅ Has no dependencies on third-party libraries.
- ✅ Is thoroughly documented.
- ✅ Is well covered by unit and end-to-end tests.

## 🔧 Building

To build library follow next simple steps:
```
cd library/build
cmake .
cmake --build . -j $(nproc)
```

***BUILD_PROFILE*** variable can be set to choose build type: Debug or Release.
See [CMakeListsCommonOptions.txt](library/build/CMakeListsCommonOptions.txt) for more details.

Number of bash scripts are available in the [bash/](bash/) folder to simplify building process. ***MSAPI_PATH*** variable should be set to the root of the MSAPI repository.

## 🧩 Features & structure:

### [Server Framework](library/source/server/)
- [**Server:**](library/source/server/server.h) Provides communication ability via TCP sockets and current-state model to manage server lifetime;
- [**Application:**](library/source/server/application.h) Provides customization ability via parameters and current-state model to manage application behavior.

Server provides data to be handled by calling a callback in case if this data is not a part of internal Application protocol.

### [Custom and another protocols](library/source/protocol/)
- [**Object protocol:**](library/source/protocol/object.h) Transfers simple copyable objects using stream and filter models.
- [**Standard protocol:**](library/source/protocol/standard.h) Handles dynamic-size messages as arrays of key-value pairs.
- [**HTTP protocol:**](library/source/protocol/http.h) Basic HTTP message parsing and handling.

### [Utility Modules](library/source/help/)
- [**Log:**](library/source/help/log.h) Multiple log levels and console/file outputs.
- [**Time:**](library/source/help/time.h) Time utilities and event scheduling.
- [**HTML:**](library/source/help/html.h) Parser module.
- [**JSON:**](library/source/help/json.h) Parser and producer module.
- [**Table:**](library/source/help/table.h) Data structure.
- [**Pthread:**](library/source/help/pthread.hpp) Module to synchronize threads.
- [**Bin:**](library/source/help/bin.h) Filesystem operations and helpers.
- [**Helper:**](library/source/help/helper.h) Miscellaneous utilities for common tasks.
- [**Meta:**](library/source/help/meta.hpp) Static enum translation, type helpers, and compile-time utilities.

### [Testing Framework](library/source/test/)

Unit/end-to-end tests utilities with ability to run servers as daemons.

## 🖥️ Frontend

[Manager app](apps/manager/source/manager.h) for orchestrating custom apps on the same host, with a user-friendly web interface.

## ⚖️ License

This software is licensed under the **Polyform Noncommercial License 1.0.0**, see [LICENSE.md](LICENSE.md).  
You may use, copy, modify, and distribute it for noncommercial purposes only.

For commercial use, please contact: maks.angels@mail.ru

## 🤝 Contributing

Contributions are welcome!  
By contributing to this repository, you agree to the [Contributor License Agreement](CONTRIBUTING.md).