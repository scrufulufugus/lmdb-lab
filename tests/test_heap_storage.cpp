#include "gtest/gtest.h"
#include "heap_storage.h"
#include "storage_engine.h"
#include <iostream>

// helper util functions
Dbt *marshal_text(std::string text);
std::string unmarshal_text(Dbt *block);

namespace
{
    TEST(sample_test_case, sample_test)
    {
        EXPECT_EQ(1, 1);
    }

    // Slotted Page
    TEST(slotted_page, add_text_data)
    {
        char block[DbBlock::BLOCK_SZ];
        memset(block, 0, sizeof(block));
        Dbt data(block, sizeof(block));

        RecordID id = 0;
        SlottedPage *page = new SlottedPage(data, id, true); // set this to true so page->add wouldn't seg fault

        std::string text = "hello";
        Dbt *text_data = marshal_text(text);
        RecordID text_id = page->add(text_data);

        Dbt *res_data = page->get(text_id);
        std::string res = unmarshal_text(res_data);
        ASSERT_EQ(text, res);
        delete page;
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

Dbt *marshal_text(std::string text) {
    uint size = text.length();
    uint offset = 0;

    char *bytes = new char[DbBlock::BLOCK_SZ];
    *(u_int16_t *)(bytes + offset) = size;
    offset += sizeof(u_int16_t);
    memcpy(bytes + offset, text.c_str(), size);
    offset += size;

    char *right_size_bytes = new char[offset];
    memcpy(right_size_bytes, bytes, offset);
    delete[] bytes;
    return new Dbt(right_size_bytes, offset);
}

std::string unmarshal_text(Dbt *data) {
    u_int16_t offset = (u_int16_t)data->get_size();
    char *bytes = (char *)data->get_data() + sizeof(u_int16_t);
    std::string text(bytes, offset - sizeof(u_int16_t));
    return text;
}