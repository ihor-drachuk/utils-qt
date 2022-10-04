#include <gtest/gtest.h>
#include <UtilsQt/qvariant_conv.h>

using namespace UtilsQt::QVariantConv;

TEST(UtilsQt, QVariantConvTest_Operators)
{
    ASSERT_EQ((Checks)1+2+4 & (Checks)2, (Checks)2);
    ASSERT_EQ((Checks)1+4 | (Checks)2, (Checks)1+2+4);

    Checks a = NoCheck;
    a |= Checks::Check_NonNull_Valid;
    a |= Checks::Check_Type;
    ASSERT_EQ(a, Check_NonNull_Valid | Check_Type);

    a &= Check_Type;
    ASSERT_EQ(a, Check_Type);
}

TEST(UtilsQt, QVariantConvTest_Load)
{
    ASSERT_EQ(load<int>(QVariant(170)).value(), 170);
    ASSERT_EQ(load<unsigned int>(QVariant(170U)).value(), 170);
    ASSERT_EQ(load<long long>(QVariant(170LL)).value(), 170);
    ASSERT_EQ(load<unsigned long long>(QVariant(170ULL)).value(), 170);

    ASSERT_DOUBLE_EQ(load<double>(QVariant((double)170.2)).value(), 170.2);
    ASSERT_FLOAT_EQ(load<float>(QVariant((float)170.2)).value(), 170.2);

    ASSERT_EQ(load<bool>(QVariant(true)).value(), true);
    ASSERT_EQ(load<bool>(QVariant(false)).value(), false);

    ASSERT_EQ(load<char>(QVariant('a')).value(), 'a');
    ASSERT_EQ(load<QChar>(QVariant(QChar('a'))).value(), 'a');
    ASSERT_EQ(load<QString>(QVariant("str")).value(), "str");


    ASSERT_DOUBLE_EQ(load<double>(QVariant("170.2"), ~Check_Type).value(), 170.2);

    ASSERT_EQ(load<QString>(QVariant(), NoCheck).value(), "");
    ASSERT_EQ(load<int>(QVariant(), NoCheck).value(), 0);
}
