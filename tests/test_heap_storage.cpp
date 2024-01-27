#include "gtest/gtest.h"
#include "heap_storage.h"
#include "storage_engine.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdio>
#include <cstdlib>

// PY  DOCS: https://docs.jcea.es/berkeleydb/latest/dbenv.html
// C++ DOCS: https://docs.oracle.com/database/bdb181/html/api_reference/CXX/frame_main.html

// helper util functions
Dbt *marshal_text(std::string text);
std::string unmarshal_text(Dbt &block);
void remove_files(std::vector<std::string> file_names);

class HeapFixture : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const char *home = std::getenv("HOME");
        DbEnv *env = new DbEnv(0U);
        env->set_message_stream(&std::cout);
        env->set_error_stream(&std::cerr);
        std::string envdir = std::string(home) + "/cpsc5300/data";
        env->open(envdir.c_str(), DB_CREATE | DB_INIT_MPOOL, 0);
        _DB_ENV = env;
    }

    void TearDown() override
    {
        _DB_ENV->close(0U);                         // deallocates memory for me
        int res = std::system("rm -f ../data/__*"); // remove the .db files in _DB_ENV home
        if (res != 0)
        {
            std::cout << "\n!!! Failed to remove .db files in /data, manually remove it before running the test again !!!\n";
        }
        std::cout << "Removed .db files in ../data\n";
    }
};

namespace
{
    TEST(slotted_page, slotted_page_basics)
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
        t2_rv = unmarshal_text(*t2_rd);
        ASSERT_EQ(t2_v, t2_rv);

        // ids
        RecordIDs *rids = page->ids();
        RecordIDs expected_rids = {1, 2};
        ASSERT_EQ(*rids, expected_rids);

        // deletion
        page->del(t1_id);
        rids = page->ids();
        expected_rids = {2};
        ASSERT_EQ(*rids, expected_rids);
        Dbt *e_rd = page->get(t3_id);
        std::string e_rv = unmarshal_text(*e_rd);
        ASSERT_EQ(t4_v, e_rv);

        // fix me?
        delete page;
        delete rids;
        delete t1_d;
        delete t1_rd;
        delete t2_d;
        delete t2_rd;
        delete t3_d;
        delete t3_rd;
        delete t4_d;
        delete t4_rd;
    }

    TEST_F(HeapFixture, heap_file_basics)
    {
        remove_files({"_my_heapfile"});

        //// create, put SlottedPage, close it
        std::string text = "_my_heapfile";
        HeapFile file(text);
        file.create();

        BlockIDs *bids = file.block_ids();
        BlockIDs expected_bids = {1};
        ASSERT_EQ(*bids, expected_bids);

        // give me new block
        char block[DbBlock::BLOCK_SZ];
        memset(block, 0, sizeof(block));
        Dbt data(block, sizeof(block));
        BlockID block_id = 1;
        SlottedPage *page = new SlottedPage(data, block_id, true);

        // add data to my page and put it into block
        std::string value = "hhancock";
        Dbt *d_value = marshal_text(value);
        RecordID record_id = page->add(d_value);
        file.put(page);

        file.close(); // The DB handle may not be accessed again after Db::close() is called, regardless of its return.
        //// open it again, get SlottedPage, drop it
        HeapFile n_file(text);
        n_file.open();

        SlottedPage *r_page = n_file.get(block_id);
        Dbt *r_data = page->get(record_id);
        std::string r_value = unmarshal_text(*r_data);
        ASSERT_EQ(value, r_value);

        n_file.drop();

        // clean up
        delete bids;
        delete page;
        delete r_page;
    }

    TEST_F(HeapFixture, heap_table_basics)
    {
        remove_files({"_test_create_drop_cpp", "_test_data_cpp"});

        // setup
        ColumnNames column_names;
        column_names.push_back("a");
        column_names.push_back("b");
        ColumnAttributes column_attributes;
        ColumnAttribute ca(ColumnAttribute::INT);
        column_attributes.push_back(ca);
        ca.set_data_type(ColumnAttribute::TEXT);
        column_attributes.push_back(ca);

        // start testing
        HeapTable table1("_test_create_drop_cpp", column_names, column_attributes);
        table1.create();
        table1.drop(); // drop makes the object unusable because of BerkeleyDB
        // restriction-- maybe want to fix this some day
        HeapTable table("_test_data_cpp", column_names, column_attributes);
        table.create_if_not_exists();
        ValueDict row;
        row["a"] = Value(12);
        row["b"] = Value("Hello!");
        table.insert(&row);
        Handles *handles = table.select();
        ValueDict *result = table.project((*handles)[0]);
        Value value = (*result)["a"];
        ASSERT_EQ(value.n, 12);
        value = (*result)["b"];
        ASSERT_EQ(value.s, "Hello!");
        table.drop();

        // clean up
        delete handles;
        delete result;
    }
}

Dbt *marshal_text(std::string text)
{
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

std::string unmarshal_text(Dbt &data)
{
    char *bytes = (char *)data.get_data();
    u_int16_t size;
    memcpy(&size, bytes, sizeof(u_int16_t));
    std::string text(bytes + sizeof(u_int16_t), size);
    return text;
}

void remove_files(std::vector<std::string> file_names)
{
    const char *home = std::getenv("HOME");
    for (const auto &file_name : file_names)
    {
        std::string path = std::string(home) + "/cpsc5300/data" + file_name + ".db";
        std::ifstream file(path);
        if (file.is_open())
        {
            std::cout << "Deleting previously created dummy file " << file_name << "\n";
            std::remove(path.c_str());
        }
    }
}
