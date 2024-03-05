#include <cstdlib>
#include <iostream>
#include "heap_storage.h"
#include "sql_shell.h"

int main(int argc, char *argv[]) {

  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " dbenvpath" << std::endl;
    return EXIT_FAILURE;
  }

  SQLShell shell;
  shell.init(argv[1]);
  shell.run();

  return EXIT_SUCCESS;
}
