/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <QObject>
#include <QColor>
#include <QSize>
#include <QPoint>
#include <QList>
#include <QStringList>
#include <utils-cpp/copy_move.h>
#include <utils-cpp/pimpl.h>

class QQuickItem;

struct PathDetails
{
    enum Location {
        Unknown,
        Windows,
        NonWindows,
        Qrc
    };

    QString path;
    Location location {};
    bool isAbsolute {};
};

// codechecker_intentional [cppcoreguidelines-virtual-class-destructor]
// This is a singleton instance
class QmlUtils : public QObject
{
    Q_OBJECT
    NO_COPY_MOVE(QmlUtils);
public:
    static QmlUtils& instance();
    static void registerTypes();

    bool eventFilter(QObject* watched, QEvent* event) override;

#ifdef UTILS_QT_OS_WIN
    Q_PROPERTY(bool displayRequired READ displayRequired WRITE setDisplayRequired NOTIFY displayRequiredChanged)
    Q_PROPERTY(bool systemRequired READ systemRequired WRITE setSystemRequired NOTIFY systemRequiredChanged)
#endif
    Q_PROPERTY(int keyModifiers READ keyModifiers /*WRITE setKeyModifiers*/ NOTIFY keyModifiersChanged)

    // Clipboard
    Q_INVOKABLE void clipboardSetText(const QString& text) const;
    Q_INVOKABLE QString clipboardGetText() const;

    // Env. variables
    Q_INVOKABLE QString getEnvironentVariable(const QString& name) const;

    // Path
    Q_INVOKABLE PathDetails analyzePath(const QString& str) const;
    Q_INVOKABLE QString toUrl(const QString& str) const;
    Q_INVOKABLE QString normalizePath(const QString& str) const;
    Q_INVOKABLE QString normalizePathUrl(const QString& str) const;
    Q_INVOKABLE QString realFileName(const QString& str) const;
    Q_INVOKABLE QString realFileNameUrl(const QString& str) const;
    Q_INVOKABLE QString extractFileName(const QString& filePath) const;
    Q_INVOKABLE bool urlFileExists(const QUrl& url) const;
    Q_INVOKABLE bool localFileExists(const QString& fileName) const;

    // Images
    Q_INVOKABLE bool isImage(const QString& fileName) const;
    Q_INVOKABLE QSize imageSize(const QString& fileName) const;

    // Size
    Q_INVOKABLE QSize fitSize(const QSize& sourceSize, const QSize& limits) const; // keep aspect ratio
    Q_INVOKABLE QSize scaleSize(const QSize& size, double scale) const;

    // Colors
    Q_INVOKABLE QColor colorMakeAccent(const QColor& color, double factor) const;
    Q_INVOKABLE QColor colorChangeAlpha(const QColor& color, double alpha) const;

    // Numbers
    Q_INVOKABLE int bound(int min, int value, int max) const;
    Q_INVOKABLE bool isFloat(const QVariant& value) const;
    Q_INVOKABLE bool isInteger(const QVariant& value) const;
    Q_INVOKABLE bool isNumber(const QVariant& value) const;
    Q_INVOKABLE bool doublesEqual(double a, double b, double accuracy) const;

    // Values
    Q_INVOKABLE bool isNull(const QVariant& value) const;
    Q_INVOKABLE bool compare(const QVariant& value1, const QVariant& value2) const;

    // Regex
    Q_INVOKABLE QString extractByRegex(const QString& source, const QString& pattern) const;
    Q_INVOKABLE QStringList extractByRegexGroups(const QString& source, const QString& pattern, const QList<int>& groups) const;

    // Convert
    Q_INVOKABLE QString toHex(int value, bool upperCase = true, int width = 0) const;
    Q_INVOKABLE QString sizeConv(int size, int limit = 1000, int decimals = 1) const;

    // Window
    Q_INVOKABLE void showWindow(QObject* win);
    Q_INVOKABLE void minimizeWindow(QObject* win);

    // Cursor
    Q_INVOKABLE void setCustomCursor(QQuickItem* item, const QString& file, const QPoint& hotPoint);
    Q_INVOKABLE void setDefaultCursor(QQuickItem* item, Qt::CursorShape cursorShape);
    Q_INVOKABLE void resetCursor(QQuickItem* item);

signals:
    void keyPressed(Qt::Key key);
    void keyReleased(Qt::Key key);

// --- Properties support ---
public:
#ifdef UTILS_QT_OS_WIN
    bool displayRequired() const;
    bool systemRequired() const;
#endif
    int keyModifiers() const;

public slots:
#ifdef UTILS_QT_OS_WIN
    void setDisplayRequired(bool value);
    void setSystemRequired(bool value);
#endif

private slots:
    void setKeyModifiers(int value);

signals:
#ifdef UTILS_QT_OS_WIN
    void displayRequiredChanged(bool displayRequired);
    void systemRequiredChanged(bool systemRequired);
#endif
    void keyModifiersChanged(int keyModifiers);
// --- ---

private:
    QmlUtils();
    ~QmlUtils() override;

    void updateExecutionState();

private:
    DECLARE_PIMPL
};
