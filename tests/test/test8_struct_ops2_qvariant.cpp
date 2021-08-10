#include <gtest/gtest.h>
#include <utils-qt/struct_ops2_qvariant.h>
#include <utils-cpp/struct_ops2.h>

namespace {
struct Sub2
{
    int a{0}, b{0}, c{0};

    bool operator== (const Sub2& rhs) const { return (a == rhs.a) && (b == rhs.b) && (c == rhs.c); }
    bool operator!= (const Sub2& rhs) const { return !(*this == rhs); }
};

struct Sub
{
    int a{0};
    Sub2 b;

    auto tie() const { return std::tie(a, b); }
    auto tieMod() { return std::tie(a, b); }
    auto names() const { return std::make_tuple("a", "b"); }
};
} // namespace

Q_DECLARE_METATYPE(Sub2);

STRUCT_COMPARISONS2_ONLY_EQ(Sub);
STRUCT_QVARIANT_LIST(Sub);
STRUCT_QVARIANT_MAP(Sub);

namespace {
struct Struct1
{
    int a { 0 };
    float b { 0 };
    QString c;
    Sub d;

    auto tie() const { return std::tie(a, b, c, d); }
    auto tieMod() { return std::tie(a, b, c, d); }
    auto names() const { return std::make_tuple("a", "b", "c", "d"); }
};

STRUCT_QVARIANT_LIST(Struct1);
STRUCT_QVARIANT_MAP(Struct1);
STRUCT_COMPARISONS2_ONLY_EQ(Struct1);
} // namespace


TEST(UtilsQt, StructOps2_qvariant)
{
    Struct1 a {1, 2.3, "3", {11, {111, 222, 333}}};
    auto resultList = toQVariantList(a);
    auto resultMap = toQVariantMap(a);

    ASSERT_EQ(resultList.toList().at(0).toInt(), 1);
    ASSERT_NEAR(resultList.toList().at(1).toFloat(), 2.3, 0.1);
    ASSERT_EQ(resultList.toList().at(2).toString(), "3");
    ASSERT_EQ(resultList.toList().at(3).toList().at(0).toInt(), 11);
    ASSERT_EQ(resultList.toList().at(3).toList().at(1), QVariant::fromValue(Sub2{111, 222, 333}));

    ASSERT_EQ(resultMap.toMap().value("a").toInt(), 1);
    ASSERT_NEAR(resultMap.toMap().value("b").toFloat(), 2.3, 0.1);
    ASSERT_EQ(resultMap.toMap().value("c").toString(), "3");
    ASSERT_EQ(resultMap.toMap().value("d").toMap().value("a").toInt(), 11);
    ASSERT_EQ(resultMap.toMap().value("d").toMap().value("b"), QVariant::fromValue(Sub2{111, 222, 333}));

    Struct1 b;
    ASSERT_NE(a, b);

    fromQVariantList(resultList, b);
    ASSERT_EQ(a, b);

    Struct1 c;
    ASSERT_NE(a, c);

    fromQVariantMap(resultMap, c);
    ASSERT_EQ(a, c);
}
