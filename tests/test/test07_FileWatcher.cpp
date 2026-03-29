/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <gtest/gtest.h>
#include <QEventLoop>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QSignalSpy>

#include <UtilsQt/Futures/Utils.h>
#include <UtilsQt/Qml-Cpp/FileWatcher.h>

#include "internal/TestWaitHelpers.h"

using namespace UtilsQt;

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

        f.write("b");
        f.close();
        ASSERT_TRUE(TestHelpers::waitUntil([&]{ return spy.size() >= 4; }));
        ASSERT_TRUE(fw.fileExists());
        ASSERT_TRUE(fw.hasAccess());
        ASSERT_EQ(fw.size(), 2);
        ASSERT_EQ(spy.size(), 4);

        QFile::remove(fullPath);
        ASSERT_TRUE(TestHelpers::waitUntil([&]{ return spy.size() >= 5; }));

        ASSERT_FALSE(fw.fileExists());
        ASSERT_FALSE(fw.hasAccess());
        ASSERT_EQ(fw.size(), 0);
        ASSERT_EQ(spy.size(), 5);

        f.open(QIODevice::WriteOnly);
        f.close();
        ASSERT_TRUE(TestHelpers::waitUntil([&]{ return spy.size() >= 6; }));

        ASSERT_TRUE(fw.fileExists());
        ASSERT_TRUE(fw.hasAccess());
        ASSERT_EQ(fw.size(), 0);
        ASSERT_EQ(spy.size(), 6);

        QFile::remove(fullPath);
    }
}
