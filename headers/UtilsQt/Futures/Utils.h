#pragma once
#include <type_traits>
#include <optional>
#include <memory>
#include <QObject>
#include <QFuture>
#include <QFutureWatcher>
#include <QTimer>
#include <UtilsQt/invoke_method.h>

/*
          Content
--------------------------

 * QFuture<Type>  -->  std::optional<Type>

   onFinished(QFuture<T>, context, handler);


 * QFuture<Type>  -->  Type  (only on success)

   onResult(QFuture<T>, context, handler);


 * QFuture<Type>  -->  void  (only on cancel)

   onCanceled(QFuture<T>, context, handler);


 * waitForFuture<QEventLoop>(future);


 * QFuture<T> createReadyFuture<T = void>();
 * QFuture<T> createReadyFuture<T>(T value);
 * QFuture<T> createCanceledFuture<T>();

 * QFuture<T> createTimedFuture<T>(int time, T value);
 * QFuture<T> createTimedFuture<void>(int time);
 * QFuture<T> createTimedCanceledFuture<T>();

 * QFutureWrapper<T> createPromise<T>();
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

} // namespace FutureUtilsInternals


template<typename T>
class Promise
{
public:
    Promise()
    {
        m_interface.reportStarted();
        m_contextTracker = std::shared_ptr<void>(reinterpret_cast<void*>(1), [f = m_interface](void*) mutable {
            if (!f.isFinished()) {
                f.reportCanceled();
                f.reportFinished();
            }
        });
    }

    Promise(const Promise<T>&) = default;
    Promise(Promise<T>&&) = default;
    ~Promise() = default;
    Promise<T>& operator= (const Promise<T>&) = default;
    Promise<T>& operator= (Promise<T>&&) = default;

    // if [T == void]
    template<typename X = T>
    typename std::enable_if<std::is_same<X, void>::value && std::is_same<X, T>::value>::type
    finish()
    {
        assert(!isFinished());

        m_interface.reportFinished();
    }

    // if [T != void]
    template<typename X = T>
    typename std::enable_if<!std::is_same<X, void>::value && std::is_same<X, T>::value>::type
    finish(const X& result)
    {
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

    bool isCanceled() const { return m_interface.isCanceled(); }
    bool isFinished() const { return m_interface.isFinished(); }

    QFuture<T> future() { return m_interface.future(); }
    QFutureInterface<T> interface() { return m_interface; };

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


template<typename Type, typename Obj,
         typename std::enable_if<std::is_base_of<QObject, Obj>::value, int>::type = 0>
void onResult(const QFuture<Type>& future,
              Obj* object,
              void (Obj::* member)(const Type&),
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
                void (Obj::* member)(const std::optional<Type>&),
                Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    Q_ASSERT(object);
    Q_ASSERT(member);

    auto callable = [object, member](const std::optional<Type>& param){ (object->*member)(param); };

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
    QFutureInterface<T> interface;
    interface.reportStarted();
    interface.reportResult(value);
    interface.reportFinished();
    return interface.future();
}

[[nodiscard]] static QFuture<void> createReadyFuture()
{
    QFutureInterface<void> interface;
    interface.reportStarted();
    interface.reportFinished();
    return interface.future();
}


template<typename T>
[[nodiscard]] QFuture<T> createCanceledFuture()
{
    QFutureInterface<T> interface;
    interface.reportStarted();
    interface.reportCanceled();
    interface.reportFinished();
    return interface.future();
}


template<typename T>
[[nodiscard]] QFuture<T> createTimedFuture(int time, const T& value, QObject* ctx = nullptr)
{
    if (!time)
        return createReadyFuture(value);

    QFutureInterface<T> interface;
    interface.reportStarted();

    auto timer = new QTimer();
    QObject::connect(timer, &QTimer::timeout, [timer, interface, value]() mutable {
        interface.reportResult(value);
        interface.reportFinished();
        timer->deleteLater();
    });
    timer->setSingleShot(true);
    timer->start(time);

    if (ctx) {
        QObject::connect(ctx, &QObject::destroyed, timer, [timer, interface]() mutable {
            timer->stop();
            interface.reportCanceled();
            interface.reportFinished();
            timer->deleteLater();
        });
    }

    return interface.future();
}

template<typename T>
[[nodiscard]] QFuture<T> createTimedFutureRef(int time, const T& value, QObject* ctx)
{
    assert(ctx);

    if (!time)
        return createReadyFuture(value);

    QFutureInterface<T> interface;
    interface.reportStarted();

    auto timer = new QTimer();
    QObject::connect(timer, &QTimer::timeout, [timer, interface, &value]() mutable {
        interface.reportResult(value);
        interface.reportFinished();
        timer->deleteLater();
    });
    timer->setSingleShot(true);
    timer->start(time);

    QObject::connect(ctx, &QObject::destroyed, timer, [timer, interface]() mutable {
        timer->stop();
        interface.reportCanceled();
        interface.reportFinished();
        timer->deleteLater();
    });

    return interface.future();
}

[[nodiscard]] static QFuture<void> createTimedFuture(int time, QObject* ctx = nullptr)
{
    if (!time)
        return createReadyFuture();

    QFutureInterface<void> interface;
    interface.reportStarted();

    auto timer = new QTimer();
    QObject::connect(timer, &QTimer::timeout, [timer, interface]() mutable {
        interface.reportFinished();
        timer->deleteLater();
    });
    timer->setSingleShot(true);
    timer->start(time);

    if (ctx) {
        QObject::connect(ctx, &QObject::destroyed, timer, [timer, interface]() mutable {
            timer->stop();
            interface.reportCanceled();
            interface.reportFinished();
            timer->deleteLater();
        });
    }

    return interface.future();
}


template<typename T>
[[nodiscard]] QFuture<T> createTimedCanceledFuture(int time, QObject* ctx = nullptr)
{
    if (!time)
        return createCanceledFuture<T>();

    QFutureInterface<T> interface;
    interface.reportStarted();

    auto timer = new QTimer();
    auto handler = [timer, interface]() mutable {
        timer->stop();
        interface.reportCanceled();
        interface.reportFinished();
        timer->deleteLater();
    };

    QObject::connect(timer, &QTimer::timeout, handler);
    timer->setSingleShot(true);
    timer->start(time);

    if (ctx)
        QObject::connect(ctx, &QObject::destroyed, timer, handler);

    return interface.future();
}

template<typename T>
[[nodiscard]] Promise<T> createPromise()
{
    return Promise<T>();
}

} // namespace UtilsQt