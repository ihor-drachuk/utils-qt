/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <UtilsQt/Futures/Utils.h>
#include <QObject>
#include <chrono>
#include <tuple>
#include <type_traits>

/*  Overview
 *
 *  There is function which creates one-time QFuture for specified signal.
 *  Usage examples:
 *    auto f = signalToFuture(&obj, &Class::someSignal);
 *    auto f = signalToFuture(&obj, &Class::someSignal, this);       // With lifetime context
 *    auto f = signalToFuture(&obj, &Class::someSignal, this, 1min); // With lifetime context and timeout
 *
 *  Return type depends on signal parameters count and types:
 *    0  - QFuture<void>
 *    1  - QFuture<T>
 *    2+ - QFuture<std::tuple<Args...>>
 */

namespace UtilsQt {

namespace Internal {

void deleteAfter(QObject* object, std::chrono::milliseconds timeout);

template<size_t Count, typename... Args>
struct ResultProviderImpl {
    using ReturnType = std::tuple<std::remove_cv_t<std::remove_reference_t<Args>>...>;

    static void apply(Promise<ReturnType>& promise, const Args&... args) { promise.finish({args...}); }
};

template<typename T>
struct ResultProviderImpl<1, T> {
    using ReturnType = std::remove_cv_t<std::remove_reference_t<T>>;
    static void apply(Promise<ReturnType>& promise, const T& arg) { promise.finish(arg); }
};

template<>
struct ResultProviderImpl<0> {
    using ReturnType = void;
    static void apply(Promise<ReturnType>& promise) { promise.finish(); }
};

template<typename... Args>
struct ResultProvider : public ResultProviderImpl<sizeof...(Args), Args...> { };


} // namespace Internal

template<typename Object, typename... Args,
         typename ResultType = typename Internal::ResultProvider<Args...>::ReturnType,
         typename std::enable_if<std::is_base_of<QObject, Object>::value>::type* = nullptr>
QFuture<ResultType> signalToFuture(Object* object, void (Object::* signal)(Args...), QObject* context = nullptr, std::chrono::milliseconds timeout = {})
{
    assert(timeout.count() >= 0);

    auto promise = UtilsQt::createPromise<ResultType>(true);

    auto ctx = new QObject();
    QObject::connect(object, &QObject::destroyed, ctx, &QObject::deleteLater);
    if (context) QObject::connect(context, &QObject::destroyed, ctx, &QObject::deleteLater);

    QObject::connect(object, signal, ctx, [promise, ctx](Args... args) mutable {
        if (!promise.isFinished()) Internal::ResultProvider<Args...>::apply(promise, args...);
        ctx->deleteLater();
    });

    QObject::connect(ctx, &QObject::destroyed, [promise]() mutable {
        if (!promise.isFinished()) promise.cancel();
    });

    if (timeout.count() > 0) {
        Internal::deleteAfter(ctx, timeout);
    }

    return promise.future();
}

} // namespace UtilsQt
