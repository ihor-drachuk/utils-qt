/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <QQuickItem>
#include <QVariant>
#include <QVariantList>
#include <utils-cpp/default_ctor_ops.h>

// Lightweight helper class for QML singleton (VSW.MinDuration, etc.)
class VSW : public QObject
{
    Q_OBJECT
public:
    explicit VSW(QObject* parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE static QVariant MinDuration(const QVariant& value, int ms);
    Q_INVOKABLE static QVariant MaxDuration(const QVariant& value, int ms);
    Q_INVOKABLE static QVariant Duration(const QVariant& value, int minMs, int maxMs);
};

// Main class, watches for a sequence of value changes
class ValueSequenceWatcher : public QQuickItem
{
    Q_OBJECT
    NO_COPY_MOVE(ValueSequenceWatcher);

    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(QVariantList sequence READ sequence WRITE setSequence NOTIFY sequenceChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool once READ once WRITE setOnce NOTIFY onceChanged)
    Q_PROPERTY(bool resetOnTrigger READ resetOnTrigger WRITE setResetOnTrigger NOTIFY resetOnTriggerChanged)
    Q_PROPERTY(int triggerCount READ triggerCount NOTIFY triggerCountChanged)

public:
    static void registerTypes();

    explicit ValueSequenceWatcher(QQuickItem* parent = nullptr);
    ~ValueSequenceWatcher() override;

    // Property getters
    QVariant value() const;
    QVariantList sequence() const;
    bool enabled() const;
    bool once() const;
    bool resetOnTrigger() const;
    int triggerCount() const;

public slots:
    void setValue(const QVariant& value);
    void setSequence(const QVariantList& sequence);
    void setEnabled(bool enabled);
    void setOnce(bool once);
    void setResetOnTrigger(bool resetOnTrigger);

signals:
    void valueChanged(const QVariant& value);
    void sequenceChanged(const QVariantList& sequence);
    void enabledChanged(bool enabled);
    void onceChanged(bool once);
    void resetOnTriggerChanged(bool resetOnTrigger);
    void triggerCountChanged(int triggerCount);
    void triggered();

protected:
    void componentComplete() override;

private:
    void onValueChanged();
    void onTimerTimeout();
    void resetHistory();
    void addToHistory(const QVariant& value);
    void checkMatch();
    void doTrigger();
    void setTriggerCount(int count);

private:
    struct impl_t;
    impl_t* _impl { nullptr };
    impl_t& impl() { return *_impl; }
    const impl_t& impl() const { return *_impl; }
};
