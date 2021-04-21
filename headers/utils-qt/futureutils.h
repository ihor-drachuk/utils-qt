#pragma once
#include <type_traits>
#include <optional>
#include <QObject>
#include <QFuture>
#include <QFutureWatcher>
#include <QTimer>

// Reminder.
// 1) QFuture<Type>  -->  std::optional<Type>   (is called both if fired and if cancelled)
// 2) QFuture<Type>  -->  Type                  (is called only if fired)

// If 'Type' is 'void', then #1 behaves as #2.


namespace FutureUtilsInternals {

template <typename C, typename T>
void call(const C& c, const QFuture<T>& future) {
    c(future.result());
}

template <typename C>
void call(const C& c, const QFuture<void>&) {
    c();
}

} // namespace FutureUtilsInternals


template<typename Type, typename Obj, typename Callable,
         typename std::enable_if<std::is_base_of<QObject, Obj>::value, int>::type = 0>
void connectFuture(const QFuture<Type>& future,
                   Obj* context,
                   const Callable& callable,
                   Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    auto resultHandler = [future, callable] () {
        if (future.isCanceled()) {
            if constexpr (!std::is_same<Type,void>::value)
                callable({});
            // Don't call <void> handler if cancelled. Behave as option #2.
        } else {
            FutureUtilsInternals::call(callable, future);
        }
    };

    if (future.isFinished()) {
        // If already done
        QMetaObject::invokeMethod(context, resultHandler, connectionType);
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
void connectFuture(const QFuture<Type>& future,
                   Obj* object,
                   void (Obj::* member)(const std::optional<Type>&),
                   Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    Q_ASSERT(object);
    Q_ASSERT(member);

    auto callable = [object, member](const std::optional<Type>& param){ (object->*member)(param); };

    connectFuture(future, object, callable, connectionType);
}


template<typename Type, typename Obj, typename Callable,
         typename std::enable_if<std::is_base_of<QObject, Obj>::value, int>::type = 0>
void connectFuture2(const QFuture<Type>& future,
                    Obj* context,
                    const Callable& callable,
                    Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    if constexpr (std::is_same<Type,void>::value) {
        connectFuture(future, context, [callable]() {
            callable();
        }, connectionType);
    } else {
        connectFuture(future, context, [callable](const std::optional<Type>& value) {
            if (value)
                callable(*value);
        }, connectionType);
    }
}


template<typename Type, typename Obj,
         typename std::enable_if<std::is_base_of<QObject, Obj>::value, int>::type = 0>
void connectFuture2(const QFuture<Type>& future,
                    Obj* object,
                    void (Obj::* member)(const Type&),
                    Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    Q_ASSERT(object);
    Q_ASSERT(member);

    auto callable = [object, member](const Type& param){ (object->*member)(param); };

    connectFuture2(future, object, callable, connectionType);
}


template<typename EventLoopType, typename T>
void waitForFuture(const QFuture<T>& future)
{
    if (future.isFinished())
        return;

    EventLoopType eventLoop;
    QFutureWatcher<T> futureWatcher;
    QObject::connect(&futureWatcher, &QFutureWatcherBase::finished, &eventLoop, &EventLoopType::quit);
    futureWatcher.setFuture(future);
    eventLoop.exec();

    Q_ASSERT(future.isFinished());
}


template<typename T>
QFuture<T> createReadyFuture(const T& value)
{
    QFutureInterface<T> interface;
    interface.reportStarted();
    interface.reportResult(value);
    interface.reportFinished();
    return interface.future();
}

static QFuture<void> createReadyFuture()
{
    QFutureInterface<void> interface;
    interface.reportStarted();
    interface.reportFinished();
    return interface.future();
}


template<typename T>
QFuture<T> createCanceledFuture()
{
    QFutureInterface<T> interface;
    interface.reportStarted();
    interface.reportCanceled();
    interface.reportFinished();
    return interface.future();
}


template<typename T>
QFuture<T> createTimedFuture(const T& value, int time)
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

    return interface.future();
}

static QFuture<void> createTimedFuture(int time)
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

    return interface.future();
}


template<typename T>
QFuture<T> createTimedCanceledFuture(int time)
{
    if (!time)
        return createCanceledFuture<T>();

    QFutureInterface<T> interface;
    interface.reportStarted();

    auto timer = new QTimer();
    QObject::connect(timer, &QTimer::timeout, [timer, interface]() mutable {
        interface.reportCanceled();
        interface.reportFinished();
        timer->deleteLater();
    });
    timer->setSingleShot(true);
    timer->start(time);

    return interface.future();
}
