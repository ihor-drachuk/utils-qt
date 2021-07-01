#include <utils-qt/qml/Multibinding/MultibindingItem.h>

#include <utils-qt/invoke_method.h>
#include <utils-qt/qml/Multibinding/Transformers/AbstractTransformer.h>
#include <QQmlEngine>
#include <QQmlProperty>
#include <cassert>


void MultibindingItem::registerTypes(const char* url)
{
    qRegisterMetaType<ReAttachBehavior>("ReAttachBehavior");
    qmlRegisterType<MultibindingItem>(url, 1, 0, "MultibindingItem");
}

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

    if (m_queuedW) {
        setQueuedWPending(true);
        UtilsQt::invokeMethod(this, [this, value](){ writeImpl(value); }, Qt::QueuedConnection);
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
    if (m_enableR == value)
        return;

    m_enableR = value;
    emit enableRChanged(m_enableR);

    updateRW();

    if (value && m_resyncR)
        changedHandler();
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

    updateQueuedRW();
}

void MultibindingItem::setQueuedWPending(bool value)
{
    if (m_queuedWPending == value)
        return;

    m_queuedWPending = value;
    emit queuedWPendingChanged(m_queuedWPending);
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

void MultibindingItem::setQueuedRW(bool value)
{
    m_queuedRW = value;
    setQueuedR(value);
    setQueuedW(value);
    emit queuedRWChanged(m_queuedRW);
}

void MultibindingItem::updateQueuedRW()
{
    auto newValue = m_queuedR  &&  m_queuedW ? true  :
                    !m_queuedR && !m_queuedW ? false :
                                               m_queuedRW;

    if (m_queuedRW != newValue) {
        m_queuedRW = newValue;
        emit enableRWChanged(newValue);
    }
}

void MultibindingItem::setQueuedR(bool value)
{
    if (m_queuedR == value)
        return;

    m_queuedR = value;
    emit queuedRChanged(m_queuedR);

    updateQueuedRW();
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
        if (m_queuedR) {
            UtilsQt::invokeMethod(this, &MultibindingItem::changed, Qt::QueuedConnection);
        } else {
            emit changed();
        }
    }
}

void MultibindingItem::writeImpl(QVariant value)
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
