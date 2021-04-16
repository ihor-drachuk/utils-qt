#pragma once
#include <functional>
#include <QObject>
#include <QSharedPointer>
#include <QFuture>
#include <QFutureInterface>
#include <QFutureWatcher>
#include <QPair>
#include <optional>
#include "pimpl.h"

// Reminder.
//
// convertFuture
//    QFuture<Src>                      --->   Bundle
//    Conv. func: optional<Dest>(Src)             Anon. bridge (to save)
//                                                QFuture<Dest>


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


template<typename Source, typename Target>
class FutureBridge : public AnonymousFutureBridge
{
public:
    using SourceFuture = QFuture<Source>;
    using SourceWatcher = QFutureWatcher<Source>;
    using Converter = std::function<std::optional<Target>(const Source&)>;

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
            const auto result = m_converter(future.result());
            if (result) {
                m_targetFutureInterface.reportResult(*result);
                m_targetFutureInterface.reportFinished();
            } else {
                doCancel();
            }
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
