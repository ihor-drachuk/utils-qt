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
    if (!m_enableW)
        return;

    assert(object());
    assert(!propertyName().isEmpty());
    assert(QQmlProperty(object(), propertyName()).isValid());

    if (m_enableR && read() == value)
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

void MultibindingItem::setEnableR(bool value)
{
    if (m_enableR == value)
        return;

    m_enableR = value;
    emit enableRChanged(m_enableR);

    updateRW();

    if (value && m_resyncR)
        emit changed();
}

void MultibindingItem::setEnableW(bool value)
{
    if (m_enableW == value)
        return;

    m_enableW = value;
    emit enableWChanged(m_enableW);

    updateRW();

    if (value && m_resyncW)
        emit needSync();
}

void MultibindingItem::setEnableRW(bool value)
{
    m_enableRW = value;
    setEnableR(value);
    setEnableW(value);
    emit enableRWChanged(m_enableRW);
}

void MultibindingItem::updateRW()
{
    auto newValue = m_enableR  &&  m_enableW ? true  :
                    !m_enableR && !m_enableW ? false :
                                               m_enableRW;

    if (m_enableRW != newValue) {
        m_enableRW = newValue;
        emit enableRWChanged(newValue);
    }
}

void MultibindingItem::setResyncR(bool value)
{
    if (m_resyncR == value)
        return;

    m_resyncR = value;
    emit resyncRChanged(m_resyncR);
}

void MultibindingItem::setResyncW(bool value)
{
    if (m_resyncW == value)
        return;

    m_resyncW = value;
    emit resyncWChanged(m_resyncW);
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
    qmpProp.connectNotifySignal(this, SLOT(changedHandler()));

    emit needSync();

    m_connected = true;
}

void MultibindingItem::changedHandler()
{
    if (m_enableR)
        emit changed();
}
