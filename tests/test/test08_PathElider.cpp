#include <gtest/gtest.h>

#include <utility>
#include <tuple>

#include <utils-qt/qml-cpp/PathElider.h>

#include <QString>
#include <QFontMetrics>

#ifndef UTILS_QT_NO_GUI_TESTS
namespace {

struct DataPack1
{
    QString sourceValue;
    std::vector<bool> skipDirs;
    QString expRecombinedValue;
};

struct DataPack2
{
    QString sourceValue;
    PathEliderDecomposition expDecomposition;

    float widthFactor {};
    QString expElided;
};

class PathElider_Basic0_Combine : public testing::TestWithParam<DataPack1>
{
public:
    PathElider m_elider;
};

class PathElider_Basic1_Regexp : public testing::TestWithParam<DataPack2>
{
public:
    QFont m_font {QString("Arial")};
};


TEST_P(PathElider_Basic0_Combine, Test)
{
    auto dataPack = GetParam();
    auto recombinedValue = m_elider.decomposePath(dataPack.sourceValue).combine(dataPack.skipDirs);
    ASSERT_EQ(recombinedValue, dataPack.expRecombinedValue);
}

TEST_P(PathElider_Basic1_Regexp, Test)
{
    PathElider elider;
    auto dataPack = GetParam();
    const auto decomposition = elider.decomposePath(dataPack.sourceValue);
    ASSERT_EQ(decomposition, dataPack.expDecomposition);
    ASSERT_EQ(decomposition.combine(), dataPack.sourceValue);

    int sourceWidth = QFontMetrics(m_font).horizontalAdvance(dataPack.sourceValue);

    elider.setFont(m_font);
    elider.setSourceText(dataPack.sourceValue);
    elider.setWidthLimit(sourceWidth * dataPack.widthFactor);
    ASSERT_EQ(elider.elidedText(), dataPack.expElided);
}

INSTANTIATE_TEST_SUITE_P(
    Test,
    PathElider_Basic0_Combine,
    testing::Values(
        DataPack1{":/devices/MX-10/EU/image.png", {0,0,0}, ":/devices/MX-10/EU/image.png"},
        DataPack1{":/devices/MX-10/EU/image.png", {0,1,0}, ":/devices/.../EU/image.png"},
        DataPack1{":/devices/MX-10/EU/image.png", {1,1,0}, ":/.../EU/image.png"},
        DataPack1{":/devices/MX-10/EU/image.png", {1,1,1}, ":/.../image.png"}
    )
);

INSTANTIATE_TEST_SUITE_P(
    Test,
    PathElider_Basic1_Regexp,
    testing::Values(
        DataPack2{":/devices/MX-10/EU/image.png",      {":/",    {"devices", "MX-10", "EU"}, '/',  "image.png"}, 0.99f, ":/devices/.../EU/image.png"},
        DataPack2{"qrc:/devices/MX-10/EU/image.png",   {"qrc:/", {"devices", "MX-10", "EU"}, '/',  "image.png"}, 0.85f, "qrc:/.../EU/image.png"},
        DataPack2{"/devices/MX-10/EU/image.png",       {"/",     {"devices", "MX-10", "EU"}, '/',  "image.png"}, 0.5f,  "/.../image.png"},
        DataPack2{"D:\\devices\\MX-10\\EU\\image.png", {"D:\\",  {"devices", "MX-10", "EU"}, '\\', "image.png"}, 0.45f, "...\\image.png"},
        DataPack2{"D:\\subdir\\image.png",             {"D:\\",  {"subdir"},                 '\\', "image.png"}, 0.55f,  "...age.png"},
        DataPack2{"D:\\image.png",                     {"D:\\",  {},                         {},   "image.png"}, 1.1f,  "D:\\image.png"},
        DataPack2{"image.png",                         {"",      {},                         {},   "image.png"}, 1.1f,  "image.png"},
        DataPack2{"somedir1/somedir2/image.png",       {"",      {"somedir1", "somedir2"},   '/',  "image.png"}, 1.1f,  "somedir1/somedir2/image.png"},
        DataPack2{"somedir1/somedir2/",                {"",      {"somedir1", "somedir2"},   '/',  "",        }, 1.1f,  "somedir1/somedir2/"},
        DataPack2{"somedir1/name",                     {"",      {"somedir1"},               '/',  "name",    }, 1.1f,  "somedir1/name"}
    )
);

} // namespace
#endif // !UTILS_QT_NO_GUI_TESTS
