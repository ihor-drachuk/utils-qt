/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/Multicontext.h>

namespace UtilsQt {

QObject* createMulticontext(const std::initializer_list<QObject*>& objects)
{
    assert(objects.size() > 0);

    auto resultObj = new QObject();

    auto it = objects.begin();

    while (it != objects.end()) {
        QObject::connect(*it++, &QObject::destroyed, resultObj, [resultObj](){ delete resultObj; });
    }

    return resultObj;
}

} // namespace UtilsQt
