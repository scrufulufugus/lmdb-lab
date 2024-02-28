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

