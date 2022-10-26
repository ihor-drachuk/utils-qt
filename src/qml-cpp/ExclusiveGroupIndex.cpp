#include "utils-qt/qml-cpp/ExclusiveGroupIndex.h"
#include "utils-cpp/scoped_guard.h"

#include <QVariant>
#include <QDebug>
#include <QQmlEngine>

#include <iostream>

namespace Internal
{

#define CURRENT_INDEX_PROPERTY "currentIndex"
#define CURRENT_INDEX_SIGNAL "currentIndexChanged()"
#define CHANGED "changed()" // special for ListModelItemProxy which is used in combobox

static int index(const QObject* obj, bool* ok = nullptr)
{
    if (!obj) {
        if (ok) {
            *ok = false;
        }

        return -1;
    }

    QVariant indexVariant = obj->property(CURRENT_INDEX_PROPERTY);

    if (!indexVariant.isValid()) {
        if (ok) {
            *ok = false;
        }

        return -1;
    }

    return indexVariant.toInt(ok);
}

} // namespace Internal

struct ExclusiveGroupIndex::impl_t
{
    QObject* m_current;
    QMetaMethod m_updateCurrentMethod;
    QVector<QObject*> m_containers;
};


void ExclusiveGroupIndex::registerTypes()
{
    qmlRegisterType<ExclusiveGroupIndex>("UtilsQt", 1, 0, "ExclusiveGroupIndex");
}

QObject* ExclusiveGroupIndex::current() const
{
    return impl().m_current;
}

ExclusiveGroupIndex::ExclusiveGroupIndex(QObject *parent)
    : QObject(parent)
{
    createImpl();

    impl().m_current = 0;

    // AK: vcalls are ok here cuz we index own methods
    int index = metaObject()->indexOfMethod("updateCurrent()");
    impl().m_updateCurrentMethod = metaObject()->method(index);
}

void ExclusiveGroupIndex::setCurrent(QObject* obj)
{
    auto _setObj = CreateScopedGuard([this, obj] () {
        impl().m_current = obj;
        emit currentChanged();
    });

    for (auto container: qAsConst(impl().m_containers)) {
        if (container == obj) {
            continue;
        }

        auto newIdx = Internal::index(obj);
        auto containerIdx = Internal::index(container);

        if (containerIdx == newIdx) {
            container->setProperty(CURRENT_INDEX_PROPERTY, QVariant(0));
        }
    }
}

void ExclusiveGroupIndex::updateCurrent()
{
    setCurrent(sender());
}

void ExclusiveGroupIndex::dispatchContainer(QObject* obj)
{
    auto signalIdx = obj->metaObject()->indexOfSignal(CURRENT_INDEX_SIGNAL);
    assert(signalIdx != -1);

    bool ok { false };
    Internal::index(obj, &ok);

    auto metaMethod = obj->metaObject()->method(signalIdx);

    // AK: Check if container have "currentIndex" property and "currentIndexChanged()" signal
    // This items are required for propper using.
    assert(ok);
    assert(metaMethod.isValid());

    auto connection = connect(obj, metaMethod, this, impl().m_updateCurrentMethod, Qt::UniqueConnection);

    if (!connection) {
        std::cout << "[ " << Q_FUNC_INFO << " ]" << "WARNING: Could not connect to updateCurrent method or connections is already established!" << std::endl;
    }

    signalIdx = obj->metaObject()->indexOfSignal(CHANGED);
    metaMethod = obj->metaObject()->method(signalIdx);

    if (metaMethod.isValid()) {
        connection = connect(obj, metaMethod, this, impl().m_updateCurrentMethod, Qt::UniqueConnection);

        if (!connection) {
            std::cout << "[ " << Q_FUNC_INFO << " ]" << "INFO: Method T::changed() not found for updating index." << std::endl;
        }
    }

    connection = connect(obj, &QObject::destroyed, this, &ExclusiveGroupIndex::clearContainer, Qt::UniqueConnection);

    if (!connection) {
        std::cout << "[ " << Q_FUNC_INFO << " ]" << "WARNING: Could not connect to destroyed() method or connections is already established!" << std::endl;
    }

    impl().m_containers.push_back(obj);

    std::cout.flush();
}

void ExclusiveGroupIndex::clearContainer(QObject* obj)
{
    if (impl().m_current == obj) {
        setCurrent(nullptr);
    }

    auto idx = obj->metaObject()->indexOfSignal(CURRENT_INDEX_SIGNAL);
    assert(idx != -1);

    auto idxMethod = obj->metaObject()->method(idx);
    if (!disconnect(obj, idxMethod, this, impl().m_updateCurrentMethod)) {
        std::cout << "[ " << Q_FUNC_INFO << " ]" << "WARNING: Could not disconnect from updateCurrent method!" << std::endl;
    }

    if (!disconnect(obj, &QObject::destroyed, this, &ExclusiveGroupIndex::clearContainer)) {
        std::cout << "[ " << Q_FUNC_INFO << " ]" << "WARNING: Could not disconnect from updateCurrent method!" << std::endl;
    }

    impl().m_containers.removeOne(obj);

    std::cout.flush();
}

