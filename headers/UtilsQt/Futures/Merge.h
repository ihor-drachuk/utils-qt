/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <QObject>
#include <QFuture>
#include <QFutureInterface>
#include <QFutureWatcher>

#include <tuple>
#include <type_traits>
#include <utility>
#include <memory>
#include <optional>

#include <UtilsQt/Futures/Utils.h>
#include <UtilsQt/Futures/Converter.h>
#include <utils-cpp/default_ctor_ops.h>

/*
      Description
-----------------------

 - QFuture mergeFuturesAll(context, flags, futures...);
 - QFuture mergeFuturesAll(context, flags, Container<Future>);
 - QFuture mergeFuturesAll(context, flags, std::tuple<Future>);
 Return future which is finished when all source futures are finished.

 - QFuture mergeFuturesAny(context, flags, futures...);
 - QFuture mergeFuturesAny(context, flags, Container<Future>);
 - QFuture mergeFuturesAny(context, flags, std::tuple<Future>);
 Return future which is finished when any of source futures is finished.

 'flags' can be omitted.

 Here are rules how return type is determined, based on source futures:
  -  QFuture<T1>, QFuture<T2>...  =>  QFuture<std::tuple<std::optional<T1>, std::optional<T2>, ...>>
  -  Container<QFuture<T>>        =>  QFuture<Container<std::optional<T>>>

 In other words:
  - QFuture is replaced by std::optional and returned in same container.
  - This container is wrapped by QFuture.

 Example:
 /
 |  auto f1 = createTimedFuture<std::string>(100, "str");
 |  auto f2 = createTimedFuture<double>     (110, 123.83);
 |  auto f = mergeFuturesAll(this, f1, f2);
 \

 In this example, f's type is
 /
 |  QFuture<
 |    std::tuple<
 |      std::optional<std::string>,    <-- it's nullopt if relevant source future is canceled
 |      std::optional<double>              and IgnoreSomeCancellation flag provided.
 |    >
 |  >
 \

 'context' controls lifetime. If 'context' is gone, resulting future will be canceled.
 If 'context' is nullptr, then resulting future will be canceled immediately.
 This behavior can be overridden by flag IgnoreNullContext.

 If any of source futures is canceled, then resulting future will be canceled as well.
 This behavior can be overridden by flag IgnoreSomeCancellation.

 If resulting(!) future is canceled, source futures will be canceled automatically as well.
 So futures cancellation is transitive and bi-directional.

 Possible 'flags' values:
  - IgnoreSomeCancellation - cancel resulting future only when ALL source futures are/become canceled.
  - IgnoreNullContext      - don't cancel resulting future if nullptr is passed as a context.
*/


namespace UtilsQt {

enum class MergeFlags
{
    IgnoreSomeCancellation = 1, // Cancel if all source futures are canceled,
                                // othwerwise cancel on first cancellation (default)

    IgnoreNullContext = 2       // Allows to pass nullptr as a context and keep mergeFutures
                                // depend only on source QFutures lifetime.
                                // Otherwise, cancel on null context (default)
};

} // namespace UtilsQt

inline UtilsQt::MergeFlags operator| (UtilsQt::MergeFlags a, UtilsQt::MergeFlags b)
{
    return static_cast<UtilsQt::MergeFlags>(static_cast<int>(a) | static_cast<int>(b));
}

inline int operator& (UtilsQt::MergeFlags a, UtilsQt::MergeFlags b)
{
    return static_cast<int>(a) & static_cast<int>(b);
}

namespace UtilsQt {

enum TriggerMode
{
    All,
    Any
};

namespace FuturesMergeInternal {

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
        for_each_tuple_pair(pred, c, watchers);
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

template<template <typename, typename...> class Container, typename T, typename... Args>
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
            pred(x, *watcher, nullptr);
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

template<template <typename...> class Container, typename... Args>
struct Functions<Container<Args...>> :
        std::conditional_t<
            std::is_same_v<Container<Args...>, std::tuple<Args...>>,
            FunctionsT<Container<Args...>>,
            FunctionsC<Container<Args...>>
        >
{ };

template<typename T>
class Context : public QObject
{
    NO_COPY_MOVE(Context);
public:
    Context(QObject* ctx, const T& futures, TriggerMode triggerMode, MergeFlags mergeFlags)
        : QObject(ctx),
          m_triggerMode(triggerMode),
          m_mergeFlags(mergeFlags)
    {
        m_totalCnt = Functions<T>::size(futures);

        QObject::connect(&m_outFutureWatcher, &QFutureWatcherBase::canceled, this, &Context<T>::onTargetCanceled);
        m_outFutureWatcher.setFuture(m_outFutureInterface.future());

        const auto status = Functions<T>::getStatus(futures);

        if (status.anyStarted)
            onStarted();

        const auto cancelByContext = (!ctx && !(mergeFlags & MergeFlags::IgnoreNullContext));
        if (status.allCanceled || cancelByContext) {
            m_outFutureInterface.reportCanceled();
            m_outFutureInterface.reportFinished();
            deleteLater();
            return;
        }

        if (!(m_mergeFlags & MergeFlags::IgnoreSomeCancellation)) {
            if (status.anyCanceled) {
                m_outFutureInterface.reportCanceled();
                m_outFutureInterface.reportFinished();
                deleteLater();
                return;
            }
        }

        if (status.allFinished || (status.anyWellFinished && m_triggerMode == Any)) {
            m_outFutureInterface.reportFinished();
            deleteLater();
            return;
        }

        auto setupItem = [this](const auto& future, auto& watcher, auto){
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

        const bool allCanceled = (m_canceledCnt == m_totalCnt);
        const bool cancelOnSingle = !(m_mergeFlags & MergeFlags::IgnoreSomeCancellation);
        if (allCanceled || cancelOnSingle) {
            m_outFutureInterface.reportCanceled();
            m_outFutureInterface.reportFinished();
            deleteLater();
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
    MergeFlags m_mergeFlags;
    typename Functions<T>::Watchers m_inWatchers;
    QFutureInterface<void> m_outFutureInterface;
    QFutureWatcher<void> m_outFutureWatcher;
    size_t m_totalCnt {};
    size_t m_canceledCnt {};
    size_t m_finishedCnt {};
};

} // namespace FuturesMergeInternal


// mergeFuturesAll

// Return type: QFuture<std::tuple<std::optional<Ts>...>>.
// Also notice: `std::optional<void>` is replaced by `bool`.
template<typename... Ts,
         typename ResultContent = typename ResultContentType<Ts...>::Type> // Hint (just for example)
QFuture<ResultContent> mergeFuturesAll(QObject* context, MergeFlags mergeFlags, const QFuture<Ts>&... futures)
{
    auto ctx = new FuturesMergeInternal::Context<std::tuple<QFuture<Ts>...>>(context, std::tie(futures...), All, mergeFlags);
    auto f = ctx->targetFuture();
    auto resultFuture = convertFuture<void, ResultContent>(context, f, ConverterFlags::IgnoreNullContext, [futures...]() -> std::optional<ResultContent> {
        return futuresToOptResults(std::make_tuple(futures...));
    });
    return resultFuture;
}

template<typename... Ts>
auto mergeFuturesAll(QObject* context, const QFuture<Ts>&... futures)
{
    return mergeFuturesAll<Ts...>(context, {}, futures...);
}

// Return type: QFuture<Container<std::optional<T>>>.
// Also notice: `std::optional<void>` is replaced by `bool`.
template<template <typename, typename...> class Container, typename T, typename... Args>
auto mergeFuturesAll(QObject* context, MergeFlags mergeFlags, const Container<T, Args...>& futures)
{
    auto ctx = new FuturesMergeInternal::Context<Container<T, Args...>>(context, futures, All, mergeFlags);
    auto f = ctx->targetFuture();
    auto resultFuture = convertFuture(context, f, ConverterFlags::IgnoreNullContext, [futures]() { // No hint
        return futuresToOptResults(futures);
    });
    return resultFuture;
}

template<template <typename, typename...> class Container, typename T, typename... Args>
auto mergeFuturesAll(QObject* context, const Container<T, Args...>& futures)
{
    return mergeFuturesAll(context, {}, futures);
}

#ifndef UTILS_QT_COMPILER_GCC
template<typename... Ts>
auto mergeFuturesAll(QObject* context, MergeFlags mergeFlags, const std::tuple<QFuture<Ts>...>& futures)
{
    return std::apply([context, mergeFlags](auto&&... xs){ return mergeFuturesAll<Ts...>(context, mergeFlags, xs...); }, futures);
}

template<typename... Ts>
auto mergeFuturesAll(QObject* context, const std::tuple<QFuture<Ts>...>& futures)
{
    return mergeFuturesAll<Ts...>(context, {}, futures);
}
#endif // UTILS_QT_COMPILER_GCC


// mergeFuturesAny

// Return type: QFuture<std::tuple<std::optional<Ts>...>>.
// Also notice: `std::optional<void>` is replaced by `bool`.
template<typename... Ts,
         typename ResultContent = typename ResultContentType<Ts...>::Type>
QFuture<ResultContent> mergeFuturesAny(QObject* context, MergeFlags mergeFlags, const QFuture<Ts>&... futures)
{
    auto ctx = new FuturesMergeInternal::Context<std::tuple<QFuture<Ts>...>>(context, std::tie(futures...), Any, mergeFlags);
    auto f = ctx->targetFuture();
    auto resultFuture = convertFuture<void, ResultContent>(context, f, ConverterFlags::IgnoreNullContext, [futures...]() -> std::optional<ResultContent> {
        return futuresToOptResults(std::make_tuple(futures...));
    });
    return resultFuture;
}

template<typename... Ts>
auto mergeFuturesAny(QObject* context, const QFuture<Ts>&... futures)
{
    return mergeFuturesAny<Ts...>(context, {}, futures...);
}

// Return type: QFuture<Container<std::optional<T>>>.
// Also notice: `std::optional<void>` is replaced by `bool`.
template<template <typename, typename...> class Container, typename T, typename... Args>
auto mergeFuturesAny(QObject* context, MergeFlags mergeFlags, const Container<T, Args...>& futures)
{
    auto ctx = new FuturesMergeInternal::Context<Container<T, Args...>>(context, futures, Any, mergeFlags);
    auto f = ctx->targetFuture();
    auto resultFuture = convertFuture(context, f, ConverterFlags::IgnoreNullContext, [futures]() {
        return futuresToOptResults(futures);
    });
    return resultFuture;
}

template<template <typename, typename...> class Container, typename T, typename... Args>
auto mergeFuturesAny(QObject* context, const Container<T, Args...>& futures)
{
    return mergeFuturesAny(context, {}, futures);
}

#ifndef UTILS_QT_COMPILER_GCC
template<typename... Ts>
auto mergeFuturesAny(QObject* context, MergeFlags mergeFlags, const std::tuple<QFuture<Ts>...>& futures)
{
    return std::apply([context, mergeFlags](auto&&... xs){ return mergeFuturesAny<Ts...>(context, mergeFlags, xs...); }, futures);
}

template<typename... Ts>
auto mergeFuturesAny(QObject* context, const std::tuple<QFuture<Ts>...>& futures)
{
    return mergeFuturesAny<Ts...>(context, {}, futures);
}
#endif // #ifndef UTILS_QT_COMPILER_GCC

} // namespace UtilsQt
