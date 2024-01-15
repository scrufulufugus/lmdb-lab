// Will Alicar
// CPSC 5300 WQ24
// Verano - milestone 1
// --------------------
// Note: The program searches for folder inside ~/
// Example Usage: ./sql5300 cpsc5300/data

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// DB dependencies
#include "db_cxx.h"
#include "SQLParser.h" // parser
#include "SQLRunner.h"

int main(int argc, char *argv[])
{
	if (argc != 2) {
		std::cout << "BAD INPUT! USAGE: ./sql5300 <directory>\n";
		return 1;
	}

	const char *home = std::getenv("HOME");
	std::string envdir = std::string(home) + "/" + argv[1];

	// setup DB environment
	DbEnv env(0U);
	env.set_message_stream(&std::cout);
	env.set_error_stream(&std::cerr);
	env.open(envdir.c_str(), DB_CREATE, 0);

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