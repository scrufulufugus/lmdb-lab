// Will Alicar
// CPSC 5300 WQ24
// Verano - milestone 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// DB dependencies
#include "db_cxx.h"
#include "SQLParser.h" // parser
#include "SQLRunner.h"

// This is example code from /usr/local/db6/example

// CREATE A DIRECTORY IN YOUR HOME DIR ~/cpsc5300/data before running this
const char *HOME = "cpsc5300/data";
const char *EXAMPLE = "example.db";
const unsigned int BLOCK_SZ = 4096;

int main(void)
{
	std::cout << "Have you created a dir: ~/" << HOME << "? (y/n) " << std::endl;
	std::string ans;
	getline(std::cin, ans);

	if (ans != "y")
		return 1;
	const char *home = std::getenv("HOME");
	std::string envdir = std::string(home) + "/" + HOME;

	// setup DB environment, check docs, pdf page 22
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