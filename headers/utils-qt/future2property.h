#pragma once
#include <QObject>
#include <QFuture>
#include <QFutureWatcher>
#include <QTimer>
#include <memory>
#include <type_traits>


namespace Internal {

template<typename Getter, typename Setter, typename ErrorHandler>
struct Future2propertyCtx
{
    using Type1 = typename std::result_of_t<Getter()>;
    using Type = decltype(Type1().result());

    Future2propertyCtx(const Getter& dataGetter, const Setter& propSetter, const ErrorHandler& errorHandler):
        dataGetter(dataGetter),
        propSetter(propSetter),
        errorHandler(errorHandler)
    { }

    ~Future2propertyCtx() {
    }

    Getter dataGetter;         // QFuture<T> func();
    Setter propSetter;         // void func(const T& value);
    ErrorHandler errorHandler; // void func();

    QObject tracker;
    std::unique_ptr<QTimer> timeoutTimer;
    std::unique_ptr<QTimer> retryTimer;
    unsigned int retryCnt {0};
    unsigned int retryInterval {0};
    std::unique_ptr<QFutureWatcher<Type>> watcher;

    void onTimeout() {
        watcher.reset();
        onRetryReq();
    }

    void onRetryReq() {
        if (retryCnt) {
            retryCnt--;

            if (retryTimer) {
                retryTimer->start();
            } else {
                onRetry();
            }
        } else {
            onError();
        }
    }

    void onRetry() {
        request();
    }

    void onFinished() {
        if (timeoutTimer)
            timeoutTimer->stop();

        if (watcher->future().isCanceled()) {
            onRetryReq();
        } else {
            propSetter(watcher->future().result());
            deleteMe();
        }
    }

    void onError() {
        errorHandler();
        deleteMe();
    }

    void deleteMe() {
        auto deleter = new QObject();
        QObject::connect(deleter, &QObject::destroyed, &tracker, [this](){ delete this; }, Qt::QueuedConnection);
        delete deleter;
    }

    void request() {
        auto future = dataGetter();

        if (timeoutTimer)
            timeoutTimer->start();

        watcher.reset(new QFutureWatcher<Type>);
        if (!future.isFinished()) QObject::connect(watcher.get(), &QFutureWatcherBase::finished, [this](){ onFinished(); });
        watcher->setFuture(future);
        if (future.isFinished())  onFinished();
    }
};

} // namespace Internal


template<typename Getter, typename Setter, typename ErrorHandler>
void future2property(const QObject* context,
                     const Getter& dataGetter,         // QFuture<T> func();
                     const Setter& propSetter,         // void func(const T& value);
                     const ErrorHandler& errorHandler, // void func();
                     unsigned int retryCount = 3,
                     unsigned int timeout = 2000,
                     unsigned int retryInterval = 100
                     )
{
    auto ctx = new Internal::Future2propertyCtx<Getter, Setter, ErrorHandler>(dataGetter, propSetter, errorHandler);
    QObject::connect(context, &QObject::destroyed, &ctx->tracker, [ctx](){ delete ctx; });

    ctx->retryCnt = retryCount;

    if (timeout) {
        ctx->timeoutTimer.reset(new QTimer);
        ctx->timeoutTimer->setInterval(timeout);
        ctx->timeoutTimer->setSingleShot(true);
        QObject::connect(ctx->timeoutTimer.get(), &QTimer::timeout, [ctx](){ ctx->onTimeout(); });
    }

    if (retryInterval) {
        ctx->retryTimer.reset(new QTimer);
        ctx->retryTimer->setInterval(retryInterval);
        ctx->retryTimer->setSingleShot(true);
        QObject::connect(ctx->retryTimer.get(), &QTimer::timeout, [ctx](){ ctx->onRetry(); });
    }

    ctx->request();
}
