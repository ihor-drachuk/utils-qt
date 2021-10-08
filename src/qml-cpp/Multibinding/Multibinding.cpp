#include <utils-qt/qml-cpp/Multibinding/Multibinding.h>

#include <QQmlEngine>
#include <cassert>
#include <utils-qt/qml-cpp/Multibinding/MultibindingItem.h>


struct Multibinding::impl_t
{
    QVariant value;
    bool recursionBlocking { false };
};


void Multibinding::registerTypes()
{
    qmlRegisterType<Multibinding>("UtilsQt", 1, 0, "Multibinding");
}

Multibinding::Multibinding(QQuickItem* parent)
    : QQuickItem(parent)
{
    _impl = new impl_t();
}

Multibinding::~Multibinding()
{
    delete _impl;
}

void Multibinding::setValue(const QVariant& value)
{
    if (impl().value == value)
        return;

    impl().value = value;
    sync();

    emit valueChanged(impl().value);
}

void Multibinding::connectChildren()
{
    if (childItems().isEmpty())
        return;

    auto children = childItems();
    for (auto item: children) {
        if (auto prop = qobject_cast<MultibindingItem*>(item)) {
            prop->initialize();
            connect(prop, &MultibindingItem::changed, this, [this, prop] { onChanged(prop); });
            connect(prop, &MultibindingItem::needSync, this, [this, prop] { onSyncNeeded(prop); });
        }
    }

    if (auto first = qobject_cast<MultibindingItem*>(children.first())) {
        first->initReAttachBehvior(MultibindingItem::SyncMultibinding);
        onChanged(first);
    }
}

void Multibinding::sync()
{
    impl().recursionBlocking = true;

    auto children = childItems();
    for (auto item: children) {
        if (auto destProp = qobject_cast<MultibindingItem*>(item))
            if (!destProp->queuedWPending())
                destProp->write(impl().value);
    }

    impl().recursionBlocking = false;
}

QVariant Multibinding::value() const
{
    return impl().value;
}

void Multibinding::componentComplete()
{
    QQuickItem::componentComplete();
    connectChildren();
}

void Multibinding::onChanged(MultibindingItem* srcProp)
{
    assert(srcProp);

    if (impl().recursionBlocking)
        return;

    emit triggered(srcProp);
    emit srcProp->triggered();
    setValue(srcProp->read());
    emit triggeredAfter(srcProp);
    emit srcProp->triggeredAfter();
}

void Multibinding::onSyncNeeded(MultibindingItem* srcProp)
{
    assert(srcProp);
    srcProp->write(impl().value);
}
