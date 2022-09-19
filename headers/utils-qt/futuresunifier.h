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
        : m_inFutures(futures)
    {
        if (autodelete) {
            using ResultType = decltype(m_outFutureResults.future().result());
            auto selfWatcher = new QFutureWatcher<ResultType>(this);
            QObject::connect(selfWatcher, &QFutureWatcherBase::finished, this, &QObject::deleteLater);
            selfWatcher->setFuture(m_outFutureResults.future());
        }

        QObject::connect(&m_outFutureWatcher, &QFutureWatcherBase::canceled, this, [this](){
            std::apply([this](auto&... xs){ (xs.cancel(), ...); }, m_inFutures);
        });

        m_outFutureWatcher.setFuture(m_outFutureResults.future());
    }

    ~FuturesUnifier()
    {
        if (!m_used) return;

        if (!m_outFutureResults.isFinished()) {
            m_outFutureResults.reportCanceled();
            m_outFutureResults.reportFinished();
        }
    }

    QFuture<std::tuple<std::optional<Ts>...>> mergeFuturesAll()
    {
        Q_ASSERT(!m_used);
        m_used = true;

        m_outFutureResults.setProgressRange(0, m_maxCounter);

        waitForAllFutures();

        return m_outFutureResults.future();
    }

    auto mergeFuturesAny()
    {
        Q_ASSERT(!m_used);
        m_used = true;

        waitForAnyFuture();

        return m_outFutureResults.future();
    }

private:
    template <size_t I = 0>
    void waitForAllFutures(typename std::enable_if<!( I < sizeof... (Ts)), void>::type* = nullptr)
    {
    }

    template <size_t I = 0>
    void waitForAllFutures(typename std::enable_if<!( I >= sizeof... (Ts)), void>::type* = nullptr)
    {
        auto inFuture = std::get<I>(m_inFutures);
        if (inFuture.isFinished()) {
            if (inFuture.isCanceled()) {
                std::get<I>(m_results) = {};
            } else {
                std::get<I>(m_results) = inFuture.result();
            }

            m_resultsCounter++;
            m_outFutureResults.setProgressValue(m_resultsCounter);

            if (m_resultsCounter == m_maxCounter) {
                m_outFutureResults.reportResult(m_results);
                m_outFutureResults.reportFinished();
            }
        } else {
            using FuturesTypeI = std::tuple_element<I, std::tuple<Ts...>>;
            QObject::connect(&std::get<I>(m_inWatchers), &QFutureWatcher<FuturesTypeI>::finished, this, [this]() {
                if (std::get<I>(m_inWatchers).isCanceled()) {
                    std::get<I>(m_results) = {};
                } else {
                    std::get<I>(m_results) = std::get<I>(m_inWatchers).result();
                }

                m_resultsCounter++;
                m_outFutureResults.setProgressValue(m_resultsCounter);

                if (m_resultsCounter == m_maxCounter) {
                    m_outFutureResults.reportResult(m_results);
                    m_outFutureResults.reportFinished();
                }
            });

            std::get<I>(m_inWatchers).setFuture(inFuture);
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
        auto inFuture = std::get<I>(m_inFutures);
        if (inFuture.isFinished()) {
            m_outFutureResults.reportFinished();
            return;
        } else if (inFuture.isCanceled()) {
            m_outFutureResults.reportCanceled();
            return;
        } else {
            using FuturesTypeI = std::tuple_element<I, std::tuple<Ts...>>;
            QObject::connect(&std::get<I>(m_inWatchers), &QFutureWatcher<FuturesTypeI>::finished, this, [this]() {
                m_outFutureResults.reportFinished();
            });

            QObject::connect(&std::get<I>(m_inWatchers), &QFutureWatcher<FuturesTypeI>::canceled, this, [this]() {
                m_outFutureResults.reportCanceled();
            });

            std::get<I>(m_inWatchers).setFuture(inFuture);
        }

        waitForAnyFuture<I+1>();
    }

private:
    bool m_used {false};
    int m_resultsCounter {};
    static constexpr int m_maxCounter {sizeof...(Ts)};
    QFutureInterface<std::tuple<std::optional<Ts>...>> m_outFutureResults;
    QFutureWatcher<std::tuple<std::optional<Ts>...>> m_outFutureWatcher;
    std::tuple<std::optional<Ts>...> m_results;
    std::tuple<QFuture<Ts>...> m_inFutures;
    std::tuple<QFutureWatcher<Ts>...> m_inWatchers;
};

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

} // namespace Internal

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
