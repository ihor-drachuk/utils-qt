#pragma once
#include <QObject>
#include <QFuture>
#include <QFutureInterface>
#include <QFutureWatcher>

#include <tuple>
#include <type_traits>
#include <utility>
#include <memory>

namespace UtilsQt {

enum CancellationBehavior
{
    CancelOnSingleCancellation,
    IgnoreSingleCancellation
};

enum TriggerMode
{
    All,
    Any
};

namespace FuturesMergeInternal {

template <typename Tuple1, typename Tuple2, typename F, std::size_t... I>
void for_each_impl(const Tuple1& tuple1, Tuple2&& tuple2, F&& f, std::index_sequence<I...>)
{
    (f(std::get<I>(tuple1), std::get<I>(tuple2)), ...);
}

struct Status
{
    bool anyStarted {};
    bool anyCanceled {};
    bool allCanceled {};
    bool anyWellFinished {};
    bool allFinished {};
};

template<typename T> struct FunctionsT;
template<typename T> struct FunctionsC;

// Parts if std::tuple<QFuture<Ts>...> provided

template<typename... Ts>
struct FunctionsT<std::tuple<QFuture<Ts>...>>
{
    using C = std::tuple<QFuture<Ts>...>;
    using Watchers = std::tuple<QFutureWatcher<Ts>...>;

    static constexpr size_t size(const C&) { return sizeof...(Ts); }

    static Status getStatus(const C& c)
    {
        Status result;

        result.anyStarted = std::apply([](auto&... xs){ return (xs.isStarted() || ...); }, c);
        result.anyCanceled = std::apply([](auto&... xs){ return (xs.isCanceled() || ...); }, c);
        result.allCanceled = std::apply([](auto&... xs){ return (xs.isCanceled() && ...); }, c);
        result.anyWellFinished = std::apply([](auto&... xs){ return ((xs.isFinished() && !xs.isCanceled()) || ...); }, c);
        result.allFinished = std::apply([](auto&... xs){ return (xs.isFinished() && ...); }, c);

        return result;
    }

    template<typename Pred>
    static void setupWatchers(const C& c, Watchers& watchers, const Pred& pred)
    {
        for_each_impl(c, watchers, pred, std::make_index_sequence<sizeof...(Ts)>());
    }

    static void cancelSources(Watchers& watchers)
    {
        std::apply([](auto&... xs){ (xs.future().cancel(), ...); }, watchers);
    }
};

// Parts if Container<QFuture<T>> provided

template <typename T> struct PayloadType;

template<typename T>
struct PayloadType<QFuture<T>>
{
    using Type = T;
};

template<>
struct PayloadType<QFuture<void>>
{
    using Type = void;
};

template<template <typename T, typename... Args> class Container, typename T, typename... Args>
struct FunctionsC<Container<T, Args...>>
{
    using C = Container<T, Args...>;
    using PT = typename PayloadType<T>::Type;
    using Watchers = Container<std::shared_ptr<QFutureWatcher<PT>>>; // Allocator type is skipped, because it could be
                                                                     // dependent on T. T is replaced from QFuture<..> to
                                                                     // std::shared_ptr<QFutureWatcher<..>>
    static size_t size(const C& c) { return c.size(); }

    static Status getStatus(const C& c) {
        Status result;
        result.allCanceled = true;
        result.allFinished = true;

        for (const auto& x : c) {
            result.anyStarted |= x.isStarted();
            result.anyCanceled |= x.isCanceled();
            result.allCanceled &= x.isCanceled();
            result.anyWellFinished |= x.isFinished() && !x.isCanceled();
            result.allFinished &= x.isFinished();
        }

        return result;
    }

    template<typename Pred>
    static void setupWatchers(const C& c, Watchers& watchers, const Pred& pred)
    {
        auto inserter = std::back_inserter(watchers);

        for (const auto& x : c) {
            auto watcher = std::make_shared<QFutureWatcher<PT>>();
            pred(x, *watcher);
            inserter++ = std::move(watcher);
        }
    }

    static void cancelSources(Watchers& watchers)
    {
        for (auto& x : watchers)
            x->future().cancel();
    }
};

template<typename S>
struct Functions;

template<template <typename T, typename... Args> class Container, typename T, typename... Args>
struct Functions<Container<T, Args...>> :
        std::conditional_t<
            std::is_same_v<Container<T, Args...>, std::tuple<T, Args...>>,
            FunctionsT<Container<T, Args...>>,
            FunctionsC<Container<T, Args...>>
        >
{ };

template<typename T>
class Context : public QObject
{
public:
    Context(QObject* ctx, const T& futures, TriggerMode triggerMode, CancellationBehavior cancellationBehavior = CancelOnSingleCancellation)
        : QObject(ctx),
          m_triggerMode(triggerMode),
          m_cancellationBehavior(cancellationBehavior)
    {
        m_totalCnt = Functions<T>::size(futures);

        QObject::connect(&m_outFutureWatcher, &QFutureWatcherBase::canceled, this, &Context<T>::onTargetCanceled);
        m_outFutureWatcher.setFuture(m_outFutureInterface.future());

        const auto status = Functions<T>::getStatus(futures);

        if (status.anyStarted)
            onStarted();

        if (status.allCanceled) {
            m_outFutureInterface.reportCanceled();
            m_outFutureInterface.reportFinished();
            deleteLater();
            return;
        }

        switch (m_cancellationBehavior) {
            case CancelOnSingleCancellation:
                if (status.anyCanceled) {
                    m_outFutureInterface.reportCanceled();
                    m_outFutureInterface.reportFinished();
                    deleteLater();
                    return;
                }

            case IgnoreSingleCancellation:
                break;
        }

        if (status.allFinished || (status.anyWellFinished && m_triggerMode == Any)) {
            m_outFutureInterface.reportFinished();
            deleteLater();
            return;
        }

        auto setupItem = [this](const auto& future, auto& watcher){
            QObject::connect(&watcher, &QFutureWatcherBase::started,  this, &Context<T>::onStarted);
            QObject::connect(&watcher, &QFutureWatcherBase::canceled, this, &Context<T>::onCanceled);
            QObject::connect(&watcher, &QFutureWatcherBase::finished, this, &Context<T>::onFinished);
            watcher.setFuture(future);
        };

        Functions<T>::setupWatchers(futures, m_inWatchers, setupItem);
    }

    ~Context() override
    {
        if (!m_outFutureInterface.isFinished()) {
            m_outFutureInterface.reportCanceled();
            m_outFutureInterface.reportFinished();
            Functions<T>::cancelSources(m_inWatchers);
        }
    }

    QFuture<void> targetFuture() { return m_outFutureInterface.future(); }

private:
    void onStarted()
    {
        if (!m_outFutureInterface.isStarted())
            m_outFutureInterface.reportStarted();
    }

    void onTargetCanceled()
    {
        if (!m_outFutureInterface.isFinished()) {
            m_outFutureInterface.reportCanceled();
            m_outFutureInterface.reportFinished();
            Functions<T>::cancelSources(m_inWatchers);
        }

        deleteLater();
    }

    void onCanceled()
    {
        if (m_outFutureInterface.isFinished())
            return;

        m_canceledCnt++;

        switch (m_cancellationBehavior) {
            case CancelOnSingleCancellation:
                m_outFutureInterface.reportCanceled();
                m_outFutureInterface.reportFinished();
                deleteLater();
                return;

            case IgnoreSingleCancellation:
                const bool allCanceled = (m_canceledCnt == m_totalCnt);
                if (allCanceled) {
                    m_outFutureInterface.reportCanceled();
                    m_outFutureInterface.reportFinished();
                    deleteLater();
                }
                break;
        }
    }

    void onFinished()
    {
        if (m_outFutureInterface.isFinished())
            return;

        m_finishedCnt++;
        const bool allFinished = (m_finishedCnt == m_totalCnt);

        if (m_triggerMode == Any || allFinished) {
            m_outFutureInterface.reportFinished();
            deleteLater();
        }
    }

private:
    TriggerMode m_triggerMode;
    CancellationBehavior m_cancellationBehavior;
    typename Functions<T>::Watchers m_inWatchers;
    QFutureInterface<void> m_outFutureInterface;
    QFutureWatcher<void> m_outFutureWatcher;
    size_t m_totalCnt {};
    size_t m_canceledCnt {};
    size_t m_finishedCnt {};
};

} // namespace FuturesMergeInternal

template<typename... Ts>
QFuture<void> mergeFuturesAll(QObject* context, const QFuture<Ts>&... futures)
{
    auto ctx = new FuturesMergeInternal::Context<std::tuple<QFuture<Ts>...>>(context, std::tie(futures...), All);
    return ctx->targetFuture();
}

template<typename... Ts>
QFuture<void> mergeFuturesAny(QObject* context, const QFuture<Ts>&... futures)
{
    auto ctx = new FuturesMergeInternal::Context<std::tuple<QFuture<Ts>...>>(context, std::tie(futures...), Any);
    return ctx->targetFuture();
}

template<template <typename T, typename... Args> class Container, typename T, typename... Args>
QFuture<void> mergeFuturesAll(QObject* context, const Container<T, Args...>& futures)
{
    auto ctx = new FuturesMergeInternal::Context<Container<T, Args...>>(context, futures, All);
    return ctx->targetFuture();
}

template<template <typename T, typename... Args> class Container, typename T, typename... Args>
QFuture<void> mergeFuturesAny(QObject* context, const Container<T, Args...>& futures)
{
    auto ctx = new FuturesMergeInternal::Context<Container<T, Args...>>(context, futures, Any);
    return ctx->targetFuture();
}

} // namespace UtilsQt
