#include "heap_storage.h"
#include "sql_shell.h"

int main() {

  std::string env_dir =
      std::string(home) + "/Projects/lmdb-lab/data/example.mdb";
  SQLShell shell;
  shell.init(env_dir);
  shell.run();

  return EXIT_SUCCESS;
}
