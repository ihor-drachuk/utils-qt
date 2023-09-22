#pragma once
#include <cassert>
#include <QObject>
#include <QTimer>
#include <UtilsQt/invoke_method.h>
#include <UtilsQt/Futures/Utils.h>

namespace UtilsQt {

enum class Comparison {
    Equal,
    NotEqual
};

enum class CancelReason {
    Unknown,
    Object,
    Context,
    Timeout
};

namespace OnPropertyInternal {

class PropertyWatcherBase : public QObject
{
    Q_OBJECT
public:
    void addDependency(QObject* obj, CancelReason reason = CancelReason::Unknown) {
        QObject::connect(obj, &QObject::destroyed, this, [this, reason](){ cancelled(reason); deleteLater(); });
    }

    virtual void changed() = 0;
    virtual void cancelled(CancelReason cancelReason) = 0;

    inline void setOnce(bool value) { m_once = value; }
    inline bool getOnce() const { return m_once; }

signals:
    void triggered();

protected:
    void trigger() {
        if (m_stop) return;

        emit triggered();

        if (m_once) {
            m_stop = true;
            deleteLater();
        }
    }

private:
    bool m_once { false };
    bool m_stop { false };
};

template<typename T, typename Getter, typename CancelHandler>
class PropertyWatcher : public PropertyWatcherBase
{
public:
    PropertyWatcher(const Getter& getter, const T& expectedValue, Comparison comparison, const CancelHandler& cancelHandler)
        : m_getter(getter),
          m_expectedValue(expectedValue),
          m_comparison(comparison),
          m_cancelHandler(cancelHandler)
    { }

    void changed() override {
        if (m_cancelled)
            return;

        assert((getOnce() && !m_triggered) || !getOnce());

        const bool eq = (m_getter() == m_expectedValue);

        if (eq ^ (m_comparison == Comparison::NotEqual)) {
            m_triggered = true;
            trigger();
        }
    }

    void cancelled(CancelReason cancelReason) override {
        if (m_triggered || m_cancelled)
            return;

        m_cancelled = true;
        m_cancelHandler(cancelReason);
    }

private:
    bool m_triggered { false };
    bool m_cancelled { false };
    Getter m_getter;
    T m_expectedValue;
    Comparison m_comparison;
    CancelHandler m_cancelHandler;
};

inline void cancelStubHandler(UtilsQt::CancelReason) {}


template<typename T, typename Getter, typename CancelHandler>
PropertyWatcher<T, Getter, CancelHandler>* createWatcher(const Getter& getter, const T& expectedValue, UtilsQt::Comparison comparison, const CancelHandler& cancelHandler)
{
    return new PropertyWatcher<T, Getter, CancelHandler>(getter, expectedValue, comparison, cancelHandler);
}

} // namespace OnPropertyInternal


template<typename T, typename... SArgs, typename Object, typename Handler, typename Handler2 = void (*)(UtilsQt::CancelReason),
         typename std::enable_if<std::is_base_of<QObject, Object>::value>::type* = nullptr>
void onProperty(Object* object,
                T (Object::* getter)() const,
                void (Object::* notifier)(SArgs...),
                const T& expectedValue,
                UtilsQt::Comparison comparison,
                bool once,
                QObject* context,
                const Handler& handler,
                int timeout = -1,
                const Handler2& cancelHandler = UtilsQt::OnPropertyInternal::cancelStubHandler,
                Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    assert(object);
    assert(getter);
    assert(notifier);
    assert(context);

    auto watcher = UtilsQt::OnPropertyInternal::createWatcher(
                [object, getter]() -> T { return (object->*getter)(); },
                expectedValue,
                comparison,
                cancelHandler);

    if (timeout > 0) {
        auto timer = new QTimer();
        QObject::connect(timer, &QTimer::timeout, timer, &QTimer::deleteLater);
        watcher->addDependency(timer, UtilsQt::CancelReason::Timeout);
        timer->start(timeout);
    }

    watcher->addDependency(object,  UtilsQt::CancelReason::Object);
    watcher->addDependency(context, UtilsQt::CancelReason::Context);

    watcher->setOnce(once);

    QObject::connect(object, notifier, watcher, &UtilsQt::OnPropertyInternal::PropertyWatcherBase::changed, connectionType);
    QObject::connect(watcher, &UtilsQt::OnPropertyInternal::PropertyWatcherBase::triggered, context, [handler = handler]() mutable { handler(); }, connectionType);
    UtilsQt::invokeMethod(watcher, [watcher](){ watcher->changed(); }, connectionType);
}

template<typename T, typename... SArgs, typename Object,
         typename std::enable_if<std::is_base_of<QObject, Object>::value>::type* = nullptr>
QFuture<void> onPropertyFuture(Object* object,
                               T (Object::* getter)() const,
                               void (Object::* notifier)(SArgs...),
                               const T& expectedValue,
                               UtilsQt::Comparison comparison,
                               QObject* context,
                               int timeout = -1,
                               Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    auto promise = UtilsQt::createPromise<void>();

    onProperty(object, getter, notifier, expectedValue, comparison, true, context,
               [promise]() mutable {
                   promise.finish();
               },
               timeout,
               [promise](UtilsQt::CancelReason) mutable {
                   promise.cancel();
               },
               connectionType);

    return promise.future();
}

} // namespace UtilsQt
