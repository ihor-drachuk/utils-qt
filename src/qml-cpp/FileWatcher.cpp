#include <utils-qt/qml-cpp/FileWatcher.h>

#include <stdexcept>
#include <limits>
#include <utils-qt/late_setter.h>

#include <QFileSystemWatcher>
#include <QFile>
#include <QFileInfo>
#include <QQmlEngine>

struct FileWatcher::impl_t
{
    QUrl fileName;
    bool fileExists {};
    bool hasAccess {};
    QString localFileName;
    int size {};

    std::unique_ptr<QFileSystemWatcher> fsWatcher;
};

void FileWatcher::registerTypes()
{
    qmlRegisterType<FileWatcher>("UtilsQt", 1, 0, "FileWatcher");
}

FileWatcher::FileWatcher(QObject* parent)
    : QObject(parent)
{
    createImpl();
}

FileWatcher::~FileWatcher()
{
}

void FileWatcher::update()
{
    updateFileInfo(nullptr, false);
}

const QUrl& FileWatcher::fileName() const
{
    return impl().fileName;
}

void FileWatcher::setFileName(const QUrl& value)
{
    if (impl().fileName == value)
        return;

    bool isChanged = false;
    LateSetter _ls1(impl().fileName, value, [this](const auto& v){ emit fileNameChanged(v); }, &isChanged);
    LateSetter _ls2(impl().localFileName, value.toLocalFile(), [this](const auto& v){ emit localFileNameChanged(v); }, &isChanged);
    recreateWatcher();
    updateFileInfo(&isChanged, false);
}

const QString& FileWatcher::localFileName() const
{
    return impl().localFileName;
}

void FileWatcher::setLocalFileName(const QString& value)
{
    if (impl().localFileName == value)
        return;

    bool isChanged = false;
    LateSetter _ls1(impl().localFileName, value,                                     [this](const auto& v){ emit localFileNameChanged(v); }, &isChanged);
    LateSetter _ls2(impl().fileName,      QUrl::fromLocalFile(impl().localFileName), [this](const auto& v){ emit fileNameChanged(v); }, &isChanged);
    recreateWatcher();
    updateFileInfo(&isChanged, false);
}

bool FileWatcher::fileExists() const
{
    return impl().fileExists;
}

void FileWatcher::setFileExists(bool value)
{
    if (impl().fileExists == value)
        return;
    impl().fileExists = value;
    emit fileExistsChanged(impl().fileExists);
}

bool FileWatcher::hasAccess() const
{
    return impl().hasAccess;
}

void FileWatcher::setHasAccess(bool value)
{
    if (impl().hasAccess == value)
        return;
    impl().hasAccess = value;
    emit hasAccessChanged(impl().hasAccess);
}

void FileWatcher::recreateWatcher()
{
    impl().fsWatcher = std::make_unique<QFileSystemWatcher>(
                         QStringList{
                           QFileInfo(impl().localFileName).absolutePath(),
                           impl().localFileName
                         }
                       );

    QObject::connect(impl().fsWatcher.get(), &QFileSystemWatcher::directoryChanged,
                     this, [this](){ updateFileInfo(); });

    QObject::connect(impl().fsWatcher.get(), &QFileSystemWatcher::fileChanged,
                     this, [this](){ updateFileInfo(); });
}

void FileWatcher::updateFileInfo(bool* changedFlag, bool readdPaths)
{
    bool ownIsChanged = false;
    if (!changedFlag) changedFlag = &ownIsChanged;

    LateSetter _ls3(impl().fileExists, QFile::exists(impl().localFileName),   [this](const auto& v){ emit fileExistsChanged(v); }, changedFlag);
    LateSetter _ls4(impl().hasAccess,  checkReadAccess(impl().localFileName), [this](const auto& v){ emit hasAccessChanged(v); }, changedFlag);
    LateSetter _ls5(impl().size,       checkSize(impl().localFileName),       [this](const auto& v){ emit sizeChanged(v); }, changedFlag);

    if (readdPaths) {
        impl().fsWatcher->addPaths(QStringList{
                                     QFileInfo(impl().localFileName).absolutePath(),
                                     impl().localFileName
                                   });
    }

    if (*changedFlag)
        emit fileChanged();
}

bool FileWatcher::checkReadAccess(const QString& fileName)
{
    QFile f(fileName);
    return f.open(QIODevice::ReadOnly);
}

int FileWatcher::checkSize(const QString& fileName)
{
    QFile f(fileName);
    auto status = f.open(QIODevice::ReadOnly);
    if (!status) return 0;

    auto result = f.size();

    if (result > std::numeric_limits<int>::max())
        throw std::runtime_error("Unsupported file size!");

    return result;
}

int FileWatcher::size() const
{
    return impl().size;
}

void FileWatcher::setSize(int value)
{
    if (impl().size == value)
        return;
    impl().size = value;
    emit sizeChanged(impl().size);
}
