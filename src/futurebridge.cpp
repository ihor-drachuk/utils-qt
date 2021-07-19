#include <utils-qt/futurebridge.h>

#include <QMap>


struct FutureBridgesList::impl_t {
    QMap<AnonymousFutureBridge*, AnonymousFutureBridgePtr> bridges;
};


FutureBridgesList::FutureBridgesList()
{
    createImpl();
}

FutureBridgesList::~FutureBridgesList()
{
}

void FutureBridgesList::append(const AnonymousFutureBridgePtr& bridge)
{
    if (!bridge) return;
    if (bridge->isFinished()) return;

    connect(bridge.data(), &AnonymousFutureBridge::finished, this, &FutureBridgesList::onFinished);
    impl().bridges.insert(bridge.data(), bridge);
}

void FutureBridgesList::clear()
{
    impl().bridges.clear();
}

void FutureBridgesList::onFinished()
{
    auto ptr = sender();
    Q_ASSERT(ptr);
    Q_ASSERT(qobject_cast<AnonymousFutureBridge*>(ptr));

    impl().bridges.remove(static_cast<AnonymousFutureBridge*>(ptr));
}
