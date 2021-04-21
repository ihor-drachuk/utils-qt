#pragma once
#include <QQuickItem>


class MultibindingItem : public QQuickItem
{
    Q_OBJECT
public:
    Q_PROPERTY(QObject* object READ object WRITE setObject NOTIFY objectChanged)
    Q_PROPERTY(QString propertyName READ propertyName WRITE setPropertyName NOTIFY propertyNameChanged)

    static void registerTypes(const char* url);

    MultibindingItem(QQuickItem* parent = nullptr): QQuickItem(parent) { }

    void initialize();
    QVariant read() const;
    void write(const QVariant& value);

signals:
    void changed();
    void needSync();

// Properies support
public:
    QObject* object() const { return m_object; }
    QString propertyName() const { return m_propertyName; }

public slots:
    void setObject(QObject* value);
    void setPropertyName(const QString& value);

signals:
    void objectChanged(QObject* object);
    void propertyNameChanged(const QString& propertyName);
// ----

private:
    void detachProperty();
    void attachProperty();

private:
    bool m_connected { false };
    QObject* m_object { nullptr };
    QString m_propertyName;
};
