#pragma once
#include <QObject>
#include <QFuture>
#include <QFutureWatcher>
#include <QTimer>
#include <memory>
#include <type_traits>


namespace Internal {

template<typename FutureProvider, typename ReturnType>
struct RetryFutureCtx
{
    RetryFutureCtx(const FutureProvider& futureProvider, const std::function<bool(const ReturnType&)>& checkFunction, unsigned int retryCount, unsigned int retryTimeout)
        : m_futureProvider(futureProvider),
          m_checker(checkFunction),
          m_retryCnt(retryCount),
          m_retryInterval(retryTimeout)
    {
        request();
    }

    ~RetryFutureCtx() {
        if (m_watcher->future().isFinished())
            return;

        m_futureInterface.reportCanceled();
        m_futureInterface.reportFinished();
    }

    QObject* getTracker() {
        return &m_tracker;
    }

    QFuture<ReturnType> getFuture() {
        return m_futureInterface.future();
    }

    void onFinished() {
        if (m_watcher->future().isCanceled()) {
            onRetry();
        } else {
            auto result = m_watcher->future().result();
            if (m_checker && !m_checker(result)) {
                onRetry();
                return;
            }

            m_futureInterface.reportResult(result);
            m_futureInterface.reportFinished();
            deleteMe();
        }
    }

    void onRetry() {
        if (m_retryCnt == 0) {
            m_futureInterface.reportCanceled();
            m_futureInterface.reportFinished();
            deleteMe();
            return;
        }

        m_retryCnt--;
        QTimer::singleShot(m_retryInterval, &m_tracker, [this](){ request(); });
    }

    void deleteMe() {
        auto deleter = new QObject();
        QObject::connect(deleter, &QObject::destroyed, &m_tracker, [this](){ delete this; }, Qt::QueuedConnection);
        delete deleter;
    }

    void request() {
        auto future = m_futureProvider();
        m_watcher.reset(new QFutureWatcher<ReturnType>);
        if (!future.isFinished()) QObject::connect(m_watcher.get(), &QFutureWatcherBase::finished, [this](){ onFinished(); });
        m_watcher->setFuture(future);
        if (future.isFinished())  onFinished();
    }

private:
    FutureProvider m_futureProvider;  // QFuture<T> func();
    std::function<bool(ReturnType)> m_checker;

    QObject m_tracker;
    unsigned int m_retryCnt {0};
    unsigned int m_retryInterval {0};
    std::unique_ptr<QFutureWatcher<ReturnType>> m_watcher;
    QFutureInterface<ReturnType> m_futureInterface;
};

} // namespace Internal

template<typename FutureProvider, typename ReturnType = std::result_of_t<FutureProvider()>>
ReturnType createRetryFuture(const QObject* context,
                             const FutureProvider& futureProvider,    // QFuture<T> func();
                             const std::function<bool(const decltype(ReturnType().result())&)>& checker = {},
                             unsigned int retryCount = 3,
                             unsigned int retryInterval = 1000)
{
    auto ctx = new ::Internal::RetryFutureCtx<FutureProvider, decltype(ReturnType().result())>(futureProvider, checker, retryCount, retryInterval);
    QObject::connect(context, &QObject::destroyed, ctx->getTracker(), [ctx](){ delete ctx; });

    return ctx->getFuture();
}
