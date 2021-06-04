#pragma once
#include <QObject>

namespace UtilsQt {

#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0) // Qt 5.10 (^) and upper

template<typename Callable>
void invokeMethod(QObject* context, const Callable& callable, Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    QMetaObject::invokeMethod(context, callable, connectionType);
}

#else // Before Qt 5.10 (v)

template<typename Callable>
void invokeMethod(QObject* context, const Callable& callable, Qt::ConnectionType connectionType = Qt::AutoConnection)
{
    QObject src;
    QObject::connect(&src, &QObject::destroyed, context, callable, connectionType);
}

#endif

} // namespace UtilsQt
