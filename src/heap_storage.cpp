#include "heap_storage.h"
#include "storage_engine.h"

MDB_env *_MDB_ENV = nullptr;

//// SlottedPage
// public

SlottedPage::SlottedPage(MDB_val &block, BlockID block_id, bool is_new) : DbBlock(block, block_id, is_new)
{
    if (is_new)
    {
        this->num_records = 0;
        this->end_free = DbBlock::BLOCK_SZ - 1;
        put_header();
    }
    else
    {
        get_header(this->num_records, this->end_free);
    }
};

RecordID SlottedPage::add(const MDB_val *data)
{
    if (!has_room(data->mv_size))
        throw DbBlockNoRoomError("not enough room for new record");
    u_int16_t id = ++this->num_records;
    u_int16_t size = (u_int16_t)data->mv_size;
    this->end_free -= size;
    u_int16_t loc = this->end_free + 1;
    put_header(); // update global header
    put_header(id, size, loc);
    memcpy(this->address(loc), data->mv_data, size);
    return id;
}

// Get MDB_val by record_id, make sure to deallocate
MDB_val *SlottedPage::get(RecordID record_id)
{
    u_int16_t size;
    u_int16_t loc;
    this->get_header(size, loc, record_id);
    if (loc == 0)
    {
        return nullptr;
    }
    return new MDB_val(size, this->address(loc));
}

// Replace the data at record_id with new MDB_val
void SlottedPage::put(RecordID record_id, const MDB_val &data)
{
    u_int16_t size;
    u_int16_t loc;
    this->get_header(size, loc, record_id);
    u_int16_t new_size = data.mv_size;
    if (new_size > size)
    {
        int extra = new_size - size;
        if (!this->has_room(extra))
        {
            throw "Not enough room in block";
        }
        // OLD: this->slide(loc + new_size, loc + size);
        this->slide(loc, loc - extra);
        // OLD: memmove(this->address(loc - extra), data.get_data(), new_size);
        memmove(this->address(loc - extra), data.mv_data, (extra + size));
    }
    else
    {
        memmove(this->address(loc), data.mv_data, new_size);
        this->slide(loc + new_size, loc + size);
    }
    this->get_header(size, loc, record_id);
    this->put_header(record_id, new_size, loc);
}

// Remove data at record_id
void SlottedPage::del(RecordID record_id)
{
    u_int16_t size;
    u_int16_t loc;

    this->get_header(size, loc, record_id);
    this->put_header(record_id, 0, 0);
    this->slide(loc, loc + size); // ?
}

// Get existing record_ids in SlottedPage, make sure to deallocate
RecordIDs *SlottedPage::ids(void)
{
    RecordIDs *record_ids = new RecordIDs();
    for (RecordID id = 1; id <= this->num_records; id++)
    {
        u_int16_t size;
        u_int16_t loc;
        this->get_header(size, loc, id);
        if (loc != 0)
        {
            record_ids->push_back(id);
        }
    }
    return record_ids;
}

// protected
// Check if SlottedPage has room
bool SlottedPage::has_room(u_int16_t size)
{
    u_int16_t available = this->end_free - (this->num_records + 1) * 4;
    return size <= available;
};

// Slide data left or right, and update the records
void SlottedPage::slide(u_int16_t start, u_int16_t end)
{
    if (start == end)
        return;

    int shift = end - start;
    if (shift < 0)
    {
        // means we are sliding data to the left because of addition
        memmove(this->address(this->end_free + 1 + shift), this->address(this->end_free + 1), abs(shift));
    }
    else
    {
        // means we are sliding data to the right because of a deletion
        uint extra = (DbBlock::BLOCK_SZ - end); // count extra bytes we don't want to remove
        uint dist = (DbBlock::BLOCK_SZ - shift - extra) - (this->end_free + 1);
        memmove(this->address(this->end_free + 1 + shift), this->address(this->end_free + 1), dist);
    }

    RecordIDs *record_ids = ids();
    for (const RecordID &id : *record_ids)
    {
        u_int16_t size;
        u_int16_t loc;
        this->get_header(size, loc, id);
        if (loc <= start) // OLD: loc < end
        {
            loc += shift;
            this->put_header(id, size, loc);
        }
    }
    this->end_free += shift;
    this->put_header();
    delete record_ids;
};

// Get 2-byte integer at given offset in block.
u_int16_t SlottedPage::get_n(u_int16_t offset)
{
    return *(u_int16_t *)this->address(offset);
}

// Put a 2-byte integer at given offset in block.
void SlottedPage::put_n(u_int16_t offset, u_int16_t n)
{
    *(u_int16_t *)this->address(offset) = n;
}

// offset = # bytes to move
void *SlottedPage::address(u_int16_t offset)
{
    return (void *)((char *)this->block.mv_data + offset);
}

// Store the size and offset for given id. For id of zero, store the block header.
void SlottedPage::put_header(RecordID id, u_int16_t size, u_int16_t loc)
{
    if (id == 0)
    { // called the put_header() version and using the default params
        size = this->num_records;
        loc = this->end_free;
    }
    put_n(4 * id, size);
    put_n(4 * id + 2, loc);
}

// Get size and offset for record_id
void SlottedPage::get_header(u_int16_t &size, u_int16_t &loc, RecordID id)
{
    // headers are 4 bytes in size
    size = get_n(4 * id);
    loc = get_n(4 * id + 2);
};

//// BTFile
// public

// Create a new block
void BTFile::create(void)
{
    this->db_open(0);
    SlottedPage *block = this->get_new();
    this->put(block);
    delete block;
};

// Drop this current file, remove the db file
void BTFile::drop(void)
{
    this->close();
    std::remove(this->dbfilename.c_str());
};

// Open current file
void BTFile::open(void)
{
    this->db_open();
};

// Close current file
void BTFile::close(void)
{

	MDB_txn *txn = nullptr;
	mdb_txn_begin(_MDB_ENV, nullptr, 0, &txn);
	mdb_dbi_close(_MDB_ENV, this->dbi);
	mdb_txn_commit(txn);

    this->closed = true;
};

// Allocate a new block and give it a block_id, make sure to deallocate
SlottedPage *BTFile::get_new(void)
{
    char block[DbBlock::BLOCK_SZ];
    memset(block, 0, sizeof(block));
    MDB_val data(sizeof(block), block);
    int block_id = ++this->last;
    MDB_val key(sizeof(block_id), &block_id);
    SlottedPage *page = new SlottedPage(data, this->last, true);
	
	MDB_txn *txn = nullptr;
	mdb_txn_begin(_MDB_ENV, nullptr, 0, &txn);
	mdb_put(txn, this->dbi, &key, &data, 0);
	mdb_get(txn, this->dbi, &key, &data);
	mdb_txn_commit(txn);

    return page;
};

// Get an existing block from the file, make sure to deallocate
SlottedPage *BTFile::get(BlockID block_id)
{
    char block[DbBlock::BLOCK_SZ];
    memset(block, 0, sizeof(block));
	MDB_val data(sizeof(block), block);
	MDB_val key(sizeof(BlockID), &block_id);

	MDB_txn *txn = nullptr;
	mdb_txn_begin(_MDB_ENV, nullptr, 0, &txn);
	mdb_get(txn, this->dbi, &key, &data);
	mdb_txn_commit(txn);

    return new SlottedPage(data, block_id);
};

// Replace an existing block in the file
void BTFile::put(DbBlock *block)
{
    BlockID block_id(block->get_block_id());
    MDB_val key(sizeof(BlockID), &block_id);
    MDB_val data(DbBlock::BLOCK_SZ, block->get_data());

	MDB_txn *txn = nullptr;
	mdb_txn_begin(_MDB_ENV, nullptr, 0, &txn);
	mdb_put(txn, this->dbi, &key, &data, 0); // Maybe use MDB_append here?
	mdb_txn_commit(txn);
};

// Get existing block_ids in the file, make sure to deallocate
BlockIDs *BTFile::block_ids()
{
    BlockIDs *block_ids = new BlockIDs;
    for (u_int32_t i = 1; i <= this->last; i++)
    {
        block_ids->push_back(i);
    }
    return block_ids;
};

// protected
void BTFile::db_open(uint flags)
{
    if (!this->closed)
        return;
    const char *path;
    mdb_env_get_path(_MDB_ENV, &path);
    dbfilename = path + name + ".mdb";

	// make TXN
	MDB_txn *txn = nullptr;
	mdb_txn_begin(_MDB_ENV, nullptr, 0, &txn); 

	// open dbi
	mdb_dbi_open(txn, dbfilename.c_str(), MDB_CREATE | flags, &dbi);
	
	// get stats
	MDB_stat stats;
	mdb_stat(txn, dbi, &stats);
    last = stats.ms_entries;

	// clean up
	mdb_txn_commit(txn);
    this->closed = false;
};

