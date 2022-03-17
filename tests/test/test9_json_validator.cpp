#include <gtest/gtest.h>
#include <utils-qt/jsonvalidator.h>

#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>

TEST(UtilsQt, JsonValidator_basic)
{
    using namespace UtilsQt::JsonValidator;

    auto validator =
            Object(
                Field("field1", Optional),
                Field("field2", Optional, String()),
                Field("field3", String())
                );

    QJsonObject test;
    //test["field1"] = 0;
    test["field2"] = "str";
    test["field3"] = "str";

    NullLogger lg1;
    auto rsl = validator->check(lg1, "", test);
    ASSERT_TRUE(rsl);
    ASSERT_FALSE(lg1.hasNotifiedError());

    test["field1"] = 0;
    test.remove("field2");
    rsl = validator->check(lg1, "", test);
    ASSERT_TRUE(rsl);
    ASSERT_FALSE(lg1.hasNotifiedError());

    NullLogger lg2;
    test["field2"] = 25;
    rsl = validator->check(lg2, "", test);
    ASSERT_FALSE(rsl);
    ASSERT_TRUE(lg2.hasNotifiedError());

    NullLogger lg3;
    test["field2"] = "str";
    test.remove("field3");
    rsl = validator->check(lg3, "", test);
    ASSERT_FALSE(rsl);
    ASSERT_TRUE(lg3.hasNotifiedError());
}

TEST(UtilsQt, JsonValidator_array)
{
    using namespace UtilsQt::JsonValidator;

    auto validator =
            Object(
                Field("field1", String()),
                Field("values",
                      Array(
                          String()
                          )
                      )
                );

    QJsonObject test;
    test["field1"] = "str";

    QJsonArray array;
    array.append("s1");
    array.append("s2");
    array.append("s3");

    QJsonArray array2;
    array2.append("s1");
    array2.append(22);
    array2.append("s3");

    test["values"] = array;

    NullLogger lg;
    auto rsl = validator->check(lg, "", test);
    ASSERT_TRUE(rsl);

    test["values"] = array2;
    rsl = validator->check(lg, "", test);
    ASSERT_FALSE(rsl);
}


TEST(UtilsQt, JsonValidator_or)
{
    using namespace UtilsQt::JsonValidator;

    auto validator =
            Object(
                Field("field1",
                      Or(
                          String(),
                          Object()
                          )
                      ),
                Field("field2",
                      Or(
                          Object(),
                          Array()
                          )
                      )
                );

    QJsonObject test;
    test["field1"] = "str";
    test["field2"] = QJsonArray();

    NullLogger lg;
    auto rsl = validator->check(lg, "", test);
    ASSERT_TRUE(rsl);
    ASSERT_FALSE(lg.hasNotifiedError());

    test["field2"] = 12;
    rsl = validator->check(lg, "", test);
    ASSERT_FALSE(rsl);
    ASSERT_TRUE(lg.hasNotifiedError());
}

TEST(UtilsQt, JsonValidator_include)
{
    using namespace UtilsQt::JsonValidator;

    auto validator =
            Array(
                Include(1,3,5,"",true)
                );

    QJsonArray test;
    test.append(1);
    test.append(3);
    test.append(5);
    test.append("");
    test.append(true);

    NullLogger lg;
    auto rsl = validator->check(lg, "", test);
    ASSERT_TRUE(rsl);
    ASSERT_FALSE(lg.hasNotifiedError());

    test.append(2);
    rsl = validator->check(lg, "", test);
    ASSERT_FALSE(rsl);
    ASSERT_TRUE(lg.hasNotifiedError());
}

TEST(UtilsQt, JsonValidator_exclude)
{
    using namespace UtilsQt::JsonValidator;

    auto validator =
            Array(
                Exclude(1,3,5,"",true)
                );

    QJsonArray test;
    test.append(2);
    test.append(4);
    test.append("a");
    test.append(false);

    NullLogger lg;
    auto rsl = validator->check(lg, "", test);
    ASSERT_TRUE(rsl);
    ASSERT_FALSE(lg.hasNotifiedError());

    test.append(1);
    rsl = validator->check(lg, "", test);
    ASSERT_FALSE(rsl);
    ASSERT_TRUE(lg.hasNotifiedError());
}


TEST(UtilsQt, JsonValidator_severalRules)
{
    using namespace UtilsQt::JsonValidator;

    auto validator =
            Array(
                Or(
                    String(),
                    Include(1)
                ),
                Exclude("hello")
            );

    QJsonArray test;
    test.append("str1");
    test.append("str2");
    test.append("str3");
    test.append("");
    test.append(1);

    NullLogger lg;
    auto rsl = validator->check(lg, "", test);
    ASSERT_TRUE(rsl);
    ASSERT_FALSE(lg.hasNotifiedError());

    test.append("hello");
    rsl = validator->check(lg, "", test);
    ASSERT_FALSE(rsl);
    ASSERT_TRUE(lg.hasNotifiedError());
}
