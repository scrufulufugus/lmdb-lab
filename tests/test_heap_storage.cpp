#include "gtest/gtest.h"
#include "heap_storage.h"
#include "storage_engine.h"
#include <iostream>

namespace
{
    TEST(sample_test_case, sample_test)
    {
        EXPECT_EQ(1, 1);
    }

    // Slotted Page
    TEST(slotted_page, add_data)
    {
        char *new_bytes = new char[DbBlock::BLOCK_SZ];
        Dbt *data = new Dbt(new_bytes, DbBlock::BLOCK_SZ);
        RecordID id = 0;
        SlottedPage *page = new SlottedPage(*data, id, true); // set this to true so page->add wouldn't seg fault

        // data marshalling
        std::string text = "hello";
        uint size = text.length();
        uint offset = 0;

        char *bytes = new char[DbBlock::BLOCK_SZ];
        *(u_int16_t*) (bytes + offset) = size;
        offset += sizeof(u_int16_t);
        memcpy(bytes+offset, text.c_str(), size);
        offset += size;

        char *right_size_bytes = new char[offset];
        memcpy(right_size_bytes, bytes, offset);
        delete [] bytes;
        Dbt *text_data = new Dbt(right_size_bytes, offset);
        //

        RecordID text_id = page->add(text_data);

        // need to unmarshal this!
        char* result = (char *)page->get(text_id);
        delete page;
        delete new_bytes;
        ASSERT_EQ(text.c_str(), result);
    }

    // Heap File
    TEST(heap_file, does_init_work)
    {
        std::string text = "my_heapfile";
        HeapFile file(text);
    }

    // Heap Table
    TEST(heap_table, does_init_work)
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
