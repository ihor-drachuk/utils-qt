/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <type_traits>
#include <optional>
#include <utility>
#include <tuple>
#include <memory>
#include <QObject>
#include <QFuture>
#include <QFutureInterface>
#include <QFutureWatcher>
#include <QTimer>

#include <utils-cpp/tuple_utils.h>
#include <UtilsQt/invoke_method.h>

/*
          Description
------------------------------

 * onFinished(QFuture<T>, context, handler);

   QFuture<Type> is converted to std::optional<Type> and passed to
   handler, when future is finished.


 * onResult(QFuture<T>, context, handler);

   QFuture<Type>  -->  Type  (only on success)


 * onCanceled(QFuture<T>, context, handler);

   QFuture<Type>  -->  void  (only on cancellation)


 * waitForFuture<QEventLoop>(future);

 * QFuture<T> createReadyFuture<T = void>();
 * QFuture<T> createReadyFuture<T>(T value);
 * QFuture<T> createCanceledFuture<T>();

 * QFuture<T> createTimedFuture<T>(int time, T value);
 * QFuture<T> createTimedFuture<void>(int time);
 * QFuture<T> createTimedCanceledFuture<T>();

 * Promise<T> createPromise<T>();
     -> finish(T);
     -> cancel();
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
    const_cast<C&>(c)();
}

template <typename Callable, typename T>
void callCanceled(const Callable& callable, const QFuture<T>&)
{
    const_cast<Callable&>(callable)(std::optional<T>{});
}

template <typename Callable>
void callCanceled(const Callable& callable, const QFuture<void>&)
{
    const_cast<Callable&>(callable)();
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

} // namespace FutureUtilsInternals


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
    ~Promise() = default;
    Promise<T>& operator= (const Promise<T>&) = default;
    Promise<T>& operator= (Promise<T>&&) noexcept = default;

    void start()
    {
        assert(!isStarted());
        m_interface.reportStarted();
    }

    // if [T == void]
    template<typename X = T>
    typename std::enable_if<std::is_same<X, void>::value && std::is_same<X, T>::value>::type
    finish()
    {
        assert(isStarted());
        assert(!isFinished());

        m_interface.reportFinished();
    }

    // if [T != void]
    template<typename X = T>
    typename std::enable_if<!std::is_same<X, void>::value && std::is_same<X, T>::value>::type
    finish(const X& result)
    {
        assert(isStarted());
        assert(!isFinished());

        m_interface.reportResult(result);
        m_interface.reportFinished();
    }

    void cancel()
    {
        assert(!isFinished());

        m_interface.reportCanceled();
        m_interface.reportFinished();
    }

    bool isStarted() const { return m_interface.isStarted(); }
    bool isRunning() const { return m_interface.isRunning(); }
    bool isCanceled() const { return m_interface.isCanceled(); }
    bool isFinished() const { return m_interface.isFinished(); }

    QFuture<T> future() { return m_interface.future(); }
    QFutureInterface<T> futureInterface() { return m_interface; };

private:
    QFutureInterface<T> m_interface;
    std::shared_ptr<void> m_contextTracker;
};

template<typename Type, typename Obj, typename Callable,
         typename std::enable_if<std::is_base_of<QObject, Obj>::value, int>::type = 0>
void onFinished(const QFuture<Type>& future,
                Obj* context,
                const Callable& callable,
                Qt::ConnectionType connectionType = Qt::AutoConnection)
{
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
    Q_ASSERT(object);
    Q_ASSERT(member);

    auto callable = [object, member](const std::optional<Type>& param){ (object->*member)(param); };

    onFinished(future, object, callable, connectionType);
}


template<typename Type, typename Obj, typename Callable,
         typename std::enable_if<std::is_base_of<QObject, Obj>::value, int>::type = 0>
void onResult(const QFuture<Type>& future,
              Obj* context,
              const Callable& callable,
              Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    if constexpr (std::is_same<Type,void>::value) {
        onFinished(future, context, [future, callable]() {
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
    Q_ASSERT(object);
    Q_ASSERT(member);

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
        onFinished(future, context, [callable, future]() {
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
    Q_ASSERT(object);
    Q_ASSERT(member);

    auto callable = [object, member](){ (object->*member)(); };

    onCanceled(future, object, callable, connectionType);
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
        Q_ASSERT(future.isFinished());
}


template<typename T>
[[nodiscard]] QFuture<T> createReadyFuture(const T& value)
{
    QFutureInterface<T> futureInterface;
    futureInterface.reportStarted();
    futureInterface.reportResult(value);
    futureInterface.reportFinished();
    return futureInterface.future();
}

[[nodiscard]] static QFuture<void> createReadyFuture()
{
    QFutureInterface<void> futureInterface;
    futureInterface.reportStarted();
    futureInterface.reportFinished();
    return futureInterface.future();
}


template<typename T>
[[nodiscard]] QFuture<T> createCanceledFuture()
{
    QFutureInterface<T> futureInterface;
    futureInterface.reportStarted();
    futureInterface.reportCanceled();
    futureInterface.reportFinished();
    return futureInterface.future();
}


template<typename T>
[[nodiscard]] QFuture<T> createTimedFuture(int time, const T& value, QObject* ctx = nullptr)
{
    if (!time)
        return createReadyFuture(value);

    QFutureInterface<T> futureInterface;
    futureInterface.reportStarted();

    auto timer = new QTimer();
    QObject::connect(timer, &QTimer::timeout, [timer, futureInterface, value]() mutable {
        futureInterface.reportResult(value);
        futureInterface.reportFinished();
        timer->deleteLater();
    });
    timer->setSingleShot(true);
    timer->start(time);

    if (ctx) {
        QObject::connect(ctx, &QObject::destroyed, timer, [timer, futureInterface]() mutable {
            timer->stop();
            futureInterface.reportCanceled();
            futureInterface.reportFinished();
            timer->deleteLater();
        });
    }

    return futureInterface.future();
}

template<typename T>
[[nodiscard]] QFuture<T> createTimedFutureRef(int time, const T& value, QObject* ctx)
{
    assert(ctx);

    if (!time)
        return createReadyFuture(value);

    QFutureInterface<T> futureInterface;
    futureInterface.reportStarted();

    auto timer = new QTimer();
    QObject::connect(timer, &QTimer::timeout, [timer, futureInterface, &value]() mutable {
        futureInterface.reportResult(value);
        futureInterface.reportFinished();
        timer->deleteLater();
    });
    timer->setSingleShot(true);
    timer->start(time);

    QObject::connect(ctx, &QObject::destroyed, timer, [timer, futureInterface]() mutable {
        timer->stop();
        futureInterface.reportCanceled();
        futureInterface.reportFinished();
        timer->deleteLater();
    });

    return futureInterface.future();
}

[[nodiscard]] static inline QFuture<void> createTimedFuture(int time, QObject* ctx = nullptr)
{
    if (!time)
        return createReadyFuture();

    QFutureInterface<void> futureInterface;
    futureInterface.reportStarted();

    auto timer = new QTimer();
    QObject::connect(timer, &QTimer::timeout, [timer, futureInterface]() mutable {
        futureInterface.reportFinished();
        timer->deleteLater();
    });
    timer->setSingleShot(true);
    timer->start(time);

    if (ctx) {
        QObject::connect(ctx, &QObject::destroyed, timer, [timer, futureInterface]() mutable {
            timer->stop();
            futureInterface.reportCanceled();
            futureInterface.reportFinished();
            timer->deleteLater();
        });
    }

    return futureInterface.future();
}


template<typename T>
[[nodiscard]] QFuture<T> createTimedCanceledFuture(int time, QObject* ctx = nullptr)
{
    if (!time)
        return createCanceledFuture<T>();

    QFutureInterface<T> futureInterface;
    futureInterface.reportStarted();

    auto timer = new QTimer();
    auto handler = [timer, futureInterface]() mutable {
        timer->stop();
        futureInterface.reportCanceled();
        futureInterface.reportFinished();
        timer->deleteLater();
    };

    QObject::connect(timer, &QTimer::timeout, handler);
    timer->setSingleShot(true);
    timer->start(time);

    if (ctx)
        QObject::connect(ctx, &QObject::destroyed, timer, handler);

    return futureInterface.future();
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
