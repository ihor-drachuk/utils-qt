#pragma once
#include <QQuickItem>
#include <utils-cpp/copy_move.h>

class MultibindingItem;

class Multibinding : public QQuickItem
{
    Q_OBJECT
    NO_COPY_MOVE(Multibinding);

    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
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
    bool running() const;
    QVariant value() const;
    const QList<QObject*>& loopbackGuarded() const;
    int loopbackGuardMs() const;

public slots:
    void setRunning(bool value);
    void setValue(const QVariant& value);
    void setLoopbackGuarded(const QList<QObject*>& newLoopbackGuarded);
    void setLoopbackGuardMs(int newLoopbackGuardMs);

signals:
    void runningChanged(bool running);
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
    void onDisabled();
    void onEnabled();

private:
    struct impl_t;
    impl_t* _impl { nullptr };
    impl_t& impl() { return *_impl; }
    const impl_t& impl() const { return *_impl; }
};
