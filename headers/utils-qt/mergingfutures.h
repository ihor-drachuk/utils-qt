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
        m_future.setProgressRange(0, sizeof... (Ts));

        waitToFuture(m_futures);

        return m_future.future();
    }

private:
    template <size_t I = 0>
    void waitToFuture(const std::tuple<QFuture<Ts>...>&,
                      typename std::enable_if<!( I < sizeof... (Ts)), void>::type* = nullptr)
    {

    }

    template <size_t I = 0>
    void waitToFuture(const std::tuple<QFuture<Ts>...>& futures,
                      typename std::enable_if<!( I >= sizeof... (Ts)), void>::type* = nullptr)
    {
        auto inFuture = std::get<I>(futures);
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
            auto fw = new QFutureWatcher<typename futuresType::type>;
            QObject::connect(fw, &QFutureWatcher<futuresType>::finished, &cnt, [this, fw]() {
                m_future.setProgressValue(m_future.progressValue() + 1);
                std::get<I>(m_tuple) = fw->result();

                if (m_future.progressValue() == m_future.progressMaximum()) {
                    m_future.reportResult(m_tuple);
                    m_future.reportFinished();
                }

                fw->deleteLater();
            });

            QObject::connect(fw, &QFutureWatcher<futuresType>::canceled, &cnt, [this, fw]() {
                m_future.setProgressValue(m_future.progressValue() + 1);
                std::get<I>(m_tuple) = {};

                if (m_future.progressValue() == m_future.progressMaximum()) {
                    m_future.reportResult(m_tuple);
                    m_future.reportFinished();
                }

                fw->deleteLater();
            });

            fw->setFuture(inFuture);
        }

        waitToFuture<I+1>(futures);
    }

private:
    QObject cnt;
    QFutureInterface<std::tuple<std::optional<Ts>...>> m_future;
    std::tuple<std::optional<Ts>...> m_tuple;
    std::tuple<QFuture<Ts>...> m_futures;
};
