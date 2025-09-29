/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <gtest/gtest.h>

#include <QVector>
#include <QQmlEngine>
#include <QJSEngine>
#include <QJSValue>
#include <QEventLoop>
#include <QTimer>
#include <QSignalSpy>
#include <UtilsQt/Qml-Cpp/QmlUtils.h>

namespace {

QVector<double> extractColorComponents(const QColor& color)
{
    switch (color.spec()) {
        case QColor::Spec::Invalid: return {};
        case QColor::Spec::Rgb:     return {color.redF(),    color.greenF(),         color.blueF(),      color.alphaF()};
        case QColor::Spec::Hsv:     return {color.hsvHueF(), color.hsvSaturationF(), color.valueF(),     color.alphaF()};
        case QColor::Spec::Cmyk:    return {color.cyanF(),   color.magentaF(),       color.yellowF(),    color.blackF(),   color.alphaF()};
        case QColor::Spec::Hsl:     return {color.hslHueF(), color.hslSaturationF(), color.lightnessF(), color.alphaF()};
        case QColor::Spec::ExtendedRgb: {
            const auto rgb64 = color.rgba64();
            const double ui16max = std::numeric_limits<decltype(rgb64.alpha())>::max();
            return {
                (double)rgb64.red()   / ui16max,
                (double)rgb64.green() / ui16max,
                (double)rgb64.blue()  / ui16max,
                (double)rgb64.alpha() / ui16max,
            };
        }
    }

    assert(false);
    return {};
}

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

struct DataPack_ColorMakeAccent
{
    QColor color;
    double factor {};
};

struct DataPack_ColorChangeAlpha
{
    QColor color;
    double newAlpha {};
};


class QmlUtils_Regex : public testing::TestWithParam<DataPack_Regex>
{ };

class QmlUtils_RegexGroups : public testing::TestWithParam<DataPack_RegexGroups>
{ };

class QmlUtils_ColorMakeAccent : public testing::TestWithParam<DataPack_ColorMakeAccent>
{ };

class QmlUtils_ColorChangeAlpha : public testing::TestWithParam<DataPack_ColorChangeAlpha>
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


TEST_P(QmlUtils_ColorMakeAccent, Test)
{
    auto dataPack = GetParam();
    const auto color1 = dataPack.color;
    const auto color2 = QmlUtils::instance().colorMakeAccent(dataPack.color, dataPack.factor);
    ASSERT_NE(color1, color2);
}

TEST_P(QmlUtils_ColorChangeAlpha, Test)
{
    auto dataPack = GetParam();
    const auto color1 = dataPack.color;
    const auto color2 = QmlUtils::instance().colorChangeAlpha(dataPack.color, dataPack.newAlpha);
    ASSERT_EQ(color1.spec(), color2.spec());
    ASSERT_TRUE(QmlUtils::instance().doublesEqual(color2.alphaF(), dataPack.newAlpha, 0.0001));

    auto colorComponents1 = extractColorComponents(color1);
    auto colorComponents2 = extractColorComponents(color2);
    colorComponents1.pop_back();
    colorComponents2.pop_back();
    ASSERT_EQ(colorComponents1, colorComponents2);
}

} // namespace

// CallLater tests
TEST(QmlUtils, CallLater_ErrorConditions)
{
    auto& qmlUtils = QmlUtils::instance();

    // Test with non-callable QJSValue
    QJSEngine jsEngine;
    QJSValue nonCallable = jsEngine.newObject();

    // These should not crash and should print warnings
    qmlUtils.callOnce(nonCallable, 100, QmlUtils::CallOnceMode::RestartOnCall);
    qmlUtils.callOnce(QJSValue(), 100, QmlUtils::CallOnceMode::ContinueOnCall);

    // Test with invalid timeout
    QJSValue validFunc = jsEngine.evaluate("(function() { console.log('test'); })");
    qmlUtils.callOnce(validFunc, 0, QmlUtils::CallOnceMode::RestartOnCall);
    qmlUtils.callOnce(validFunc, -100, QmlUtils::CallOnceMode::ContinueOnCall);
}

TEST(QmlUtils, CallLater_BasicValidation)
{
    auto& qmlUtils = QmlUtils::instance();
    QJSEngine jsEngine;

    // Create a callable function
    QJSValue callableFunc = jsEngine.evaluate("(function() { return 42; })");
    ASSERT_TRUE(callableFunc.isCallable());

    // These should not crash (though they may not execute without proper QML engine setup)
    qmlUtils.callOnce(callableFunc, 50, QmlUtils::CallOnceMode::RestartOnCall);
    qmlUtils.callOnce(callableFunc, 100, QmlUtils::CallOnceMode::ContinueOnCall);

    // Wait briefly to ensure no crashes
    QEventLoop loop;
    QTimer::singleShot(10, &loop, &QEventLoop::quit);
    loop.exec();
}

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
    QmlUtils_ColorMakeAccent,
    testing::Values(
        DataPack_ColorMakeAccent{Qt::GlobalColor::red, 0.2},
        DataPack_ColorMakeAccent{QColor::fromRgbF(0.1, 1.0, 0.7, 0.5), 0.5},
        DataPack_ColorMakeAccent{QColor::fromRgbF(0.1, 0.1, 0.1, 0.5), 0.7},
        DataPack_ColorMakeAccent{QColor::fromRgbF(0.1, 0.1, 0.1, 0.1), 1.0}
    )
);

INSTANTIATE_TEST_SUITE_P(
    Test,
    QmlUtils_ColorChangeAlpha,
    testing::Values(
        DataPack_ColorChangeAlpha{Qt::GlobalColor::red, 0.2},
        DataPack_ColorChangeAlpha{QColor::fromRgbF(0.1, 1.0, 0.7, 0.5), 0.11},
        DataPack_ColorChangeAlpha{QColor::fromHsvF(0.1, 0.1, 0.1, 0.5), 0.12},
        DataPack_ColorChangeAlpha{QColor::fromHslF(0.1, 0.1, 0.1, 0.1), 0.13},
        DataPack_ColorChangeAlpha{QColor::fromCmykF(0.1, 0.2, 0.1, 0.1, 0.1), 0.14}
    )
);
