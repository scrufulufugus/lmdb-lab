#include <cstdlib>
#include "heap_storage.h"
#include "sql_shell.h"

int main() {

  const char *home = std::getenv("HOME");
  std::string env_dir =
      std::string(home) + "/Projects/lmdb-lab/data/example.mdb";
  SQLShell shell;
  shell.init(env_dir.c_str());
  shell.run();

  return EXIT_SUCCESS;
}
