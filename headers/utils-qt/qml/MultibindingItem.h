#pragma once
#include <QQuickItem>


class MultibindingItem : public QQuickItem
{
    Q_OBJECT
public:
    Q_PROPERTY(QObject* object READ object WRITE setObject NOTIFY objectChanged)
    Q_PROPERTY(QString propertyName READ propertyName WRITE setPropertyName NOTIFY propertyNameChanged)
    Q_PROPERTY(QVariant value READ read WRITE write NOTIFY changed)

    Q_PROPERTY(bool enableR READ enableR WRITE setEnableR NOTIFY enableRChanged)
    Q_PROPERTY(bool enableW READ enableW WRITE setEnableW NOTIFY enableWChanged)
    Q_PROPERTY(bool enableRW READ enableRW WRITE setEnableRW NOTIFY enableRWChanged)

    Q_PROPERTY(bool resyncR READ resyncR WRITE setResyncR NOTIFY resyncRChanged)
    Q_PROPERTY(bool resyncW READ resyncW WRITE setResyncW NOTIFY resyncWChanged)

    static void registerTypes(const char* url);

    MultibindingItem(QQuickItem* parent = nullptr): QQuickItem(parent) { }

    void initialize();
    QVariant read() const;
    void write(const QVariant& value);

    Q_INVOKABLE void sync() { emit needSync(); }

signals:
    void changed();
    void needSync();

// Properies support
public:
    QObject* object() const { return m_object; }
    QString propertyName() const { return m_propertyName; }
    bool enableR() const { return m_enableR; }
    bool enableW() const { return m_enableW; }
    bool enableRW() const { return m_enableRW; }
    bool resyncR() const { return m_resyncR; }
    bool resyncW() const { return m_resyncW; }

public slots:
    void setObject(QObject* value);
    void setPropertyName(const QString& value);
    void setEnableR(bool value);
    void setEnableW(bool value);
    void setEnableRW(bool value);
    void updateRW();
    void setResyncR(bool value);
    void setResyncW(bool value);

signals:
    void objectChanged(QObject* object);
    void propertyNameChanged(const QString& propertyName);
    void enableRChanged(bool enableR);
    void enableWChanged(bool enableW);
    void enableRWChanged(bool enableRW);
    void resyncRChanged(bool resyncR);
    void resyncWChanged(bool resyncW);
// ----

private slots:
    void changedHandler();

private:
    void detachProperty();
    void attachProperty();

private:
    bool m_connected { false };
    QObject* m_object { nullptr };
    QString m_propertyName;
    bool m_enableR  { true };
    bool m_enableW  { true };
    bool m_enableRW { true };
    bool m_resyncR { true };
    bool m_resyncW { true };
};
