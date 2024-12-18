/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/Futures/SignalToFuture.h>

#include <QTimer>

namespace UtilsQt {

namespace Internal {

void deleteAfter(QObject* object, std::chrono::milliseconds timeout)
{
    QTimer::singleShot(timeout.count(), object, &QObject::deleteLater);
}

} // namespace Internal

} // namespace UtilsQt
