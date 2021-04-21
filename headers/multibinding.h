#pragma once
#include <QQuickItem>

class MultibindingItem;

class Multibinding : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)
public:
    static void registerTypes(const char* url);

    Multibinding(QQuickItem* parent = nullptr): QQuickItem(parent) { }
    void connectChildren();

// Properties support
public:
    QVariant value() const { return m_value; }
public slots:
    void setValue(const QVariant& value);
signals:
    void valueChanged(const QVariant& value);
// ----

protected:
    void componentComplete() override;

private:
    void onChanged(MultibindingItem* srcProp);
    void onSyncNeeded(MultibindingItem* srcProp);

private:
    QVariant m_value;
    bool m_recursionBlocking { false };
};
