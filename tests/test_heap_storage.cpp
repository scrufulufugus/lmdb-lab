#include "gtest/gtest.h"
#include "heap_storage.h"
#include "storage_engine.h"
#include <iostream>

// helper util functions
Dbt *marshal_text(std::string text);
std::string unmarshal_text(Dbt &block);

namespace
{
    TEST(sample_test_case, sample_test)
    {
        EXPECT_EQ(1, 1);
    }

    // Slotted Page
    TEST(slotted_page, test_basics)
    {
        char block[DbBlock::BLOCK_SZ];
        memset(block, 0, sizeof(block));
        Dbt data(block, sizeof(block));

        RecordID id = 0;
        SlottedPage *page = new SlottedPage(data, id, true); // set this to true so page->add wouldn't seg fault

        // adding
        std::string t1_v = "hello";
        Dbt *t1_d = marshal_text(t1_v);
        RecordID t1_id = page->add(t1_d);
        Dbt *t1_rd = page->get(t1_id);
        std::string t1_rv = unmarshal_text(*t1_rd);
        ASSERT_EQ(t1_v, t1_rv);

        // replacement
        std::string t2_v = "goodbye";
        Dbt *t2_d = marshal_text(t2_v);
        page->put(t1_id, *t2_d);
        Dbt *t2_rd = page->get(t1_id);
        std::string t2_rv = unmarshal_text(*t2_rd);
        ASSERT_EQ(t2_v, t2_rv);

        // add another
        std::string t3_v = "mdavis";
        Dbt *t3_d = marshal_text(t3_v);
        RecordID t3_id = page->add(t3_d);
        Dbt *t3_rd = page->get(t3_id);
        std::string t3_rv = unmarshal_text(*t3_rd);
        ASSERT_EQ(t3_v, t3_rv);
        t2_rd = page->get(t1_id);
        t2_rv = unmarshal_text(*t2_rd);
        ASSERT_EQ(t2_v, t2_rv);

        // replace again
        std::string t4_v = "jcoltrane";
        Dbt *t4_d = marshal_text(t4_v);
        page->put(t3_id, *t4_d);
        Dbt *t4_rd = page->get(t3_id);
        std::string t4_rv = unmarshal_text(*t4_rd);
        ASSERT_EQ(t4_v, t4_rv);
        t2_rd = page->get(t1_id);
        std::cout << "was able to get em" << std::endl;
        t2_rv = unmarshal_text(*t2_rd); // it's "ane" which means jcoltrane overwrote the bytes where "goodbye" was located.
        ASSERT_EQ(t2_v, t2_rv);

        // ids
        RecordIDs *rids = page->ids();
        RecordIDs expected_rids = {1,2};
        ASSERT_EQ(*rids, expected_rids);
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

std::string unmarshal_text(Dbt &data) {
    u_int16_t offset = (u_int16_t)data.get_size();
    char *bytes = (char *)data.get_data();
    u_int16_t size;
    memcpy(&size, bytes, sizeof(u_int16_t));
    std::string text(bytes + sizeof(u_int16_t), size);
    return text;
}