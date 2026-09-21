#pragma once

#include "Core.h"

#include <type_traits>
#include <utility>

namespace Aether {

//
// Move-only callable wrapper (replaces std::function). Stores any copyable
// closure in a heap cell.
//
template<typename Sig>
class Function;

template<typename R, typename... Args>
class Function<R(Args...)>
{
public:
    Function() = default;

    Function(std::nullptr_t) {}

    template<typename F> requires std::is_invocable_r_v<R, F&, Args...> && (!std::is_same_v<std::decay_t<F>, Function>)
    Function(F&& callable)
        : mImpl(new Impl<std::decay_t<F>>(std::forward<F>(callable)))
    {
    }

    Function(Function&& other) noexcept
        : mImpl(other.mImpl)
    {
        other.mImpl = nullptr;
    }

    Function& operator=(Function&& other) noexcept
    {
        if (this != &other)
        {
            delete mImpl;
            mImpl = other.mImpl;
            other.mImpl = nullptr;
        }
        return *this;
    }

    Function(const Function&) = delete;
    Function& operator=(const Function&) = delete;

    ~Function()
    {
        delete mImpl;
    }

    R Invoke(Args... args) const
    {
        AETHER_ASSERT(mImpl);
        return mImpl->Invoke(std::forward<Args>(args)...);
    }

    R operator()(Args... args) const
    {
        return Invoke(std::forward<Args>(args)...);
    }

    bool IsValid() const { return mImpl != nullptr; }
    explicit operator bool() const { return mImpl != nullptr; }

private:
    struct Base
    {
        virtual ~Base() = default;
        virtual R Invoke(Args... args) = 0;
    };

    template<typename F>
    struct Impl : Base
    {
        explicit Impl(F&& f)
            : Callable(std::move(f))
        {
        }

        explicit Impl(const F& f)
            : Callable(f)
        {
        }

        R Invoke(Args... args) override
        {
            return Callable(std::forward<Args>(args)...);
        }

        F Callable;
    };

    Base* mImpl = nullptr;
};

}
