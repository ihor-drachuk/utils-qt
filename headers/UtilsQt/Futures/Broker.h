/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <QObject>
#include <QFuture>
#include <QFutureWatcher>
#include <QList>
#include <optional>
#include <cassert>
#include <type_traits>

#include <UtilsQt/Futures/Utils.h>
#include <utils-cpp/default_ctor_ops.h>

/**
 * Description:
 * A broker/proxy class that provides transparent QFuture replacement capabilities.
 *
 * The Broker class acts as an intermediary between QFuture producers and consumers, enabling
 * dynamic replacement of the underlying source QFuture without disrupting the consumer.
 *
 * Also allows delayed setting of the source QFuture, which can be useful in scenarios where
 * the source is not known/selected at the time of the consumer's request.
 *
 * Allows to disconnect from the source future and cancel it without disrupting the consumer.
 *
 * Use cases:
 * - Dynamic task switching where the computation source can change.
 * - Decoupling producers from consumers in complex async workflows.
 * - New request can replace the old one while the old one is still running without disrupting the consumer.
 * - Delayed decision of which async task to execute to fulfill the request.
 */

namespace UtilsQt {

namespace Internal {

template<typename T>
struct BrokerData
{
    DEFAULT_MOVE(BrokerData);
    NO_COPY(BrokerData);
    BrokerData() = default;
    BrokerData(Promise<T>&& promise): dstPromise(std::move(promise)) { }

    Promise<T> dstPromise;
    std::optional<QFuture<T>> optSrcFuture;

    QFutureWatcher<T> srcWatcher;
    QFutureWatcher<T> dstWatcher;
};

} // namespace Internal

class BrokerBase : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
    BrokerBase(const BrokerBase&) = delete;
    BrokerBase& operator=(const BrokerBase&) = delete;
    ~BrokerBase() override = default;
};

template<typename T>
class Broker : public BrokerBase
{
public:
    Broker();
    Broker(const QFuture<T>& f);
    ~Broker() override = default;

    void rebind(const QFuture<T>& f);
    void reset();

    bool hasRunningFuture() const { return m_data->optSrcFuture && !m_data->optSrcFuture->isFinished(); }
    bool isCanceled() const { return m_data->dstPromise.isCanceled() || (m_data->optSrcFuture && m_data->optSrcFuture->isCanceled()); }

    QFuture<T> future() const { return m_data->dstPromise.future(); }

private:
    void rebind(const std::optional<QFuture<T>>& optSrcFuture);

private:
    std::unique_ptr<Internal::BrokerData<T>> m_data { std::make_unique<Internal::BrokerData<T>>() };
};

template<typename T>
inline Broker<T>::Broker()
{
    rebind(std::nullopt);
}

template<typename T>
inline Broker<T>::Broker(const QFuture<T>& f)
{
    rebind(f);
}

template<typename T>
inline void Broker<T>::rebind(const QFuture<T>& f)
{
    rebind(std::make_optional(f));
}

template<typename T>
inline void Broker<T>::reset()
{
    rebind(std::nullopt);
}

template<typename T>
inline void Broker<T>::rebind(const std::optional<QFuture<T>>& optSrcFuture)
{
    // If finished or canceled - clear state
    if (m_data->dstPromise.isFinished() || m_data->dstPromise.isCanceled())
        m_data.reset();

    // Destroy irrelevant objects (and disconnect from irrelevant signals), cancel previous source future
    if (m_data) {
        auto optOldSrcFuture = m_data->optSrcFuture;

        Promise<T> promise = std::move(m_data->dstPromise);
        m_data = std::make_unique<Internal::BrokerData<T>>(std::move(promise));

        if (optOldSrcFuture && !optOldSrcFuture->isFinished())
            optOldSrcFuture->cancel();
    }

    // Connect to the new future
    if (!m_data)
        m_data = std::make_unique<Internal::BrokerData<T>>();

    m_data->optSrcFuture = optSrcFuture;

    if (m_data->optSrcFuture) {
        QObject::connect(&m_data->srcWatcher, &QFutureWatcher<T>::started, [this]() {
            assert(m_data);
            auto& dstPromise = m_data->dstPromise;

            if (!dstPromise.isStarted() && !dstPromise.isCanceled())
                dstPromise.start();
        });

        QObject::connect(&m_data->srcWatcher, &QFutureWatcher<T>::finished, [this]() {
            assert(m_data);
            assert(m_data->optSrcFuture);
            auto& srcFuture = *m_data->optSrcFuture;
            auto& dstPromise = m_data->dstPromise;

            if (srcFuture.isCanceled()) {
                try {
                    srcFuture.waitForFinished();
                    dstPromise.cancel();
                } catch (...) {
                    if (!dstPromise.isStarted() && !dstPromise.isCanceled())
                        dstPromise.start();

                    dstPromise.finishWithException(std::current_exception());
                }
            } else {
                if (!dstPromise.isStarted() && !dstPromise.isCanceled())
                    dstPromise.start();

                if constexpr (std::is_void_v<T>) {
                    dstPromise.finish();
                } else {
                    dstPromise.finish(srcFuture.result());
                }
            }
        });

        QObject::connect(&m_data->dstWatcher, &QFutureWatcher<T>::canceled, [this]() {
            assert(m_data);
            assert(m_data->optSrcFuture);
            m_data->optSrcFuture->cancel();
        });

        m_data->srcWatcher.setFuture(*m_data->optSrcFuture);
        m_data->dstWatcher.setFuture(m_data->dstPromise.future());
    }
}

} // namespace UtilsQt
