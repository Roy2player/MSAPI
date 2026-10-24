---
name: Concurrency
description: "Locking patterns and thread-safety requirements"
---

# Concurrency

## Principles

- Concurrently available data must be protected by the MSAPI::Lock.
- Locks should be held as briefly and as precisely as possible, with their scope minimized within the required synchronization guarantees.
- Performance considerations must not leave shared data unprotected or split a critical section that requires atomicity. The atomicity of such a section must be preserved.
- RAII guards are preferred over manual locking and unlocking.
- Fields whose accesses are expected to be protected together must share one common lock.
- An abstraction should not protect each method internally when its methods are expected to be accessed together. In that case, synchronization should be performed outside the abstraction using its lock.
- When internal locking is chosen, the reasons should be described in the abstraction's `@concurrency` tag.
- Each call to an internally locking abstraction must have the comment `// Read/Write lock inside`.

## Lock fields and constness

- Locking must not force a method to be non-const. A method that does not change the abstraction's logical state must be `const`, even if it locks.
- Lock fields held by value, such as `MSAPI::Lock::Atomic` or `MSAPI::Lock::AtomicRW`, must be declared `mutable`. Getters returning references to such locks must be `const`.

For pointer-held synchronization objects, locking the pointed-to object does not itself require making the pointer `mutable`.

- Existing pointer ownership and constness conventions must be preserved.
- `mutable` must not bypass const-correctness of logical object state. See [Code Syntax](codeSyntax.md#declaration) for the restriction to lock fields.

```cpp
class Data {
private:
    std::map<uint64_t, Value> m_values;
    mutable MSAPI::Lock::AtomicRW m_valuesLock;

public:
    FORCE_INLINE [[nodiscard]] uint64_t GetSize() const noexcept
    {
        const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::READ> _{ m_valuesLock };
        return m_values.size();
    }
};
```

## Access

Additional lifetime block scoping should be used to release a guard as soon as possible. Example:

```cpp
... code of method before accessing the data ...
{
    MSAPI::Lock::AtomicRW<MSAPI::Lock::WRITE>::Guard _{ m_dataLock };
    m_data.erase(key);
}
... code of method after accessing the data ...
```

### Object lifetime

When access to a container element continues after the container lock is released, the element and its synchronization objects must remain alive until all accesses and associated lock guards have completed, including accesses from other threads or callbacks. Removing an element from the container must not invalidate an active user's references or allow its storage to be reused prematurely.

Standard-library smart pointers are preferred when splitting global and more precise locking for tree-like data organization. Other mechanisms may be used when they provide the same lifetime guarantees:

- Guarded borrowing: a caller-held guard prevents erasure and destruction for the entire duration of borrowed access.
- Unique or external ownership: an owner outlives all users, and removing a container entry does not destroy an object still in use.
- Custom reference counting: references are acquired while the object is known to be alive, concurrent reference-count updates are synchronized, and destruction waits until the last reference is released.
- Deferred reclamation: removed objects are retained until all users that could still access them have completed; only then is destruction or storage reuse possible.

Lifetime protection must be established while the object is known to be alive and retained through the final access, including lock-guard destruction. Lifetime and reclamation conditions must be evident from the code or its documentation; non-obvious assumptions must be explained as required by [Comments](codeSyntax.md#comments). Lifetime protection does not replace synchronization of mutable state.

The following example acquires shared ownership while holding the container lock, then uses the element's own lock after releasing the container lock:

```cpp
... code of method before accessing the data ...

std::shared_ptr<ConnectionRelatedData> data;
{
    MSAPI::Lock::AtomicRW<MSAPI::Lock::READ>::Guard _{ m_dataLock };
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
  MSAPI::Lock::AtomicRW<MSAPI::Lock::WRITE>::Guard _{ data->GetLock() };
  data->HandleAnyOperation();
}

... code of method after accessing the data ...
```

Even if another thread removes the data from the container, it remains alive until the last shared pointer is released. Releasing the data's lock lets other threads access it while shared ownership preserves its lifetime.

### Logical removal

When a data abstraction is pending destruction but remains accessible in another thread, it must be marked as released to notify other threads that the data is no longer valid. Example:

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

Here, `m_isUsable` notifies other threads that the data is no longer valid. The `std::memory_order_release` ordering publishes preceding writes to threads that synchronize with the flag. The `m_isClosed` variable prevents repeated calls to `Close()` from repeating the operation.

A validity flag indicates logical state; it does not keep the object or its lock alive. The lifetime guarantees above still apply.
