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
 * Use cases:
 * - Dynamic task switching where the computation source can change.
 * - Decoupling producers from consumers in complex async workflows.
 * - New request can replace the old one while the old one is still running without disrupting the consumer.
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
    QFuture<T> srcFuture;

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
    using BrokerBase::BrokerBase;
    ~Broker() override = default;

    void bind(const QFuture<T>& f);
    void bindOrReplace(const QFuture<T>& f);

    bool hasFuture() const { return m_optData.has_value(); } // Can be finished or running.
    bool hasRunningFuture() const { return m_optData.has_value() && !m_optData->srcFuture.isFinished(); }
    bool isCanceled() const { return m_optData.has_value() && (m_optData->dstPromise.isCanceled() || m_optData->srcFuture.isCanceled()); }

    QFuture<T> future() const { assert(hasFuture()); return m_optData->dstPromise.future(); }

private:
    std::optional<Internal::BrokerData<T>> m_optData;
};

template<typename T>
inline void Broker<T>::bind(const QFuture<T>& f)
{
    assert(!hasRunningFuture() || isCanceled());
    bindOrReplace(f);
}

template<typename T>
inline void Broker<T>::bindOrReplace(const QFuture<T>& f)
{
    // If finished or canceled - clear state
    if (hasFuture() && (m_optData->dstPromise.isFinished() || m_optData->dstPromise.isCanceled()))
        m_optData.reset();

    // Cancel existing future, destroy irrelevant objects (and disconnect from irrelevant signals)
    if (m_optData) {
        if (!m_optData->srcFuture.isFinished())
            m_optData->srcFuture.cancel();

        Promise<T> promise = std::move(m_optData->dstPromise);
        m_optData.emplace(std::move(promise));
    }

    // Connect to the new future
    if (!m_optData)
        m_optData.emplace();

    m_optData->srcFuture = f;

    QObject::connect(&m_optData->srcWatcher, &QFutureWatcher<T>::started, [this]() {
        assert(m_optData);
        auto& dstPromise = m_optData->dstPromise;

        if (!dstPromise.isStarted() && !dstPromise.isCanceled())
            dstPromise.start();
    });

    QObject::connect(&m_optData->srcWatcher, &QFutureWatcher<T>::finished, [this]() {
        assert(m_optData);
        auto& srcFuture = m_optData->srcFuture;
        auto& dstPromise = m_optData->dstPromise;

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

    QObject::connect(&m_optData->dstWatcher, &QFutureWatcher<T>::canceled, [this]() {
        assert(m_optData);
        m_optData->srcFuture.cancel();
    });

    m_optData->srcWatcher.setFuture(m_optData->srcFuture);
    m_optData->dstWatcher.setFuture(m_optData->dstPromise.future());
}

} // namespace UtilsQt
