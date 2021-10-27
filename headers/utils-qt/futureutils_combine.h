#pragma once
#include <tuple>
#include <array>
#include <memory>
#include <QObject>
#include <QTimer>
#include <QFuture>
#include <QFutureInterface>
#include <QFutureWatcher>
#include <QList>

namespace FutureUtils {

enum class CombineTrigger
{
    All,
    Any
};

} // namespace FutureUtils


namespace Internal {

inline bool passBool(bool v) { return v; }

template<typename... Args>
struct CombineResult
{
    using Types = std::tuple<Args...>;
    static constexpr size_t Size = sizeof...(Args);

    std::tuple<QFuture<Args>...> sourceFutures;
    std::array<bool, Size> iFinished;
    std::array<bool, Size> iHasResult;
    std::array<bool, Size> iCanceled;

    bool timeout { false };
    bool lostContext { false };
    size_t finishedCnt { 0 };
    size_t hasResultCnt { 0 };
    size_t canceledCnt { 0 };

    bool allFinished() const { return (finishedCnt == Size); };
    bool anyFinished() const { return (finishedCnt > 0); };

    bool allHasResult() const { return (hasResultCnt == Size); };
    bool anyHasResult() const { return (hasResultCnt > 0); };

    bool allCanceled() const { return (canceledCnt == Size); };
    bool anyCanceled() const { return (canceledCnt > 0); };

    bool aborted() const { return timeout || lostContext; }

    template<size_t I>
    const QFuture<std::tuple_element_t<I, Types>> getFuture() const { return std::get<I>(sourceFutures); }
};

template<typename... Args>
using CombineResultPtr = std::shared_ptr<CombineResult<Args...>>;

template<typename... Args>
using CombineResultCPtr = std::shared_ptr<const CombineResult<Args...>>;

struct CRContextBase : public QObject
{
    Q_OBJECT
};

template<typename... Args>
struct CRContext : public CRContextBase
{
public:
    CRContext(FutureUtils::CombineTrigger trigger, QObject* context, int timeout)
        : trigger(trigger)
    {
        result = std::make_shared<CombineResult<Args...>>();

        futureResult.reportStarted();

        if (context) {
            QObject::connect(context, &QObject::destroyed, this, &CRContext<Args...>::onLostContext);
        }

        if (timeout > 0) {
            QObject::connect(&timer, &QTimer::timeout, this, &CRContext<Args...>::onTimeout);
            timer.start(timeout);
        }
    }

    ~CRContext() override
    {
        for (auto x : watchers)
            delete x;
    }

    void onLostContext() { if (done) return; result->lostContext = true; onAbort(false); }
    void onTimeout()     { if (done) return; result->timeout = true;     onAbort(true); }

    template<size_t I>
    void onFinished()
    {
        if (done) return;

        auto future = std::get<I>(result->sourceFutures);
        result->iFinished[I] = future.isFinished();
        result->iHasResult[I] = future.isFinished() && !future.isCanceled();
        result->iCanceled[I] = future.isCanceled();

        if (result->iFinished[I])  result->finishedCnt++;
        if (result->iHasResult[I]) result->hasResultCnt++;
        if (result->iCanceled[I])  result->canceledCnt++;

        onCheck();
    }

    void onCheck()
    {
        if (trigger == FutureUtils::CombineTrigger::Any && result->anyFinished()) {
            onResult();

        } else if (result->allFinished()) {
            assert(trigger == FutureUtils::CombineTrigger::All);
            onResult();
        }
    }


    void onResult()
    {
        assert(!done);
        done = true;
        futureResult.reportResult(result);
        futureResult.reportFinished();
        deleteLater();
    }

    void onAbort(bool allowPartial)
    {
        assert(!done);
        done = true;

        if (allowPartial) {
            futureResult.reportResult(result);
        } else {
            futureResult.reportCanceled();
        }

        futureResult.reportFinished();
        deleteLater();
    }

    bool done { false };
    FutureUtils::CombineTrigger trigger;
    QTimer timer;
    QFutureInterface<CombineResultCPtr<Args...>> futureResult;
    QList<QFutureWatcherBase*> watchers;
    CombineResultPtr<Args...> result;
};

template<size_t I = 0, typename... Args,
         typename std::enable_if<I == sizeof...(Args)>::type* = nullptr>
void setupContext(CRContext<Args...>&)
{
}

template<size_t I = 0, typename... Args,
         typename std::enable_if<!(I >= sizeof...(Args))>::type* = nullptr>
void setupContext(CRContext<Args...>& ctx)
{
    using Type = std::tuple_element_t<I, std::tuple<Args...>>;

    auto future = std::get<I>(ctx.result->sourceFutures);
    ctx.result->iFinished[I] = future.isFinished();
    ctx.result->iHasResult[I] = future.isFinished() && !future.isCanceled();
    ctx.result->iCanceled[I] = future.isCanceled();

    if (ctx.result->iFinished[I])  ctx.result->finishedCnt++;
    if (ctx.result->iHasResult[I]) ctx.result->hasResultCnt++;
    if (ctx.result->iCanceled[I])  ctx.result->canceledCnt++;

    if (!future.isFinished()) {
        auto watcher = new QFutureWatcher<Type>();
        QObject::connect(watcher, &QFutureWatcherBase::finished, &ctx, [&ctx](){ ctx.template onFinished<I>(); });
        ctx.watchers.append(watcher);
        watcher->setFuture(future);
    }

    setupContext<I+1>(ctx);
}

} // namespace Internal


template<typename... Args>
QFuture<Internal::CombineResultCPtr<Args...>> combineFutures(FutureUtils::CombineTrigger trigger, QObject* context, int timeout, const QFuture<Args>&... futures)
{
    using namespace Internal;

    auto* data = new CRContext<Args...>(trigger, context, timeout);
    auto future = data->futureResult.future();
    data->result->sourceFutures = std::forward_as_tuple(futures...);
    data->result->iFinished.fill(false);
    data->result->iHasResult.fill(false);
    data->result->iCanceled.fill(false);

    setupContext(*data);
    data->onCheck();

    return future;
}
