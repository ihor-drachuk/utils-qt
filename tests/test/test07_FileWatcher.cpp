/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <gtest/gtest.h>
#include <chrono>
#include <QEventLoop>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QSignalSpy>

#include <UtilsQt/Futures/Utils.h>
#include <UtilsQt/Futures/SignalToFuture.h>
#include <UtilsQt/Qml-Cpp/FileWatcher.h>

#include "internal/TestWaitHelpers.h"

using namespace UtilsQt;
using namespace std::chrono_literals;

namespace {

// Maximum time to wait for file system events (generous for slow CI/macOS)
constexpr auto FileChangeTimeout = 5s;

// Helper to wait for FileWatcher::fileChanged signal with timeout
bool waitForFileChange(FileWatcher& fw, std::chrono::milliseconds timeout = FileChangeTimeout)
{
    auto f = signalToFuture(&fw, &FileWatcher::fileChanged, nullptr, timeout);
    waitForFuture<QEventLoop>(f);
    return !f.isCanceled();
}

} // namespace

TEST(UtilsQt, FileWatcher_Basic)
{
    auto path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(path);
    auto fileName = "file.ext";
    auto fullPath = QFileInfo(path, fileName).absoluteFilePath();

    {
        QFile::remove(fullPath);

        FileWatcher fw;
        QSignalSpy spy(&fw, &FileWatcher::fileChanged);

        auto url = QUrl::fromLocalFile(fullPath);
        fw.setFileName(url);

        ASSERT_EQ(fw.fileName(), url);
        ASSERT_EQ(fw.localFileName(), url.toLocalFile());
        ASSERT_FALSE(fw.fileExists());
        ASSERT_FALSE(fw.hasAccess());
        ASSERT_EQ(fw.size(), 0);
        ASSERT_EQ(spy.size(), 1);
    }

    {
        QFile f(fullPath);
        f.open(QIODevice::WriteOnly);
        f.write("a");
        f.flush();

        FileWatcher fw;
        QSignalSpy spy(&fw, &FileWatcher::fileChanged);
        auto url = QUrl::fromLocalFile(fullPath);
        fw.setLocalFileName(fullPath);

        ASSERT_EQ(fw.fileName(), url);
        ASSERT_EQ(fw.localFileName(), url.toLocalFile());
        ASSERT_TRUE(fw.fileExists());
        ASSERT_TRUE(fw.hasAccess());
        ASSERT_EQ(fw.size(), 1);
        ASSERT_EQ(spy.size(), 1);

        fw.setLocalFileName(fullPath + "2");
        ASSERT_EQ(fw.fileName(), QUrl::fromLocalFile(url.toLocalFile() + "2"));
        ASSERT_EQ(fw.localFileName(), url.toLocalFile() + "2");
        ASSERT_FALSE(fw.fileExists());
        ASSERT_FALSE(fw.hasAccess());
        ASSERT_EQ(fw.size(), 0);
        ASSERT_EQ(spy.size(), 2);

        fw.setFileName(url);
        ASSERT_EQ(fw.fileName(), url);
        ASSERT_EQ(fw.localFileName(), url.toLocalFile());
        ASSERT_TRUE(fw.fileExists());
        ASSERT_TRUE(fw.hasAccess());
        ASSERT_EQ(fw.size(), 1);
        ASSERT_EQ(spy.size(), 3);

        // Write more data and wait for file change event
        f.write("b");
        f.close();
        ASSERT_TRUE(waitForFileChange(fw)) << "Timeout waiting for file change after write";
        ASSERT_TRUE(fw.fileExists());
        ASSERT_TRUE(fw.hasAccess());
        ASSERT_EQ(fw.size(), 2);
        ASSERT_EQ(spy.size(), 4);

        // Remove file and wait for event
        QFile::remove(fullPath);
        ASSERT_TRUE(waitForFileChange(fw)) << "Timeout waiting for file change after delete";

        ASSERT_FALSE(fw.fileExists());
        ASSERT_FALSE(fw.hasAccess());
        ASSERT_EQ(fw.size(), 0);
        ASSERT_EQ(spy.size(), 5);

        // Recreate file and wait for event
        f.open(QIODevice::WriteOnly);
        f.close();
        ASSERT_TRUE(waitForFileChange(fw)) << "Timeout waiting for file change after recreate";

        ASSERT_TRUE(fw.fileExists());
        ASSERT_TRUE(fw.hasAccess());
        ASSERT_EQ(fw.size(), 0);
        ASSERT_EQ(spy.size(), 6);

        QFile::remove(fullPath);
    }
}
