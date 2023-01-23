#include <gtest/gtest.h>
#include <UtilsQt/Qml-Cpp/Geometry/Geometry.h>


TEST(UtilsQt, Geomtry)
{
    QPolygonF rect({
                  {0, 0},
                  {1, 0},
                  {1, 1},
                  {0, 1}
              });

    QPolygonF nonRect({
                  {0, 0},
                  {1, 0},
                  {1, 1},
                  {0, 1.01}
              });

    ASSERT_TRUE(Geometry::instance().isPolygonRectangular(rect));
    ASSERT_FALSE(Geometry::instance().isPolygonRectangular(nonRect));
}
