#include "Platform.h"
#include "Threading/Event.h"
#include "Threading/Mutex.h"
#include "Threading/Thread.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <new>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <avrt.h>
#pragma comment(lib, "avrt.lib")
#endif

namespace Aether::Platform {

//
// Default threading implementation over the C++ standard library, shared by
// the desktop and web builds. Win32 specifics (event objects, thread naming,
// MMCSS) are #ifdef'd inside; swapping the whole file for a native
// implementation (SRWLOCK, WaitOnAddress, fiber workers) later only touches
// the CMake source list.
//
// All access to the opaque storage members goes through the friend Impl
// structs declared in the public headers.
//

// ============================================================
// Thread
// ============================================================

struct ThreadImpl
{
    static std::thread& Get(Thread& thread)
    {
        static_assert(sizeof(std::thread) <= sizeof(thread.mStorage),
                      "Thread storage too small for std::thread");
        return *reinterpret_cast<std::thread*>(&thread.mStorage[0]);
    }
};

#ifdef _WIN32

static void ApplyThreadPriorityWin32(HANDLE handle, ThreadPriority priority)
{
    switch (priority)
    {
        case ThreadPriority::AboveNormal:
            SetThreadPriority(handle, THREAD_PRIORITY_ABOVE_NORMAL);
            break;
        case ThreadPriority::Background:
            SetThreadPriority(handle, THREAD_PRIORITY_BELOW_NORMAL);
            break;
        case ThreadPriority::Audio:
        {
            // MMCSS boost; the registration is released when the thread exits.
            DWORD taskIndex = 0;
            AvSetMmThreadCharacteristicsW(L"Audio", &taskIndex);
            break;
        }
        case ThreadPriority::Normal:
        default:
            SetThreadPriority(handle, THREAD_PRIORITY_NORMAL);
            break;
    }
}

static void SetThreadNameWin32(HANDLE handle, const char* name)
{
    wchar_t wide[64];
    int n = MultiByteToWideChar(CP_UTF8, 0, name, -1, wide, 63);
    if (n > 0)
    {
        wide[n - 1] = 0;
        SetThreadDescription(handle, wide);
    }
}

#endif // _WIN32

namespace {

struct ThreadStartArgs
{
    Function<void()> Entry;
    const char* Name;
    ThreadPriority Priority;
};

void ThreadEntryPoint(ThreadStartArgs* args)
{
    if (args->Name)
    {
        Thread::SetCurrentThreadName(args->Name);
    }
    Thread::SetCurrentThreadPriority(args->Priority);
    args->Entry();
    delete args;
}

} // namespace

Thread::Thread()
{
    new (&mStorage[0]) std::thread();
}

Thread::~Thread()
{
    AETHER_ASSERT(!ThreadImpl::Get(*this).joinable());
    ThreadImpl::Get(*this).~thread();
}

void Thread::Run(Function<void()> entry, const char* name, ThreadPriority priority)
{
    std::thread& thread = ThreadImpl::Get(*this);
    AETHER_ASSERT(!thread.joinable());
    thread.~thread();

    auto* args = new ThreadStartArgs{std::move(entry), name, priority};
    new (&mStorage[0]) std::thread(ThreadEntryPoint, args);
}

bool Thread::Joinable() const
{
    return ThreadImpl::Get(const_cast<Thread&>(*this)).joinable();
}

void Thread::Join()
{
    std::thread& thread = ThreadImpl::Get(*this);
    if (thread.joinable())
    {
        thread.join();
    }
}

void Thread::Detach()
{
    std::thread& thread = ThreadImpl::Get(*this);
    if (thread.joinable())
    {
        thread.detach();
    }
}

void Thread::SetCurrentThreadName(const char* name)
{
#ifdef _WIN32
    SetThreadNameWin32(GetCurrentThread(), name);
#else
    AETHER_UNUSED(name);
#endif
}

void Thread::SetCurrentThreadPriority(ThreadPriority priority)
{
#ifdef _WIN32
    ApplyThreadPriorityWin32(GetCurrentThread(), priority);
#else
    AETHER_UNUSED(priority);
#endif
}

void Thread::SleepMillis(u32 millis)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(millis));
}

void Thread::YieldCpu()
{
    std::this_thread::yield();
}

// ============================================================
// Mutex
// ============================================================

struct MutexImpl
{
    static std::mutex& Get(Mutex& mutex)
    {
        static_assert(sizeof(std::mutex) <= sizeof(mutex.mStorage),
                      "Mutex storage too small for std::mutex");
        return *reinterpret_cast<std::mutex*>(&mutex.mStorage[0]);
    }
};

Mutex::Mutex()
{
    new (&mStorage[0]) std::mutex();
}

Mutex::~Mutex()
{
    MutexImpl::Get(*this).~mutex();
}

void Mutex::Lock()
{
    MutexImpl::Get(*this).lock();
}

void Mutex::Unlock()
{
    MutexImpl::Get(*this).unlock();
}

bool Mutex::TryLock()
{
    return MutexImpl::Get(*this).try_lock();
}

// ============================================================
// Event
// ============================================================

#ifdef _WIN32

struct EventImpl
{
    static HANDLE Get(Event& event)
    {
        static_assert(sizeof(HANDLE) <= sizeof(event.mStorage),
                      "Event storage too small for HANDLE");
        return *reinterpret_cast<HANDLE*>(&event.mStorage[0]);
    }
};

Event::Event()
{
    *reinterpret_cast<HANDLE*>(&mStorage[0]) =
        CreateEventW(nullptr, FALSE, FALSE, nullptr);
}

Event::~Event()
{
    HANDLE handle = EventImpl::Get(*this);
    if (handle)
    {
        CloseHandle(handle);
    }
}

void Event::Signal()
{
    SetEvent(EventImpl::Get(*this));
}

void Event::Reset()
{
    ResetEvent(EventImpl::Get(*this));
}

bool Event::Wait(u32 timeoutMillis)
{
    HANDLE handle = EventImpl::Get(*this);
    if (!handle)
    {
        return false;
    }
    DWORD ms = timeoutMillis == INFINITE_TIMEOUT ? INFINITE : timeoutMillis;
    return WaitForSingleObject(handle, ms) == WAIT_OBJECT_0;
}

void* Event::NativeHandle()
{
    return EventImpl::Get(*this);
}

#else // !_WIN32: condvar emulation of an auto-reset event

namespace {

struct EventData
{
    std::mutex Mutex;
    std::condition_variable Cond;
    bool Signaled = false;
};

} // namespace

struct EventImpl
{
    static EventData& Get(Event& event)
    {
        static_assert(sizeof(EventData) <= sizeof(event.mStorage),
                      "Event storage too small for EventData");
        return *reinterpret_cast<EventData*>(&event.mStorage[0]);
    }
};

Event::Event()
{
    new (&mStorage[0]) EventData();
}

Event::~Event()
{
    EventImpl::Get(*this).~EventData();
}

void Event::Signal()
{
    EventData& data = EventImpl::Get(*this);
    std::lock_guard<std::mutex> lock(data.Mutex);
    data.Signaled = true;
    data.Cond.notify_one();
}

void Event::Reset()
{
    EventData& data = EventImpl::Get(*this);
    std::lock_guard<std::mutex> lock(data.Mutex);
    data.Signaled = false;
}

bool Event::Wait(u32 timeoutMillis)
{
    EventData& data = EventImpl::Get(*this);
    std::unique_lock<std::mutex> lock(data.Mutex);
    while (!data.Signaled)
    {
        if (timeoutMillis == INFINITE_TIMEOUT)
        {
            data.Cond.wait(lock);
        }
        else if (data.Cond.wait_for(lock, std::chrono::milliseconds(timeoutMillis)) ==
                 std::cv_status::timeout)
        {
            return false;
        }
    }
    data.Signaled = false;
    return true;
}

void* Event::NativeHandle()
{
    return nullptr;
}

#endif // _WIN32

// ============================================================
// CpuInfo
// ============================================================

namespace CpuInfo {

u32 LogicalCoreCount()
{
    u32 count = std::thread::hardware_concurrency();
    return count > 0 ? count : 1;
}

u32 CacheLineSize()
{
#if defined(__cpp_lib_hardware_interference_size)
    if constexpr (std::hardware_destructive_interference_size != 0)
    {
        return (u32)std::hardware_destructive_interference_size;
    }
#endif
    // All current targets (x64, arm64, wasm) use 64-byte lines.
    return CACHE_LINE_SIZE;
}

} // namespace CpuInfo

} // namespace Aether::Platform
