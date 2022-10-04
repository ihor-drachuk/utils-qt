#include <UtilsQt/Qml-Cpp/Multibinding/Multibinding.h>

#include <QQmlEngine>
#include <QTimer>
#include <cassert>
#include <UtilsQt/Qml-Cpp/Multibinding/MultibindingItem.h>


struct Multibinding::impl_t
{
    MultibindingItem* lastChanged { nullptr };

    bool running { true };
    bool outdated { false };
    QVariant value;
    bool recursionBlocking { false };
    QList<QObject*> loopbackGuarded;
    int loopbackGuardMs { 0 };
    QTimer loopbackGaurdTimer;
};


void Multibinding::registerTypes()
{
    qmlRegisterType<Multibinding>("UtilsQt", 1, 0, "Multibinding");
}

Multibinding::Multibinding(QQuickItem* parent)
    : QQuickItem(parent)
{
    _impl = new impl_t();
    impl().loopbackGaurdTimer.setSingleShot(true);
    QObject::connect(&impl().loopbackGaurdTimer, &QTimer::timeout, this, &Multibinding::onTimeout);
}

Multibinding::~Multibinding()
{
    delete _impl;
}

void Multibinding::setValue(const QVariant& value)
{
    if (!impl().outdated && impl().value == value)
        return;

    impl().outdated = false;
    impl().value = value;

    if (impl().running)
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
            connect(prop, &MultibindingItem::triggered, this, [this, prop] { onTriggered(prop); });
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

    if (!running())
        return;

    if (impl().recursionBlocking)
        return;

    impl().lastChanged = srcProp;

    emit triggered(srcProp);
    emit srcProp->triggered();

    if (!impl().loopbackGaurdTimer.isActive() || impl().loopbackGuarded.contains(srcProp))
        setValue(srcProp->read());

    emit triggeredAfter(srcProp);
    emit srcProp->triggeredAfter();
}

void Multibinding::onSyncNeeded(MultibindingItem* srcProp)
{
    assert(srcProp);

    if (!running())
        return;

    srcProp->write(impl().value);
}

void Multibinding::onTriggered(MultibindingItem* srcProp)
{
    assert(srcProp);

    if (!running())
        return;

    if (!impl().loopbackGuardMs) return;

    if (impl().loopbackGuarded.contains(srcProp)) {
        impl().loopbackGaurdTimer.start(impl().loopbackGuardMs);
    }
}

void Multibinding::onTimeout()
{
    onChanged(impl().lastChanged);
}

void Multibinding::onDisabled()
{
    assert(!running());
    impl().outdated = true;
}

void Multibinding::onEnabled()
{
    assert(running());

    MultibindingItem* first {};
    MultibindingItem* source {};

    auto children = childItems();
    for (auto item: children) {
        if (auto mbItem = qobject_cast<MultibindingItem*>(item)) {
            if (!first)
                first = mbItem;

            if (mbItem->reAttachBehvior() == MultibindingItem::ReAttachBehavior::SyncMultibinding) {
                source = mbItem;
                break;
            }
        }
    }

    MultibindingItem* selection = source ? source : first;
    if (!selection)
        return;

    selection->announce();
}

bool Multibinding::running() const
{
    return impl().running;
}

void Multibinding::setRunning(bool value)
{
    if (impl().running == value)
        return;

    impl().running = value;

    if (impl().running) {
        onEnabled();
    } else {
        onDisabled();
    }

    emit runningChanged(impl().running);
}

const QList<QObject*>& Multibinding::loopbackGuarded() const
{
    return impl().loopbackGuarded;
}

void Multibinding::setLoopbackGuarded(const QList<QObject*>& newLoopbackGuarded)
{
    if (impl().loopbackGuarded == newLoopbackGuarded)
        return;
    impl().loopbackGuarded = newLoopbackGuarded;
    emit loopbackGuardedChanged(impl().loopbackGuarded);
}

int Multibinding::loopbackGuardMs() const
{
    return impl().loopbackGuardMs;
}

void Multibinding::setLoopbackGuardMs(int newLoopbackGuardMs)
{
    if (impl().loopbackGuardMs == newLoopbackGuardMs)
        return;
    impl().loopbackGuardMs = newLoopbackGuardMs;
    emit loopbackGuardMsChanged(impl().loopbackGuardMs);
}
