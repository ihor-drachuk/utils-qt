#include "utils-qt/multibinding_item.h"

#include <QQmlProperty>


void MultibindingItem::registerTypes(const char* url)
{
    qmlRegisterType<MultibindingItem>(url, 1, 0, "MultibindingItem");
}


void MultibindingItem::initialize()
{
}

QVariant MultibindingItem::read() const
{
    assert(object());
    assert(!propertyName().isEmpty());
    assert(QQmlProperty(object(), propertyName()).isValid());

    auto value = QQmlProperty::read(object(), propertyName());
    return value;
}

void MultibindingItem::write(const QVariant& value)
{
    assert(object());
    assert(!propertyName().isEmpty());
    assert(QQmlProperty(object(), propertyName()).isValid());

    if (read() == value)
        return;

    QQmlProperty::write(object(), propertyName(), value);
}

void MultibindingItem::setObject(QObject* value)
{
    if (m_object == value)
        return;

    detachProperty();

    m_object = value;

    attachProperty();
    emit objectChanged(m_object);
}

void MultibindingItem::setPropertyName(const QString& value)
{
    if (m_propertyName == value)
        return;

    detachProperty();

    m_propertyName = value;

    attachProperty();
    emit propertyNameChanged(m_propertyName);
}

void MultibindingItem::detachProperty()
{
    if (m_object)
        m_object->disconnect(this);

    m_connected = false;
}

void MultibindingItem::attachProperty()
{
    assert(!m_connected);

    if (!m_object || m_propertyName.isEmpty())
        return;

    connect(m_object, &QObject::destroyed, this, [this]()
    {
        m_object = nullptr;
        detachProperty();
        emit objectChanged(nullptr);
    });

    auto qmpProp = QQmlProperty(object(), propertyName());
    qmpProp.connectNotifySignal(this, SIGNAL(changed()));

    emit needSync();

    m_connected = true;
}
