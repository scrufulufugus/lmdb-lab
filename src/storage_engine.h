/**
 * @file storage_engine.h - Storage engine abstract classes.
 * DbBlock
 * DbFile
 * DbRelation
 *
 * @author Kevin Lundeen
 * @see "Seattle University, CPSC5300, Winter Quarter 2024"
 */
#pragma once

#include <exception>
#include <system_error>
#include <map>
#include <utility>
#include <vector>
#include <lmdb++.h>
#include <lmdb.h>

/**
 * Global variable to hold dbenv.
 */
extern MDB_env *_MDB_ENV;

/*
 * Convenient aliases for types
 */
typedef u_int16_t RecordID;
typedef u_int32_t BlockID;
typedef std::vector<RecordID> RecordIDs;
typedef std::length_error DbBlockNoRoomError;
typedef std::system_error DbException;

class DbBlock {
public:
    static const uint BLOCK_SZ = 4096;

    /**
     * ctor/dtor (subclasses should handle the big-5)
     */
    DbBlock(MDB_val &block, BlockID block_id, bool is_new = false) : block(block), block_id(block_id) {}

	DbBlock(const DbBlock &other) {
		block_id = other.block_id;
		void *data = malloc(DbBlock::BLOCK_SZ);
		memcpy(data, other.block.mv_data, other.block.mv_size);
		MDB_val o_block(DbBlock::BLOCK_SZ, data);
		block = o_block;
	}

    virtual ~DbBlock() {}

    virtual void initialize_new() {}

    virtual RecordID add(const MDB_val *data) = 0;

    virtual MDB_val *get(RecordID record_id) = 0;

    virtual void put(RecordID record_id, const MDB_val &data) = 0;

    virtual void del(RecordID record_id) = 0;

    virtual RecordIDs *ids() = 0;

    virtual MDB_val *get_block() { return &block; }

    virtual void *get_data() { return block.mv_data; }

    virtual BlockID get_block_id() { return block_id; }

protected:
    MDB_val block;
    BlockID block_id;
};

// convenience type alias
typedef std::vector<BlockID> BlockIDs;

class DbFile {
public:
    // ctor/dtor -- subclasses should handle big-5
    DbFile(std::string name) : name(name) {}

    virtual ~DbFile() {}

    virtual void create() = 0;

    virtual void drop() = 0;

    virtual void open() = 0;

    virtual void close() = 0;

    virtual DbBlock *get_new() = 0;

    virtual DbBlock *get(BlockID block_id) = 0;

    virtual void put(DbBlock *block) = 0;

    virtual BlockIDs *block_ids() = 0;

protected:
    std::string name;  // filename (or part of it)
};


class ColumnAttribute {
public:
    enum DataType {
        INT, TEXT
    };

    ColumnAttribute(DataType data_type) : data_type(data_type) {}

    virtual ~ColumnAttribute() {}

    virtual DataType get_data_type() { return data_type; }

    virtual void set_data_type(DataType data_type) { this->data_type = data_type; }

protected:
    DataType data_type;
};


class Value {
public:
    ColumnAttribute::DataType data_type;
    int32_t n;
    std::string s;

    Value() : n(0) { data_type = ColumnAttribute::INT; }

    Value(int32_t n) : n(n) { data_type = ColumnAttribute::INT; }

    Value(std::string s) : n(0), s(s) { data_type = ColumnAttribute::TEXT; }
};

// More type aliases
typedef std::string Identifier;
typedef std::vector<Identifier> ColumnNames;
typedef std::vector<ColumnAttribute> ColumnAttributes;
typedef std::pair<BlockID, RecordID> Handle;
typedef std::vector<Handle> Handles;  // FIXME: will need to turn this into an iterator at some point
typedef std::map<Identifier, Value> ValueDict;


class DbRelationError : public std::runtime_error {
public:
    explicit DbRelationError(std::string s) : runtime_error(s) {}
};


class DbRelation {
public:
    // ctor/dtor
    DbRelation(Identifier table_name, ColumnNames column_names, ColumnAttributes column_attributes) : table_name(
            table_name), column_names(column_names), column_attributes(column_attributes) {}

    virtual ~DbRelation() {}

    virtual void create() = 0;

    virtual void create_if_not_exists() = 0;

    virtual void drop() = 0;

    virtual void open() = 0;

    virtual void close() = 0;

    virtual Handle insert(const ValueDict *row) = 0;

    virtual void update(const Handle handle, const ValueDict *new_values) = 0;

    virtual void del(const Handle handle) = 0;

    virtual Handles *select() = 0;

    virtual Handles *select(const ValueDict *where) = 0;

    virtual ValueDict *project(Handle handle) = 0;

    virtual ValueDict *project(Handle handle, const ColumnNames *column_names) = 0;

protected:
    Identifier table_name;
    ColumnNames column_names;
    ColumnAttributes column_attributes;
};


