#include <utils-qt/qml-cpp/QmlUtils.h>

#include <optional>

#include <QFile>
#include <QQmlEngine>
#include <QImageReader>
#include <QFileInfo>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QUrl>
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

bool QmlUtils::isFloat(const QVariant& value) const
{
    return value.type() == QVariant::Type::Double ||
           value.type() == (QVariant::Type)QMetaType::Type::Float ||
           value.type() == (QVariant::Type)QMetaType::Type::QReal;
}

bool QmlUtils::isInteger(const QVariant& value) const
{
    return value.type() == QVariant::Type::Int ||
           value.type() == QVariant::Type::UInt ||
           value.type() == QVariant::Type::LongLong ||
           value.type() == QVariant::Type::ULongLong ||
           value.type() == (QVariant::Type)QMetaType::Type::Long ||
           value.type() == (QVariant::Type)QMetaType::Type::ULong ||
           value.type() == (QVariant::Type)QMetaType::Type::Short ||
           value.type() == (QVariant::Type)QMetaType::Type::UShort;
}

bool QmlUtils::isNumber(const QVariant& value) const
{
    return isFloat(value) || isInteger(value);
}

bool QmlUtils::compare(const QVariant& value1, const QVariant& value2) const
{
    if (isNumber(value1) || isNumber(value2)) {
        bool ok1, ok2;
        auto result = qFuzzyCompare(value1.toDouble(&ok1), value2.toDouble(&ok2));

        if (ok1 && ok2)
            return result;
    }

    return (value1 == value2);
}

QString QmlUtils::extractByRegex(const QString& source, const QString& pattern) const
{
    QRegularExpression regex(pattern);
    auto match = regex.match(source);
    return match.hasMatch() ? match.captured() : "";
}

QStringList QmlUtils::extractByRegexGroups(const QString& source, const QString& pattern, const QList<int>& groups) const
{
    QRegularExpression regex(pattern);
    auto match = regex.match(source);
    if (!match.hasMatch()) return {};

    QStringList result;
    for (auto i : groups) {
        if (i > match.lastCapturedIndex()) return {};
        result.append(match.captured(i));
    }

    return result;
}

QString QmlUtils::toHex(int value, bool upperCase, int width) const
{
    auto result = QStringLiteral("%1").arg(value, width, 16, QChar('0'));
    return upperCase ? result.toUpper() : result;
}

QString QmlUtils::sizeConv(int size, int limit, int decimals) const
{
    if (!size)
        return "0 bytes";

    const char* prefixes[] = {"bytes", "Kb", "Mb", "Gb", "Tb"};
    constexpr int prefixesLen = sizeof(prefixes) / sizeof(*prefixes);
    float result = size;
    int i = 0;

    while (i < prefixesLen - 1 && result >= limit) {
        result /= 1024;
        i++;
    }

    return QStringLiteral("%1 %2")
            .arg(result, 0, 'f', i == 0 ? 0 : decimals)
            .arg(prefixes[i]);
}

bool QmlUtils::urlFileExists(const QUrl& url) const
{
    return localFileExists(url.toLocalFile());
}

bool QmlUtils::localFileExists(const QString& fileName) const
{
    return QFile::exists(fileName);
}

void QmlUtils::showWindow(QObject* win)
{
    auto window = qobject_cast<QQuickWindow*>(win);
    assert(window);

    auto flags = window->windowStates();
    flags &= ~Qt::WindowState::WindowMinimized;
    window->setWindowStates(flags);

#ifdef WIN32
    //auto status = AllowSetForegroundWindow(GetCurrentProcessId()); assert(status);
    auto hWnd = (void*)window->winId();
    auto winId = (HWND)hWnd;
    ShowWindow(winId, SW_RESTORE);
    DWORD windowThreadProcessId = GetWindowThreadProcessId(GetForegroundWindow(), LPDWORD(0));
    DWORD currentThreadId = GetCurrentThreadId();
    AttachThreadInput(windowThreadProcessId, currentThreadId, true);
    BringWindowToTop(winId);
    AttachThreadInput(windowThreadProcessId, currentThreadId, false);
#else
    window->requestActivate();
#endif // WIN32
}

void QmlUtils::minimizeWindow(QObject* win)
{
    auto window = qobject_cast<QQuickWindow*>(win);
    assert(window);

    auto flags = window->windowStates();
    flags |= Qt::WindowState::WindowMinimized;
    window->setWindowStates(flags);
}

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
