#include "gtest/gtest.h"
#include "heap_storage.h"
namespace
{
    TEST(sample_test_case, sample_test)
    {
        EXPECT_EQ(1, 1);
    }

    TEST(make_slotted_page, does_init_work) 
    {
        std::string text = "Hello";
        void *data = (void*)(&text);
        Dbt dbt(data, (u_int32_t)(sizeof(data)));
        SlottedPage(dbt, 1);
    }
}
