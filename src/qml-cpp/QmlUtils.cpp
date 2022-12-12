#include <UtilsQt/Qml-Cpp/QmlUtils.h>

#include <optional>

#include <QFile>
#include <QQmlEngine>
#include <QImageReader>
#include <QFileInfo>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QGuiApplication>
#include <QUrl>
#ifdef UTILS_QT_OS_WIN
#include <Windows.h>
#endif


namespace {
    template<size_t N> constexpr size_t length(char const (&)[N]) { return N-1; }

    const char filePrefixWin[] = "file:///";
    constexpr int filePrefixWinLen = length(filePrefixWin);

    const char filePrefix[] = "file://";
    constexpr int filePrefixLen = length(filePrefix);

    const char qrcPrefix[] = "qrc:/";
    constexpr int qrcPrefixLen = length(qrcPrefix);
    const char qrcPrefixReplacement[] = ":/";
    constexpr int qrcPrefixReplacementLen = length(qrcPrefixReplacement);
}


struct QmlUtils::impl_t
{
#ifdef UTILS_QT_OS_WIN
    bool displayRequired { false };
    bool systemRequired { false };
#endif
    int keyModifiers {};
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

bool QmlUtils::eventFilter(QObject* /*watched*/, QEvent* event)
{
    switch (event->type()) {
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
            break;

        default:
            return false;
    }

    const auto km = QGuiApplication::queryKeyboardModifiers();
    setKeyModifiers(static_cast<int>(km));

    assert(dynamic_cast<QKeyEvent*>(event));
    auto keyEvent = static_cast<QKeyEvent*>(event);
    auto key = static_cast<Qt::Key>(keyEvent->key());

    switch (event->type()) {
        case QEvent::KeyPress:   emit keyPressed(key);  break;
        case QEvent::KeyRelease: emit keyReleased(key); break;
        default:                 assert(!"Unexpected event type!");
    }

    return false;
}

PathDetails QmlUtils::analyzePath(const QString& str) const
{
    if ((str.size() >= 2 && str[1] == ":")) {
        return {str, PathDetails::Windows, true};

    } else if (str.startsWith(filePrefixWin)) {
        return {str.mid(filePrefixWinLen), PathDetails::Windows, true};

    } else if (str.startsWith("/")) {
        return {str, PathDetails::NonWindows, true};

    } else if (str.startsWith(filePrefix)) {
        return {str.mid(filePrefixLen), PathDetails::NonWindows, true};

    } else if (str.startsWith(qrcPrefix)) {
        return {qrcPrefixReplacement + str.mid(qrcPrefixLen), PathDetails::Qrc, true};

    } else if (str.startsWith(qrcPrefixReplacement)) {
        return {str, PathDetails::Qrc, true};
    }

    return {str, PathDetails::Unknown, false};
}

QString QmlUtils::toUrl(const QString& str) const
{
    const auto details = analyzePath(str);

    if (!details.isAbsolute)
        return str;

    switch (details.location) {
        case PathDetails::Unknown:
            return details.path;
            break;

        case PathDetails::Windows:
            return "file:///" + details.path;
            break;

        case PathDetails::NonWindows:
            return "file://" + details.path;
            break;

        case PathDetails::Qrc:
            return qrcPrefix + details.path;
            break;
    }

    assert(!"Unexpected control flow!");
    return {};
}

bool QmlUtils::isImage(const QString& fileName) const
{
    return !QImageReader::imageFormat(realFileName(fileName)).isEmpty();
}

QSize QmlUtils::imageSize(const QString& fileName) const
{
    return QImageReader(realFileName(fileName)).size();
}

QColor QmlUtils::colorMakeAccent(const QColor& color, double factor) const
{
    const auto hsla = color.toHsl();

    if (hsla.lightnessF() > 0.3) {
        const auto rgba = color.toRgb();
        return QColor::fromRgbF(rgba.redF(), rgba.greenF(), rgba.blueF(), rgba.alphaF() * factor);
    } else {
        return QColor::fromHslF(hsla.hslHueF(), hsla.hslSaturationF(), hsla.lightnessF() + (factor - 1.0) / 1.5, hsla.alphaF());
    }
}

QColor QmlUtils::colorChangeAlpha(const QColor& color, double alpha) const
{
    const auto rgba = color.toRgb();
    return QColor::fromRgbF(rgba.redF(), rgba.greenF(), rgba.blueF(), alpha);
}

QString QmlUtils::normalizePath(const QString& str) const
{
    return analyzePath(str).path;
}

QString QmlUtils::normalizePathUrl(const QString& str) const
{
    return toUrl(normalizePath(str));
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
    return toUrl(realFileName(str));
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

#ifdef UTILS_QT_OS_WIN
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
#endif // UTILS_QT_OS_WIN
}

void QmlUtils::minimizeWindow(QObject* win)
{
    auto window = qobject_cast<QQuickWindow*>(win);
    assert(window);

    auto flags = window->windowStates();
    flags |= Qt::WindowState::WindowMinimized;
    window->setWindowStates(flags);
}

#ifdef UTILS_QT_OS_WIN
bool QmlUtils::displayRequired() const
{
    return impl().displayRequired;
}
#endif


#ifdef UTILS_QT_OS_WIN
bool QmlUtils::systemRequired() const
{
    return impl().systemRequired;
}
#endif


#ifdef UTILS_QT_OS_WIN
void QmlUtils::setDisplayRequired(bool value)
{
    if (impl().displayRequired == value)
        return;

    impl().displayRequired = value;
    updateExecutionState();
    emit displayRequiredChanged(impl().displayRequired);
}
#endif


#ifdef UTILS_QT_OS_WIN
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

    auto app = QGuiApplication::instance();
    Q_ASSERT(app);
    app->installEventFilter(this);
}

QmlUtils::~QmlUtils()
{
    if (auto app = QGuiApplication::instance())
        app->removeEventFilter(this);
}

void QmlUtils::updateExecutionState()
{
#ifdef UTILS_QT_OS_WIN
    SetThreadExecutionState(ES_CONTINUOUS |
                            (impl().systemRequired ? ES_SYSTEM_REQUIRED : 0) |
                            (impl().displayRequired ? ES_DISPLAY_REQUIRED : 0));
#endif
}

int QmlUtils::keyModifiers() const
{
    return impl().keyModifiers;
}

void QmlUtils::setKeyModifiers(int value)
{
    if (impl().keyModifiers == value)
        return;
    impl().keyModifiers = value;
    emit keyModifiersChanged(impl().keyModifiers);
}
