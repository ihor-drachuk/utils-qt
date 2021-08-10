#include <gtest/gtest.h>
#include <utils-qt/struct_ops2_qhash.h>

namespace {

struct Struct1
{
    int a = 1;
    auto tie() const { return std::tie(a); }
};

struct Struct2
{
    int a = 1;
    int b = 0;
    auto tie() const { return std::tie(a, b); }
};

struct Struct3
{
    int a = 0;
    int b = 1;
    auto tie() const { return std::tie(a, b); }
};

struct Struct4
{
    auto tie() const { return std::tie(); }
};

} // namespace

STRUCT_QHASH(Struct1);
STRUCT_QHASH(Struct2);
STRUCT_QHASH(Struct3);
STRUCT_QHASH(Struct4);

TEST(UtilsQt, StructOps2_qhash)
{
    Struct1 s1;
    Struct2 s2;
    Struct3 s3;

    auto hash1 = qHash(s1);
    auto hash2 = qHash(s2);
    auto hash3 = qHash(s3);

    ASSERT_EQ(hash1, qHash(s1));
    ASSERT_EQ(hash2, qHash(s2));
    ASSERT_EQ(hash3, qHash(s3));

    ASSERT_NE(hash1, hash2);
    ASSERT_NE(hash1, hash3);
    ASSERT_NE(hash2, hash3);

    auto hash1s = qHash(s1, 1);
    auto hash2s = qHash(s2, 1);
    auto hash3s = qHash(s3, 1);

    ASSERT_EQ(hash1s, qHash(s1, 1));
    ASSERT_EQ(hash2s, qHash(s2, 1));
    ASSERT_EQ(hash3s, qHash(s3, 1));

    ASSERT_NE(hash1s, hash2s);
    ASSERT_NE(hash1s, hash3s);
    ASSERT_NE(hash2s, hash3s);

    ASSERT_NE(hash1s, hash1);
    ASSERT_NE(hash2s, hash2);
    ASSERT_NE(hash3s, hash3);
}

TEST(UtilsQt, StructOps2_qhash_empty)
{
    Struct4 s;
    auto hash1 = qHash(s);
    auto hash2 = qHash(s, 1);

    ASSERT_EQ(hash1, qHash(s));
    ASSERT_EQ(hash2, qHash(s, 1));

    ASSERT_NE(hash1, hash2);
}
