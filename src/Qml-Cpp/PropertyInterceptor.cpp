/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/Qml-Cpp/PropertyInterceptor.h>
#include <QQmlEngine>
#include <private/qqmlproperty_p.h>

void PropertyInterceptor::registerTypes()
{
    qmlRegisterType<PropertyInterceptor>("UtilsQt", 1, 0, "PropertyInterceptor");
}

PropertyInterceptor::PropertyInterceptor(QObject* parent)
    : QObject(parent)
{
}

PropertyInterceptor::~PropertyInterceptor() = default;

void PropertyInterceptor::setTarget(const QQmlProperty& property)
{
    m_property = property;
}

void PropertyInterceptor::write(const QVariant& value)
{
    const auto oldValue = m_property.read();
    const auto& newValue = value;

    emit beforeUpdated(oldValue, newValue);

    QQmlPropertyPrivate::write(m_property, newValue,
                               QQmlPropertyData::BypassInterceptor | QQmlPropertyData::DontRemoveBinding);

    emit afterUpdated(oldValue, newValue);
}
