/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <QObject>
#include <QFuture>
#include <QFutureWatcher>
#include <QTimer>
#include <optional>
#include <utils-cpp/copy_move.h>

/*            Reminder
  -------------------------------

  QFuture<T> f = createRetryingFuture(
                       context,       // controls lifetime
                       asyncCall,     // []() -> QFuture<T> { ... }
                       validator,     // [](const std::optional<T>& result) -> ValidatorDecision { ... }
                       callsLimit,    // Default: 3
                       callsInterval  // Default: 1000
  );

  If `T` is `void`, then validator has another form:
  [](bool result) -> ValidatorDecision { ... }

  `result` is `nullopt` if `asyncCall` returned canceled future.

  `f` is cancelled if:
    - Validator decided to cancel. It's recommended to do so if `result` is `nullopt`.
    - Validator decided to retry, but retries amount reached limit.
    - `context` is gone

  `f` is finished with result:
    - IF    validator replied `ResultIsValid`
    - WHILE received `result` is NOT `nullopt`
    - WHILE `context` is alive

  If `f` is canceled by user, then source future will be canceled as well.
  No retries are executed in this case.
*/

namespace UtilsQt {

enum ValidatorDecision
{
    Cancel,
    NeedRetry,
    ResultIsValid
};

template<typename T>
struct RetryingResult
{
    T result {};
    bool isOk {};
};

template<>
struct RetryingResult<void>
{
    bool isOk {};
};

namespace RetryingFutureInternal {

template<typename T>
struct QFutureUnwrapper;

template<typename T>
struct QFutureUnwrapper<QFuture<T>>
{
    using Type = T;
};

struct ContextHelperBase
{
    template<typename T>
    static void reportCanceled(QFutureInterface<RetryingResult<T>>& target) {
        target.reportCanceled();
        target.reportFinished();
    }
};

template<typename T>
struct ContextHelper;

template<typename T>
struct ContextHelper<RetryingResult<T>> : ContextHelperBase
{
    static void reportFinished(QFutureInterface<RetryingResult<T>>& target, const QFuture<T>& src, bool ok) {
        target.reportResult(RetryingResult<T> {src.result(), ok});
        target.reportFinished();
    }

    template<typename Validator>
    static ValidatorDecision callValidator(const QFuture<T>& src, const Validator& validator) {
        return validator(src.isCanceled() ? std::optional<T>() :
                                            std::optional<T>(src.result()));
    }

    static auto getDefaultValidator() {
        return [](const std::optional<T>& result){
            return result ? ValidatorDecision::ResultIsValid :
                            ValidatorDecision::Cancel;
        };
    }
};

template<>
struct ContextHelper<RetryingResult<void>> : ContextHelperBase
{
    static void reportFinished(QFutureInterface<RetryingResult<void>>& target, const QFuture<void>&, bool ok) {
        target.reportResult(RetryingResult<void>{ok});
        target.reportFinished();
    }

    template<typename Validator>
    static ValidatorDecision callValidator(const QFuture<void>& src, const Validator& validator) {
        return validator(!src.isCanceled());
    }

    static auto getDefaultValidator() {
        return [](bool result){
            return result ? ValidatorDecision::ResultIsValid :
                            ValidatorDecision::Cancel;
        };
    }
};

template<typename T>
struct SmartValidator;

template<>
struct SmartValidator<bool>
{
    static auto getValidator() {
        return [](const std::optional<bool>& result){
            return result ?  *result ? ValidatorDecision::ResultIsValid :
                                       ValidatorDecision::NeedRetry :
                             ValidatorDecision::Cancel;
        };
    }
};

template<typename AsyncCall, typename ResultValidator, typename PayloadType>
struct Context : public QObject
{
    NO_COPY_MOVE(Context);
    using This = Context<AsyncCall, ResultValidator, PayloadType>;
    using Helper = ContextHelper<RetryingResult<PayloadType>>;

    Context(QObject* context,
            const AsyncCall& asyncCall,
            ResultValidator resultValidator,
            unsigned int callsLimit,
            unsigned int callsInterval)
        : QObject(context),
          m_asyncCall(asyncCall),
          m_resultValidator(resultValidator),
          m_callsLimit(callsLimit),
          m_callsInterval(callsInterval)
    {
        assert(m_callsLimit >= 1);

        QObject::connect(&m_targetFutureWatcher, &QFutureWatcherBase::canceled, this, &This::onTargetCanceled);
        m_targetFutureWatcher.setFuture(m_targetFuture.future());

        QObject::connect(&m_inFutureWatcher, &QFutureWatcherBase::finished, this, &This::onFinished);

        m_targetFuture.reportStarted();
        doCall();
    }

    ~Context() override
    {
        if (!m_targetFuture.isFinished())
            Helper::reportCanceled(m_targetFuture);

        if (!m_inFutureWatcher.isFinished())
            m_inFutureWatcher.cancel();
    }

    QFuture<RetryingResult<PayloadType>> getTargetFuture() { return m_targetFuture.future(); }

private:
    void onTargetCanceled() {
        Helper::reportCanceled(m_targetFuture);

        if (!m_inFutureWatcher.isFinished())
            m_inFutureWatcher.cancel();

        deleteLater();
    }

    void onFinished() {
        auto decision = Helper::callValidator(m_inFutureWatcher.future(), m_resultValidator);

        switch (decision) {
            case ValidatorDecision::Cancel:
                Helper::reportCanceled(m_targetFuture);
                deleteLater();
                break;

            case ValidatorDecision::NeedRetry:
                if (m_callsDone == m_callsLimit) {
                    Helper::reportFinished(m_targetFuture, m_inFutureWatcher.future(), false);
                    deleteLater();
                    return;
                }

                assert(m_callsDone < m_callsLimit);

                if (m_callsInterval) {
                    QTimer::singleShot(m_callsInterval, this, &This::doCall);
                } else {
                    doCall();
                }

                break;

            case ValidatorDecision::ResultIsValid:
                assert(!m_inFutureWatcher.isCanceled());
                Helper::reportFinished(m_targetFuture, m_inFutureWatcher.future(), true);
                deleteLater();
                break;
        }
    }

    void doCall() {
        m_callsDone++;

        auto f = m_asyncCall();
        m_inFutureWatcher.setFuture(f);
    }

private:
    AsyncCall m_asyncCall;
    ResultValidator m_resultValidator;
    unsigned int m_callsLimit {};
    unsigned int m_callsInterval {};
    unsigned int m_callsDone {};

    QFutureInterface<RetryingResult<PayloadType>> m_targetFuture;
    QFutureWatcher<RetryingResult<PayloadType>> m_targetFutureWatcher;

    QFutureWatcher<PayloadType> m_inFutureWatcher;
};

} // namespace RetryingFutureInternal

template<typename AsyncCall,
         typename RT = std::result_of_t<AsyncCall()>,
         typename PT = typename RetryingFutureInternal::QFutureUnwrapper<RT>::Type,
         typename Helper = RetryingFutureInternal::ContextHelper<RetryingResult<PT>>,
         typename ResultValidator = decltype(Helper::getDefaultValidator())
         >
QFuture<RetryingResult<PT>> createRetryingFuture(QObject* context,
                                const AsyncCall& asyncCall,
                                ResultValidator resultValidator = Helper::getDefaultValidator(),
                                unsigned int callsLimit = 3,
                                unsigned int callsInterval = 1000)
{
    using Context = RetryingFutureInternal::Context<AsyncCall, ResultValidator, PT>;
    auto ctx = new Context(context, asyncCall, resultValidator, callsLimit, callsInterval);

    return ctx->getTargetFuture();
}

template<typename T>
auto getSmartValidator()
{
    return RetryingFutureInternal::SmartValidator<T>::getValidator();
}

} // namespace UtilsQt
