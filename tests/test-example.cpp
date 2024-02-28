#include "gtest/gtest.h"

namespace
{
	TEST(test_example, basic_check)
	{
		int value = 3 + 1;
		int expected = 4;
		ASSERT_EQ(value, expected);
	}
}

