/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <gtest/gtest.h>
#include <UtilsQt/Multicontext.h>


TEST(UtilsQt, Multicontext_Basic)
{
    bool removed {false};

    auto obj1 = new QObject();
    auto obj2 = new QObject();
    auto obj3 = new QObject();

    auto mc = UtilsQt::createMulticontext({obj1, obj2, obj3});
    QObject::connect(mc, &QObject::destroyed, [&removed](){ removed = true; });

    EXPECT_FALSE(removed);
    delete obj1;
    EXPECT_TRUE(removed);

    delete obj2;
    delete obj3;
}
