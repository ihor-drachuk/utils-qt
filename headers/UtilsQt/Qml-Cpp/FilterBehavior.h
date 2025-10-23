/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <QObject>
#include <QQmlProperty>
#include <private/qqmlpropertyvalueinterceptor_p.h>
#include <utils-cpp/default_ctor_ops.h>

class FilterBehavior : public QObject, public QQmlPropertyValueInterceptor
{
    Q_OBJECT
    Q_INTERFACES(QQmlPropertyValueInterceptor)
    NO_COPY_MOVE(FilterBehavior);

    Q_PROPERTY(int delay READ delay WRITE setDelay NOTIFY delayChanged)
    Q_PROPERTY(bool when READ when WRITE setWhen NOTIFY whenChanged)

public:
    static void registerTypes();

    FilterBehavior(QObject* parent = nullptr);
    ~FilterBehavior() override;

    void setTarget(const QQmlProperty& property) override;
    void write(const QVariant& value) override;

    int delay() const;
    bool when() const;

public slots:
    void setDelay(int delay);
    void setWhen(bool when);

signals:
    void delayChanged(int delay);
    void whenChanged(bool when);

private:
    void onTimerTimeout();
    void applyValue(const QVariant& value);

private:
    struct impl_t;
    impl_t* _impl { nullptr };
    impl_t& impl() { return *_impl; }
    const impl_t& impl() const { return *_impl; }
};

