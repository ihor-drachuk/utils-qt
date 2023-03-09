#pragma once
#include <UtilsQt/Futures/Utils.h>
#include <UtilsQt/invoke_method.h>

// TODO:  Add support of QFuture<void> sequences.

/*
  Reminder

  connectFutureSeq(QFuture<T>)
      .then([](optional<T>) -> QFuture<G> {})
      .then([](optional<G>) -> QFuture<L> {})
      .then([](optional<L>) {})
      .onError([](std::exception_ptr) {});


  QFuture<L> finalResultFuture;

  connectFutureSeq(QFuture<T>)
      .then([](optional<T>) -> QFuture<G> {})
      .then([](optional<G>) -> QFuture<L> {})
      .getFuture(finalResultFuture)
      .onError([](std::exception_ptr) {});


  Notice!
  If intermediate handler returned canceled future, then next handlers must return canceled future as well!
*/

namespace FutureUtilsInternals {

template <typename T>
struct ExpandQFuture { };

template <typename T>
struct ExpandQFuture<QFuture<T>>
{
    using value_type = T;
};

template<typename T>
struct IsQFuture : public std::false_type { };

template <template<typename> typename F, typename T>
struct IsQFuture<F<T>> : public std::is_same<F<T>, QFuture<T>> {};


struct Context {
    Context (QObject* context, Qt::ConnectionType connectionType)
            : context(context),
              connectionType(connectionType)
    { }

    QObject ownContext;
    QObject* context;
    Qt::ConnectionType connectionType;

    std::function<void(std::exception_ptr)> errorHandler;

    std::exception_ptr ex;
    bool errorFlag { false };
    bool delayedError { false };
    bool needInternalHandler { true };

    void errorOccured() {
        if (errorHandler) {
            UtilsQt::invokeMethod(
                        context,
                        [handler = errorHandler, ex = ex](){ handler(ex); },
                        connectionType);
        } else {
            delayedError = true;
        }
    }

    void errorOccured_Internal() {
        if (!needInternalHandler) return;
        errorOccured();
    }
};

using ContextPtr = std::shared_ptr<Context>;

class FutureResultBase
{
public:
    FutureResultBase(const ContextPtr& internalContext)
        : m_internalContext(internalContext)
    { }


    FutureResultBase& onError(const std::function<void(std::exception_ptr)>& callable) {
        m_internalContext->errorHandler = callable;

        if (getContext()->delayedError) {
            getContext()->delayedError = false;
            getContext()->errorOccured();
        }

        return *this;
    }

    QFuture<void> readyPromise()
    {
        auto f = UtilsQt::createPromise<void>();
        QObject::connect(&getContext()->ownContext, &QObject::destroyed, [f]() mutable { f.finish(); });
        return f.future();
    }

protected:
    ContextPtr getContext() { return m_internalContext; }

private:
    ContextPtr m_internalContext;
};

template<typename T>
class FutureResult : public FutureResultBase {
public:
    FutureResult(const QFuture<T>& future, QObject* context, Qt::ConnectionType connectionType)
        : FutureResultBase(std::make_shared<Context>(context, connectionType)),
          m_future(future),
          m_context(context),
          m_connectionType(connectionType)
    {
    }

    FutureResult(const QFuture<T>& future, QObject* context, Qt::ConnectionType connectionType, const ContextPtr& internalContext)
        : FutureResultBase(internalContext),
          m_future(future),
          m_context(context),
          m_connectionType(connectionType)
    {
        getContext()->needInternalHandler = false;
    }

    // QFuture<T> --> std::optional<T> --> QFuture<Next>
    template<typename Callable, typename Ret = typename std::invoke_result<Callable, std::optional<T>>::type,
             typename std::enable_if<IsQFuture<Ret>::value>::type* = nullptr,
             typename RetFutureType = typename ExpandQFuture<Ret>::value_type>
    FutureResult<RetFutureType> then(Callable&& callable) {
        getContext()->needInternalHandler = false;

        QFutureInterface<RetFutureType> interface;
        interface.reportStarted();

        UtilsQt::onFinished(m_future, m_context,
                   [interface, callable, internalContext = getContext(), m_context = m_context, m_connectionType = m_connectionType]
                   (const std::optional<T>& result) mutable {
            auto interface2 = interface;

            if (!internalContext->errorFlag) {
                QFuture<RetFutureType> nextFuture;

                try {
                    nextFuture = callable(result);
                } catch (...) {
                    // Just notify about error
                    internalContext->errorFlag = true;

                    nextFuture = UtilsQt::createCanceledFuture<RetFutureType>();
                    internalContext->ex = std::current_exception();
                    internalContext->errorOccured();
                    interface2.reportCanceled();
                    interface2.reportFinished();
                    return;
                }

                UtilsQt::onFinished(nextFuture, m_context, [interface, internalContext](const std::optional<RetFutureType>& result) {
                    auto interface2 = interface;

                    if (result) {
                        interface2.reportResult(result.value());
                        interface2.reportFinished();
                    } else {
                        interface2.reportCanceled();
                        interface2.reportFinished();
                    }
                }, m_connectionType);
            } else {
                interface2.reportCanceled();
                interface2.reportFinished();
            }

        }, m_connectionType);

        return FutureResult<RetFutureType>(interface.future(), m_context, m_connectionType, getContext());
    }

    // QFuture<T> --> std::optional<T> --> void
    template<typename Callable, typename Ret = typename std::invoke_result<Callable, std::optional<T>>::type,
             typename std::enable_if<std::is_same<Ret, void>::value>::type* = nullptr>
    FutureResultBase then(Callable&& callable) {
        getContext()->needInternalHandler = false;

        UtilsQt::onFinished(m_future, m_context, [callable, internalContext = getContext()](const std::optional<T>& result) mutable {
            if (!internalContext->errorFlag) {
                try {
                    callable(result);
                } catch (...) {
                    internalContext->ex = std::current_exception();
                    internalContext->errorOccured();
                }
            }
        }, m_connectionType);

        return FutureResultBase(getContext());
    }

    FutureResult<T>& getFuture(QFuture<T>& future) {
        future = m_future;
        return *this;
    }

private:
    QFuture<T> m_future;
    QObject* m_context;
    Qt::ConnectionType m_connectionType;
};

} // namespace FutureUtilsInternals


template<typename Type>
::FutureUtilsInternals::FutureResult<Type> connectFutureSeq(const QFuture<Type>& future,
                    QObject* context,
                    Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    return ::FutureUtilsInternals::FutureResult<Type>(future, context, connectionType);
}
