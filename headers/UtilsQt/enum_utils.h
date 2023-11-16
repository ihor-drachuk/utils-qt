/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <QMetaEnum>
#include <QStringList>
#include <optional>

namespace Enum {

template<class T>
const QMetaEnum& cachedMetaEnum()
{
    static const QMetaEnum metaEnum = QMetaEnum::fromType<T>();
    return metaEnum;
}

template<class T>
std::enable_if_t<std::is_enum<T>::value, std::optional<T>>
fromString(const QString& value)
{
    bool ok;
    auto intValue = cachedMetaEnum<T>().keyToValue(value.toLatin1().data(), &ok);
    return ok ? static_cast<T>(intValue) : std::optional<T>();
}

template<class T>
std::enable_if_t<std::is_enum<T>::value, QString>
toString(T value)
{
    return cachedMetaEnum<T>().valueToKey(static_cast<int>(value));
}

template<class T>
std::enable_if_t<std::is_enum<T>::value, bool>
isValid(T value)
{
    return !toString(value).isEmpty();
}

template<class T>
std::enable_if_t<std::is_enum<T>::value, QStringList>
allNames()
{
    const auto metaEnum = cachedMetaEnum<T>();
    QStringList result;

    for (int i = 0; i < metaEnum.keyCount(); i++) {
        result.append(metaEnum.key(i));
    }

    return result;
}

template<class T>
std::enable_if_t<std::is_enum<T>::value, void>
makeValid(T& value)
{
    if (isValid<T>(value)) return;

    const auto metaEnum = cachedMetaEnum<T>();
    Q_ASSERT(metaEnum.keyCount() > 0);
    value = static_cast<T>(metaEnum.keyToValue(metaEnum.key(0)));
}

} // namespace Enum
