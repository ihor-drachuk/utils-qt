#pragma once
#include <QQuickItem>

class MultibindingItem;

class Multibinding : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(QList<QObject*> loopbackGuarded READ loopbackGuarded WRITE setLoopbackGuarded NOTIFY loopbackGuardedChanged)
    Q_PROPERTY(int loopbackGuardMs READ loopbackGuardMs WRITE setLoopbackGuardMs NOTIFY loopbackGuardMsChanged)
public:
    static void registerTypes();

    Multibinding(QQuickItem* parent = nullptr);
    ~Multibinding() override;

    Q_INVOKABLE void sync();

signals:
    void triggered(MultibindingItem* source);
    void triggeredAfter(MultibindingItem* source);

// Properties support
public:
    QVariant value() const;
    const QList<QObject*>& loopbackGuarded() const;
    int loopbackGuardMs() const;

public slots:
    void setValue(const QVariant& value);
    void setLoopbackGuarded(const QList<QObject*>& newLoopbackGuarded);
    void setLoopbackGuardMs(int newLoopbackGuardMs);

signals:
    void valueChanged(const QVariant& value);
    void loopbackGuardedChanged(const QList<QObject*>& loopbackGuarded);
    void loopbackGuardMsChanged(int loopbackGuardMs);
// ----

protected:
    void componentComplete() override;

private:
    void connectChildren();
    void onChanged(MultibindingItem* srcProp);
    void onSyncNeeded(MultibindingItem* srcProp);
    void onTriggered(MultibindingItem* srcProp);
    void onTimeout();

private:
    struct impl_t;
    impl_t* _impl { nullptr };
    impl_t& impl() { return *_impl; }
    const impl_t& impl() const { return *_impl; }
};
