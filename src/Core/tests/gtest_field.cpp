#include <gtest/gtest.h>
#include <Core/Field.h>

using namespace DB;

GTEST_TEST(Field, FromBool)
{
    {
        Field f{false};
        ASSERT_EQ(f.getType(), Field::Types::Bool);
        ASSERT_EQ(f.safeGet<UInt64>(), 0);
        ASSERT_EQ(f.safeGet<bool>(), false);
    }

    {
        Field f{true};
        ASSERT_EQ(f.getType(), Field::Types::Bool);
        ASSERT_EQ(f.safeGet<UInt64>(), 1);
        ASSERT_EQ(f.safeGet<bool>(), true);
    }

    {
        Field f;
        f = false;
        ASSERT_EQ(f.getType(), Field::Types::Bool);
        ASSERT_EQ(f.safeGet<UInt64>(), 0);
        ASSERT_EQ(f.safeGet<bool>(), false);
    }

    {
        Field f;
        f = true;
        ASSERT_EQ(f.getType(), Field::Types::Bool);
        ASSERT_EQ(f.safeGet<UInt64>(), 1);
        ASSERT_EQ(f.safeGet<bool>(), true);
    }
}


GTEST_TEST(Field, Move)
{
    Field f;

    f = Field{String{"Hello, world (1)"}};
    ASSERT_EQ(f.safeGet<String>(), "Hello, world (1)");
    f = Field{String{"Hello, world (2)"}};
    ASSERT_EQ(f.safeGet<String>(), "Hello, world (2)");
    f = Field{Array{Field{String{"Hello, world (3)"}}}};
    ASSERT_EQ(f.safeGet<Array>()[0].safeGet<String>(), "Hello, world (3)");
    f = String{"Hello, world (4)"};
    ASSERT_EQ(f.safeGet<String>(), "Hello, world (4)");
    f = Array{Field{String{"Hello, world (5)"}}};
    ASSERT_EQ(f.safeGet<Array>()[0].safeGet<String>(), "Hello, world (5)");
    f = Array{String{"Hello, world (6)"}};
    ASSERT_EQ(f.safeGet<Array>()[0].safeGet<String>(), "Hello, world (6)");
}

GTEST_TEST(Field, MoveBoolFieldBigEndian)
{
    Field f{true};
    ASSERT_EQ(f.getType(), Field::Types::Bool);
    ASSERT_EQ(f.safeGet<bool>(), true);
    ASSERT_EQ(f.safeGet<UInt64>(), 1);

    // On big endian platforms, before the change to properly cast the
    // storage type in Field::assignConcrete, this assignment would not
    // work and f would remain true for both f.safeGet<bool>() and
    // f.safeGet<UInt64>().
    // It happened because Bools are stored in UInt64 storage but because
    // of the improper cast, only the first byte of the storage was modified.
    // On little endian platforms, that happened to work since the first byte
    // holds the least significant bits of the UInt64 storage. But on big
    // endian platforms it only modified the most significant byte and left
    // the UInt64 value in memory unchanged.
    f = Field{false};
    ASSERT_EQ(f.getType(), Field::Types::Bool);
    ASSERT_EQ(f.safeGet<bool>(), false);
    ASSERT_EQ(f.safeGet<UInt64>(), 0);
}
