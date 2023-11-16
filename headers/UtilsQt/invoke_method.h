/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <QObject>
#include <type_traits>

namespace UtilsQt {

#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0) // Qt 5.10 (^) and upper

template<typename Callable>
void invokeMethod(QObject* context, const Callable& callable, Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    QMetaObject::invokeMethod(context, callable, connectionType);
}

template<typename Obj,
         typename std::enable_if<std::is_base_of<QObject, Obj>::value>::type* = nullptr>
void invokeMethod(Obj* context, void (Obj::* member)(), Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    QMetaObject::invokeMethod(context, member, connectionType);
}

#else // Before Qt 5.10 (v)

template<typename Callable>
void invokeMethod(QObject* context, const Callable& callable, Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    QObject src;
    QObject::connect(&src, &QObject::destroyed, context, callable, connectionType);
}

template<typename Obj,
         typename std::enable_if<std::is_base_of<QObject, Obj>::value>::type* = nullptr>
void invokeMethod(Obj* context, void (Obj::* member)(), Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    QObject src;
    QObject::connect(&src, &QObject::destroyed, context, member, connectionType);
}

#endif

} // namespace UtilsQt
