/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/qvariant_traits.h>
#include <UtilsQt/qvariant_migration.h>

namespace UtilsQt::QVariantTraits {

bool isUnknown(const QVariant& value)
{
    return QVariantMigration::getTypeId(value) == QVariantMigration::Invalid;
}

bool isDouble(const QVariant& value)
{
    return QVariantMigration::getTypeId(value) == QVariantMigration::Double;
}

bool isFloat(const QVariant& value)
{
    const auto typeId = QVariantMigration::getTypeId(value);
    return typeId == QVariantMigration::Double ||
           typeId == QVariantMigration::Float ||
           typeId == QVariantMigration::QReal;
}

bool isInteger(const QVariant& value)
{
    const auto typeId = QVariantMigration::getTypeId(value);
    return typeId == QVariantMigration::Int ||
           typeId == QVariantMigration::UInt ||
           typeId == QVariantMigration::LongLong ||
           typeId == QVariantMigration::ULongLong ||
           typeId == QVariantMigration::Long ||
           typeId == QVariantMigration::ULong ||
           typeId == QVariantMigration::Short ||
           typeId == QVariantMigration::UShort;
}

bool isList(const QVariant& value)
{
    return QVariantMigration::getTypeId(value) == QVariantMigration::List;
}

bool isString(const QVariant& value)
{
    return QVariantMigration::getTypeId(value) == QVariantMigration::String;
}

bool isByteArray(const QVariant& value)
{
    return QVariantMigration::getTypeId(value) == QVariantMigration::ByteArray;
}

} // namespace UtilsQt::QVariantTraits
