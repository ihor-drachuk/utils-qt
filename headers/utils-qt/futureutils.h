#pragma once
#include <type_traits>
#include <optional>
#include <memory>
#include <QObject>
#include <QFuture>
#include <QFutureWatcher>
#include <QTimer>
#include <utils-qt/invoke_method.h>

/*
          Content
--------------------------

 * QFuture<Type>  -->  std::optional<Type>

   onFinished(QFuture<T>, context, handler);
      - if [Type == void] then behaves as `onResult` by default.


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

 * QFutureWrapper<T> createFuture<T>();
     -> finish(T);
     -> cancel();
     -> isFinished();
     -> future();
*/



namespace FutureUtilsInternals {

class QFutureInterfaceWrapperBase : public QObject
{
    Q_OBJECT
};

template<typename T>
class QFutureInterfaceWrapper : public QFutureInterfaceWrapperBase
{
public:
    QFutureInterfaceWrapper(QFutureInterface<T> interface)
        : m_interface(interface)
    {
        if (!m_interface.isStarted() && !m_interface.isFinished()) m_interface.reportStarted();
    }

    QFutureInterfaceWrapper(const QFutureInterfaceWrapper<T>&) = delete;
    QFutureInterfaceWrapper(QFutureInterfaceWrapper<T>&&) = delete;

    ~QFutureInterfaceWrapper()
    {
        if (!isFinished())
            cancel();
    }

    QFutureInterfaceWrapper<T>& operator= (const QFutureInterfaceWrapper<T>&) = delete;
    QFutureInterfaceWrapper<T>& operator= (QFutureInterfaceWrapper<T>&&) = delete;

    void finish(const T& result)
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

    bool isFinished() const { return m_interface.isFinished(); }

    QFuture<T> future() { return m_interface.future(); }

private:
    QFutureInterface<T> m_interface;
};

template<typename T>
using QFutureInterfaceWrapperPtr = std::shared_ptr<QFutureInterfaceWrapper<T>>;

template <typename C, typename T>
void call(const C& c, const QFuture<T>& future)
{
    c(future.result());
}

template <typename C>
void call(const C& c, const QFuture<void>&)
{
    c();
}

template <typename Callable, typename T>
void callCanceled(const Callable& callable, const QFuture<T>&, bool)
{
    callable({});
}

template <typename Callable>
void callCanceled(const Callable& callable, const QFuture<void>&, bool callOnVoidCancel)
{
    if (callOnVoidCancel)
        callable();
}

} // namespace FutureUtilsInternals


template<typename Type, typename Obj, typename Callable,
         typename std::enable_if<std::is_base_of<QObject, Obj>::value, int>::type = 0>
void onFinished(const QFuture<Type>& future,
                Obj* context,
                const Callable& callable,
                bool callOnVoidCancel = false,
                Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    auto resultHandler = [future, callOnVoidCancel, callable] () {
        if (future.isCanceled()) {
            FutureUtilsInternals::callCanceled(callable, future, callOnVoidCancel);
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
                bool callOnVoidCancel = false,
                Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    Q_ASSERT(object);
    Q_ASSERT(member);

    auto callable = [object, member](const std::optional<Type>& param){ (object->*member)(param); };

    onFinished(future, object, callable, callOnVoidCancel, connectionType);
}


template<typename Type, typename Obj, typename Callable,
         typename std::enable_if<std::is_base_of<QObject, Obj>::value, int>::type = 0>
void onResult(const QFuture<Type>& future,
              Obj* context,
              const Callable& callable,
              Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    if constexpr (std::is_same<Type,void>::value) {
        onFinished(future, context, [callable]() {
            callable();
        }, false, connectionType);
    } else {
        onFinished(future, context, [callable](const std::optional<Type>& value) {
            if (value)
                callable(*value);
        }, false, connectionType);
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

    auto callable = [object, member](const Type& param){ (object->*member)(param); };

    onResult(future, object, callable, connectionType);
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
        }, true, connectionType);
    } else {
        onFinished(future, context, [callable](const std::optional<Type>& value) {
            if (!value)
                callable();
        }, false, connectionType);
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
QFuture<T> createTimedFuture2(int time, const T& value)
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

template<typename T>
FutureUtilsInternals::QFutureInterfaceWrapperPtr<T> createFuture()
{
    QFutureInterface<T> interface;
    return std::make_shared<FutureUtilsInternals::QFutureInterfaceWrapper<T>>(interface);
}
