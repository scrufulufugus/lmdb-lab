#include "heap_storage.h"
#include "storage_engine.h"

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

    return new Dbt(this->address(loc), (u_int32_t)size);
}

void SlottedPage::put(RecordID record_id, const Dbt &data)
{
    u_int16_t size;
    u_int16_t loc;
    this->get_header(size, loc, record_id);
    u_int16_t new_size = (u_int16_t)data.get_size();
    if (new_size > size)
    {
        u_int16_t extra = new_size - size;
        if (!this->has_room(extra))
        {
            throw "Not enough room in block";
        }
        this->slide(loc + new_size, loc + size);
        memcpy(this->address(loc - extra), data.get_data(), new_size);
    }
    else
    {
        memcpy(this->address(loc), data.get_data(), new_size);
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
    for (RecordID id = 0; id < this->num_records; id++)
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
void SlottedPage::get_header(u_int16_t &size, u_int16_t &loc, RecordID id)
{
    // headers are 4 bytes in size
    size = get_n(4 * id);
    loc = get_n(4 * id + 2);
};

bool SlottedPage::has_room(u_int16_t size)
{
    u_int16_t available = this->end_free - (this->num_records + 1) * 4;
    return size <= available;
};

void SlottedPage::slide(u_int16_t start, u_int16_t end)
{
    u_int16_t shift = end - start;
    if (shift == 0)
        return;

    // slide data
    *(u_int16_t *)this->address(this->end_free + 1 + shift) = *(u_int16_t *)this->address(this->end_free + 1);

    RecordIDs *record_ids = ids();
    for (const RecordID &id : *record_ids)
    {
        u_int16_t size;
        u_int16_t loc;
        this->get_header(size, loc, id);
        if (loc <= start)
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

//// HeapFile
// public
void HeapFile::create(void){};

void HeapFile::drop(void){};

void HeapFile::open(void){};

void HeapFile::close(void){};

SlottedPage *HeapFile::get_new(void) { return nullptr; };

SlottedPage *HeapFile::get(BlockID block_id) { return nullptr; };

void HeapFile::put(DbBlock *block){};

BlockIDs *HeapFile::block_ids() { return nullptr; };
// protected
void HeapFile::db_open(uint flags){};

//// HeapTable
// public
HeapTable::HeapTable(Identifier table_name, ColumnNames column_names, ColumnAttributes column_attributes) : DbRelation(table_name, column_names, column_attributes),
                                                                                                            file(table_name){

                                                                                                            };

void HeapTable::create()
{
    return;
}

void HeapTable::create_if_not_exists()
{
    return;
}

void HeapTable::open()
{
    return;
}

void HeapTable::close()
{
    return;
}

void HeapTable::drop()
{
    return;
}

Handle HeapTable::insert(const ValueDict *row)
{
    u_int32_t stub_blockID = 1;
    u_int16_t stub_recordID = 2;
    for (auto const &item : *row)
    {
        if (item.first == "INT")
        {

            return {stub_blockID, stub_recordID};
        }
        else if (item.first == "TEXT")
        {
            return {stub_blockID, stub_recordID};
        }
        else
        {
            return {stub_blockID, stub_recordID};
        }
    }

    return {stub_blockID, stub_recordID};
}

void HeapTable::update(const Handle handle, const ValueDict *new_values){};

void HeapTable::del(const Handle handle){};

Handles *HeapTable::select() { return nullptr; };

Handles *HeapTable::select(const ValueDict* where) {
    Handles* handles = new Handles();
    BlockIDs* block_ids = file.block_ids();
    for (auto const& block_id: *block_ids) {
        SlottedPage* block = file.get(block_id);
        RecordIDs* record_ids = block->ids();
        for (auto const& record_id: *record_ids)
            handles->push_back(Handle(block_id, record_id));
        delete record_ids;
        delete block;
    }
    delete block_ids;
    return handles;
}

ValueDict *HeapTable::project(Handle handle) { return nullptr; };

ValueDict *HeapTable::project(Handle handle, const ColumnNames *column_names) { return nullptr; };

// protected

ValueDict *HeapTable::validate(const ValueDict *row) { return nullptr; };

Handle HeapTable::append(const ValueDict *row) { return {0, 0}; };

Dbt* HeapTable::marshal(const ValueDict* row) {
    char *bytes = new char[DbBlock::BLOCK_SZ]; // more than we need (we insist that one row fits into DbBlock::BLOCK_SZ)
    uint offset = 0;
    uint col_num = 0;
    for (auto const& column_name: this->column_names) {
        ColumnAttribute ca = this->column_attributes[col_num++];
        ValueDict::const_iterator column = row->find(column_name);
        Value value = column->second;
        if (ca.get_data_type() == ColumnAttribute::DataType::INT) {
            *(int32_t*) (bytes + offset) = value.n;
            offset += sizeof(int32_t);
        } else if (ca.get_data_type() == ColumnAttribute::DataType::TEXT) {
            uint size = value.s.length();
            *(u_int16_t*) (bytes + offset) = size;
            offset += sizeof(u_int16_t);
            memcpy(bytes+offset, value.s.c_str(), size); // assume ascii for now
            offset += size;
        } else {
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
