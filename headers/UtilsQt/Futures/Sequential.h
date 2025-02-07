/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <algorithm>
#include <cassert>
#include <exception>
#include <mutex>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <variant>

#include <UtilsQt/Futures/Utils.h>

#include <utils-cpp/default_ctor_ops.h>
#include <utils-cpp/threadid.h>
#include <utils-cpp/scoped_guard.h>
#include <utils-cpp/function_traits.h>

/*
          Description
-------------------------------
  The UtilsQt::Sequential utility simplifies managing sequential asynchronous operations
  in a clear and structured manner. It allows chaining multiple asynchronous steps (QFutures)
  where the output of one step feeds into the next.

  Under regular conditions:
  1. Each handler in the chain is executed sequentially, passing the result (AsyncResult)
     of the previous step to the next.
  2. Execution proceeds to completion unless interrupted by external cancellation or
     internal exceptions/cancellations (if configured to handle them).
  3. Users can optionally access cancellation state via the SequentialMediator object during
     each step to cancel asynchronous operations if requested.

  This design makes it easy to compose complex asynchronous workflows while maintaining
  readable and maintainable code.

  Schematic example:
  QFuture<L> f = UtilsQt::Sequential(this)
    .start([]()                                  -> QFuture<T>    { ... })
    .then([](AsyncResult<T>, SequentialMediator) -> QFuture<G>    { ... })
    .then([](AsyncResult<G>)                     -> QFuture<void> { ... })
    .then([](AsyncResult<void>)                  -> QFuture<L>    { ... })
    .execute();

  Common usage example:
   /
  |  auto f = UtilsQt::Sequential(this)
  |  .start([](const SequentialMediator& c) -> QFuture<int> {
  |      return UtilsQt::createReadyFuture(123);
  |  })
  |  .then([](const AsyncResult<int>& result) {
  |      result.tryRethrow(); // Propagate exceptions, if we want.
  |
  |      if (result.isCanceled()) {
  |          return QFuture<void>(); // Propagate cancellation, if we want.
  |      }
  |
  |      return UtilsQt::createReadyFuture(QString::number(result.value()));
  |  }
  |  .then([](const AsyncResult<QString>& result, SequentialMediator& sm) {
  |      result.tryRethrow(); // Propagate exceptions, if we want.
  |
  |      if (result.isCanceled()) {
  |          return QFuture<void>(); // Propagate cancellation, if we want.
  |      }
  |
  |      Promise<int> promise(true);
  |
  |      // Long-running operation
  |      auto fThr = QtConcurrent::run([promise, sm, result](){
  |          int x = result.value().toInt();
  |
  |          while (x < 1000000 && !c.isCancelRequested()) {
  |              x++;
  |          }
  |
  |          if (sm.isCancelRequested()) {
  |              p.cancel();
  |          } else {
  |              p.finish(x);
  |          }
  |      });
  |
  |      sm.registerAwaitable(fThr);
  |
  |      return promise.future();
  |  })
  |  .execute(savedAwaitables);
  \

  If a future from one handler is canceled, an empty AsyncResult is passed to next handler. This
  does not cancel the entire chain by default, allowing users to produce non-canceled futures in the sequence.

  The same applies to exceptions: if a handler raises an exception or sets an exception in the QFuture,
  the exception is passed to the next handler. By default, this does not terminate the chain, enabling
  users to handle exceptions and to continue the sequence.

  Each handler in the chain:
  - Receives the AsyncResult from the previous handler (contains result, canceled state or exception).
  - Optionally receives a SequentialMediator object, which provides:
     - Cancel checking: Detect whether cancellation was requested due to external events via `SequentialMediator::isCancelRequested`.
     - Event subscription: React to cancel events via `SequentialMediator::subscribe`.
       (Notice! Cancellation of the resulting future and deletion of the context inside `SequentialMediator::subscribe` is prohibited).

  SequentialMediator transitions to a "canceled" state in the following scenarios:
   - When the user explicitly cancels the resulting QFuture.
   - When the associated context is destroyed.
  Notice: after an external cancellation request, no further handlers in the chain will be invoked.

  The `Sequential` utility supports the following configuration options to modify its behavior:
  - AutoFinishOnCanceled
    Automatically cancels the entire chain if any future in the sequence is canceled.
    In this mode, no subsequent handlers in the chain are executed after the handler that returns a canceled future.

  - AutoFinishOnException
    Automatically finishes the chain if any handler operation raises an exception.
    In this mode, no handlers are executed after the one that encounters an exception.

       Cautions!
  ------------------
  - Avoid long-running operations in handlers, as they execute in the main thread.
  - For long-running tasks, consider using `QtConcurrent::run` within the handler.
  - If a handler starts a thread, Sequential does not guarantee that the context will remain live for the thread.
    To ensure safety:
     - Capture SequentialMediator and all necessary data by value so the thread has its own copy or shared pointer.
     - Call `SequentialMediator::registerAwaitable` to register QFuture of true asynchronous operation.
     - Pass the `Awaitables` object via reference to `execute` method to save the `Awaitables` object.
     - Call `Awaitables::confirmWait` in destructor of context (`this`).
*/

namespace UtilsQt {

namespace Futures_Seq_Internal {

class Awaitables
{
    friend class SequentialMediator;
public:
    struct AwaitableData
    {
        std::function<void()> expectant;
        std::function<bool()> checker;
    };

    Awaitables() = default;
    ~Awaitables() = default;
    Awaitables(const Awaitables&) = delete;

    Awaitables(Awaitables&& other) noexcept
        : m_anchor(std::move(other.m_anchor)),
          m_weakData(m_anchor)
    { }

    Awaitables& operator=(const Awaitables&) = delete;

    Awaitables& operator=(Awaitables&& other) noexcept
    {
        if (this != &other) {
            m_anchor = std::move(other.m_anchor);
            m_weakData = m_anchor;
        }

        return *this;
    }

    void confirmWait()
    {
        auto data = lock();
        //assert(!data->confirmed && "Don't confirm `Awaitables` twice!");
        data->confirmed = true;
    }

    void wait()
    {
        auto data = lock();
        //assert(!data->confirmed && "Don't call `wait` after `confirmWait`!");
        data->confirmed = true;
        data->wait();
    }

    bool isRunning()
    {
        auto aws = lock()->awaitables;
        return std::any_of(aws.begin(), aws.end(), [](const AwaitableData& x) { return !x.checker || !x.checker(); });
    }

private: // For SequentialMediator
    Awaitables& operator+=(const AwaitableData& f)
    {
        auto data = lock();
        assert(!data->confirmed && "Don't add `Awaitable` after `confirmWait`!");

        auto& aws = data->awaitables;
        aws.erase(std::remove_if(aws.begin(), aws.end(), [](const AwaitableData& x) { return x.checker && x.checker(); }), aws.end());

        data->awaitables += f;

        return *this;
    }

    bool isMoved() const
    {
        return !m_anchor;
    }

    bool isEmpty() const
    {
        return lock()->awaitables.isEmpty();
    }

private:
    struct Data {
        const uintmax_t mainThreadId { currentThreadId() };
        QVector<AwaitableData> awaitables;
        bool confirmed { false };

        void wait() {
            for (const auto& x : std::as_const(awaitables))
                x.expectant();

            awaitables.clear();
        }

        ~Data() {
            assert((confirmed || awaitables.isEmpty()) &&
                   "Seems you forgot to save `Awaitables` or to call `confirmWait`!");

            wait();
        }
    };

    std::shared_ptr<Data> lock()
    {
        auto ptr = m_weakData.lock();
        assert(ptr && "Awaitables is gone!");
        assert(currentThreadId() == ptr->mainThreadId &&
               "Awaitable-related functionality shouldn't be called from another thread!");
        return ptr;
    }

    std::shared_ptr<const Data> lock() const
    {
        auto ptr = m_weakData.lock();
        assert(ptr && "Awaitables is gone!");
        assert(currentThreadId() == ptr->mainThreadId &&
               "Awaitable-related functionality shouldn't be called from another thread!");
        return ptr;
    }

    std::shared_ptr<Data> m_anchor { std::make_shared<Data>() };
    std::weak_ptr<Data> m_weakData { m_anchor };
};

class SequentialMediator
{
    template<typename... Fs>
    friend class Executor;
public:
    using Handler = std::function<void()>;
    using AwaitableData = Awaitables::AwaitableData;

    SequentialMediator() = default;
    SequentialMediator(const SequentialMediator&) = default;
    SequentialMediator(SequentialMediator&&) = default;
    SequentialMediator& operator=(const SequentialMediator&) = default;
    SequentialMediator& operator=(SequentialMediator&&) = default;

    bool isCancelRequested() const // Called from another thread
    {
        std::lock_guard lock(m_data->mutex);
        return m_data->cancelRequested;
    }

    [[nodiscard]] auto onCancellation(const Handler& handler) const // Called from another thread
    {
        std::lock_guard lock(m_data->mutex);

        auto id = ++m_data->ids;
        assert (m_data->handlers.find(id) == m_data->handlers.end());

        auto unsubscribeGuard = CreateScopedGuard([this, id](){
            std::lock_guard lock(m_data->mutex);
            m_data->handlers.erase(id);
        });

        m_data->handlers[id] = handler;
        return unsubscribeGuard;
    }

    void registerAwaitable(QFuture<void> f)
    {
        registerAwaitable([f]() mutable { f.waitForFinished(); },
                          [f]() { return f.isFinished(); });
    }

    void registerAwaitable(const std::function<void()>& expectant, const std::function<bool()>& checker)
    {
        registerAwaitable(AwaitableData{expectant, checker});
    }

    void registerAwaitable(const AwaitableData& f)
    {
        m_data->awaitables += f;
    }

private: // For Executor
    void cancel()
    {
        std::lock_guard lock(m_data->mutex);

        if (m_data->cancelRequested)
            return;

        m_data->cancelRequested = true;

        for (const auto& [_, handler] : m_data->handlers)
            handler();
    }

    bool hasAwaitables() const
    {
        return !m_data->awaitables.isEmpty();
    }

    Awaitables moveAwaitables() const
    {
        assert(!m_data->awaitables.isMoved());
        return std::move(m_data->awaitables);
    }

private:
    struct Data {
        std::mutex mutex;
        bool cancelRequested {false};
        int ids {0};
        std::unordered_map<int, Handler> handlers;
        Awaitables awaitables;
    };

    std::shared_ptr<Data> m_data { std::make_shared<Data>() };
};

struct VoidType
{
    bool operator==(const VoidType&) const { return true; }
    bool operator!=(const VoidType&) const { return false; }
};

template<typename T>
struct AsyncResultBase
{
    using Variant = std::variant<
        std::monostate,       // Empty result (canceled future)
        T,                    // Result
        std::exception_ptr>;  // Exception caught in handler

    AsyncResultBase() = default;
    AsyncResultBase(const Variant& value) : result(value) { }
    AsyncResultBase(Variant&& value) : result(std::move(value)) { }

    AsyncResultBase(const AsyncResultBase& other) = default;
    AsyncResultBase(AsyncResultBase&& other) = default;
    AsyncResultBase& operator=(const AsyncResultBase& other) = default;
    AsyncResultBase& operator=(AsyncResultBase&& other) = default;

    bool operator==(const AsyncResultBase& other) const { return result == other.result; }
    bool operator!=(const AsyncResultBase& other) const { return !(*this == other); }

    bool isCanceled() const { return std::holds_alternative<std::monostate>(result); }
    bool hasValue() const { return std::holds_alternative<T>(result); }
    bool hasException() const { return std::holds_alternative<std::exception_ptr>(result); }

    std::exception_ptr exception() const { return hasException() ? std::get<std::exception_ptr>(result) : std::exception_ptr(); }
    [[noreturn]] void rethrow() const { assert(hasException()); std::rethrow_exception(exception()); }
    void tryRethrow() const { if (hasException()) std::rethrow_exception(exception()); }

protected:
    Variant result {std::monostate{}};
};

template<typename T>
struct AsyncResult : public AsyncResultBase<T>
{
    using AsyncResultBase<T>::AsyncResultBase;

    T& value() { assert(this->hasValue()); return std::get<T>(this->result); }
    const T& value() const { assert(this->hasValue()); return std::get<T>(this->result); }
    std::optional<T> valueTry() const { return this->hasValue() ? std::optional<T>(value()) : std::nullopt; }
    T valueOr(const T& def) const { return valueTry().value_or(def); }

    T* operator->() { return &value(); }
    const T* operator->() const { return &value(); }
    T& operator*() { return value(); }
    const T& operator*() const { return value(); }
};

template<>
struct AsyncResult<void> : public AsyncResultBase<VoidType>
{
    using AsyncResultBase<VoidType>::AsyncResultBase;
};

template<typename T>
struct QFutureUnwrap { };

template<typename T>
struct QFutureUnwrap<QFuture<T>> { using type = T; };

template<typename T>
struct IsQFuture : std::false_type { };

template<typename T>
struct IsQFuture<QFuture<T>> : std::true_type { };

enum SequentialOptions
{
    Default = 0,
    AutoFinishOnCanceled = 1 << 0,
    AutoFinishOnException = 1 << 1,
    AutoFinish = AutoFinishOnCanceled | AutoFinishOnException
};

struct Settings
{
    QObject* context {};
    SequentialOptions options {Default};
};

template<typename... Fs>
class Executor : public QObject
{
public:
    using Tuple = std::tuple<Fs...>;
    static constexpr size_t Length = sizeof...(Fs);
    using LastFunc = std::tuple_element_t<Length - 1, Tuple>;
    using LastFuncQFuture = typename function_traits<LastFunc>::return_type;
    using LastFuncResult = typename QFutureUnwrap<LastFuncQFuture>::type;
    using LastAsyncResult = AsyncResult<LastFuncResult>;

    Executor(Settings&& settings, Tuple&& tuple)
        : m_settings(std::move(settings)),
          m_handlers(std::move(tuple))
    {
        if (m_settings.context)
            QObject::connect(m_settings.context, &QObject::destroyed, this, [this](){ cancel(); });

        UtilsQt::onCancelNotified(m_promise.future(), this, [this](){ cancel(); });
        UtilsQt::onFinishedNP(m_promise.future(), this, &QObject::deleteLater);
    }

    template<size_t I, typename... Args>
    void call(Args&&... args) // AsyncResult<T-1> is passed, when I > 0.
                              // sizeof...(args) always <= 1.
    {
        static_assert(sizeof...(Args) <= 1, "Only one optional argument is possible!");

        // If promise is already finished, don't call handlers anymore.
        if (m_promise.isFinished())
            return;

        // Last result obtained, report finish.
        if constexpr (I == Length) {
            AsyncResult<LastFuncResult> lastAsyncResult = std::move(args...);

            if (m_sequentialMediator.isCancelRequested()) {
                m_promise.cancel();

            } else if (lastAsyncResult.hasException()) {
                m_promise.finishWithException(lastAsyncResult.exception());

            } else if (lastAsyncResult.isCanceled()) {
                m_promise.cancel();

            } else {
                assert(lastAsyncResult.hasValue());

                if constexpr (std::is_same_v<LastFuncResult, void>) {
                    m_promise.finish();
                } else {
                    m_promise.finish(lastAsyncResult.value());
                }
            }

            // return;

        } else { // I < Length
            using Func = std::tuple_element_t<I, Tuple>;                           // []([const T&]?, SequentialMediator&) -> QFuture<T> {}
            using Ret = std::invoke_result_t<Func, Args..., SequentialMediator&>;  // QFuture<T>
            using T = typename QFutureUnwrap<Ret>::type;                           // T
            using QFutureT = Ret;

            // Report start
            if constexpr (I == 0) {
                assert(!m_sequentialMediator.isCancelRequested());
                m_promise.start();
            }

            if constexpr (sizeof...(args) == 1) { // I >= 1
                constexpr auto cref = [](const auto& x) -> const auto& { return x; };
                const auto& prevAsyncResult = cref(args...);

                // Check previous result according to options
                if ((m_settings.options & SequentialOptions::AutoFinishOnException) && prevAsyncResult.hasException()) {
                    m_promise.finishWithException(prevAsyncResult.exception());
                    return;
                }

                if ((m_settings.options & SequentialOptions::AutoFinishOnCanceled) && prevAsyncResult.isCanceled()) {
                    m_promise.cancel();
                    return;
                }
            }

            std::exception_ptr eptr;
            QFutureT fResult;

            try {
                fResult = std::get<I>(m_handlers)(args..., m_sequentialMediator);

                if (!fResult.isFinished())
                    m_futures.append(QFuture<void>(fResult));

            } catch (...) {
                eptr = std::current_exception();
            }

            if (eptr) {
                AsyncResult<T> asyncResult {eptr};
                call<I + 1>(std::move(asyncResult));

            } else {
                auto futureToResult = [fResult]() mutable {
                    assert(fResult.isFinished() && "We should get here only when future is finished!");

                    // Future can contain exception (and thus be cancelled as well).
                    // So check for exception first.
                    try {
                        fResult.waitForFinished(); // throws
                    } catch (...) {
                        return AsyncResult<T> {std::current_exception()};
                    }

                    // Check cancelled and possible result
                    if constexpr (std::is_same_v<T, void>) {
                        return fResult.isCanceled() ? AsyncResult<T>() :
                                                      AsyncResult<T> {VoidType()};
                    } else {
                        return fResult.isCanceled() ? AsyncResult<T>() :
                                                      AsyncResult<T> {fResult.result()};
                    }
                };

                UtilsQt::onFinishedNP(fResult, this, [this, futureToResult]() mutable {
                    auto asyncResult = futureToResult();
                    call<I + 1>(std::move(asyncResult));
                });
            }
        } // if constexpr (I == Length), else
    }

    LastFuncQFuture execute()
    {
        assert(!m_sequentialMediator.hasAwaitables() && "You can't call `execute` without saving `Awaitables`!");

        call<0>();
        return m_promise.future();
    }

    LastFuncQFuture execute(Awaitables& awaitables)
    {
        awaitables = m_sequentialMediator.moveAwaitables();
        call<0>();
        return m_promise.future();
    }

    void cancel()
    {
        m_sequentialMediator.cancel();

        if (!m_promise.isFinished())
            m_promise.cancel();

        for (auto& f : m_futures)
            f.cancel();
    }

private:
    Settings m_settings;
    Tuple m_handlers;
    QVector<QFuture<void>> m_futures;
    SequentialMediator m_sequentialMediator;
    Promise<LastFuncResult> m_promise;
};

template<typename T, typename... Fs>
class SequentialPart
{
public:
    template<typename CT, typename... CFs>
    static auto create(Settings&& settings, CFs&&... fs)
    {
        return SequentialPart<CT, CFs...>(std::move(settings), std::forward<CFs>(fs)...);
    }

    SequentialPart(Settings&& settings, Fs&&... fs)
        : m_settings(std::move(settings)),
          m_handlers(std::make_tuple(std::forward<std::decay_t<Fs>>(fs)...))
    { }

    template<typename F>
    [[nodiscard]] auto then(F&& f)
    {
        using Func = decltype(std::function(std::move(std::declval<decltype(f)>())))*;
        return thenImpl(std::forward<F>(f), Func());
    }

    [[nodiscard]] QFuture<T> execute()
    {
        auto executor = new Executor(std::move(m_settings), std::move(m_handlers)); // "detached" lifetime
        return executor->execute();
    }

    [[nodiscard]] QFuture<T> execute(Awaitables& awaitables)
    {
        auto executor = new Executor(std::move(m_settings), std::move(m_handlers)); // "detached" lifetime
        return executor->execute(awaitables);
    }

private:
    template<typename F, typename R>
    auto thenImpl(F&& f, std::function<QFuture<R>(const AsyncResult<T>&)>*)
    {
        auto f2 = [f = std::forward<F>(f)](const AsyncResult<T>& ar, SequentialMediator&) mutable { return f(ar); };
        using Func = decltype(std::function(std::move(std::declval<decltype(f2)>())))*;
        return thenImpl(std::move(f2), Func());
    }

    template<typename F, typename R>
    auto thenImpl(F&& f, std::function<QFuture<R>(const AsyncResult<T>&, SequentialMediator)>*)
    {
        return std::apply(
            [this, f = std::forward<F>(f)](auto&... xs) mutable {
                return create<R>(std::move(m_settings), std::move(xs)..., std::forward<F>(f));
            },
            m_handlers);
    }

    template<typename F, typename R>
    auto thenImpl(F&& f, std::function<QFuture<R>(const AsyncResult<T>&, const SequentialMediator&)>*)
    {
        return std::apply(
            [this, f = std::forward<F>(f)](auto&... xs) mutable {
                return create<R>(std::move(m_settings), std::move(xs)..., std::forward<F>(f));
            },
            m_handlers);
    }

    template<typename F, typename R>
    auto thenImpl(F&& f, std::function<QFuture<R>(const AsyncResult<T>&, SequentialMediator&)>*)
    {
        return std::apply(
            [this, f = std::forward<F>(f)](auto&... xs) mutable {
                return create<R>(std::move(m_settings), std::move(xs)..., std::forward<F>(f));
            },
            m_handlers);
    }

private:
    Settings m_settings;
    std::tuple<std::decay_t<Fs>...> m_handlers;
};

class Sequential
{
    NO_COPY_MOVE(Sequential);
public:
    Sequential(QObject* context, SequentialOptions options = Default)
        : m_settings{context, options}
    { }

    template<typename F,
             typename = std::enable_if_t<!IsQFuture<std::decay_t<F>>::value>
             >
    [[nodiscard]] auto start(F&& f)
    {
        using Func = decltype(std::function(std::move(std::declval<decltype(f)>())))*;
        return startImpl(std::forward<F>(f), Func());
    }

    template<typename T>
    [[nodiscard]] auto start(const QFuture<T>& f)
    {
        return start([f](){ return f; });
    }

private:
    template<typename F, typename R>
    auto startImpl(F&& f, std::function<QFuture<R>()>*)
    {
        auto f2 = [f = std::forward<F>(f)](const SequentialMediator&){ return f(); };
        using Func = decltype(std::function(std::move(std::declval<decltype(f2)>())))*;
        return startImpl(std::move(f2), Func());
    }

    template<typename F, typename R>
    auto startImpl(F&& f, std::function<QFuture<R>(SequentialMediator)>*)
    {
        return SequentialPart<R, F>(std::move(m_settings), std::forward<F>(f));
    }

    template<typename F, typename R>
    auto startImpl(F&& f, std::function<QFuture<R>(const SequentialMediator&)>*)
    {
        return SequentialPart<R, F>(std::move(m_settings), std::forward<F>(f));
    }

    template<typename F, typename R>
    auto startImpl(F&& f, std::function<QFuture<R>(SequentialMediator&)>*)
    {
        return SequentialPart<R, F>(std::move(m_settings), std::forward<F>(f));
    }

private:
    Settings m_settings;
};

} // namespace Futures_Seq_Internal

using Sequential = Futures_Seq_Internal::Sequential;

template<typename T>
using AsyncResult = Futures_Seq_Internal::AsyncResult<T>;

using Awaitables = Futures_Seq_Internal::Awaitables;

using SequentialMediator = Futures_Seq_Internal::SequentialMediator;

using SequentialOptions = Futures_Seq_Internal::SequentialOptions;

} // namespace UtilsQt
