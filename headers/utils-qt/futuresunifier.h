#pragma once

#include <QObject>
#include <QFuture>
#include <QFutureInterface>
#include <QFutureWatcher>

#include <optional>
#include <tuple>
#include <type_traits>
#include <memory>

template<class... Ts>
class FuturesUnifier
{
public:
    FuturesUnifier(const std::tuple<QFuture<Ts>...>& futures)
        : m_futures(futures)
    { }

    FuturesUnifier(const QFuture<Ts>&... futures)
    {
        m_futures = std::make_tuple(futures...);
    }

    ~FuturesUnifier()
    {
        if (!m_unused && !m_future.isFinished()) {
            m_future.reportCanceled();
            m_future.reportFinished();
        }
    }

    QFuture<std::tuple<std::optional<Ts>...>> mergeFuturesAll()
    {
        Q_ASSERT(m_unused);
        m_unused = false;

        m_future.setProgressRange(0, sizeof... (Ts));

        waitForAllFutures();

        return m_future.future();
    }

    QFuture<void> mergeFuturesAny()
    {
        Q_ASSERT(m_unused);
        m_unused = false;

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
            using futuresType = std::tuple_element<I, std::tuple<Ts...>>;
            QObject::connect(&std::get<I>(m_watcher), &QFutureWatcher<futuresType>::finished, &ctx, [this]() {
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
            using futuresType = std::tuple_element<I, std::tuple<Ts...>>;
            QObject::connect(&std::get<I>(m_watcher), &QFutureWatcher<futuresType>::finished, &ctx, [this]() {
                m_future.reportFinished();
            });

            QObject::connect(&std::get<I>(m_watcher), &QFutureWatcher<futuresType>::canceled, &ctx, [this]() {
                m_future.reportCanceled();
            });

            std::get<I>(m_watcher).setFuture(inFuture);
        }

        waitForAnyFuture<I+1>();
    }

private:
    bool m_unused {true};
    QObject ctx;
    QFutureInterface<std::tuple<std::optional<Ts>...>> m_future;
    std::tuple<std::optional<Ts>...> m_tuple;
    std::tuple<QFuture<Ts>...> m_futures;
    std::tuple<QFutureWatcher<Ts>...> m_watcher;
};

template<typename... Ts>
std::shared_ptr<FuturesUnifier<Ts...>> createFuturesUnifier(const std::tuple<QFuture<Ts>...>& futures)
{
    return std::make_shared<FuturesUnifier<Ts...>>(futures);
}

template<typename... Ts>
std::shared_ptr<FuturesUnifier<Ts...>> createFuturesUnifier(const QFuture<Ts>&... futures)
{
    return std::make_shared<FuturesUnifier<Ts...>>(futures...);
}
