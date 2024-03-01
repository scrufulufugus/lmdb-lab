# lmdb
- Testing out lmdb

## Dependencies
- bison
- flex
- googletest
- lmdb
- lmdbxx
- hsql-parser

## Notes
- Ran into an issue 'llmdb.so could not be opened' -> add `$LD_LIBRARY_PATH`
- Ran into an issue with `MDB_val` constructor, it seems like the function signatures are switched?
- lmdb [forces transactions](http://www.lmdb.tech/doc/starting.html)
	- BDB doesn't require transactions to be used
	- db handle creation must take place in a transaction
- lmdb has only one access method: key-value store
	- unlike BDB where we have multiple access methods, e.g. RECNO
- lmdb uses B+ tree data structure
	- which means we can only implement a BTree internal representation
- lmdb does not want you to modify the memory inside `MDB_val`'s data
### Minor Notes
- call `mdb_env_set_mapsize` after calling `mdb_env_create` and before `mdb_env_open`
- we get `bt_ndata` stat in the BDB version even though we are using a BTree access method because it has stores the amount of records in the DB if it was set with `RECNO` access method
- view snippets of the C API being used in this [repo](https://github.com/ahupowerdns/ahutils)
- using `MDB_APPEND` will cause duplicate blocks to exist in your DB!

