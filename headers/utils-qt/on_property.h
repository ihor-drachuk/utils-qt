#pragma once
#include <cassert>
#include <QObject>
#include <utils-qt/invoke_method.h>

namespace UtilsQt {
namespace Internal {

class PropertyWatcherBase : public QObject
{
    Q_OBJECT
public:
    void addDependency(QObject* obj) {
        QObject::connect(obj, &QObject::destroyed, this, &QObject::deleteLater);
    }

    virtual void changed() = 0;

    void setOnce(bool value) { m_once = value; }
    bool getOnce() const { return m_once; }

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

template<typename Getter, typename T>
class PropertyWatcher : public PropertyWatcherBase
{
public:
    PropertyWatcher(const Getter& getter, const T& expectedValue)
        : m_getter(getter),
          m_expectedValue(expectedValue)
    { }

    void changed() override {
        if (m_getter() == m_expectedValue)
            trigger();
    }

private:
    Getter m_getter;
    const T& m_expectedValue;
};


template<typename Getter, typename T>
PropertyWatcher<Getter, T>* createWatcher(const Getter& getter, const T& expectedValue)
{
    return new PropertyWatcher<Getter, T>(getter, expectedValue);
}

} // namespace Internal
} // namespace UtilsQt

template<typename T, typename T2, typename Object, typename Handler,
         typename std::enable_if<std::is_base_of<QObject, Object>::value>::type* = nullptr>
void onProperty(Object* object,
                T (Object::* getter)() const,
                void (Object::* notifier)(T2),
                const T& expectedValue,
                bool once,
                QObject* context,
                const Handler& handler,
                Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    assert(object);
    assert(getter);
    assert(notifier);
    assert(context);

    auto watcher = UtilsQt::Internal::createWatcher(
                [object, getter]() -> T { return (object->*getter)(); },
                expectedValue);

    watcher->addDependency(object);
    watcher->addDependency(context);

    watcher->setOnce(once);

    QObject::connect(object, notifier, watcher, &UtilsQt::Internal::PropertyWatcherBase::changed, connectionType);
    QObject::connect(watcher, &UtilsQt::Internal::PropertyWatcherBase::triggered, context, [handler](){ handler(); }, connectionType);
    UtilsQt::invokeMethod(watcher, [watcher](){ watcher->changed(); }, connectionType);
}
