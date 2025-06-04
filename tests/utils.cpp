#include "lib/scheduler.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(UtilsTest, any_type_error) {
    AnyType val;

    ASSERT_ANY_THROW(val.type());
    ASSERT_ANY_THROW(val.clone());
}

TEST(UtilsTest, any_cast_error) {
    AnyType val = 10;

    ASSERT_ANY_THROW(any_cast<std::string>(val));
}

TEST(UtilsTest, any_type_base_case) {
    AnyType val = 10;

    ASSERT_THAT(any_cast<int>(val), 10);

    val = std::string("string");

    ASSERT_THAT(any_cast<std::string>(val), "string");
}
