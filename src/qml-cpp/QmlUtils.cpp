#include <utils-qt/qml-cpp/QmlUtils.h>

#include <QQmlEngine>
#include <QImageReader>
#include <QFileInfo>
#include <QQuickWindow>
#ifdef WIN32
#include <Windows.h>
#endif


namespace {
    template<size_t N> constexpr size_t length(char const (&)[N]) { return N-1; }

    const char filePrefix[] = "file:///";
    constexpr int filePrefixLen = length(filePrefix);
}


struct QmlUtils::impl_t
{
#ifdef WIN32
    bool displayRequired { false };
    bool systemRequired { false };
#endif
};


QmlUtils& QmlUtils::instance()
{
    static QmlUtils object;
    return object;
}

void QmlUtils::registerTypes()
{
    qmlRegisterSingletonType<QmlUtils>("UtilsQt", 1, 0, "QmlUtils", [] (QQmlEngine *engine, QJSEngine *) -> QObject* {
        auto ret = &QmlUtils::instance();
        engine->setObjectOwnership(ret, QQmlEngine::CppOwnership);
        return ret;
    });
}

bool QmlUtils::isImage(const QString& fileName) const
{
    return !QImageReader::imageFormat(realFileName(fileName)).isEmpty();
}

QString QmlUtils::normalizePath(const QString& str) const
{
    if (str.isEmpty())
        return QString();

    if (str.startsWith(filePrefix)) {
        return str.mid(filePrefixLen);
    } else {
        return str;
    }
}

QString QmlUtils::normalizePathUrl(const QString& str) const
{
    if (str.isEmpty())
        return QString();

    return "file:///" + normalizePath(str);
}

int QmlUtils::bound(int min, int value, int max) const
{
    return qBound(min, value, max);
}

QString QmlUtils::realFileName(const QString& str) const
{
    const auto normStr = normalizePath(str);
    QFileInfo fileInfo(normStr);
    return fileInfo.isSymLink() ? fileInfo.symLinkTarget() : normStr; // Add normalizePath to symLink?
}

QString QmlUtils::realFileNameUrl(const QString& str) const
{
    return "file:///" + realFileName(str);
}

QString QmlUtils::extractFileName(const QString& filePath) const
{
    return QFileInfo(normalizePath(filePath)).fileName();
}

bool QmlUtils::isNull(const QVariant& value) const
{
    return value.isNull() || !value.isValid();
}


#ifdef WIN32
void QmlUtils::showWindow(void* hWnd, bool maximize)
{
    assert(hWnd);
    auto winId = (HWND)hWnd;

    ShowWindow(winId, SW_RESTORE);
    DWORD windowThreadProcessId = GetWindowThreadProcessId(GetForegroundWindow(), LPDWORD(0));
    DWORD currentThreadId = GetCurrentThreadId();
    AttachThreadInput(windowThreadProcessId, currentThreadId, true);
    BringWindowToTop(winId);
    ShowWindow(winId, maximize ? SW_MAXIMIZE : SW_SHOW);
    AttachThreadInput(windowThreadProcessId, currentThreadId, false);
}
#endif


#ifdef WIN32
void QmlUtils::showWindow(QObject* win)
{
    auto window = qobject_cast<QQuickWindow*>(win);
    assert(window);
    showWindow((void*)window->winId());
}
#endif


#ifdef WIN32
bool QmlUtils::displayRequired() const
{
    return impl().displayRequired;
}
#endif


#ifdef WIN32
bool QmlUtils::systemRequired() const
{
    return impl().systemRequired;
}
#endif


#ifdef WIN32
void QmlUtils::setDisplayRequired(bool value)
{
    if (impl().displayRequired == value)
        return;

    impl().displayRequired = value;
    updateExecutionState();
    emit displayRequiredChanged(impl().displayRequired);
}
#endif


#ifdef WIN32
void QmlUtils::setSystemRequired(bool value)
{
    if (impl().systemRequired == value)
        return;

    impl().systemRequired = value;
    updateExecutionState();
    emit systemRequiredChanged(impl().systemRequired);
}
#endif


QmlUtils::QmlUtils()
{
    createImpl();
    updateExecutionState();
}

QmlUtils::~QmlUtils()
{
}

void QmlUtils::updateExecutionState()
{
#ifdef WIN32
    SetThreadExecutionState(ES_CONTINUOUS |
                            (impl().systemRequired ? ES_SYSTEM_REQUIRED : 0) |
                            (impl().displayRequired ? ES_DISPLAY_REQUIRED : 0));
#endif
}
