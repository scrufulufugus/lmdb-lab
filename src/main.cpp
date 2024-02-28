#include <cstdio>
#include <cstdlib>
#include <lmdb++.h>
#include <string>

int main() {
  /* Create and open the LMDB environment: */
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 1024UL); /* 1 GiB */
  const char *home = std::getenv("HOME");
  std::string env_dir = std::string(home) + "/Projects/lmdb-lab/data/example.mdb";
  env.open(env_dir.c_str(), 0, 0664);

  /* Insert some key/value pairs in a write transaction: */
  auto wtxn = lmdb::txn::begin(env);
  auto dbi = lmdb::dbi::open(wtxn, nullptr);
  dbi.put(wtxn, "username", "jhacker");
  dbi.put(wtxn, "email", "jhacker@example.org");
  dbi.put(wtxn, "fullname", "J. Random Hacker");
  wtxn.commit();

  /* Fetch key/value pairs in a read-only transaction: */
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto cursor = lmdb::cursor::open(rtxn, dbi);
  std::string key, value;
  while (cursor.get(key, value, MDB_NEXT)) {
    std::printf("key: '%s', value: '%s'\n", key.c_str(), value.c_str());
  }
  cursor.close();
  rtxn.abort();

  /* The enviroment is closed automatically. */

  return EXIT_SUCCESS;
}

