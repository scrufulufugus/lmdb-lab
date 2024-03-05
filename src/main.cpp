// Will Alicar
// CPSC 5300 WQ24
// Verano - milestone 2
// --------------------
// Note: The program searches for folder inside ~/
// Example Usage: ./sql5300 cpsc5300/data

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <cstdio>
#include <cstdlib>

// DB dependencies
#include "db_cxx.h"
#include "SQLParser.h" // parser
#include "SQLRunner.h"
#include <benchmark.h>

#include "heap_storage.h"
#include "storage_engine.h"

bool test_heap_storage();

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		std::cout << "BAD INPUT! USAGE: ./sql5300 <directory>\n";
		return -1;
	}

  std::cout << "Opening db " << argv[1] << std::endl;
  std::string envdir = argv[1];

	// setup DB environment
	DbEnv env(0U);
	env.set_message_stream(&std::cout);
	env.set_error_stream(&std::cerr);
	env.open(envdir.c_str(), DB_CREATE | DB_INIT_MPOOL, 0);
	_DB_ENV = &env;

	// start loop
	bool running = true;
	while (running)
	{
		// get input
		std::string input;
		std::cout << "SQL> ";
		getline(std::cin, input);

		if (input == "quit")
		{
			running = false;
		}
    else if (input == "benchmark") {
      Benchmark::run();
    }
		else if (input == "test")
		{
			std::cout << "Testing..." << std::endl;
			bool check = test_heap_storage();
			if (check)
			{
				std::cout << "TEST PASSED" << std::endl;
			}
			else
			{
				std::cout << "TEST FAILED" << std::endl;
			}
		}
		else
		{
			// parse statement
			hsql::SQLParserResult *result = hsql::SQLParser::parseSQLString(input);
			if (result->isValid())
			{
				SQLRunner runner;
				runner.execute(result);
				runner.print();
				delete result;
			}
			else
			{
				std::cout << "Invalid SQL: " << input << "\n";
				delete result;
			}
		}
	}
	std::cout << "exited out of program\n";

	return EXIT_SUCCESS;
}

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
	table1.drop(); // drop makes the object unusable because of BerkeleyDB restriction -- maybe want to fix this some day
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

	int res = std::system("rm -f ../data/__*"); // remove the .db files in _DB_ENV home
	if (res != 0)
	{
		std::cout << "\n!!! Failed to remove .db files in /data, manually remove it before running the test again !!!\n";
	}
	std::cout << "Removed .db files in ../data\n";
	delete handles;
    delete result;

	return true;
}
