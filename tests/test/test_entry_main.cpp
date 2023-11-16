/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include "gtest/gtest.h"

#ifdef UTILS_QT_NO_GUI_TESTS
#include <QCoreApplication>
#else
#include <QGuiApplication>
#endif

int main(int argc, char **argv) {
#ifdef UTILS_QT_NO_GUI_TESTS
    QCoreApplication app(argc, argv);
#else
    QGuiApplication app(argc, argv);
#endif
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
