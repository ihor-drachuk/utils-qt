#pragma once

#include <QObject>
#include <QFuture>
#include <QFutureInterface>
#include <QFutureWatcher>

#include <optional>
#include <tuple>
#include <type_traits>
#include <memory>

namespace Internal {

class FuturesUnifierBase : public QObject
{
    Q_OBJECT
};

template<class... Ts>
class FuturesUnifier : public FuturesUnifierBase
{
public:
    FuturesUnifier(const std::tuple<QFuture<Ts>...>& futures, bool autodelete = false)
        : m_futures(futures)
    {
        if (autodelete) {
            using ResultType = decltype(m_future.future().result());
            auto selfWatcher = new QFutureWatcher<ResultType>(this);
            QObject::connect(selfWatcher, &QFutureWatcherBase::finished, this, &QObject::deleteLater);
            selfWatcher->setFuture(m_future.future());
        }
    }

    ~FuturesUnifier()
    {
        if (!m_used) return;

        if (!m_future.isFinished()) {
            m_future.reportCanceled();
            m_future.reportFinished();
        }
    }

    QFuture<std::tuple<std::optional<Ts>...>> mergeFuturesAll()
    {
        Q_ASSERT(!m_used);
        m_used = true;

        m_future.setProgressRange(0, sizeof... (Ts));

        waitForAllFutures();

        return m_future.future();
    }

    auto mergeFuturesAny()
    {
        Q_ASSERT(!m_used);
        m_used = true;

        waitForAnyFuture();

        return m_future.future();
    }

private:
    template <size_t I = 0>
    void waitForAllFutures(typename std::enable_if<!( I < sizeof... (Ts)), void>::type* = nullptr)
    {
    }

    template <size_t I = 0>
    void waitForAllFutures(typename std::enable_if<!( I >= sizeof... (Ts)), void>::type* = nullptr)
    {
        auto inFuture = std::get<I>(m_futures);
        if (inFuture.isFinished()) {
            m_future.setProgressValue(m_future.progressValue() + 1);

            if (inFuture.isCanceled()) {
                std::get<I>(m_tuple) = {};
            } else {
                std::get<I>(m_tuple) = inFuture.result();
            }

            if (m_future.progressValue() == m_future.progressMaximum()) {
                m_future.reportResult(m_tuple);
                m_future.reportFinished();
            }
        } else {
            using FuturesTypeI = std::tuple_element<I, std::tuple<Ts...>>;
            QObject::connect(&std::get<I>(m_watcher), &QFutureWatcher<FuturesTypeI>::finished, this, [this]() {
                m_future.setProgressValue(m_future.progressValue() + 1);

                if (std::get<I>(m_watcher).isCanceled()) {
                    std::get<I>(m_tuple) = {};
                } else {
                    std::get<I>(m_tuple) = std::get<I>(m_watcher).result();
                }

                if (m_future.progressValue() == m_future.progressMaximum()) {
                    m_future.reportResult(m_tuple);
                    m_future.reportFinished();
                }
            });

            std::get<I>(m_watcher).setFuture(inFuture);
        }

        waitForAllFutures<I+1>();
    }

    template <size_t I = 0>
    void waitForAnyFuture(typename std::enable_if<!( I < sizeof... (Ts)), void>::type* = nullptr)
    {
    }

    template <size_t I = 0>
    void waitForAnyFuture(typename std::enable_if<!( I >= sizeof... (Ts)), void>::type* = nullptr)
    {
        auto inFuture = std::get<I>(m_futures);
        if (inFuture.isFinished()) {
            m_future.reportFinished();
            return;
        } else if (inFuture.isCanceled()) {
            m_future.reportCanceled();
            return;
        } else {
            using FuturesTypeI = std::tuple_element<I, std::tuple<Ts...>>;
            QObject::connect(&std::get<I>(m_watcher), &QFutureWatcher<FuturesTypeI>::finished, this, [this]() {
                m_future.reportFinished();
            });

            QObject::connect(&std::get<I>(m_watcher), &QFutureWatcher<FuturesTypeI>::canceled, this, [this]() {
                m_future.reportCanceled();
            });

            std::get<I>(m_watcher).setFuture(inFuture);
        }

        waitForAnyFuture<I+1>();
    }

private:
    bool m_used {false};
    QFutureInterface<std::tuple<std::optional<Ts>...>> m_future;
    std::tuple<std::optional<Ts>...> m_tuple;
    std::tuple<QFuture<Ts>...> m_futures;
    std::tuple<QFutureWatcher<Ts>...> m_watcher;
};

} // namespace Internal

template<typename... Ts>
std::shared_ptr<Internal::FuturesUnifier<Ts...>> createFuturesUnifier(const std::tuple<QFuture<Ts>...>& futures)
{
    return std::make_shared<Internal::FuturesUnifier<Ts...>>(futures);
}

template<typename... Ts>
std::shared_ptr<Internal::FuturesUnifier<Ts...>> createFuturesUnifier(const QFuture<Ts>&... futures)
{
    return createFuturesUnifier(std::make_tuple(futures...));
}

template<typename... Ts>
QFuture<std::tuple<std::optional<Ts>...>> mergeFuturesAll(QObject* ctx, const std::tuple<QFuture<Ts>...>& futures)
{
    auto unifier = new Internal::FuturesUnifier<Ts...>(futures, true);

    if (ctx)
        QObject::connect(ctx, &QObject::destroyed, unifier, &QObject::deleteLater);

    return unifier->mergeFuturesAll();
}

template<typename... Ts>
QFuture<std::tuple<std::optional<Ts>...>> mergeFuturesAll(QObject* ctx, const QFuture<Ts>&... futures)
{
    return mergeFuturesAll(ctx, std::make_tuple(futures...));
}

template<typename... Ts>
auto mergeFuturesAny(QObject* ctx, const std::tuple<QFuture<Ts>...>& futures)
{
    auto unifier = new Internal::FuturesUnifier<Ts...>(futures, true);

    if (ctx)
        QObject::connect(ctx, &QObject::destroyed, unifier, &QObject::deleteLater);

    return unifier->mergeFuturesAny();
}

template<typename... Ts>
auto mergeFuturesAny(QObject* ctx, const QFuture<Ts>&... futures)
{
    return mergeFuturesAny(ctx, std::make_tuple(futures...));
}
