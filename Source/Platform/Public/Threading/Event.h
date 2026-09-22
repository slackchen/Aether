#pragma once

#include "Core.h"
#include "Threading/Atomic.h"

namespace Aether::Platform {

//
// Auto-reset event: Signal() releases exactly one waiting thread and clears
// the flag, matching Win32 auto-reset events. Wait() with INFINITE_TIMEOUT
// blocks until signaled; otherwise returns false on timeout.
//
// On Windows this is a native event object (NativeHandle() exposes the
// HANDLE for APIs that require one, e.g. WASAPI SetEventHandle). Other
// platforms emulate it over a mutex + condition variable.
//
class Event
{
public:
    static constexpr u32 INFINITE_TIMEOUT = 0xFFFFFFFFu;

    Event();
    ~Event();

    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;

    void Signal();
    void Reset();

    // Returns true if the event was signaled, false on timeout.
    bool Wait(u32 timeoutMillis);

    // Platform event handle (HANDLE on Windows, nullptr elsewhere). Only
    // meaningful for OS APIs that need to wait on the same object.
    void* NativeHandle();

private:
    friend struct EventImpl;

    alignas(CACHE_LINE_SIZE) u8 mStorage[128]{};
};

} // namespace Aether::Platform
