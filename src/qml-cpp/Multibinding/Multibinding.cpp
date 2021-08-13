#include <utils-qt/qml-cpp/Multibinding/Multibinding.h>

#include <QQmlEngine>
#include <cassert>
#include <utils-qt/qml-cpp/Multibinding/MultibindingItem.h>


void Multibinding::registerTypes()
{
    qmlRegisterType<Multibinding>("UtilsQt", 1, 0, "Multibinding");
}


void Multibinding::setValue(const QVariant& value)
{
    if (m_value == value)
        return;

    m_value = value;
    sync();

    emit valueChanged(m_value);
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
    m_recursionBlocking = true;

    auto children = childItems();
    for (auto item: children) {
        if (auto destProp = qobject_cast<MultibindingItem*>(item))
            if (!destProp->queuedWPending())
                destProp->write(m_value);
    }

    m_recursionBlocking = false;
}

void Multibinding::componentComplete()
{
    QQuickItem::componentComplete();
    connectChildren();
}

void Multibinding::onChanged(MultibindingItem* srcProp)
{
    assert(srcProp);

    if (m_recursionBlocking)
        return;

    setValue(srcProp->read());
}

void Multibinding::onSyncNeeded(MultibindingItem* srcProp)
{
    assert(srcProp);
    srcProp->write(m_value);
}
