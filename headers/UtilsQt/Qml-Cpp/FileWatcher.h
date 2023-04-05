#pragma once
#include <QObject>
#include <QString>
#include <QUrl>
#include <utils-cpp/copy_move.h>
#include <utils-cpp/pimpl.h>

class FileWatcher : public QObject
{
    Q_OBJECT
    NO_COPY_MOVE(FileWatcher);
public:
    Q_PROPERTY(QUrl fileName READ fileName WRITE setFileName NOTIFY fileNameChanged)
    Q_PROPERTY(QString localFileName READ localFileName WRITE setLocalFileName NOTIFY localFileNameChanged)
    Q_PROPERTY(bool fileExists READ fileExists /*WRITE setFileExists*/ NOTIFY fileExistsChanged)
    Q_PROPERTY(bool hasAccess READ hasAccess /*WRITE setHasAccess*/ NOTIFY hasAccessChanged)
    Q_PROPERTY(int size READ size /*WRITE setSize*/ NOTIFY sizeChanged)

    static void registerTypes();

    explicit FileWatcher(QObject* parent = nullptr);
    ~FileWatcher() override;

    Q_INVOKABLE void update();

signals:
    void fileChanged();

    // Properties support
public:
    const QUrl& fileName() const;
    const QString& localFileName() const;
    bool fileExists() const;
    bool hasAccess() const;
    int size() const;

public slots:
    void setFileName(const QUrl& value);
    void setLocalFileName(const QString& value);

private:
    void setFileExists(bool value);
    void setHasAccess(bool value);
    void setSize(int value);

signals:
    void fileNameChanged(const QUrl& fileName);
    void localFileNameChanged(const QString& localFileName);
    void fileExistsChanged(bool fileExists);
    void hasAccessChanged(bool hasAccess);
    void sizeChanged(int size);
    // -----

private:
    void recreateWatcher();
    void updateFileInfo(bool* changedFlag = nullptr, bool readdPaths = true);
    static bool checkReadAccess(const QString& fileName);
    static int checkSize(const QString& fileName);

private:
    DECLARE_PIMPL
};
