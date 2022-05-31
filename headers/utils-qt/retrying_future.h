#pragma once
#include <QObject>
#include <QFuture>
#include <QFutureWatcher>
#include <QTimer>
#include <memory>
#include <type_traits>


namespace Internal {

template<typename Process, typename ReturnType>
struct RetryFutureCtx
{
    RetryFutureCtx(const Process& process, const std::function<bool(const ReturnType&)>& checkFunction, unsigned int retryCount, unsigned int retryTimeout)
        : futureProcess(process),
          checker(checkFunction),
          retryCnt(retryCount),
          retryInterval(retryTimeout)
    {
        request();
    }

    ~RetryFutureCtx() {}

    Process futureProcess;  // QFuture<T> func();
    std::function<bool(ReturnType)> checker;

    QObject tracker;
    unsigned int retryCnt {0};
    unsigned int retryInterval {0};
    std::unique_ptr<QFutureWatcher<ReturnType>> watcher;
    QFutureInterface<ReturnType> futureInterface;

    void onFinished() {
        retryCnt--;
        if (watcher->future().isCanceled()) {
            onRetry();
        } else {
            auto result = watcher->future().result();
            if (checker) {
                if (!checker(result)) {
                    onRetry();
                    return;
                }
            }

            futureInterface.reportResult(result);
            futureInterface.reportFinished();
            deleteMe();
        }
    }

    void onRetry() {
        if (retryCnt == 0) {
            futureInterface.cancel();
            futureInterface.reportFinished();
            deleteMe();
            return;
        }

        QTimer::singleShot(retryInterval, &tracker, [this](){ request(); });
    }

    void deleteMe() {
        auto deleter = new QObject();
        QObject::connect(deleter, &QObject::destroyed, &tracker, [this](){ delete this; }, Qt::QueuedConnection);
        delete deleter;
    }

    void request() {
        auto future = futureProcess();
        watcher.reset(new QFutureWatcher<ReturnType>);
        if (!future.isFinished()) QObject::connect(watcher.get(), &QFutureWatcherBase::finished, [this](){ onFinished(); });
        watcher->setFuture(future);
        if (future.isFinished())  onFinished();
    }
};

} // namespace Internal

template<typename Process, typename ReturnType = std::result_of_t<Process()>>
ReturnType createRetryFuture(const QObject* context,
                             const Process& process,    // QFuture<T> func();
                             const std::function<bool(const decltype(ReturnType().result())&)>& checker = {},
                             unsigned int retryCount = 3,
                             unsigned int retryInterval = 1000)
{
    auto ctx = new ::Internal::RetryFutureCtx<Process, decltype(ReturnType().result())>(process, checker, retryCount, retryInterval);
    QObject::connect(context, &QObject::destroyed, &ctx->tracker, [ctx](){ delete ctx; });

    return ctx->futureInterface.future();
}
