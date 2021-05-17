#include "utils-qt/multibinding.h"

#include "utils-qt/multibinding_item.h"


void Multibinding::registerTypes(const char* url)
{
    qmlRegisterType<Multibinding>(url, 1, 0, "Multibinding");
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

    for (auto item: childItems()) {
        if (auto prop = qobject_cast<MultibindingItem*>(item)) {
            prop->initialize();
            connect(prop, &MultibindingItem::changed, this, [this, prop] { onChanged(prop); });
            connect(prop, &MultibindingItem::needSync, this, [this, prop] { onSyncNeeded(prop); });
        }
    }

    if (auto first = qobject_cast<MultibindingItem*>(childItems().first())) {
        onChanged(first);
    }
}

void Multibinding::sync()
{
    m_recursionBlocking = true;

    for (auto item: childItems()) {
        if (auto destProp = qobject_cast<MultibindingItem*>(item))
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
