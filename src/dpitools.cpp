#include <utils-qt/dpitools.h>

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
    bool nativeEventFilter(const QByteArray& /*eventType*/, void* message, long* /*result*/) override {
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

void DpiTools::setScaleFactor(float value)
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
