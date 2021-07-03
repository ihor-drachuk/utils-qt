#include <utils-qt/qml/Multibinding/MultibindingItem.h>

#include <utils-qt/invoke_method.h>
#include <utils-qt/qml/Multibinding/Transformers/AbstractTransformer.h>
#include <QQmlEngine>
#include <QQmlProperty>
#include <cassert>

namespace {

bool isFloat(const QVariant& value) {
    return value.type() == QVariant::Type::Double ||
           value.type() == (QVariant::Type)QMetaType::Type::Float;
}

bool comapre(const QVariant& a, const QVariant& b) {
    if (isFloat(a) || isFloat(b))
        return qFuzzyCompare(a.toDouble(), b.toDouble());
    else
        return (a == b);
}

} // namespace


void MultibindingItem::registerTypes(const char* url)
{
    qRegisterMetaType<ReAttachBehavior>("ReAttachBehavior");
    qRegisterMetaType<DelayBehvior>("DelayBehvior");
    qmlRegisterType<MultibindingItem>(url, 1, 0, "MultibindingItem");
}


MultibindingItem::MultibindingItem(QQuickItem* parent)
    : QQuickItem(parent)
{
    m_delayMsRTimer.setSingleShot(true);
    m_delayMsWTimer.setSingleShot(true);
    m_enableRDelayTimer.setSingleShot(true);
    m_enableWDelayTimer.setSingleShot(true);

    QObject::connect(&m_delayMsRTimer, &QTimer::timeout, this, &MultibindingItem::changedHandler2);
    QObject::connect(&m_delayMsWTimer, &QTimer::timeout, this, [this](){ writeImpl(m_delayedWriteValue); });
    QObject::connect(&m_enableRDelayTimer, &QTimer::timeout, this, [this](){ setEnableRImpl(m_enableRCached); });
    QObject::connect(&m_enableWDelayTimer, &QTimer::timeout, this, [this](){ setEnableWImpl(m_enableWCached); });
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

    if (m_transformer)
        value = m_transformer->readConverter(value);

    return value;
}

void MultibindingItem::write(const QVariant& value)
{
    if (!m_enableW)
        return;

    if (m_delayMsW) {
        m_delayedWriteValue = value;

        if (m_delayBehW == RestartTimerOnChange || !m_delayMsWTimer.isActive()) {
            m_delayMsWTimer.start(m_delayMsW);
        }
    } else {
        writeImpl(value);
    }
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
    if (m_enableRDelayTimer.isActive()) {
        if (m_enableRCached == value) return;
    } else {
        if (m_enableR == value) return;
    }

    int delay = value ? m_enableRDelayOn : m_enableRDelayOff;

    m_enableRDelayTimer.stop();

    if (delay) {
        m_enableRCached = value;
        m_enableRDelayTimer.start(delay);
    } else {
        setEnableRImpl(value);
    }
}

void MultibindingItem::setEnableRImpl(bool value)
{
    m_enableR = value;
    emit enableRChanged(m_enableR);

    if (value && m_resyncR)
        changedHandler();
}

void MultibindingItem::setEnableW(bool value)
{
    if (m_enableWDelayTimer.isActive()) {
        if (m_enableWCached == value) return;
    } else {
        if (m_enableW == value) return;
    }

    int delay = value ? m_enableWDelayOn : m_enableWDelayOff;

    m_enableWDelayTimer.stop();

    if (delay) {
        m_enableWCached = value;
        m_enableWDelayTimer.start(delay);
    } else {
        setEnableWImpl(value);
    }
}

void MultibindingItem::setEnableWImpl(bool value)
{
    m_enableW = value;
    emit enableWChanged(m_enableW);

    if (value && m_resyncW)
        emit needSync();
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

void MultibindingItem::setTransformer(AbstractTransformer* value)
{
    if (m_transformer == value)
        return;

    QVariant origValue = isReady() ? read() : QVariant();

    if (m_transformer) {
        disconnect(m_transformer, nullptr, this, nullptr);
    }

    m_transformer = value;

    if (m_transformer) {
        connect(m_transformer, &AbstractTransformer::beforeTransformerUpdated, this, &MultibindingItem::onBeforeTransformerUpdated);
        connect(m_transformer, &AbstractTransformer::afterTransformerUpdated, this, &MultibindingItem::onAfterTransformerUpdated);
    }

    if (isReady())
        write(origValue);

    emit transformerChanged(m_transformer);
}

void MultibindingItem::setQueuedW(bool value)
{
    if (m_queuedW == value)
        return;

    m_queuedW = value;
    emit queuedWChanged(m_queuedW);
}

void MultibindingItem::setQueuedWPending(bool value)
{
    if (m_queuedWPending == value)
        return;

    m_queuedWPending = value;
    emit queuedWPendingChanged(m_queuedWPending);
}

void MultibindingItem::setDelayMsR(int value)
{
    assert(value >= 0);

    if (m_delayMsR == value)
        return;

    m_delayMsR = value;
    emit delayMsRChanged(m_delayMsR);
}

void MultibindingItem::setDelayMsW(int value)
{
    assert(value >= 0);

    if (m_delayMsW == value)
        return;

    m_delayMsW = value;
    emit delayMsWChanged(m_delayMsW);
}

void MultibindingItem::setDelayBehR(MultibindingItem::DelayBehvior value)
{
    if (m_delayBehR == value)
        return;

    m_delayBehR = value;
    emit delayBehRChanged(m_delayBehR);
}

void MultibindingItem::setDelayBehW(MultibindingItem::DelayBehvior value)
{
    if (m_delayBehW == value)
        return;

    m_delayBehW = value;
    emit delayBehWChanged(m_delayBehW);
}

void MultibindingItem::setEnableRDelayOn(int value)
{
    if (m_enableRDelayOn == value)
        return;

    m_enableRDelayOn = value;
    emit enableRDelayOnChanged(m_enableRDelayOn);
}

void MultibindingItem::setEnableRDelayOff(int value)
{
    if (m_enableRDelayOff == value)
        return;

    m_enableRDelayOff = value;
    emit enableRDelayOffChanged(m_enableRDelayOff);
}

void MultibindingItem::setEnableWDelayOn(int value)
{
    if (m_enableWDelayOn == value)
        return;

    m_enableWDelayOn = value;
    emit enableWDelayOnChanged(m_enableWDelayOn);
}

void MultibindingItem::setEnableWDelayOff(int value)
{
    if (m_enableWDelayOff == value)
        return;

    m_enableWDelayOff = value;
    emit enableWDelayOffChanged(m_enableWDelayOff);
}

void MultibindingItem::setReAttachBehvior(MultibindingItem::ReAttachBehavior value)
{
    m_reAttachBehviorIsSet = true;

    if (m_reAttachBehvior == value)
        return;

    m_reAttachBehvior = value;
    emit reAttachBehviorChanged(m_reAttachBehvior);
}

void MultibindingItem::initReAttachBehvior(MultibindingItem::ReAttachBehavior value)
{
    if (!m_reAttachBehviorIsSet)
        setReAttachBehvior(value);
}

void MultibindingItem::setQueuedR(bool value)
{
    if (m_queuedR == value)
        return;

    m_queuedR = value;
    emit queuedRChanged(m_queuedR);
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

    switch (m_reAttachBehvior) {
        case SyncMultibinding:
            emit changed();
            break;

        case SyncNewProperty:
            emit needSync();
            break;
    }

    m_connected = true;
}

void MultibindingItem::onBeforeTransformerUpdated()
{
    if (isReady())
        m_orig = read();
}

void MultibindingItem::onAfterTransformerUpdated()
{
    if (isReady())
        write(m_orig);
}

bool MultibindingItem::isReady() const
{
    return object() && !propertyName().isEmpty() && QQmlProperty(object(), propertyName()).isValid();
}

void MultibindingItem::changedHandler()
{
    if (m_enableR) {
        if (m_delayMsR) {
            if (m_delayBehR == RestartTimerOnChange || !m_delayMsRTimer.isActive()) {
                m_delayMsRTimer.start(m_delayMsR);
            }
        } else {
            changedHandler2();
        }
    }
}

void MultibindingItem::changedHandler2()
{
    if (m_queuedR) {
        UtilsQt::invokeMethod(this, &MultibindingItem::changed, Qt::QueuedConnection);
    } else {
        emit changed();
    }
}

void MultibindingItem::writeImpl(const QVariant& value)
{
    if (m_queuedW) {
        setQueuedWPending(true);
        UtilsQt::invokeMethod(this, [this, value](){ writeImpl2(value); }, Qt::QueuedConnection);
    } else {
        writeImpl2(value);
    }
}

void MultibindingItem::writeImpl2(QVariant value)
{
    setQueuedWPending(false);

    if (!m_enableW)
        return;

    assert(object());
    assert(!propertyName().isEmpty());
    assert(QQmlProperty(object(), propertyName()).isValid());

    if (m_enableR && comapre(read(), value))
        return;

    if (m_transformer)
        value = m_transformer->writeConverter(value, QQmlProperty::read(object(), propertyName()));

    QQmlProperty::write(object(), propertyName(), value);
}
