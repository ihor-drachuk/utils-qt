#pragma once
#include <QPair>
#include <QFuture>
#include <QFutureInterface>
#include <QFutureWatcher>
#include <QVector>
#include <initializer_list>
#include <variant>

namespace UtilsQt {
namespace Internals {

template<typename T> struct ResultType       { using Type = T; };
template<>           struct ResultType<void> { using Type = std::monostate; };

class ContextBase : public QObject
{
    Q_OBJECT
public:
signals:
    void finished();
};

template<typename T>
class Context : public ContextBase
{
    using RetType = typename ResultType<T>::Type;
public:
    ~Context() override {
        m_destroying = true;

        if (!m_targetFuture.isFinished()) {
            if (!m_targetFuture.isCanceled())
                m_targetFuture.reportCanceled();

            m_targetFuture.reportFinished();
        }
    }

    template<template<typename...> class Container>
    void setFutures(const Container<QFuture<T>>& container) {
        const auto size = std::cend(container) - std::cbegin(container);
        m_targetFuture.setExpectedResultCount(size);
        m_targetFuture.setProgressRange(0, size);
        m_targetFuture.setProgressValue(0);
        m_maxResults = size;

        QObject::connect(&m_targetFutureWatcher, &QFutureWatcherBase::canceled, this, &Context<T>::onTargetCanceled);

        for (int i = 0; i < size; i++) {
            auto watcher = std::make_shared<QFutureWatcher<T>>();
            QObject::connect(watcher.get(), &QFutureWatcherBase::started, this, &Context<T>::onStarted);
            QObject::connect(watcher.get(), &QFutureWatcherBase::canceled, this, &Context<T>::onCanceled);
            QObject::connect(watcher.get(), &QFutureWatcherBase::finished, this, [this, i](){ onFinished(i); });
            m_sourceFutureWatchers.append(watcher);
        }

        for (int i = 0; i < size; i++) {
            m_sourceFutureWatchers[i]->setFuture(*(container.begin() + i));
        }

        m_targetFutureWatcher.setFuture(m_targetFuture.future());
    }

    QFuture<RetType> future() { return m_targetFuture.future(); }

private:
    template<typename K>
    static void reportResult(const QFuture<K>& future, QFutureInterface<K>& promise, int index) {
        promise.reportResult(future.result(), index);
    }

    static void reportResult(const QFuture<void>& /*future*/, QFutureInterface<std::monostate>& promise, int index) {
        promise.reportResult(std::monostate(), index);
    }

    void onStarted() {
        if (m_destroying) return;

        if (!m_targetFuture.isStarted() &&
            !m_targetFuture.isCanceled() &&
            !m_targetFuture.isFinished())
        {
            m_targetFuture.reportStarted();
        }
    }

    void onCanceled() {
    }

    void onFinished(int index) {
        if (m_destroying) return;
        assert(!m_targetFuture.isFinished());

        QFuture<T> f = m_sourceFutureWatchers[index]->future();

        if (!f.isCanceled())
            reportResult(f, m_targetFuture, index);

        auto newProgress = m_targetFuture.progressValue() + 1;
        m_targetFuture.setProgressValue(newProgress);

        m_gotResults++;

        if (m_gotResults == m_maxResults) {
            m_targetFuture.reportFinished();
            emit finished();
        }
    }

    void onTargetCanceled() {
        for (const auto& x : m_sourceFutureWatchers)
            x->future().cancel();
    }

private:
    int m_maxResults {};
    int m_gotResults {};
    bool m_destroying { false };
    QFutureInterface<RetType> m_targetFuture;
    QFutureWatcher<RetType> m_targetFutureWatcher;
    QVector<std::shared_ptr<QFutureWatcher<T>>> m_sourceFutureWatchers;
};

} // namespace Internals

template<template<typename...> class Container,
        typename T>
QFuture<typename Internals::ResultType<T>::Type> combineFutures(const Container<QFuture<T>>& container, QObject* lifetimeCtx = nullptr)
{
    assert(std::cend(container) - std::cbegin(container) > 0);

    auto ctx = new Internals::Context<T>();

    if (lifetimeCtx)
        QObject::connect(lifetimeCtx, &QObject::destroyed, ctx, &QObject::deleteLater);

    QObject::connect(ctx, &Internals::ContextBase::finished, ctx, &QObject::deleteLater);

    ctx->setFutures(container);

    return ctx->future();
}

template<typename T>
auto combineFutures(const std::initializer_list<QFuture<T>>& container, QObject* lifetimeCtx = nullptr)
{
    return combineFutures<std::initializer_list, T>(container, lifetimeCtx);
}

} // namespace UtilsQt
