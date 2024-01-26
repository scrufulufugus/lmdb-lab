#include "heap_storage.h"
#include "storage_engine.h"
#include <iomanip> // remove me
/** NOTES:
 * When you get a block from BerkDB with db->get,
 * you supply a Dbt structure but BerkDB fills it with a pointer to their memory--
 * you don't ever free this memory.
 */

// util function to view the block in a SlottedPage
void print_mem(const void *data, size_t size)
{
    auto *bytes = static_cast<const unsigned char *>(data);
    for (size_t i = 0; i < size; ++i)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)bytes[i];
        if ((i + 1) % 16 == 0)
            std::cout << std::endl;
        else
            std::cout << " ";
    }
    std::cout << std::dec << std::endl;
}

// API DOCS: https://docs.jcea.es/berkeleydb/latest/dbenv.html
// Set global _DB_ENV, probably not here...
DbEnv *_DB_ENV = nullptr;

//// SlottedPage
// public
SlottedPage::SlottedPage(Dbt &block, BlockID block_id, bool is_new) : DbBlock(block, block_id, is_new)
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

RecordID SlottedPage::add(const Dbt *data)
{
    if (!has_room(data->get_size()))
        throw DbBlockNoRoomError("not enough room for new record");
    u_int16_t id = ++this->num_records;
    u_int16_t size = (u_int16_t)data->get_size();
    this->end_free -= size;
    u_int16_t loc = this->end_free + 1;
    put_header(); // update global header
    put_header(id, size, loc);
    memcpy(this->address(loc), data->get_data(), size);
    return id;
}

// don't forget to delete data after calling get
Dbt *SlottedPage::get(RecordID record_id)
{
    u_int16_t size;
    u_int16_t loc;
    this->get_header(size, loc, record_id);
    if (loc == 0)
    {
        return nullptr;
    }
    return new Dbt(this->address(loc), size);
}

void SlottedPage::put(RecordID record_id, const Dbt &data)
{
    u_int16_t size;
    u_int16_t loc;
    this->get_header(size, loc, record_id);
    u_int16_t new_size = data.get_size();
    if (new_size > size)
    {
        int extra = new_size - size;
        if (!this->has_room(extra))
        {
            throw "Not enough room in block";
        }
        this->slide(loc + new_size, loc + size);
        //std::cout << "put, before memmove\n";
        //print_mem(this->address(0), DbBlock::BLOCK_SZ);
        memmove(this->address(loc - extra), data.get_data(), new_size);
        //std::cout << "put, after memmove\n";
        //print_mem(this->address(0), DbBlock::BLOCK_SZ);
    }
    else
    {
        memmove(this->address(loc), data.get_data(), new_size);
        this->slide(loc + new_size, loc + size);
    }
    this->get_header(size, loc, record_id);
    this->put_header(record_id, new_size, loc);
}

void SlottedPage::del(RecordID record_id)
{
    u_int16_t size;
    u_int16_t loc;
    this->get_header(size, loc, record_id);
    this->put_header(record_id, 0, 0);
}

RecordIDs *SlottedPage::ids(void)
{
    // caller is responsible for cleaning up memory
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
bool SlottedPage::has_room(u_int16_t size)
{
    u_int16_t available = this->end_free - (this->num_records + 1) * 4;
    return size <= available;
};

void SlottedPage::slide(u_int16_t start, u_int16_t end)
{
    if (start == end)
        return;
    int shift = end - start;
    memmove(this->address(this->end_free + 1 + shift), this->address(this->end_free + 1), abs(shift));
    // std::cout << "BEGIN" << start << ":start " << end << ":end " << std::endl;
    RecordIDs *record_ids = ids();
    for (const RecordID &id : *record_ids)
    {
        u_int16_t size;
        u_int16_t loc;
        this->get_header(size, loc, id);
        //std::cout << size << ":size, " << loc << ":loc, " << id << ":id, " << shift << ":shift" << std::endl;
        if (loc < end)
        {
            loc += shift;
            //std::cout << "UPDATING -> record_id: " << id << size << ":size, " << loc << ":loc, \n";
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
    return (void *)((char *)this->block.get_data() + offset);
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

void SlottedPage::get_header(u_int16_t &size, u_int16_t &loc, RecordID id)
{
    // headers are 4 bytes in size
    size = get_n(4 * id);
    loc = get_n(4 * id + 2);
};

//// HeapFile
// NOTE: Don't need to implement constructor

// public
void HeapFile::create(void)
{
    this->db_open(DB_CREATE | DB_EXCL);
    SlottedPage *block = this->get_new();
    this->put(block);
};

void HeapFile::drop(void)
{
    this->close();
    std::remove(this->dbfilename.c_str());
};

void HeapFile::open(void)
{
    this->db_open();
};

void HeapFile::close(void)
{
    this->db.close(0);
    this->closed = true;
};

// use profs code
SlottedPage *HeapFile::get_new(void)
{
    char block[DbBlock::BLOCK_SZ];
    memset(block, 0, sizeof(block));
    Dbt data(block, sizeof(block));
    int block_id = ++this->last;
    Dbt key(&block_id, sizeof(block_id));
    SlottedPage *page = new SlottedPage(data, this->last, true);
    this->db.put(nullptr, &key, &data, 0); // write it out with initialization applied
    this->db.get(nullptr, &key, &data, 0);
    return page;
};

/**
 * db.get()
 * If key is an integer then the DB_SET_RECNO flag is automatically set
 * for BTree databases and the actual key and the data value are returned as a tuple.
 * -- python api, and read the C++ api too.
 */
SlottedPage *HeapFile::get(BlockID block_id)
{
    BlockID *key_data = new BlockID(block_id); // likely bad
    Dbt key(key_data, sizeof(BlockID));
    Dbt data;
    this->db.get(nullptr, &key, &data, DB_SET_RECNO);
    return new SlottedPage(data, block_id);
    delete key_data;
};

// questionable
void HeapFile::put(DbBlock *block)
{
    BlockID *key_data = new BlockID(block->get_block_id());
    Dbt key(key_data, sizeof(BlockID));
    this->db.put(nullptr, &key, block->get_block(), 0);
};

// deallocate me !
BlockIDs *HeapFile::block_ids()
{
    BlockIDs *block_ids = new BlockIDs;
    for (u_int32_t i = 1; i <= this->last; i++)
    {
        block_ids->push_back(i);
    }
    return block_ids;
};

// protected
void HeapFile::db_open(uint flags)
{
    // might want to consider using C++17 so I can use filesystem
    const char *home;
    _DB_ENV->get_home(&home);
    dbfilename = home + name + ".db";
    this->db.open(nullptr, dbfilename.c_str(), nullptr, DB_RECNO, flags, 0);
    void *stats;                      // deallocate me?
    this->db.stat(nullptr, stats, 0); // could use DB_FAST_STAT FLAG? check Db:stat() docs
    last = ((DB_BTREE_STAT *)stats)->bt_ndata;
    this->closed = false;
    delete (DB_BTREE_STAT *)stats; // is this ok?
};

//// HeapTable
// public
HeapTable::HeapTable(Identifier table_name, ColumnNames column_names, ColumnAttributes column_attributes) : DbRelation(table_name, column_names, column_attributes),
                                                                                                            file(table_name){

                                                                                                            };

void HeapTable::create()
{
    this->file.create();
}

void HeapTable::create_if_not_exists()
{
    try
    {
        this->open();
    }
    catch (const DbException &e)
    {
        if (e.get_errno() == ENOENT)
        {
            this->create();
        }
        else
        { // unexpected exception
            throw e;
        }
    }
}

void HeapTable::open()
{
    this->file.open();
}

void HeapTable::close()
{
    this->file.close();
}

void HeapTable::drop()
{
    this->file.drop();
}

Handle HeapTable::insert(const ValueDict *row)
{
    this->open();
    return this->append(validate(row));
}

// not required for Milestone 2
void HeapTable::update(const Handle handle, const ValueDict *new_values){};

// not required for Milestone 2
void HeapTable::del(const Handle handle){};

Handles *HeapTable::select()
{
    Handles *handles = new Handles();
    BlockIDs *block_ids = file.block_ids();
    for (auto const &block_id : *block_ids)
    {
        SlottedPage *block = file.get(block_id);
        RecordIDs *record_ids = block->ids();
        for (auto const &record_id : *record_ids)
            handles->push_back(Handle(block_id, record_id));
        delete record_ids;
        delete block;
    }
    delete block_ids;
    return handles;
};

Handles *HeapTable::select(const ValueDict *where)
{
    Handles *handles = new Handles();
    BlockIDs *block_ids = file.block_ids();
    for (auto const &block_id : *block_ids)
    {
        SlottedPage *block = file.get(block_id);
        RecordIDs *record_ids = block->ids();
        for (auto const &record_id : *record_ids)
            handles->push_back(Handle(block_id, record_id));
        delete record_ids;
        delete block;
    }
    delete block_ids;
    return handles;
}

// not required for Milestone 2
ValueDict *HeapTable::project(Handle handle) { return nullptr; };

// not required for Milestone 2
ValueDict *HeapTable::project(Handle handle, const ColumnNames *column_names) { return nullptr; };

// protected

ValueDict *HeapTable::validate(const ValueDict *row)
{
    ValueDict *full_row = new ValueDict;
    for (const auto &col : *row)
    {
        if (col.first == "INT" || col.first == "TEXT")
        {
            (*full_row)[col.first] = col.second;
        }
        else
        {
            throw std::invalid_argument("Can't handle this type: " + col.first);
        }
    }
    return full_row;
};

Handle HeapTable::append(const ValueDict *row)
{
    RecordID record_id;
    BlockID block_id;
    Dbt *data = marshal(row);
    SlottedPage *block = this->file.get(this->file.get_last_block_id());
    try
    {
        record_id = block->add(data);
    }
    catch (const DbBlockNoRoomError &e)
    {
        block = this->file.get_new();
        record_id = block->add(data);
    }
    this->file.put(block);
    return {record_id, block_id};
};

// caller responsible for freeing the returned Dbt and its enclosed ret->get_data().
Dbt *HeapTable::marshal(const ValueDict *row)
{
    char *bytes = new char[DbBlock::BLOCK_SZ]; // more than we need (we insist that one row fits into DbBlock::BLOCK_SZ)
    uint offset = 0;
    uint col_num = 0;
    for (auto const &column_name : this->column_names)
    {
        ColumnAttribute ca = this->column_attributes[col_num++];
        ValueDict::const_iterator column = row->find(column_name);
        Value value = column->second;
        if (ca.get_data_type() == ColumnAttribute::DataType::INT)
        {
            *(int32_t *)(bytes + offset) = value.n;
            offset += sizeof(int32_t);
        }
        else if (ca.get_data_type() == ColumnAttribute::DataType::TEXT)
        {
            uint size = value.s.length();
            *(u_int16_t *)(bytes + offset) = size;
            offset += sizeof(u_int16_t);
            memcpy(bytes + offset, value.s.c_str(), size); // assume ascii for now
            offset += size;
        }
        else
        {
            throw DbRelationError("Only know how to marshal INT and TEXT");
        }
    }
    char *right_size_bytes = new char[offset];
    memcpy(right_size_bytes, bytes, offset);
    delete[] bytes;
    Dbt *data = new Dbt(right_size_bytes, offset);
    return data;
}

ValueDict *HeapTable::unmarshal(Dbt *data) { return nullptr; };

/// TEST

// test function provided by professor
bool test_heap_storage()
{
    ColumnNames column_names;
    column_names.push_back("a");
    column_names.push_back("b");
    ColumnAttributes column_attributes;
    ColumnAttribute ca(ColumnAttribute::INT);
    column_attributes.push_back(ca);
    ca.set_data_type(ColumnAttribute::TEXT);
    column_attributes.push_back(ca);
    HeapTable table1("_test_create_drop_cpp", column_names, column_attributes);
    table1.create();
    std::cout << "create ok" << std::endl;
    table1.drop(); // drop makes the object unusable because of BerkeleyDB
    // restriction-- maybe want to fix this some day
    std::cout << "drop ok" << std::endl;
    HeapTable table("_test_data_cpp", column_names, column_attributes);
    table.create_if_not_exists();
    std::cout << "create_if_not_exsts ok" << std::endl;
    ValueDict row;
    row["a"] = Value(12);
    row["b"] = Value("Hello!");
    std::cout << "try insert" << std::endl;
    table.insert(&row);
    std::cout << "insert ok" << std::endl;
    Handles *handles = table.select();
    std::cout << "select ok " << handles->size() << std::endl;
    ValueDict *result = table.project((*handles)[0]);
    std::cout << "project ok" << std::endl;
    Value value = (*result)["a"];
    if (value.n != 12)
        return false;
    value = (*result)["b"];
    if (value.s != "Hello!")
        return false;
    table.drop();
    return true;
}
