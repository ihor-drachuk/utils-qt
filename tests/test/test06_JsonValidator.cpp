/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <gtest/gtest.h>
#include <UtilsQt/JsonValidator.h>

#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QList>

TEST(UtilsQt, JsonValidator_basic)
{
    using namespace UtilsQt::JsonValidator;

    auto validator =
          RootValidator(
            Object(
              Field("field1", Optional),
              Field("field2", Optional, String()),
              Field("field3", String())
            )
          );

    QJsonObject test;
    //test["field1"] = 0;
    test["field2"] = "str";
    test["field3"] = "str";

    ErrorInfo lg1;
    auto rsl = validator->check(lg1, test);
    ASSERT_TRUE(rsl);
    ASSERT_FALSE(lg1.hasError());

    test["field1"] = 0;
    test.remove("field2");
    rsl = validator->check(lg1, test);
    ASSERT_TRUE(rsl);
    ASSERT_FALSE(lg1.hasError());

    ErrorInfo lg2;
    test["field2"] = 25;
    rsl = validator->check(lg2, test);
    ASSERT_FALSE(rsl);
    ASSERT_TRUE(lg2.hasError());

    ErrorInfo lg3;
    test["field2"] = "str";
    test.remove("field3");
    rsl = validator->check(lg3, test);
    ASSERT_FALSE(rsl);
    ASSERT_TRUE(lg3.hasError());
}


TEST(UtilsQt, JsonValidator_string)
{
    using namespace UtilsQt::JsonValidator;

    auto validator =
        RootValidator(
          Object(
              Field("field1", String()),
              Field("field2", String(NonEmpty)),
              Field("field3", String(Hex)),
              Field("field4", String(Base64)),
              Field("field5", String(IPv4))
            )
          );

    ErrorInfo lg;
    const QJsonObject original {
        {"field1", "str"},
        {"field2", "str"},
        {"field3", "0x1a2b3c"},
        {"field4", "dGVzdA=="},
        {"field5", "127.0.12.255"}
    };

    QJsonObject test = original;
    ASSERT_TRUE(validator->check(lg, test));

    test = original;
    test["field1"] = "";
    ASSERT_TRUE(validator->check(lg, test));

    test = original;
    test["field2"] = "";
    ASSERT_FALSE(validator->check(lg, test));

    test = original;
    test["field3"] = "0x1a2b3";
    ASSERT_FALSE(validator->check(lg, test));

    test = original;
    test["field4"] = "dGVzdA=";
    ASSERT_FALSE(validator->check(lg, test));

    test = original;
    test["field5"] = "127.0.12.256";
    ASSERT_FALSE(validator->check(lg, test));
}

TEST(UtilsQt, JsonValidator_number)
{
    using namespace UtilsQt::JsonValidator;

    auto validator =
          RootValidator(
            Object(
              Field("field_double", Number()),
              Field("field_int", Number(Integer)),
              Field("field_double_1_11__6_15", Number({1.11}, {6.15})),
              Field("field_int_2__7", Number(Integer, {2}, {7}))
            )
          );

    ErrorInfo lg;
    QJsonObject test;
    test["field_double"] = 1.1;
    test["field_int"] = 2;
    test["field_double_1_11__6_15"] = 2.51;
    test["field_int_2__7"] = 3;
    ASSERT_TRUE(validator->check(lg, test));

    test["field_double_1_11__6_15"] = 1.11;
    test["field_int_2__7"] = 2;
    ASSERT_TRUE(validator->check(lg, test));

    test["field_double_1_11__6_15"] = 6.15;
    test["field_int_2__7"] = 7;
    ASSERT_TRUE(validator->check(lg, test));

    test["field_double"] = 1;
    ASSERT_TRUE(validator->check(lg, test));

    ASSERT_TRUE(validator->check(lg, test));
    test["field_int"] = 2.1;
    ASSERT_FALSE(validator->check(lg, test));
    test["field_int"] = 2;

    ASSERT_TRUE(validator->check(lg, test));
    test["field_double_1_11__6_15"] = 1.10;
    ASSERT_FALSE(validator->check(lg, test));
    test["field_double_1_11__6_15"] = 2.51;

    ASSERT_TRUE(validator->check(lg, test));
    test["field_int_2__7"] = 1;
    ASSERT_FALSE(validator->check(lg, test));
    test["field_int_2__7"] = 3;

    ASSERT_TRUE(validator->check(lg, test));
    test["field_int_2__7"] = 3.5;
    ASSERT_FALSE(validator->check(lg, test));
    test["field_int_2__7"] = 3;
}

TEST(UtilsQt, JsonValidator_number_64bit)
{
    using namespace UtilsQt::JsonValidator;

    // Test with 64-bit integer ranges that exceed 32-bit limits
    const int64_t largeMin = 5000000000LL;  // > INT32_MAX (2147483647)
    const int64_t largeMax = 9000000000LL;  // Much larger than INT32_MAX

    auto validator =
          RootValidator(
            Object(
              Field("field_large_int", Number(Integer, {largeMin}, {largeMax})),
              Field("field_large_int_min", Number(Integer, {largeMin}, {})),
              Field("field_large_int_max", Number(Integer, {}, {largeMax}))
            )
          );

    ErrorInfo lg;

    // Test valid large integers using JSON parsing
    const QString validJson = R"({
        "field_large_int": 7000000000,
        "field_large_int_min": 6000000000,
        "field_large_int_max": 8000000000
    })";

    auto validDoc = QJsonDocument::fromJson(validJson.toUtf8());
    auto validTest = validDoc.object();
    ASSERT_TRUE(validator->check(lg, validTest));

    // Test boundary values using JSON parsing
    const QString boundaryJson = R"({
        "field_large_int": 5000000000,
        "field_large_int_min": 5000000000,
        "field_large_int_max": 9000000000
    })";

    auto boundaryDoc = QJsonDocument::fromJson(boundaryJson.toUtf8());
    auto boundaryTest = boundaryDoc.object();
    ASSERT_TRUE(validator->check(lg, boundaryTest));

    // Test values below minimum using JSON parsing
    const QString belowMinJson = R"({
        "field_large_int": 4999999999,
        "field_large_int_min": 6000000000,
        "field_large_int_max": 8000000000
    })";

    auto belowMinDoc = QJsonDocument::fromJson(belowMinJson.toUtf8());
    auto belowMinTest = belowMinDoc.object();
    ASSERT_FALSE(validator->check(lg, belowMinTest));

    // Test values above maximum using JSON parsing
    const QString aboveMaxJson = R"({
        "field_large_int": 9000000001,
        "field_large_int_min": 6000000000,
        "field_large_int_max": 8000000000
    })";

    auto aboveMaxDoc = QJsonDocument::fromJson(aboveMaxJson.toUtf8());
    auto aboveMaxTest = aboveMaxDoc.object();
    ASSERT_FALSE(validator->check(lg, aboveMaxTest));

    // Test that floating point values still fail for integer type
    const QString floatJson = R"({
        "field_large_int": 7000000000.5,
        "field_large_int_min": 6000000000,
        "field_large_int_max": 8000000000
    })";

    auto floatDoc = QJsonDocument::fromJson(floatJson.toUtf8());
    auto floatTest = floatDoc.object();
    ASSERT_FALSE(validator->check(lg, floatTest));
}

TEST(UtilsQt, JsonValidator_array)
{
    using namespace UtilsQt::JsonValidator;

    auto validator =
            RootValidator(
              Object(
                Field("field1", String()),
                Field("values",
                  Array(
                    String()
                  )
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

    ErrorInfo lg;
    auto rsl = validator->check(lg, test);
    ASSERT_TRUE(rsl);

    test["values"] = array2;
    rsl = validator->check(lg, test);
    ASSERT_FALSE(rsl);
}


TEST(UtilsQt, JsonValidator_or)
{
    using namespace UtilsQt::JsonValidator;

    auto validator =
            RootValidator(
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
              )
            );

    QJsonObject test;
    test["field1"] = "str";
    test["field2"] = QJsonArray();

    ErrorInfo lg;
    auto rsl = validator->check(lg, test);
    ASSERT_TRUE(rsl);
    ASSERT_FALSE(lg.hasError());

    test["field2"] = 12;
    rsl = validator->check(lg, test);
    ASSERT_FALSE(rsl);
    ASSERT_TRUE(lg.hasError());
}

TEST(UtilsQt, JsonValidator_or_exclusive)
{
    using namespace UtilsQt::JsonValidator;

    auto validator =
            RootValidator(
              Object(
                Or(Exclusive,
                  Field("one"),
                  Field("two")
                )
              )
            );

    QList<QJsonObject> successCases {
        QJsonObject({{"one", "1"}}),
        QJsonObject({{"two", "2"}})
    };

    QList<QJsonObject> failCases {
        QJsonObject({{"one", "1"}, {"two", "2"}}),
        QJsonObject()
    };

    for (const auto& x : successCases) {
        ErrorInfo lg;
        auto rsl = validator->check(lg, x);
        ASSERT_TRUE(rsl);
        ASSERT_FALSE(lg.hasError());
    }

    for (const auto& x : failCases) {
        ErrorInfo lg;
        auto rsl = validator->check(lg, x);
        ASSERT_FALSE(rsl);
        ASSERT_TRUE(lg.hasError());
    }
}

TEST(UtilsQt, JsonValidator_include)
{
    using namespace UtilsQt::JsonValidator;

    auto validator =
            RootValidator(
              Array(
                Include(1,3,5,"",true)
              )
            );

    QJsonArray test;
    test.append(1);
    test.append(3);
    test.append(5);
    test.append("");
    test.append(true);

    ErrorInfo lg;
    auto rsl = validator->check(lg, test);
    ASSERT_TRUE(rsl);
    ASSERT_FALSE(lg.hasError());

    test.append(2);
    rsl = validator->check(lg, test);
    ASSERT_FALSE(rsl);
    ASSERT_TRUE(lg.hasError());
}

TEST(UtilsQt, JsonValidator_exclude)
{
    using namespace UtilsQt::JsonValidator;

    auto validator =
            RootValidator(
              Array(
                Exclude(1,3,5,"",true)
              )
            );

    QJsonArray test;
    test.append(2);
    test.append(4);
    test.append("a");
    test.append(false);

    ErrorInfo lg;
    auto rsl = validator->check(lg, test);
    ASSERT_TRUE(rsl);
    ASSERT_FALSE(lg.hasError());

    test.append(1);
    rsl = validator->check(lg, test);
    ASSERT_FALSE(rsl);
    ASSERT_TRUE(lg.hasError());
}


TEST(UtilsQt, JsonValidator_severalRules)
{
    using namespace UtilsQt::JsonValidator;

    auto validator =
            RootValidator(
              Array(
                Or(
                  String(),
                  Include(1)
                ),
                Exclude("hello")
              )
            );

    QJsonArray test;
    test.append("str1");
    test.append("str2");
    test.append("str3");
    test.append("");
    test.append(1);

    ErrorInfo lg;
    auto rsl = validator->check(lg, test);
    ASSERT_TRUE(rsl);
    ASSERT_FALSE(lg.hasError());

    test.append("hello");
    rsl = validator->check(lg, test);
    ASSERT_FALSE(rsl);
    ASSERT_TRUE(lg.hasError());
}


TEST(UtilsQt, JsonValidator_CtxCheckArrayLength)
{
    using namespace UtilsQt::JsonValidator;

    auto validator =
            RootValidator(
              Object(
                Field("array1", CtxWriteArrayLength("array1-size")),
                Field("array2", CtxCheckArrayLength("array1-size"))
              ),
              CtxClearRecord("array1-size")
            );

    QJsonObject obj;
    obj["array1"] = QJsonArray{ 1,  2,  3,  4};
    obj["array2"] = QJsonArray{"a","b","c","d"};

    ErrorInfo lg;
    auto rsl = validator->check(lg, obj);
    ASSERT_TRUE(rsl);
    ASSERT_FALSE(lg.hasError());

    obj["array2"] = QJsonArray{1,2,3};
    rsl = validator->check(lg, obj);
    ASSERT_FALSE(rsl);
    ASSERT_TRUE(lg.hasError());
}

TEST(UtilsQt, JsonValidator_CtxCheckValue)
{
    using namespace UtilsQt::JsonValidator;

    auto validator =
            RootValidator(
              Object(
                Field("array1", Array(CtxAppendToList("array1-values"))),
                Field("some-data",
                  Array(
                    Object(
                      Field("field1", Number()),
                      Field("reference", CtxCheckInList("array1-values"))
                    )
                  )
                )
              ),
              CtxClearRecord("array1-values")
            );

    QJsonObject obj;
    obj["array1"] = QJsonArray{ 1,  2,  3,  4, "Hello"};
    obj["some-data"] = QJsonArray{
        QJsonObject{
            {"field1", 1},
            {"reference", 3}
        },
        QJsonObject{
            {"field1", 2},
            {"reference", "Hello"}
        },
    };

    QJsonObject obj2;
    obj2["array1"] = QJsonArray{ 1,  2,  3,  4, "Hello"};
    obj2["some-data"] = QJsonArray{
        QJsonObject{
            {"field1", 1},
            {"reference", 3}
        },
        QJsonObject{
            {"field1", 2},
            {"reference", "str"}
        },
    };

    ErrorInfo lg;
    auto rsl = validator->check(lg, obj);
    ASSERT_TRUE(rsl);
    ASSERT_FALSE(lg.hasError());

    rsl = validator->check(lg, obj2);
    ASSERT_FALSE(rsl);
    ASSERT_TRUE(lg.hasError());
}

TEST(UtilsQt, JsonValidator_ArrayLength)
{
    using namespace UtilsQt::JsonValidator;

    auto validatorOk1 =
            RootValidator(
              Object(
                Field("array1_3", Array(ArrayLength(1,3))),
                Field("array2_", Array(ArrayLength(2, {}))),
                Field("array_3", Array(ArrayLength({},3)))
              )
            );

    auto validatorWrong1 =
        RootValidator(
          Object(
            Field("array1_3", Array(ArrayLength(2,3)))
          )
        );

    auto validatorWrong2 =
        RootValidator(
          Object(
            Field("array2_", Array(ArrayLength(3, {})))
          )
        );

    auto validatorWrong3 =
        RootValidator(
          Object(
            Field("array_3", Array(ArrayLength({},2)))
          )
        );

    auto validatorOk2 =
        RootValidator(
          Object(
            Field("array1", ArrayLength(1)),
            Field("array2", ArrayLength(2)),
            Field("array3", ArrayLength(3))
          )
        );

    QJsonObject obj;
    QJsonArray arrays1_3 {
        QJsonArray{1},
        QJsonArray{1, 2},
        QJsonArray{1, 2, "3"},
    };

    QJsonArray arrays2_ {
        QJsonArray{1, 2},
        QJsonArray{1, 2, "3"},
    };

    QJsonArray arrays_3 {
        QJsonArray{1},
        QJsonArray{1, 2},
        QJsonArray{1, 2, "3"},
    };
    obj["array1_3"] = arrays1_3;
    obj["array2_"] = arrays2_;
    obj["array_3"] = arrays_3;

    ErrorInfo lg;
    auto rsl = validatorOk1->check(lg, obj);
    ASSERT_TRUE(rsl);
    ASSERT_FALSE(lg.hasError());
    lg.clear();

    rsl = validatorWrong1->check(lg, obj);
    ASSERT_FALSE(rsl);
    ASSERT_TRUE(lg.hasError());
    lg.clear();

    rsl = validatorWrong2->check(lg, obj);
    ASSERT_FALSE(rsl);
    ASSERT_TRUE(lg.hasError());
    lg.clear();

    rsl = validatorWrong3->check(lg, obj);
    ASSERT_FALSE(rsl);
    ASSERT_TRUE(lg.hasError());
    lg.clear();

    QJsonObject obj2;
    obj2["array1"] = QJsonArray{1};
    obj2["array2"] = QJsonArray{1,2};
    obj2["array3"] = QJsonArray{1,2,3};
    rsl = validatorOk2->check(lg, obj2);
    ASSERT_TRUE(rsl);
    ASSERT_FALSE(lg.hasError());
    lg.clear();
}


TEST(UtilsQt, JsonValidator_CustomValidator)
{
    using namespace UtilsQt::JsonValidator;

    auto validator =
        RootValidator(
          Object(
            Field("field1", CustomValidator([](const QJsonValue& val) {
                return val.isString() && val.toString().toInt() > 5;
            }))
          )
        );

    QJsonObject obj;
    obj["field1"] = "6";

    ErrorInfo lg;
    auto rsl = validator->check(lg, obj);
    ASSERT_TRUE(rsl);
    ASSERT_FALSE(lg.hasError());

    obj["field1"] = 6;
    rsl = validator->check(lg, obj);
    ASSERT_FALSE(rsl);
    ASSERT_TRUE(lg.hasError());

    obj["field1"] = "5";
    rsl = validator->check(lg, obj);
    ASSERT_FALSE(rsl);
    ASSERT_TRUE(lg.hasError());
}
