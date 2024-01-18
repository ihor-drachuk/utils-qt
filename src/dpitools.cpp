/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/dpitools.h>

#include <QtGlobal>
#include <QCoreApplication>
#include <QString>
#include <QByteArray>


// Ignore messages about DPI, which change our windows size
#ifdef Q_OS_WINDOWS
#include <QAbstractNativeEventFilter>
#include <Windows.h>

class WinEventFilter : public QAbstractNativeEventFilter
{
public:
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    bool nativeEventFilter(const QByteArray& /*eventType*/, void* message, qintptr* /*result*/) override {
#else
    bool nativeEventFilter(const QByteArray& /*eventType*/, void* message, long* /*result*/) override {
#endif
        MSG* msg = static_cast<MSG*>( message );

        if (msg->message == 0x02e0 /*WM_DPICHANGED*/) {
            return true; // Intercept WM_DPICHANGED
        }

        return false;
    }
};
#endif // Q_OS_WINDOWS


void DpiTools::setAutoScreenScale(bool value)
{
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", value ? "1" : "0");
}

void DpiTools::setScaleFactor(double value)
{
    qputenv("QT_SCALE_FACTOR", QString::number(value, 'f', 2).toLatin1());
}

void DpiTools::ignoreSystemAutoResize(QCoreApplication& app)
{
#ifdef Q_OS_WINDOWS
    auto filter = new WinEventFilter;
    app.installNativeEventFilter(filter);
    QObject::connect(&app, &QObject::destroyed, [filter](){ delete filter; });
#else
    (void)app;
#endif
}
