/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/qvariant_migration.h>

namespace UtilsQt::QVariantMigration {

int getTypeId(const QVariant &value)
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    return value.typeId();
#else
    return static_cast<int>(value.type());
#endif
}

} // namespace UtilsQt::QVariantMigration
