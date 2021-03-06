#pragma once
#include <functional>
#include <optional>
#include <QObject>
#include <QSharedPointer>
#include <QFuture>
#include <QFutureInterface>
#include <QFutureWatcher>
#include <QPair>
#include <utils-cpp/pimpl.h>

/*            Reminder
  -------------------------------

  convertFuture
     QFuture<Src>                      --->   Bundle
     Conv. func: optional<Dest>(Src)             Anon. bridge (to save)
                                                 QFuture<Dest>

  -------------------------------

  FutureBridgesList futuresList;
  ---

  auto f = createFuture<int>();    // QFuture<int>

  auto bundle = convertFuture<int, std::string>(f, [](int value) -> std::optional<std::string>
  {
      return std::to_string(value);
  }

  futuresList.append(bundle.bridge);

  auto newFuture = bundle.future;  // QFuture<std::string>

  -------------------------------

  Converted future will be canceled if/when:
   - Source future canceled
   - Converter returned nothing / nullopt
   - Bridge destroyed (it controls converter lifetime)

  Converted future will return result:
   - IF/WHEN  Source future has result
   - THEN     Converter returned new result
   - WHILE    Bridge still alive

  To store bridges use FutureBridgesList.
*/


class AnonymousFutureBridge : public QObject
{
    Q_OBJECT
public:
    //virtual ~AnonymousFutureBridge() = default;

signals:
    void finished();

protected:
    void emitFinished() { emit finished(); }
};

using AnonymousFutureBridgePtr = QSharedPointer<AnonymousFutureBridge>;


namespace FutureBridgeInternals {

template<typename Source, typename Target>
struct FutureBridgeConverterType
{
    using Converter = std::function<std::optional<Target>(const Source&)>;
};

template<typename Target>
struct FutureBridgeConverterType<void, Target>
{
    using Converter = std::function<std::optional<Target>()>;
};

template <typename SrcFutureType, typename Converter, typename DstFutureType>
bool finished(const QFuture<SrcFutureType>& srcFuture, const Converter& converter, QFutureInterface<DstFutureType>& interface)
{
    const auto result = converter(srcFuture.result());

    if (result) {
        interface.reportResult(*result);
        interface.reportFinished();
    }

    return result.has_value();
}

template <typename Converter, typename DstFutureType>
bool finished(const QFuture<void>& /*srcFuture*/, const Converter& converter, QFutureInterface<DstFutureType>& interface)
{
    const auto result = converter();

    if (result) {
        interface.reportResult(*result);
        interface.reportFinished();
    }

    return result.has_value();
}

} // namespace FutureBridgeInternals


template<typename Source, typename Target>
class FutureBridge : public AnonymousFutureBridge
{
public:
    using SourceFuture = QFuture<Source>;
    using SourceWatcher = QFutureWatcher<Source>;
    using Converter = typename FutureBridgeInternals::FutureBridgeConverterType<Source, Target>::Converter;

    FutureBridge(const SourceFuture& future, const Converter& converter)
        : m_converter(converter)
    {
        m_targetFutureInterface.reportStarted();

        if (future.isFinished()) {
            handleFinished(future);
        } else {
            QObject::connect(&m_watcher, &SourceWatcher::finished, [this](){ onFinished(); });
            m_watcher.setFuture(future);
        }
    }

    ~FutureBridge()
    {
        if (!m_targetFutureInterface.isFinished())
            doCancel();
    }

    QFuture<Target> getTargetFuture() {
        return m_targetFutureInterface.future();
    }

private:
    void onFinished() {
        handleFinished(m_watcher.future());
    }

    void handleFinished(const SourceFuture& future) {
        if (future.isCanceled()) {
            doCancel();
        } else {
            auto convertResult = FutureBridgeInternals::finished(future, m_converter, m_targetFutureInterface);

            if (!convertResult)
                doCancel();
        }

        emitFinished();
    }

    void doCancel() {
        m_targetFutureInterface.reportCanceled();
        m_targetFutureInterface.reportFinished();
    }

private:
    Converter m_converter;
    QFutureInterface<Target> m_targetFutureInterface;
    SourceWatcher m_watcher;
};


template<typename Target>
struct FutureBundle {
    AnonymousFutureBridgePtr bridge;
    QFuture<Target> future;
};


template<typename Source, typename Target>
FutureBundle<Target> convertFuture(const QFuture<Source>& srcFuture,
                                      const typename FutureBridge<Source, Target>::Converter& converter)
{
    using BridgeType = FutureBridge<Source, Target>;

    auto bridgePtr = QSharedPointer<BridgeType>::create(srcFuture, converter);
    auto destFuture = bridgePtr->getTargetFuture();

    auto anonBridgePtr = destFuture.isFinished() ?
                             AnonymousFutureBridgePtr() :
                             qSharedPointerCast<AnonymousFutureBridge>(bridgePtr);

    return { anonBridgePtr, destFuture };
}


class FutureBridgesList : public QObject
{
    Q_OBJECT
public:
    FutureBridgesList();
    ~FutureBridgesList();
    void append(const AnonymousFutureBridgePtr& bridge);
    void clear();

private slots:
    void onFinished();

private:
    DECLARE_PIMPL
};
