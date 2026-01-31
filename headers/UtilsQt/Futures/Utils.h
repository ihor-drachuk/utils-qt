/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <type_traits>
#include <optional>
#include <utility>
#include <tuple>
#include <memory>
#include <exception>
#include <chrono>
#include <QObject>
#include <QFuture>
#include <QFutureInterface>
#include <QFutureWatcher>
#include <QTimer>
#include <QException>

#include <utils-cpp/tuple_utils.h>
#include <UtilsQt/invoke_method.h>

/*
          Description
------------------------------

 * onFinished(QFuture<Type>, context, handler);

   QFuture<Type>  -->  std::optional<Type>  (always)

   QFuture<Type> is converted to std::optional<Type> and passed to
   handler, when future is finished.

   If T == void, then QFuture<Type> is converted to std::optional<std::monostate>.

 * onFinishedNP(QFuture<T>, context, handler);

   QFuture<Type>  -->  void  (always)

   Similar to onFinished, but it doesn't call `QFuture::result()` internally and
   doesn't pass it to handler. This is mitigation solution to work safely with
   futures which contain exception.


 * onResult(QFuture<T>, context, handler);

   QFuture<Type>  -->  Type  (only on success)


 * onCanceled(QFuture<T>, context, handler);

   QFuture<Type>  -->  void  (only on cancellation)

   Is called when future is canceled AND finished.


* onCancelNotified(QFuture<T>, context, handler);

  QFuture<Type>  -->  void  (only on cancellation)

  Is called when future is canceled (immediately).


 * waitForFuture<QEventLoop>(future);


 * QFuture<T> createReadyFuture<T>(T value);
 * QFuture<T> createCanceledFuture<T>();
 * QFuture<T> createExceptionFuture<T>(exception e);

 * QFuture<T> createTimedFuture<T>(std::chrono::milliseconds time, T value, QObject* ctx);
 * QFuture<T> createTimedCanceledFuture<T>(std::chrono::milliseconds time, QObject* ctx);
 * QFuture<T> createTimedExceptionFuture<T>(std::chrono::milliseconds time, exception e, QObject* ctx);

 * getFutureState(QFuture<T>) -> [NotStarted, Running, Completed, CompletedWrong, Canceled, Exception]

 * hasResult(QFuture<T>)

 * futureCompleted(QFuture<T>)

 * analyzeFutures(Container<QFuture<T>...>) ->
   [allFinished, someFinished, noneFinished, allCanceled, someCanceled, noneCanceled, allCompleted, someCompleted]

 * futuresToOptResults(Container<QFuture<T>...>) -> Container<std::optional<T>...>


 * Promise<T> createPromise<T>(autoStart = false);
     -> start();
     -> finish(T);
     -> cancel();
     -> finishWithException(std::exception or std::exception_ptr);

     -> isStarted();
     -> isRunning();
     -> isCanceled();
     -> isFinished();

     -> future();
*/

namespace UtilsQt {

namespace FutureUtilsInternals {

template <typename C, typename T>
void call(const C& c, const QFuture<T>& future)
{
    const_cast<C&>(c)(std::optional<T>(future.result()));
}

template <typename C>
void call(const C& c, const QFuture<void>&)
{
    const_cast<C&>(c)(std::optional<std::monostate>(std::monostate()));
}

template <typename Callable, typename T>
void callCanceled(const Callable& callable, const QFuture<T>&)
{
    const_cast<Callable&>(callable)(std::optional<T>{});
}

template <typename Callable>
void callCanceled(const Callable& callable, const QFuture<void>&)
{
    const_cast<Callable&>(callable)(std::optional<std::monostate>(std::nullopt));
}

template<typename T>
struct ResultContentTypeImpl;

template<typename T0, typename... Ts>
struct ResultContentTypeImpl<std::tuple<T0, Ts...>>
{
    using CurrentType = std::conditional_t<std::is_same_v<T0, void>, bool, std::optional<T0>>;
    using Type = typename tuple_cat_type<std::tuple<CurrentType>, typename ResultContentTypeImpl<std::tuple<Ts...>>::Type>::type;
};

template<>
struct ResultContentTypeImpl<std::tuple<>>
{
    using Type = std::tuple<>;
};

template<typename T>
void connectTimer(QTimer* timer, QObject* context, const T& handler)
{
    if (context) {
        QObject::connect(timer, &QTimer::timeout, context, handler);
    } else {
        QObject::connect(timer, &QTimer::timeout, handler);
    }
}

} // namespace FutureUtilsInternals

enum class FutureState
{
    NotStarted,
    Running,
    Completed,
    CompletedWrong,
    Canceled,
    Exception
};

struct FuturesSetProperties
{
    bool allFinished {};
    bool someFinished {};
    bool noneFinished {};

    bool allCanceled {};
    bool someCanceled {};
    bool noneCanceled {};

    bool allCompleted {};  // Finished & !Canceled
    bool someCompleted {};

    [[nodiscard]] static FuturesSetProperties GetBoolFriendly() {
        return {true, false, true, true, false, true, true, false};
    }

    auto asTuple() const { return std::make_tuple(allFinished, someFinished, noneFinished,
                                                  allCanceled, someCanceled, noneCanceled,
                                                  allCompleted, someCompleted); }

    bool operator== (const FuturesSetProperties& rhs) const { return asTuple() == rhs.asTuple(); }
};

class QExceptionPtr : public QException
{
public:
    QExceptionPtr(const std::exception_ptr& eptr) : m_eptr(eptr) {} // NOLINT(bugprone-throw-keyword-missing)
    QExceptionPtr(std::exception_ptr&& eptr) : m_eptr(std::move(eptr)) {} // NOLINT(bugprone-throw-keyword-missing)
    QExceptionPtr(const QExceptionPtr&) = delete;
    QExceptionPtr(QExceptionPtr&&) noexcept = default;

    void raise() const override { if (m_eptr) std::rethrow_exception(m_eptr); }
    QException* clone() const override { return new QExceptionPtr(m_eptr); }

private:
    std::exception_ptr m_eptr;
};

template<typename T>
class Promise
{
public:
    Promise(bool autoStartFuture = false)
    {
        if (autoStartFuture)
            m_interface.reportStarted();

        m_contextTracker = std::shared_ptr<void>(reinterpret_cast<void*>(1), [f = m_interface](void*) mutable {
            if (!f.isFinished()) {
                f.reportCanceled();
                f.reportFinished();
            }
        });
    }

    Promise(const Promise<T>&) = default;
    Promise(Promise<T>&&) noexcept = default;
    ~Promise() noexcept = default;
    Promise<T>& operator= (const Promise<T>&) = default;
    Promise<T>& operator= (Promise<T>&&) noexcept = default;

    Promise& start()
    {
        assert(!isStarted());
        assert(!isCanceled());
        m_interface.reportStarted();
        assert(isStarted());
        return *this;
    }

    // if [T == void]
    template<typename X = T>
    typename std::enable_if<std::is_same<X, void>::value && std::is_same<X, T>::value, Promise&>::type
    finish()
    {
        assert(isStarted() || isCanceled());
        assert(!isFinished());

        m_interface.reportFinished();
        return *this;
    }

    // if [T != void]
    template<typename X = T>
    typename std::enable_if<!std::is_same<X, void>::value && std::is_same<X, T>::value, Promise&>::type
    finish(const X& result)
    {
        assert(isStarted() || isCanceled());
        assert(!isFinished());

        m_interface.reportResult(result);
        m_interface.reportFinished();
        return *this;
    }

    Promise& finishWithException(const std::exception_ptr& e)
    {
        assert(isStarted() || isCanceled());
        assert(!isFinished());

        m_interface.reportException(QExceptionPtr(e));
        m_interface.reportFinished();
        return *this;
    }

    template<typename Ex,
             typename = std::enable_if_t<std::is_base_of_v<std::exception, Ex>>
             >
    Promise& finishWithException(const Ex& ex)
    {
        finishWithException(std::make_exception_ptr(ex));
        return *this;
    }

    Promise& cancel()
    {
        assert(!isFinished());

        m_interface.reportCanceled();
        m_interface.reportFinished();
        return *this;
    }

    bool isStarted() const { return m_interface.isStarted(); }
    bool isRunning() const { return m_interface.isRunning(); }
    bool isCanceled() const { return m_interface.isCanceled(); }
    bool isFinished() const { return m_interface.isFinished(); }

    QFuture<T> future() const { return m_interface.future(); }

private:
    mutable QFutureInterface<T> m_interface;
    std::shared_ptr<void> m_contextTracker;
};

template<typename Type, typename Obj, typename Callable,
         typename std::enable_if<std::is_base_of<QObject, Obj>::value, int>::type = 0>
void onFinished(const QFuture<Type>& future,
                Obj* context,
                const Callable& callable,
                Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    assert(context);

    auto resultHandler = [future, callable] () {
        if (future.isCanceled()) {
            FutureUtilsInternals::callCanceled(callable, future);
        } else {
            FutureUtilsInternals::call(callable, future);
        }
    };

    if (future.isFinished()) {
        // If already done
        UtilsQt::invokeMethod(context, resultHandler, connectionType);
    } else {
        // If not finished yet...
        auto watcherPtr = new QFutureWatcher<Type>();
        QObject::connect(context, &QObject::destroyed, watcherPtr, &QObject::deleteLater, connectionType);
        QObject::connect(watcherPtr, &QFutureWatcherBase::finished, context, [watcherPtr, resultHandler]() {
            resultHandler();
            watcherPtr->deleteLater();
        },
        connectionType);

        watcherPtr->setFuture(future);
    }
}


template<typename Type, typename Obj,
         typename std::enable_if<std::is_base_of<QObject, Obj>::value, int>::type = 0>
void onFinished(const QFuture<Type>& future,
                Obj* object,
                void (Obj::* member)(const std::optional<Type>&),
                Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    assert(object);
    assert(member);

    auto callable = [object, member](const std::optional<Type>& param){ (object->*member)(param); };

    onFinished(future, object, callable, connectionType);
}


template<typename Type, typename Callable>
void onFinishedNP(const QFuture<Type>& future,
                  QObject* context,
                  const Callable& callable,
                  Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    assert(context);

    auto callable2 = [context, callable](){
        if constexpr (std::is_invocable_v<Callable>) {
            return callable;

        } else {
            static_assert(std::is_member_function_pointer_v<Callable>);

            assert(context && "'context' is required for member function pointer!");

            return [context, callable = std::move(callable)]() mutable { (context->*callable)(); };
        }
    }();

    if (future.isFinished()) {
        // If already done
        UtilsQt::invokeMethod(context, std::move(callable2), connectionType);
    } else {
        // If not finished yet...
        auto watcherPtr = new QFutureWatcher<Type>();

        QObject::connect(context, &QObject::destroyed, watcherPtr, &QObject::deleteLater, connectionType);

        QObject::connect(watcherPtr, &QFutureWatcherBase::finished, context, [watcherPtr, callable2 = std::move(callable2)]() mutable {
            callable2();
            watcherPtr->deleteLater();
        },
        connectionType);

        watcherPtr->setFuture(future);
    }
}


template<typename Type, typename Obj, typename Callable,
         typename std::enable_if<std::is_base_of<QObject, Obj>::value, int>::type = 0>
void onResult(const QFuture<Type>& future,
              Obj* context,
              const Callable& callable,
              Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    assert(context);

    if constexpr (std::is_same<Type,void>::value) {
        onFinished(future, context, [future, callable](const auto&) {
            if (!future.isCanceled())
                callable();
        }, connectionType);
    } else {
        onFinished(future, context, [callable](const std::optional<Type>& value) {
            if (value)
                callable(*value);
        }, connectionType);
    }
}


template<typename Type, typename Context, typename Obj, typename SArg,
         typename std::enable_if<std::is_base_of<QObject, Context>::value &&
                                 std::is_base_of<QObject, Obj>::value &&
                                 (std::is_same_v<Context, Obj> || std::is_base_of_v<Obj, Context>),
                                 int>::type = 0>
void onResult(const QFuture<Type>& future,
              Context* object,
              void (Obj::* member)(SArg),
              Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    assert(object);
    assert(member);

    if constexpr (std::is_same<Type,void>::value) {
        auto callable = [object, member](){ (object->*member)(); };
        onResult(future, object, callable, connectionType);
    } else {
        auto callable = [object, member](const Type& param){ (object->*member)(param); };
        onResult(future, object, callable, connectionType);
    }
}


template<typename Type, typename Obj, typename Callable,
         typename std::enable_if<std::is_base_of<QObject, Obj>::value, int>::type = 0>
void onCanceled(const QFuture<Type>& future,
                Obj* context,
                const Callable& callable,
                Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    if constexpr (std::is_same<Type, void>::value) {
        onFinished(future, context, [callable, future](const auto&) {
            if (future.isCanceled())
                callable();
        }, connectionType);
    } else {
        onFinished(future, context, [callable](const std::optional<Type>& value) {
            if (!value)
                callable();
        }, connectionType);
    }
}


template<typename Type, typename Obj,
         typename std::enable_if<std::is_base_of<QObject, Obj>::value, int>::type = 0>
void onCanceled(const QFuture<Type>& future,
                Obj* object,
                void (Obj::* member)(),
                Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    assert(object);
    assert(member);

    auto callable = [object, member](){ (object->*member)(); };

    onCanceled(future, object, callable, connectionType);
}


template<typename Type, typename Callable>
void onCancelNotified(const QFuture<Type>& future,
                QObject* context,
                const Callable& callable,
                Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    assert(context);

    auto callable2 = [context, callable](){
        if constexpr (std::is_invocable_v<Callable>) {
            return callable;

        } else {
            static_assert(std::is_member_function_pointer_v<Callable>);

            assert(context && "'context' is required for member function pointer!");

            return [context, callable = std::move(callable)]() mutable { (context->*callable)(); };
        }
    }();

    if (future.isCanceled()) {
        UtilsQt::invokeMethod(context, callable2, connectionType);

    } else {
        auto watcherPtr = new QFutureWatcher<Type>();

        QObject::connect(context, &QObject::destroyed, watcherPtr, &QObject::deleteLater, connectionType);

        QObject::connect(watcherPtr, &QFutureWatcherBase::canceled, context, [watcherPtr, callable2]() {
            callable2();
            watcherPtr->deleteLater();
        }, connectionType);

        QObject::connect(watcherPtr, &QFutureWatcherBase::finished, watcherPtr, &QObject::deleteLater);
        watcherPtr->setFuture(future);
    }
}


template<typename EventLoopType, typename T>
void waitForFuture(const QFuture<T>& future, const std::optional<unsigned>& timeout = {})
{
    if (future.isFinished())
        return;

    std::unique_ptr<QTimer> timer;
    EventLoopType eventLoop;
    QFutureWatcher<T> futureWatcher;
    QObject::connect(&futureWatcher, &QFutureWatcherBase::finished, &eventLoop, &EventLoopType::quit);

    if (timeout) {
        timer = std::make_unique<QTimer>();
        QObject::connect(&*timer, &QTimer::timeout, &eventLoop, &EventLoopType::quit);
        timer->setSingleShot(true);
        timer->start(*timeout);
    }

    futureWatcher.setFuture(future);
    eventLoop.exec();

    if (!timeout)
        assert(future.isFinished());
}


template<typename EventLoopType, typename T>
std::optional<T> waitForFutureRet(const QFuture<T>& future, const std::optional<unsigned>& timeout = {})
{
    waitForFuture<EventLoopType, T>(future, timeout);

    if (!future.isFinished() || future.isCanceled())
        return {};

    return future.result();
}


template<typename T>
[[nodiscard]] QFuture<T> createReadyFuture(const T& value)
{
    return Promise<T>(true).finish(value).future();
}

[[nodiscard]] static QFuture<void> createReadyFuture()
{
    return Promise<void>(true).finish().future();
}


template<typename T>
[[nodiscard]] QFuture<T> createCanceledFuture()
{
    return Promise<T>(true).cancel().future();
}


template<typename T>
[[nodiscard]] QFuture<T> createTimedFuture(std::chrono::milliseconds time, const T& value, QObject* ctx = nullptr)
{
    if (time.count() <= 0)
        return createReadyFuture(value);

    Promise<T> promise(true);

    auto timer = new QTimer();
    FutureUtilsInternals::connectTimer(timer, ctx, [timer, promise, value]() mutable {
        promise.finish(value);
        timer->deleteLater();
    });
    timer->setSingleShot(true);
    timer->start(time);

    if (ctx)
        QObject::connect(ctx, &QObject::destroyed, timer, &QObject::deleteLater);

    return promise.future();
}

template<typename T>
[[nodiscard]] QFuture<T> createTimedFutureRef(std::chrono::milliseconds time, const T& value, QObject* ctx)
{
    assert(ctx);

    if (time.count() <= 0)
        return createReadyFuture(value);

    Promise<T> promise(true);

    auto timer = new QTimer();
    FutureUtilsInternals::connectTimer(timer, ctx, [timer, promise, &value]() mutable {
        promise.finish(value);
        timer->deleteLater();
    });
    timer->setSingleShot(true);
    timer->start(time);

    QObject::connect(ctx, &QObject::destroyed, timer, &QObject::deleteLater);

    return promise.future();
}

[[nodiscard]] static inline QFuture<void> createTimedFuture(std::chrono::milliseconds time, QObject* ctx = nullptr)
{
    if (time.count() <= 0)
        return createReadyFuture();

    Promise<void> promise(true);

    auto timer = new QTimer();

    FutureUtilsInternals::connectTimer(timer, ctx, [timer, promise]() mutable {
        promise.finish();
        timer->deleteLater();
    });

    timer->setSingleShot(true);
    timer->start(time);

    if (ctx)
        QObject::connect(ctx, &QObject::destroyed, timer, &QObject::deleteLater);

    return promise.future();
}


template<typename T>
[[nodiscard]] QFuture<T> createTimedCanceledFuture(std::chrono::milliseconds time, QObject* ctx = nullptr)
{
    if (time.count() <= 0)
        return createCanceledFuture<T>();

    Promise<T> promise(true);

    auto timer = new QTimer();
    auto handler = [timer, promise]() mutable {
        promise.cancel();
        timer->deleteLater();
    };

    FutureUtilsInternals::connectTimer(timer, ctx, handler);
    timer->setSingleShot(true);
    timer->start(time);

    if (ctx)
        QObject::connect(ctx, &QObject::destroyed, timer, &QObject::deleteLater);

    return promise.future();
}

template<typename T>
[[nodiscard]] QFuture<T> createExceptionFuture(const std::exception_ptr& eptr)
{
    return Promise<T>(true).finishWithException(eptr).future();
}

template<typename T,
         typename Ex,
         typename = std::enable_if_t<std::is_base_of_v<std::exception, Ex>>
         >
[[nodiscard]] QFuture<T> createExceptionFuture(const Ex& ex)
{
    return createExceptionFuture<T>(std::make_exception_ptr(ex));
}

template<typename T>
[[nodiscard]] QFuture<T> createTimedExceptionFuture(std::chrono::milliseconds time, const std::exception_ptr& eptr, QObject* ctx = nullptr)
{
    if (time.count() <= 0)
        return createExceptionFuture<T>(eptr);

    Promise<T> promise(true);

    auto timer = new QTimer();
    auto handler = [timer, promise, eptr]() mutable {
        promise.finishWithException(eptr);
        timer->deleteLater();
    };

    FutureUtilsInternals::connectTimer(timer, ctx, handler);
    timer->setSingleShot(true);
    timer->start(time);

    if (ctx)
        QObject::connect(ctx, &QObject::destroyed, timer, &QObject::deleteLater);

    return promise.future();
}

template<typename T,
         typename Ex,
         typename = std::enable_if_t<std::is_base_of_v<std::exception, Ex>>
         >
[[nodiscard]] QFuture<T> createTimedExceptionFuture(std::chrono::milliseconds time, const Ex& ex, QObject* ctx = nullptr)
{
    return createTimedExceptionFuture<T>(time, std::make_exception_ptr(ex), ctx);
}

template<typename T>
[[nodiscard]] Promise<T> createPromise(bool autoStartFuture = false)
{
    return Promise<T>(autoStartFuture);
}

template<typename T>
bool futureCompleted(const QFuture<T>& value)
{
    return value.isFinished() && !value.isCanceled();
}

template<typename T>
FutureState getFutureState(QFuture<T> value)
{
    if (value.isFinished()) {
        if (value.isCanceled()) {
            bool hasException { false };

            try {
                value.waitForFinished();
            } catch (...) {
                hasException = true;
            }

            return hasException ? FutureState::Exception :
                                  FutureState::Canceled;
        } else {
            if constexpr (std::is_same_v<T, void>) {
                return FutureState::Completed;
            } else {
                return value.resultCount() > 0 ? FutureState::Completed :
                                                 FutureState::CompletedWrong;
            }
        }

    } else if (value.isStarted()) {
        return FutureState::Running;

    } else {
        return FutureState::NotStarted;
    }
}

template<typename T>
bool hasResult(const QFuture<T>& value)
{
    return value.isFinished() && (value.resultCount() > 0);
}

template<template <typename, typename...> class Container, typename T, typename... Args,
         typename std::enable_if_t<!std::is_same_v<Container<QFuture<T>, Args...>, std::tuple<QFuture<T>, Args...>>>* = nullptr>
FuturesSetProperties analyzeFutures(const Container<QFuture<T>, Args...>& futures)
{
    if (std::distance(futures.begin(), futures.end()) == 0)
        return {true, true, false, false, false, true, true, true};

    auto properties = FuturesSetProperties::GetBoolFriendly();

    for (const auto& x : futures) {
        properties.allFinished &= x.isFinished();
        properties.someFinished |= x.isFinished();

        properties.allCanceled &= x.isCanceled();
        properties.someCanceled |= x.isCanceled();

        properties.someCompleted |= futureCompleted(x);
    }

    properties.noneFinished = !properties.someFinished;
    properties.noneCanceled = !properties.someCanceled;
    properties.allCompleted = properties.allFinished && properties.noneCanceled;

    return properties;
}

template<typename... Args>
FuturesSetProperties analyzeFutures(const std::tuple<QFuture<Args>...>& futures)
{
    if constexpr (sizeof...(Args) == 0)
        return {true, true, false, false, false, true, true, true};

    auto properties = FuturesSetProperties::GetBoolFriendly();
    const auto lambda = [&](const auto& x){
        properties.allFinished &= x.isFinished();
        properties.someFinished |= x.isFinished();

        properties.allCanceled &= x.isCanceled();
        properties.someCanceled |= x.isCanceled();

        properties.someCompleted |= futureCompleted(x);
    };

    std::apply([&](const auto&... xs){ (lambda (xs), ...); }, futures);

    properties.noneFinished = !properties.someFinished;
    properties.noneCanceled = !properties.someCanceled;
    properties.allCompleted = properties.allFinished && properties.noneCanceled;

    return properties;
}

template<typename... Ts>
struct ResultContentType : FutureUtilsInternals::ResultContentTypeImpl<std::tuple<Ts...>> {};

template<typename... Ts,
         typename ResultContent = typename ResultContentType<Ts...>::Type>
ResultContent futuresToOptResults(const std::tuple<QFuture<Ts>...>& futures)
{
    ResultContent resultsTuple;

    for_each_tuple_pair([](const auto& f, auto& r, const auto&){
        using ResultBaseType = std::remove_cv_t<std::remove_reference_t<decltype (r)>>;
        if constexpr (std::is_same_v<ResultBaseType, bool>) {
            r = futureCompleted(f);
        } else {
            if (futureCompleted(f)) {
                r = f.result();
            }
        }
    }, futures, resultsTuple);

    return resultsTuple;
}

template<template <typename, typename...> class Container, typename T, typename... Args,
         typename ItemType = std::conditional_t<std::is_same_v<T, void>, bool, std::optional<T>>,
         typename std::enable_if_t<!std::is_same_v<Container<QFuture<T>, Args...>, std::tuple<QFuture<T>, Args...>>>* = nullptr>
Container<ItemType> futuresToOptResults(const Container<QFuture<T>, Args...>& futures)
{
    Container<ItemType> result;
    auto it = std::back_inserter(result);

    const auto resultGetter = [](const auto& f){
        if constexpr (std::is_same_v<ItemType, bool>) {
            return futureCompleted(f);
        } else {
            return futureCompleted(f) ? f.result() : ItemType();
        }
    };

    for (const auto& x : futures)
        *it = resultGetter(x);

    return result;
}

} // namespace UtilsQt
