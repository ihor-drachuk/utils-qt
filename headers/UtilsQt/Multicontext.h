/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <QObject>
#include <initializer_list>
#include <cassert>

/*
      Description
-----------------------
 - QObject* createMulticontext(QObject...)
   Creates QObject which's lifetime equals to shortest lifetime of provided QObjects.

 Can be useful when some object, operation or connection should be alive only while
 some other required objects are still alive.
*/

namespace UtilsQt {

QObject* createMulticontext(const std::initializer_list<QObject*>& objects);

} // namespace UtilsQt
