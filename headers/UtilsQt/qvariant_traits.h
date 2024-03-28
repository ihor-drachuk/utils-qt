/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <QVariant>

namespace UtilsQt::QVariantTraits {

bool isUnknown(const QVariant &value);
bool isDouble(const QVariant &value);
bool isFloat(const QVariant &value);
bool isInteger(const QVariant &value);
bool isList(const QVariant &value);
bool isString(const QVariant &value);
bool isByteArray(const QVariant &value);

} // namespace UtilsQt::QVariantTraits
