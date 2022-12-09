#include <gtest/gtest.h>

#include <UtilsQt/Qml-Cpp/QmlUtils.h>

namespace {

struct DataPack_Regex
{
    QString pattern;
    QString string;
    QString result;
};

struct DataPack_RegexGroups
{
    QString pattern;
    QString string;
    QList<int> groups;
    QStringList result;
};

struct DataPack_ColorAccent
{
    QColor color;
    double factor {};
};


class QmlUtils_Regex : public testing::TestWithParam<DataPack_Regex>
{ };


class QmlUtils_RegexGroups : public testing::TestWithParam<DataPack_RegexGroups>
{ };

class QmlUtils_ColorAccent : public testing::TestWithParam<DataPack_ColorAccent>
{ };


TEST_P(QmlUtils_Regex, Test)
{
    auto dataPack = GetParam();
    auto result = QmlUtils::instance().extractByRegex(dataPack.string, dataPack.pattern);
    ASSERT_EQ(result, dataPack.result);
}


TEST_P(QmlUtils_RegexGroups, Test)
{
    auto dataPack = GetParam();
    auto result = QmlUtils::instance().extractByRegexGroups(dataPack.string, dataPack.pattern, dataPack.groups);
    ASSERT_EQ(result, dataPack.result);
}


TEST_P(QmlUtils_ColorAccent, Test)
{
    auto dataPack = GetParam();
    const auto color1 = dataPack.color;
    const auto color2 = QmlUtils::instance().colorMakeAccent(dataPack.color, dataPack.factor);
    ASSERT_NE(color1, color2);
}

} // namespace

INSTANTIATE_TEST_SUITE_P(
    Test,
    QmlUtils_Regex,
    testing::Values(
        DataPack_Regex{"\\d\\d", "abc1g57ty", "57"},
        DataPack_Regex{"\\d\\d", "00", "00"},
        DataPack_Regex{"\\d\\d", "1 2 3", ""}
    )
);

INSTANTIATE_TEST_SUITE_P(
    Test,
    QmlUtils_RegexGroups,
    testing::Values(
        DataPack_RegexGroups{"(\\d+)\\.(\\d+)\\.(\\d+)", "1.2.3", {0, 1, 2, 3}, {"1.2.3", "1", "2", "3"}},
        DataPack_RegexGroups{"(\\d+)\\.(\\d+)\\.(\\d+)", "1.2.3", {1, 1, 2}, {"1", "1", "2"}},
        DataPack_RegexGroups{"(\\d+)\\.(\\d+)\\.(\\d+)", "1.2.3", {1}, {"1"}},
        DataPack_RegexGroups{"(\\d+)\\.(\\d+)\\.(\\d+)", "1.2.3", {10}, {}},
        DataPack_RegexGroups{"(\\d+)\\.(\\d+)\\.(\\d+)", "1a.2a.3a", {1}, {}}
    )
);

INSTANTIATE_TEST_SUITE_P(
    Test,
    QmlUtils_ColorAccent,
    testing::Values(
        DataPack_ColorAccent{Qt::GlobalColor::red, 1.2},
        DataPack_ColorAccent{QColor::fromRgbF(0.1, 1, 0.7, 0.5), 1.2},
        DataPack_ColorAccent{QColor::fromRgbF(0.1, 0.1, 0.1, 0.5), 1.2},
        DataPack_ColorAccent{QColor::fromRgbF(0.1, 0.1, 0.1, 0.1), 1.2}
    )
);
