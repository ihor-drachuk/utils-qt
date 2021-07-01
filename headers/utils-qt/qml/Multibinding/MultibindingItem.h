#pragma once
#include <QQuickItem>

class AbstractTransformer;

class MultibindingItem : public QQuickItem
{
    Q_OBJECT
public:
    enum ReAttachBehavior {
        SyncNewProperty,
        SyncMultibinding
    };
    Q_ENUM(ReAttachBehavior);

    Q_PROPERTY(QObject* object READ object WRITE setObject NOTIFY objectChanged)
    Q_PROPERTY(QString propertyName READ propertyName WRITE setPropertyName NOTIFY propertyNameChanged)
    Q_PROPERTY(QVariant value READ read WRITE write NOTIFY changed)
    Q_PROPERTY(AbstractTransformer* transformer READ transformer WRITE setTransformer NOTIFY transformerChanged)
    Q_PROPERTY(ReAttachBehavior reAttachBehvior READ reAttachBehvior WRITE setReAttachBehvior NOTIFY reAttachBehviorChanged)

    Q_PROPERTY(bool enableR READ enableR WRITE setEnableR NOTIFY enableRChanged)
    Q_PROPERTY(bool enableW READ enableW WRITE setEnableW NOTIFY enableWChanged)
    Q_PROPERTY(bool enableRW READ enableRW WRITE setEnableRW NOTIFY enableRWChanged)

    Q_PROPERTY(bool resyncR READ resyncR WRITE setResyncR NOTIFY resyncRChanged)
    Q_PROPERTY(bool resyncW READ resyncW WRITE setResyncW NOTIFY resyncWChanged)

    Q_PROPERTY(bool queuedR READ queuedR WRITE setQueuedR NOTIFY queuedRChanged)
    Q_PROPERTY(bool queuedW READ queuedW WRITE setQueuedW NOTIFY queuedWChanged)
    Q_PROPERTY(bool queuedRW READ queuedRW WRITE setQueuedRW NOTIFY queuedRWChanged)
    Q_PROPERTY(bool queuedWPending READ queuedWPending NOTIFY queuedWPendingChanged)

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
    ReAttachBehavior reAttachBehvior() const { return m_reAttachBehvior; }
    bool enableR() const { return m_enableR; }
    bool enableW() const { return m_enableW; }
    bool enableRW() const { return m_enableRW; }
    bool resyncR() const { return m_resyncR; }
    bool resyncW() const { return m_resyncW; }
    bool queuedR() const { return m_queuedR; }
    bool queuedW() const { return m_queuedW; }
    bool queuedRW() const { return m_queuedRW; }
    bool queuedWPending() const { return m_queuedWPending; }

public slots:
    void setObject(QObject* value);
    void setPropertyName(const QString& value);
    void setTransformer(AbstractTransformer* value);
    void setReAttachBehvior(ReAttachBehavior value);
    void initReAttachBehvior(ReAttachBehavior value);
    void setEnableR(bool value);
    void setEnableW(bool value);
    void setEnableRW(bool value);
    void updateRW();
    void setResyncR(bool value);
    void setResyncW(bool value);
    void setQueuedR(bool value);
    void setQueuedW(bool value);
    void setQueuedRW(bool value);
    void updateQueuedRW();
    void setQueuedWPending(bool value);

signals:
    void objectChanged(QObject* object);
    void propertyNameChanged(const QString& propertyName);
    void transformerChanged(AbstractTransformer* transformer);
    void reAttachBehviorChanged(ReAttachBehavior reAttachBehvior);
    void enableRChanged(bool enableR);
    void enableWChanged(bool enableW);
    void enableRWChanged(bool enableRW);
    void resyncRChanged(bool resyncR);
    void resyncWChanged(bool resyncW);
    void queuedRChanged(bool queuedR);
    void queuedWChanged(bool queuedW);
    void queuedRWChanged(bool queuedRW);
    void queuedWPendingChanged(bool queuedWPending);
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
    ReAttachBehavior m_reAttachBehvior { SyncNewProperty };
    bool m_reAttachBehviorIsSet { false };
    bool m_enableR  { true };
    bool m_enableW  { true };
    bool m_enableRW { true };
    bool m_resyncR { true };
    bool m_resyncW { true };
    bool m_queuedR { false };
    bool m_queuedW { false };
    bool m_queuedRW { false };
    AbstractTransformer* m_transformer { nullptr };
    QVariant m_orig;
    bool m_queuedWPending { false };
};
