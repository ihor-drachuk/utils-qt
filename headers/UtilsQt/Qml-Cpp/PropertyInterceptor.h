/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <QObject>
#include <QQmlProperty>
#include <QVariant>
#include <private/qqmlpropertyvalueinterceptor_p.h>
#include <utils-cpp/default_ctor_ops.h>

class PropertyInterceptor : public QObject, public QQmlPropertyValueInterceptor
{
    Q_OBJECT
    Q_INTERFACES(QQmlPropertyValueInterceptor)
    NO_COPY_MOVE(PropertyInterceptor);

public:
    static void registerTypes();

    PropertyInterceptor(QObject* parent = nullptr);
    ~PropertyInterceptor() override;

    // QQmlPropertyValueInterceptor interface
    void setTarget(const QQmlProperty& property) override;
    void write(const QVariant& value) override;

signals:
    void beforeUpdated(QVariant oldValue, QVariant newValue);
    void afterUpdated(QVariant oldValue, QVariant newValue);

private:
    QQmlProperty m_property;
};
