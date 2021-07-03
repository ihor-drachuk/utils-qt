#pragma once
#include <QQuickItem>
#include <QVariant>
#include <QString>
#include <QTimer>

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

    enum DelayBehvior {
        RestartTimerOnChange,     // When long sequence of changes is finished, then after N ms last value is applied
        DontRestartTimerOnChange, // While long sequence of changes is going, each N ms most recent value is applied
    };
    Q_ENUM(DelayBehvior);

    Q_PROPERTY(QObject* object READ object WRITE setObject NOTIFY objectChanged)
    Q_PROPERTY(QString propertyName READ propertyName WRITE setPropertyName NOTIFY propertyNameChanged)
    Q_PROPERTY(QVariant value READ read WRITE write NOTIFY changed)
    Q_PROPERTY(AbstractTransformer* transformer READ transformer WRITE setTransformer NOTIFY transformerChanged)
    Q_PROPERTY(ReAttachBehavior reAttachBehvior READ reAttachBehvior WRITE setReAttachBehvior NOTIFY reAttachBehviorChanged)

    Q_PROPERTY(bool enableR READ enableR WRITE setEnableR NOTIFY enableRChanged)
    Q_PROPERTY(bool enableW READ enableW WRITE setEnableW NOTIFY enableWChanged)
    Q_PROPERTY(int enableRDelayOn READ enableRDelayOn WRITE setEnableRDelayOn NOTIFY enableRDelayOnChanged)
    Q_PROPERTY(int enableRDelayOff READ enableRDelayOff WRITE setEnableRDelayOff NOTIFY enableRDelayOffChanged)
    Q_PROPERTY(int enableWDelayOn READ enableWDelayOn WRITE setEnableWDelayOn NOTIFY enableWDelayOnChanged)
    Q_PROPERTY(int enableWDelayOff READ enableWDelayOff WRITE setEnableWDelayOff NOTIFY enableWDelayOffChanged)

    Q_PROPERTY(bool resyncR READ resyncR WRITE setResyncR NOTIFY resyncRChanged)
    Q_PROPERTY(bool resyncW READ resyncW WRITE setResyncW NOTIFY resyncWChanged)

    Q_PROPERTY(bool queuedR READ queuedR WRITE setQueuedR NOTIFY queuedRChanged)
    Q_PROPERTY(bool queuedW READ queuedW WRITE setQueuedW NOTIFY queuedWChanged)
    Q_PROPERTY(bool queuedWPending READ queuedWPending NOTIFY queuedWPendingChanged)

    Q_PROPERTY(int delayMsR READ delayMsR WRITE setDelayMsR NOTIFY delayMsRChanged)
    Q_PROPERTY(int delayMsW READ delayMsW WRITE setDelayMsW NOTIFY delayMsWChanged)
    Q_PROPERTY(DelayBehvior delayBehR READ delayBehR WRITE setDelayBehR NOTIFY delayBehRChanged)
    Q_PROPERTY(DelayBehvior delayBehW READ delayBehW WRITE setDelayBehW NOTIFY delayBehWChanged)

    static void registerTypes(const char* url);

    MultibindingItem(QQuickItem* parent = nullptr);

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
    bool resyncR() const { return m_resyncR; }
    bool resyncW() const { return m_resyncW; }
    bool queuedR() const { return m_queuedR; }
    bool queuedW() const { return m_queuedW; }
    bool queuedWPending() const { return m_queuedWPending; }
    int delayMsR() const { return m_delayMsR; }
    int delayMsW() const { return m_delayMsW; }
    DelayBehvior delayBehR() const { return m_delayBehR; }
    DelayBehvior delayBehW() const { return m_delayBehW; }
    int enableRDelayOn() const { return m_enableRDelayOn; }
    int enableRDelayOff() const { return m_enableRDelayOff; }
    int enableWDelayOn() const { return m_enableWDelayOn; }
    int enableWDelayOff() const { return m_enableWDelayOff; }

public slots:
    void setObject(QObject* value);
    void setPropertyName(const QString& value);
    void setTransformer(AbstractTransformer* value);
    void setReAttachBehvior(ReAttachBehavior value);
    void initReAttachBehvior(ReAttachBehavior value);
    void setEnableR(bool value);
    void setEnableRImpl(bool value);
    void setEnableW(bool value);
    void setEnableWImpl(bool value);
    void setResyncR(bool value);
    void setResyncW(bool value);
    void setQueuedR(bool value);
    void setQueuedW(bool value);
    void setQueuedWPending(bool value);
    void setDelayMsR(int value);
    void setDelayMsW(int value);
    void setDelayBehR(DelayBehvior value);
    void setDelayBehW(DelayBehvior value);
    void setEnableRDelayOn(int value);
    void setEnableRDelayOff(int value);
    void setEnableWDelayOn(int value);
    void setEnableWDelayOff(int value);

signals:
    void objectChanged(QObject* object);
    void propertyNameChanged(const QString& propertyName);
    void transformerChanged(AbstractTransformer* transformer);
    void reAttachBehviorChanged(ReAttachBehavior reAttachBehvior);
    void enableRChanged(bool enableR);
    void enableWChanged(bool enableW);
    void resyncRChanged(bool resyncR);
    void resyncWChanged(bool resyncW);
    void queuedRChanged(bool queuedR);
    void queuedWChanged(bool queuedW);
    void queuedWPendingChanged(bool queuedWPending);
    void delayMsRChanged(int delayMsR);
    void delayMsWChanged(int delayMsW);
    void delayBehRChanged(DelayBehvior delayBehR);
    void delayBehWChanged(DelayBehvior delayBehW);
    void enableRDelayOnChanged(int enableRDelayOn);
    void enableRDelayOffChanged(int enableRDelayOff);
    void enableWDelayOnChanged(int enableWDelayOn);
    void enableWDelayOffChanged(int enableWDelayOff);
// ----

private slots:
    void changedHandler();
    void changedHandler2();

private:
    void writeImpl(const QVariant& value);
    void writeImpl2(QVariant value);
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
    bool m_resyncR { true };
    bool m_resyncW { true };
    bool m_queuedR { false };
    bool m_queuedW { false };
    AbstractTransformer* m_transformer { nullptr };
    QVariant m_orig;
    bool m_queuedWPending { false };
    int m_delayMsR { 0 };
    int m_delayMsW { 0 };
    QTimer m_delayMsRTimer;
    QTimer m_delayMsWTimer;
    QVariant m_delayedWriteValue;
    DelayBehvior m_delayBehR { RestartTimerOnChange };
    DelayBehvior m_delayBehW { RestartTimerOnChange };
    int m_enableRDelayOn { 0 };
    int m_enableRDelayOff { 0 };
    int m_enableWDelayOn { 0 };
    int m_enableWDelayOff { 0 };
    QTimer m_enableRDelayTimer;
    QTimer m_enableWDelayTimer;
    bool m_enableRCached { false };
    bool m_enableWCached { false };
};
