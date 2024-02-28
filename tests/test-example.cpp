#include <gtest/gtest.h>
#include <lmdb++.h>

namespace
{
	TEST(test_example, basic_check)
	{
		int value = 3 + 1;
		int expected = 4;
		ASSERT_EQ(value, expected);
	}

	TEST(mdb_val, store_and_get_data)
	{	
		char block[4096];
		memset(block, 0, sizeof(block));
		MDB_val mdbt(sizeof(block), block);
		void* ret_data = mdbt.mv_data;
		ASSERT_EQ((char*)block, (char*)ret_data);
	}
}

