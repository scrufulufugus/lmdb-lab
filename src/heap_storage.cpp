#include "heap_storage.h"
#include "storage_engine.h"

MDB_env *_MDB_ENV = nullptr;

//// SlottedPage
// public

SlottedPage::SlottedPage(MDB_val &block, BlockID block_id, bool is_new)
    : DbBlock(block, block_id, is_new) {
  if (is_new) {
    this->num_records = 0;
    this->end_free = DbBlock::BLOCK_SZ - 1;
    put_header();
  } else {
    get_header(this->num_records, this->end_free);
  }
};

SlottedPage::SlottedPage(const SlottedPage &other)
	: DbBlock(other)
{
	num_records = other.num_records;
	end_free = other.end_free;
}

RecordID SlottedPage::add(const MDB_val *data) {
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
MDB_val *SlottedPage::get(RecordID record_id) {
  u_int16_t size;
  u_int16_t loc;
  this->get_header(size, loc, record_id);
  if (loc == 0) {
    return nullptr;
  }
  return new MDB_val(size, this->address(loc));
}

// Replace the data at record_id with new MDB_val
void SlottedPage::put(RecordID record_id, const MDB_val &data) {
  u_int16_t size;
  u_int16_t loc;
  this->get_header(size, loc, record_id);
  u_int16_t new_size = data.mv_size;
  if (new_size > size) {
    int extra = new_size - size;
    if (!this->has_room(extra)) {
      throw "Not enough room in block";
    }
    // OLD: this->slide(loc + new_size, loc + size);
    this->slide(loc, loc - extra);
    // OLD: memmove(this->address(loc - extra), data.get_data(), new_size);
    memcpy(this->address(loc - extra), data.mv_data, (extra + size));
  } else {
    memcpy(this->address(loc), data.mv_data, new_size);
    this->slide(loc + new_size, loc + size);
  }
  this->get_header(size, loc, record_id);
  this->put_header(record_id, new_size, loc);
}

// Remove data at record_id
void SlottedPage::del(RecordID record_id) {
  u_int16_t size;
  u_int16_t loc;

  this->get_header(size, loc, record_id);
  this->slide(loc, loc + size); // ?
  this->put_header(record_id, 0, 0);
}

// Get existing record_ids in SlottedPage, make sure to deallocate
RecordIDs *SlottedPage::ids(void) {
  RecordIDs *record_ids = new RecordIDs();
  for (RecordID id = 1; id <= this->num_records; id++) {
    u_int16_t size;
    u_int16_t loc;
    this->get_header(size, loc, id);
    if (loc != 0) {
      record_ids->push_back(id);
    }
  }
  return record_ids;
}

// protected
// Check if SlottedPage has room
bool SlottedPage::has_room(u_int16_t size) {
  u_int16_t available = this->end_free - (this->num_records + 2) * 4;
  return size <= available;
};

void SlottedPage::slide(u_int16_t start, u_int16_t end) {
    int shift = end - start;
    if (shift == 0)
        return;

    // slide data
    void *to = this->address((uint16_t) (this->end_free + 1 + shift));
    void *from = this->address((uint16_t) (this->end_free + 1));
    int bytes = start - (this->end_free + 1U);
    memmove(to, from, bytes);

    // fix up headers to the right
    RecordIDs *record_ids = ids();
    for (auto const &record_id : *record_ids) {
        uint16_t size, loc;
        get_header(size, loc, record_id);
        if (loc <= start) {
            loc += shift;
            put_header(record_id, size, loc);
        }
    }
    delete record_ids;
    this->end_free += shift;
    put_header();
}

// Get 2-byte integer at given offset in block.
u_int16_t SlottedPage::get_n(u_int16_t offset) {
  return *(u_int16_t *)this->address(offset);
}

// Put a 2-byte integer at given offset in block.
void SlottedPage::put_n(u_int16_t offset, u_int16_t n) {
  *(u_int16_t *)this->address(offset) = n;
}

// offset = # bytes to move
void *SlottedPage::address(u_int16_t offset) {
  return (void *)((char *)this->block.mv_data + offset);
}

// Store the size and offset for given id. For id of zero, store the block
// header.
void SlottedPage::put_header(RecordID id, u_int16_t size, u_int16_t loc) {
  if (id == 0) { // called the put_header() version and using the default params
    size = this->num_records;
    loc = this->end_free;
  }
  put_n(4 * id, size);
  put_n(4 * id + 2, loc);
}

// Get size and offset for record_id
void SlottedPage::get_header(u_int16_t &size, u_int16_t &loc, RecordID id) {
  // headers are 4 bytes in size
  size = get_n(4 * id);
  loc = get_n(4 * id + 2);
};

//// BTFile
// public

// Create a new block
void BTFile::create(void) {
  this->db_open(MDB_CREATE);
  SlottedPage *block = this->get_new();
  this->put(block);
  delete block;
};

// Drop this current file, remove the db file
void BTFile::drop(void) {
  this->close();
  std::remove(this->dbfilename.c_str());
};

// Open current file
void BTFile::open(void) { this->db_open(); }; // Create the named databse if it doesn't exist

// Close current file
void BTFile::close(void) {

  MDB_txn *txn = nullptr;
  mdb_txn_begin(_MDB_ENV, nullptr, 0, &txn);
  mdb_dbi_close(_MDB_ENV, this->dbi);
  mdb_txn_commit(txn);

  this->closed = true;
};

// Allocate a new block and give it a block_id, make sure to deallocate
SlottedPage *BTFile::get_new(void) {
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
SlottedPage *BTFile::get(BlockID block_id) {
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
void BTFile::put(DbBlock *block) {
  BlockID block_id(block->get_block_id());
  MDB_val key(sizeof(BlockID), &block_id);
  MDB_val data(DbBlock::BLOCK_SZ, block->get_data());

  MDB_txn *txn = nullptr;
  mdb_txn_begin(_MDB_ENV, nullptr, 0, &txn);
  mdb_put(txn, this->dbi, &key, &data, 0); // Maybe use MDB_append here?
  mdb_txn_commit(txn);
};

// Get existing block_ids in the file, make sure to deallocate
BlockIDs *BTFile::block_ids() {
  BlockIDs *block_ids = new BlockIDs;
  for (u_int32_t i = 1; i <= this->last; i++) {
    block_ids->push_back(i);
  }
  return block_ids;
};

// protected
void BTFile::db_open(uint flags) {
  if (!this->closed)
    return;
	
  const char *path;
  mdb_env_get_path(_MDB_ENV, &path);
  dbfilename = path + name + ".mdb";

  // make TXN
  MDB_txn *txn = nullptr;
  mdb_txn_begin(_MDB_ENV, nullptr, 0, &txn);

  // open dbi
  int status = mdb_dbi_open(txn, dbfilename.c_str(), flags, &dbi);

  if (status) {
		if (status == MDB_NOTFOUND) {
			mdb_txn_abort(txn);
			throw DbException(status, std::generic_category(), "FILE DOES NOT EXIST");
		}
	}

  // get stats
  MDB_stat stats;
  mdb_stat(txn, dbi, &stats);
  last = stats.ms_entries;

  // clean up
  mdb_txn_commit(txn);
  this->closed = false;
};

//// BTTable
// public
BTTable::BTTable(Identifier table_name, ColumnNames column_names,
                     ColumnAttributes column_attributes)
    : DbRelation(table_name, column_names, column_attributes), file(table_name){

                                                               };

void BTTable::create() { this->file.create(); }

void BTTable::create_if_not_exists() { 
	try {
		this->file.open(); 
	} catch (DbException &e) {
		this->file.create();
	}
}

void BTTable::open() { this->file.open(); }

void BTTable::close() { this->file.close(); }

void BTTable::drop() { this->file.drop(); }

Handle BTTable::insert(const ValueDict *row) {
  this->open();
  return this->append(validate(row));
}

// not required for Milestone 2
void BTTable::update(const Handle handle, const ValueDict *new_values){};

void BTTable::del(const Handle handle){
	BlockID block_id = handle.first;
	RecordID record_id = handle.second;
	SlottedPage *block = this->file.get(block_id);
	SlottedPage block_copy(*block);

	block_copy.del(record_id);
	this->file.put(&block_copy);
	// delete block?
};

// Select all, return existing handles in this table
Handles *BTTable::select() {
  Handles *handles = new Handles();
  BlockIDs *block_ids = file.block_ids();
  for (auto const &block_id : *block_ids) {
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

// not required for Milestone 2
Handles *BTTable::select(const ValueDict *where) {
  Handles *handles = new Handles();
  BlockIDs *block_ids = file.block_ids();
  for (auto const &block_id : *block_ids) {
    SlottedPage *block = file.get(block_id);
    RecordIDs *record_ids = block->ids();
    for (auto const &record_id : *record_ids) {
		Handle handle(block_id, record_id);
	    if (selected(handle, where))
      		handles->push_back(Handle(block_id, record_id));
	}
    delete record_ids;
    delete block;
  }
  delete block_ids;
  return handles;
}

// Display the row with the associated handle
ValueDict *BTTable::project(Handle handle) {
  return this->project(handle, &this->column_names);
};

// Display the row with the associated handle and its column names
ValueDict *BTTable::project(Handle handle, const ColumnNames *column_names) {
  BlockID block_id = handle.first;
  RecordID record_id = handle.second;
  SlottedPage *page = this->file.get(block_id);
  ValueDict *rows = unmarshal(page->get(record_id));
  ValueDict *p_rows = new ValueDict();
  for (const auto &column_name : *column_names) {
    if (rows->find(column_name) != rows->end()) {
      (*p_rows)[column_name] = (*rows)[column_name];
    }
  }
  return p_rows;
};

// protected
// Check if row can be inserted, throw exception if unable to
ValueDict *BTTable::validate(const ValueDict *row) {
  ValueDict *full_row = new ValueDict;
  for (const auto &column_name : this->column_names) {
    if (row->find(column_name) == row->end()) {
      throw std::invalid_argument("can't handle this type");
    } else {
      ValueDict::const_iterator item = row->find(column_name);
      (*full_row)[column_name] = item->second;
    }
  }
  return full_row;
};

// Add a new row to the file
Handle BTTable::append(const ValueDict *row) {
  RecordID record_id;
  BlockID block_id = this->file.get_last_block_id();
  MDB_val *data = marshal(row); // row we want to insert
								//
  SlottedPage *page = this->file.get(block_id); // we can't modify the block directly
  SlottedPage page_copy(*page);

  page_copy.ids();
  try {
    record_id = page_copy.add(data);
  } catch (const DbBlockNoRoomError &e) { // FIXME: implement me
	throw DbException(404, std::generic_category(), "BLOCK OVER FILL NOT IMPLEMENTED!");
    // page_copy = this->file.get_new(); 
    // record_id = page_copy.add(data);
  }
  page_copy.ids();
  this->file.put(&page_copy);
  return {block_id, record_id};
};

bool BTTable::selected(Handle handle, const ValueDict *where) {
	if (where == nullptr)
		return true;
	ValueDict *row = this->project(handle, where);
	bool is_selected = *row == *where;
	delete row;
	return is_selected;
}

MDB_val *BTTable::marshal(const ValueDict *row) {
  char *bytes =
      new char[DbBlock::BLOCK_SZ]; // more than we need (we insist that one row
                                   // fits into DbBlock::BLOCK_SZ)
  uint offset = 0;
  uint col_num = 0;
  for (auto const &column_name : this->column_names) {
    ColumnAttribute ca = this->column_attributes[col_num++];
    ValueDict::const_iterator column = row->find(column_name);
    Value value = column->second;
    if (ca.get_data_type() == ColumnAttribute::DataType::INT) {
      *(int32_t *)(bytes + offset) = value.n;
      offset += sizeof(int32_t);
    } else if (ca.get_data_type() == ColumnAttribute::DataType::TEXT) {
      uint size = value.s.length();
      *(u_int16_t *)(bytes + offset) = size;
      offset += sizeof(u_int16_t);
      memcpy(bytes + offset, value.s.c_str(), size); // assume ascii for now
      offset += size;
    } else {
      throw DbRelationError("Only know how to marshal INT and TEXT");
    }
  }
  char *right_size_bytes = new char[offset];
  memcpy(right_size_bytes, bytes, offset);
  delete[] bytes;
  MDB_val *data = new MDB_val(offset, right_size_bytes);
  return data;
}

ValueDict *BTTable::unmarshal(MDB_val *data) {
  ValueDict *row = new ValueDict();
  uint offset = 0;
  uint col_num = 0;
  char *bytes = (char *)data->mv_data;
  for (auto const &column_name : this->column_names) {
    ColumnAttribute ca = this->column_attributes[col_num++];
    if (ca.get_data_type() == ColumnAttribute::DataType::INT) {
      (*row)[column_name].n = *(int32_t *)(bytes + offset);
      offset += sizeof(int32_t);
    } else if (ca.get_data_type() == ColumnAttribute::DataType::TEXT) {
      u_int16_t size;
      memcpy(&size, bytes + offset, sizeof(u_int16_t));
      offset += sizeof(u_int16_t);
      std::string text(bytes + offset, size);
      (*row)[column_name].s = text;
      offset += size;
    } else {
      throw DbRelationError("Only know how to unmarshal INT and TEXT");
    }
  }
  return row;
};
