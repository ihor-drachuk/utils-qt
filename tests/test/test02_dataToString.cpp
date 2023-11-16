/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <gtest/gtest.h>
#include <UtilsQt/datatostring.h>


TEST(UtilsQt, DataToString_Empty)
{
    auto str = dataToString(nullptr, 0);
    ASSERT_EQ(str, "");
}

TEST(UtilsQt, DataToString_String)
{
    auto data = QString( "Test").toLatin1();
    auto str = dataToString(data.data(), data.size());
    ASSERT_EQ(str, "Test");
}

TEST(UtilsQt, DataToString_Data)
{
    QByteArray data(5, Qt::Uninitialized);
    data[0] = 1;
    data[1] = 2;
    data[2] = 0;
    data[3] = 3;
    *(unsigned char*)(data.data()+4) = 255;

    auto str = dataToString(data.data(), data.size());
    ASSERT_EQ(str, QStringLiteral("<01 02 00 03 FF>"));
}

TEST(UtilsQt, DataToString_Mixed)
{
    std::string someData = "1234My567Data12";
    someData[0] = 1;
    someData[1] = 2;
    someData[2] = 3;
    someData[3] = 4;
    someData[6] = 5;
    someData[7] = 6;
    someData[8] = 7;
    someData[13] = 8;
    someData[14] = 9;
    auto str = dataToString(someData.data(), someData.size());
    ASSERT_EQ(str, QStringLiteral("<01 02 03 04>My<05 06 07>Data<08 09>"));
}

TEST(UtilsQt, DataToString_Border1)
{
    std::string someData = "1MyData2";
    someData[0] = 1;
    someData[someData.size()-1] = 0;
    auto str = dataToString(someData.data(), someData.size());
    ASSERT_EQ(str, QStringLiteral("<01>MyData<00>"));
}

