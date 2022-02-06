#pragma once

#include <QObject>

#include <optional>
#include <tuple>
#include <QFuture>
#include <QFutureInterface>
#include <QFutureWatcher>
#include <type_traits>

template<class... Ts>
class FuturesUnifier
{
public:
    FuturesUnifier(const QFuture<Ts>&... futures)
    {
        m_futures = std::make_tuple(futures...);
    }

    QFuture<std::tuple<std::optional<Ts>...>> mergeAllFutures()
    {
        Q_ASSERT(m_unused);
        m_unused = false;

        m_future.setProgressRange(0, sizeof... (Ts));

        waitToAllFutures();

        return m_future.future();
    }

    QFuture<void> mergeFuturesAny()
    {
        Q_ASSERT(m_unused);
        m_unused = false;

        waitToAnyFuture();

        return m_future.future();
    }

private:
    template <size_t I = 0>
    void waitToAllFutures(typename std::enable_if<!( I < sizeof... (Ts)), void>::type* = nullptr)
    {

    }

    template <size_t I = 0>
    void waitToAllFutures(typename std::enable_if<!( I >= sizeof... (Ts)), void>::type* = nullptr)
    {
        auto inFuture = std::get<I>(m_futures);
        if (inFuture.isFinished()) {
            m_future.setProgressValue(m_future.progressValue() + 1);
            std::get<I>(m_tuple) = inFuture.result();

            if (m_future.progressValue() == m_future.progressMaximum()) {
                m_future.reportResult(m_tuple);
                m_future.reportFinished();
            }
        } else if (inFuture.isCanceled()) {
            m_future.setProgressValue(m_future.progressValue() + 1);
            std::get<I>(m_tuple) = {};

            if (m_future.progressValue() == m_future.progressMaximum()) {
                m_future.reportResult(m_tuple);
                m_future.reportFinished();
            }
        } else {
            using futuresType = std::tuple_element<I, std::tuple<Ts...>>;
            QObject::connect(&std::get<I>(m_watcher), &QFutureWatcher<futuresType>::finished, &cnt, [this]() {
                m_future.setProgressValue(m_future.progressValue() + 1);
                std::get<I>(m_tuple) = std::get<I>(m_watcher).result();

                if (m_future.progressValue() == m_future.progressMaximum()) {
                    m_future.reportResult(m_tuple);
                    m_future.reportFinished();
                }
            });

            QObject::connect(&std::get<I>(m_watcher), &QFutureWatcher<futuresType>::canceled, &cnt, [this]() {
                m_future.setProgressValue(m_future.progressValue() + 1);
                std::get<I>(m_tuple) = {};

                if (m_future.progressValue() == m_future.progressMaximum()) {
                    m_future.reportResult(m_tuple);
                    m_future.reportFinished();
                }
            });

            std::get<I>(m_watcher).setFuture(inFuture);
        }

        waitToAllFutures<I+1>();
    }

    template <size_t I = 0>
    void waitToAnyFuture(typename std::enable_if<!( I < sizeof... (Ts)), void>::type* = nullptr)
    {

    }

    template <size_t I = 0>
    void waitToAnyFuture(typename std::enable_if<!( I >= sizeof... (Ts)), void>::type* = nullptr)
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
            QObject::connect(&std::get<I>(m_watcher), &QFutureWatcher<futuresType>::finished, &cnt, [this]() {
                m_future.reportFinished();
            });

            QObject::connect(&std::get<I>(m_watcher), &QFutureWatcher<futuresType>::canceled, &cnt, [this]() {
                m_future.reportCanceled();
            });

            std::get<I>(m_watcher).setFuture(inFuture);
        }

        waitToAnyFuture<I+1>();
    }

private:
    bool m_unused {true};
    QObject cnt;
    QFutureInterface<std::tuple<std::optional<Ts>...>> m_future;
    std::tuple<std::optional<Ts>...> m_tuple;
    std::tuple<QFuture<Ts>...> m_futures;
    std::tuple<QFutureWatcher<Ts>...> m_watcher;
};
