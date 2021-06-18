#pragma once
#include <QQuickItem>

class AbstractTransformer;

class MultibindingItem : public QQuickItem
{
    Q_OBJECT
public:
    Q_PROPERTY(QObject* object READ object WRITE setObject NOTIFY objectChanged)
    Q_PROPERTY(QString propertyName READ propertyName WRITE setPropertyName NOTIFY propertyNameChanged)
    Q_PROPERTY(QVariant value READ read WRITE write NOTIFY changed)
    Q_PROPERTY(AbstractTransformer* transformer READ transformer WRITE setTransformer NOTIFY transformerChanged)

    Q_PROPERTY(bool enableR READ enableR WRITE setEnableR NOTIFY enableRChanged)
    Q_PROPERTY(bool enableW READ enableW WRITE setEnableW NOTIFY enableWChanged)
    Q_PROPERTY(bool enableRW READ enableRW WRITE setEnableRW NOTIFY enableRWChanged)

    Q_PROPERTY(bool resyncR READ resyncR WRITE setResyncR NOTIFY resyncRChanged)
    Q_PROPERTY(bool resyncW READ resyncW WRITE setResyncW NOTIFY resyncWChanged)

    Q_PROPERTY(bool delayedR READ delayedR WRITE setDelayedR NOTIFY delayedRChanged)
    Q_PROPERTY(bool delayedW READ delayedW WRITE setDelayedW NOTIFY delayedWChanged)
    Q_PROPERTY(bool delayedRW READ delayedRW WRITE setDelayedRW NOTIFY delayedRWChanged)
    Q_PROPERTY(bool delayedWPending READ delayedWPending NOTIFY delayedWPendingChanged)

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
    AbstractTransformer* transformer() const { return m_transformer; }
    bool enableR() const { return m_enableR; }
    bool enableW() const { return m_enableW; }
    bool enableRW() const { return m_enableRW; }
    bool resyncR() const { return m_resyncR; }
    bool resyncW() const { return m_resyncW; }
    bool delayedR() const { return m_delayedR; }
    bool delayedW() const { return m_delayedW; }
    bool delayedRW() const { return m_delayedRW; }
    bool delayedWPending() const { return m_delayedWPending; }

public slots:
    void setObject(QObject* value);
    void setPropertyName(const QString& value);
    void setTransformer(AbstractTransformer* value);
    void setEnableR(bool value);
    void setEnableW(bool value);
    void setEnableRW(bool value);
    void updateRW();
    void setResyncR(bool value);
    void setResyncW(bool value);
    void setDelayedR(bool value);
    void setDelayedW(bool value);
    void setDelayedRW(bool value);
    void updateDelayedRW();
    void setDelayedWPending(bool value);

signals:
    void objectChanged(QObject* object);
    void propertyNameChanged(const QString& propertyName);
    void transformerChanged(AbstractTransformer* transformer);
    void enableRChanged(bool enableR);
    void enableWChanged(bool enableW);
    void enableRWChanged(bool enableRW);
    void resyncRChanged(bool resyncR);
    void resyncWChanged(bool resyncW);
    void delayedRChanged(bool delayedR);
    void delayedWChanged(bool delayedW);
    void delayedRWChanged(bool delayedRW);
    void delayedWPendingChanged(bool delayedWPending);
// ----

private slots:
    void changedHandler();

private:
    void writeImpl(QVariant value);
    bool isReady() const;
    void detachProperty();
    void attachProperty();
    void onBeforeTransformerUpdated();
    void onAfterTransformerUpdated();

private:
    bool m_connected { false };
    QObject* m_object { nullptr };
    QString m_propertyName;
    bool m_enableR  { true };
    bool m_enableW  { true };
    bool m_enableRW { true };
    bool m_resyncR { true };
    bool m_resyncW { true };
    bool m_delayedR { false };
    bool m_delayedW { false };
    bool m_delayedRW { false };
    AbstractTransformer* m_transformer { nullptr };
    QVariant m_orig;
    bool m_delayedWPending { false };
};
