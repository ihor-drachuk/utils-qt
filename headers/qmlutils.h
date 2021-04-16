#pragma once
#include <QObject>
#include "pimpl.h"


class QmlUtils : public QObject
{
    Q_OBJECT
public:
    static QmlUtils& instance();
    static void registerTypes(const char* url);

#ifdef WIN32
    Q_PROPERTY(bool displayRequired READ displayRequired WRITE setDisplayRequired NOTIFY displayRequiredChanged)
    Q_PROPERTY(bool systemRequired READ systemRequired WRITE setSystemRequired NOTIFY systemRequiredChanged)
#endif

    Q_INVOKABLE bool isImage(const QString& fileName) const;
    Q_INVOKABLE QString normalizePath(const QString& str) const;
    Q_INVOKABLE QString normalizePathUrl(const QString& str) const;
    Q_INVOKABLE int bound(int min, int value, int max) const;
    Q_INVOKABLE QString realFileName(const QString& str) const;
    Q_INVOKABLE QString realFileNameUrl(const QString& str) const;
    Q_INVOKABLE QString extractFileName(const QString& filePath) const;

#ifdef WIN32
    void showWindow(void* hWnd, bool maximize = false);
    Q_INVOKABLE void showWindow(QObject* win);
#endif

// --- Properties support ---
public:
#ifdef WIN32
    bool displayRequired() const;
    bool systemRequired() const;
#endif

public slots:
#ifdef WIN32
    void setDisplayRequired(bool value);
    void setSystemRequired(bool value);
#endif

signals:
#ifdef WIN32
    void displayRequiredChanged(bool displayRequired);
    void systemRequiredChanged(bool systemRequired);
#endif
// --- ---

private:
    QmlUtils();
    ~QmlUtils();

    void updateExecutionState();

private:
    DECLARE_PIMPL
};
