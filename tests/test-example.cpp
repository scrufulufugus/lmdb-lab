#include <gtest/gtest.h>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <cstdlib>

#include "storage_engine.h"
#include "heap_storage.h"

// helper util functions
MDB_val *marshal_text(std::string text);
std::string unmarshal_text(MDB_val &block);
void remove_files(std::vector<std::string> file_names);

class BTFixture : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const char *home = std::getenv("HOME");
		/* BDB Version
        DbEnv *env = new DbEnv(0U);
        env->set_message_stream(&std::cout);
        env->set_error_stream(&std::cerr);
        std::string envdir = std::string(home) + "/cpsc5300/data";
        env->open(envdir.c_str(), DB_CREATE | DB_INIT_MPOOL, 0);
        _DB_ENV = env;
		*/
		// lmdb version
		MDB_env *env;
        std::string envdir = std::string(home) + "/Projects/lmdb-lab/data/example.mdb";
		mdb_env_create(&env);
		mdb_env_set_mapsize(env, 1UL * 1024UL * 1024UL * 1024UL); // 1Gb
		mdb_env_open(env, envdir.c_str(), 0, 0664); // unlike in BDB, we can't pass in DB_CREATE
		_MDB_ENV = env;

    }

    void TearDown() override
    {
        mdb_env_close(_MDB_ENV);
		int res = std::system("rm -f ./data/example.mdb/*.mdb"); // remove the .db files in _MDB_ENV dir
        if (res != 0)
        {
            std::cout << "\n!!! Failed to remove .mdb files in /data/example.mdb, manually remove it before running the test again !!!\n";
        }
        std::cout << "Removed .mdb files in ../data/example.mdb\n";
    }
};


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
	
	TEST(slotted_page, slotted_page_basics)
    	{
		char block[DbBlock::BLOCK_SZ];
		memset(block, 0, sizeof(block));
		MDB_val data(sizeof(block), block);

		RecordID id = 0;
		SlottedPage *page = new SlottedPage(data, id, true); // set this to true so page->add wouldn't seg fault

		// adding
		std::string t1_v = "hello";
		MDB_val *t1_d = marshal_text(t1_v);
		RecordID t1_id = page->add(t1_d);
		MDB_val *t1_rd = page->get(t1_id);
		std::string t1_rv = unmarshal_text(*t1_rd);
		ASSERT_EQ(t1_v, t1_rv);

		// replacement
		std::string t2_v = "goodbye";
		MDB_val *t2_d = marshal_text(t2_v);
		page->put(t1_id, *t2_d);
		MDB_val *t2_rd = page->get(t1_id);
		std::string t2_rv = unmarshal_text(*t2_rd);
		ASSERT_EQ(t2_v, t2_rv);

		// add another
		std::string t3_v = "mdavis";
		MDB_val *t3_d = marshal_text(t3_v);
		RecordID t3_id = page->add(t3_d);
		MDB_val *t3_rd = page->get(t3_id);
		std::string t3_rv = unmarshal_text(*t3_rd);
		ASSERT_EQ(t3_v, t3_rv);
		t2_rd = page->get(t1_id);
		t2_rv = unmarshal_text(*t2_rd);
		ASSERT_EQ(t2_v, t2_rv);

		// replace again
		std::string t4_v = "jcoltrane";
		MDB_val *t4_d = marshal_text(t4_v);
		page->put(t3_id, *t4_d);
		MDB_val *t4_rd = page->get(t3_id);
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
		MDB_val *e_rd = page->get(t3_id);
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
		delete e_rd;
	}

	TEST_F(BTFixture, BT_file_basics)
    {
        remove_files({"_my_btfile"});

        //// create, put SlottedPage, close it
        std::string text = "_my_btfile";
        BTFile file(text);
        file.create();

        BlockIDs *bids = file.block_ids();
        BlockIDs expected_bids = {1};
        ASSERT_EQ(*bids, expected_bids);

        // give me new block
        char block[DbBlock::BLOCK_SZ];
        memset(block, 0, sizeof(block));
        MDB_val data(sizeof(block), block);
        BlockID block_id = 1;
        SlottedPage *page = new SlottedPage(data, block_id, true);

        // add data to my page and put it into block
        std::string value = "hhancock";
        MDB_val *d_value = marshal_text(value);
        RecordID record_id = page->add(d_value);
        file.put(page);

        file.close(); // The DB handle may not be accessed again after Db::close() is called, regardless of its return.
        //// open it again, get SlottedPage, drop it
        BTFile n_file(text);
        n_file.open();

        SlottedPage *r_page = n_file.get(block_id);
        MDB_val *r_data = page->get(record_id);
        std::string r_value = unmarshal_text(*r_data);
        ASSERT_EQ(value, r_value);

        n_file.drop();

        // clean up
        delete bids;
        delete page;
        delete r_page;
    }
}

MDB_val *marshal_text(std::string text)
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
    return new MDB_val(offset, right_size_bytes);
}

std::string unmarshal_text(MDB_val &data)
{
    char *bytes = (char *)data.mv_data;
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
        std::ifstream file(path.c_str());
        if (file.is_open())
        {
            std::cout << "Deleting previously created dummy file " << file_name << "\n";
            std::remove(path.c_str());
        }
    }
}
