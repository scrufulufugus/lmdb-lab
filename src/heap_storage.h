/**
 * @file heap_storage.h - Implementation of storage_engine with a heap file structure.
 * SlottedPage: DbBlock
 * BTFile: DbFile
 * BTTable: DbRelation
 *
 * @author Kevin Lundeen
 * @see "Seattle University, CPSC5300, Winter Quarter 2024"
 */
#pragma once

#include <cstring>
#include <lmdb++.h>
#include "storage_engine.h"

/**
 * @class SlottedPage - heap file implementation of DbBlock.
 *
 *      Manage a database block that contains several records.
        Modeled after slotted-page from Database Systems Concepts, 6ed, Figure 10-9.

        Record id are handed out sequentially starting with 1 as records are added with add().
        Each record has a header which is a fixed offset from the beginning of the block:
            Bytes 0x00 - Ox01: number of records
            Bytes 0x02 - 0x03: offset to end of free space
            Bytes 0x04 - 0x05: size of record 1
            Bytes 0x06 - 0x07: offset to record 1
            etc.
 *
 */
class SlottedPage : public DbBlock {
public:
    SlottedPage(MDB_val &block, BlockID block_id, bool is_new = false);

    virtual ~SlottedPage() {}

    SlottedPage(const SlottedPage &other);

    SlottedPage(SlottedPage &&temp) = delete;

    SlottedPage &operator=(const SlottedPage &other) = delete;

    SlottedPage &operator=(SlottedPage &temp) = delete;

    virtual RecordID add(const MDB_val *data);

    virtual MDB_val *get(RecordID record_id);

    virtual void put(RecordID record_id, const MDB_val &data);

    virtual void del(RecordID record_id);

    virtual RecordIDs *ids(void);

protected:
    u_int16_t num_records;
    u_int16_t end_free;

    virtual void get_header(u_int16_t &size, u_int16_t &loc, RecordID id = 0);

    virtual void put_header(RecordID id = 0, u_int16_t size = 0, u_int16_t loc = 0);

    virtual bool has_room(u_int16_t size);

    virtual void slide(u_int16_t start, u_int16_t end);

    virtual u_int16_t get_n(u_int16_t offset);

    virtual void put_n(u_int16_t offset, u_int16_t n);

    virtual void *address(u_int16_t offset);
};

class BTFile : public DbFile {
public:
    BTFile(std::string name) : DbFile(name), dbfilename(""), last(0), closed(true), dbi(0) {}

    virtual ~BTFile() {}

    BTFile(const BTFile &other) = delete;

    BTFile(BTFile &&temp) = delete;

    BTFile &operator=(const BTFile &other) = delete;

    BTFile &operator=(BTFile &&temp) = delete;

    virtual void create(void);

    virtual void drop(void);

    virtual void open(void);

    virtual void close(void);

    virtual SlottedPage *get_new(void);

    virtual SlottedPage *get(BlockID block_id);

    virtual void put(DbBlock *block);

    virtual BlockIDs *block_ids();

    virtual u_int32_t get_last_block_id() { return last; }

protected:
    std::string dbfilename;
    u_int32_t last;
    bool closed;
    MDB_dbi dbi;

    virtual void db_open(uint flags = 0);
};

class BTTable : public DbRelation {
public:
    BTTable(Identifier table_name, ColumnNames column_names, ColumnAttributes column_attributes);

    virtual ~BTTable() {}

    BTTable(const BTTable &other) = delete;

    BTTable(BTTable &&temp) = delete;

    BTTable &operator=(const BTTable &other) = delete;

    BTTable &operator=(BTTable &&temp) = delete;

    virtual void create();

    virtual void create_if_not_exists();

    virtual void drop();

    virtual void open();

    virtual void close();

    virtual Handle insert(const ValueDict *row);

    virtual void update(const Handle handle, const ValueDict *new_values);

    virtual void del(const Handle handle);

    virtual Handles *select();

    virtual Handles *select(const ValueDict *where);

    virtual ValueDict *project(Handle handle);

    virtual ValueDict *project(Handle handle, const ColumnNames *column_names);

	using DbRelation::project;

protected:
    BTFile file;

    virtual ValueDict *validate(const ValueDict *row);

    virtual Handle append(const ValueDict *row);

    virtual MDB_val *marshal(const ValueDict *row);

    virtual ValueDict *unmarshal(MDB_val *data);

	virtual bool selected(Handle handle, const ValueDict *where);
};

