#include "gtest/gtest.h"
#include "heap_storage.h"
#include "storage_engine.h"

namespace
{
    TEST(sample_test_case, sample_test)
    {
        EXPECT_EQ(1, 1);
    }

    TEST(make_slotted_page, does_init_work)
    {
        std::string text = "Hello";
        void *data = (void *)(&text);
        Dbt dbt(data, (u_int32_t)(sizeof(data)));
        SlottedPage(dbt, 1);
    }

    TEST(make_heap_file, does_init_work)
    {
        std::string text = "my_heapfile";
        HeapFile file(text);
    }

    TEST(make_heap_table, does_init_work)
    {
        Identifier identifier = "my_identifier";
        ColumnNames column_names{"a", "b"};
        ColumnAttributes column_attribs;
        ColumnAttribute column_attrib(ColumnAttribute::INT);
        column_attribs.push_back(column_attrib);
        column_attrib.set_data_type(ColumnAttribute::TEXT);
        column_attribs.push_back(column_attrib);
        HeapTable(identifier, column_names, column_attribs);
    }
}
