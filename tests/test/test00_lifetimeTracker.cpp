/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <gtest/gtest.h>

#include "internal/LifetimeTracker.h"


TEST(UtilsQt, Internal_LifetimeTracker)
{
    LifetimeData data;
    ASSERT_EQ(data.count(), 0);

    {
        LifetimeTracker tracker;

        data = tracker.getData();
        ASSERT_EQ(data.count(), 1);
        ASSERT_EQ(tracker.getData().count(), 1);

        const auto a = tracker;
        ASSERT_EQ(data.count(), 2);
        ASSERT_EQ(tracker.getData().count(), 2);
        {
            auto d = [tracker](){ (void)tracker; };
            ASSERT_EQ(tracker.getData().count(), 3);
            d();
            ASSERT_EQ(tracker.getData().count(), 3);
        }
        ASSERT_EQ(tracker.getData().count(), 2);
    }

    ASSERT_EQ(data.count(), 0);
}
