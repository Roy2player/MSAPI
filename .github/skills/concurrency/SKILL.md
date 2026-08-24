---
name: concurrency
description: "Use when writing or fixing concurrently executed code in files. Applies a compact, strict guideline."
---

# Concurrency

## Principles

- Concurrently available data must be protected by the MSAPI::Lock.
- Lock must be locked as shortly and as precisely as possible.
- Locking by the RAII guard is preferred over manual locking and unlocking.
- One lock must cover all fields that expected to be accessed together should be protected by the common lock.
- Abstraction should not be internally protected (e.g. each method is protected by its own lock) in case, when it expected to be accessed together with other methods of the same abstraction - the synchronization should be done outside of the abstraction by the usage of internal abstraction lock.
- If internal locking of the abstraction is a choice, the reasons should be described in abstraction tag `@concurrency` and each call marked by the comment `// Read/Write lock inside`.

## Access

Additional life time block scoping should be used to release guard as soon as possible. Example 
```cpp
... code of method before accessing the data ...
{
    MSAPI::Lock::AtomicRW<MSAPI::Lock::write>::Guard _{ m_dataLock };
    m_data.erase(key);
}
... code of method after accessing the data ...
```

Smart pointers from standard library should must be used to split the global and more precise locking for tree-like data organization, when element erasing from container can take place. Example
```cpp
... code of method before accessing the data ...

std::shared_ptr<ConnectionRelatedData> data;
{
    MSAPI::Lock::AtomicRW<MSAPI::Lock::read>::Guard _{ m_dataLock };
    const auto it{ m_data.find(key) };
    if (it != m_data.end()) {
        data = it->second;
    }
}

if (data == nullptr) [[unlikely]] {
    LOG_WARNING_NEW("Data is not found for key: {}", key);
    return;
}

{
  MSAPI::Lock::AtomicRW<MSAPI::Lock::write>::Guard _{ data->GetLock() };
  data->HandleAnyOperation();
}

... code of method after accessing the data ...
```
Even if the data is removed from the container by another thread, the data will be alive until the last shared pointer is alive. The lock of the data will be released as soon as possible, and the other threads can access the data concurrently.

In case, when the data abstraction should be destructed but can be still accessible in other thread, it should be market as realized to notify other threads that the data is not valid anymore. Example
```cpp
FORCE_INLINE void Close()
	{
		if (m_isClosed) [[unlikely]] {
			return;
		}

		m_isUsable.store(false, std::memory_order_release);
		m_isClosed = true;
		... other code ...
	}
```
Here, the `m_isUsable` atomic variable is used as a flag to notify other threads that the data is not valid anymore. The `std::memory_order_release` ensures that all previous writes to the data are visible to other threads before the flag is set. The `m_isClosed` variable is used to prevent multiple calls to the `Close()` method.