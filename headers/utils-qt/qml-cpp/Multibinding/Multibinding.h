#pragma once
#include <QQuickItem>

class MultibindingItem;

class Multibinding : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)
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
public slots:
    void setValue(const QVariant& value);
signals:
    void valueChanged(const QVariant& value);
// ----

protected:
    void componentComplete() override;

private:
    void connectChildren();
    void onChanged(MultibindingItem* srcProp);
    void onSyncNeeded(MultibindingItem* srcProp);

private:
    struct impl_t;
    impl_t* _impl { nullptr };
    impl_t& impl() { return *_impl; }
    const impl_t& impl() const { return *_impl; }
};
