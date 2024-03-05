/**
 * @file sql_shell.cpp - Implementation of SQL Shell to initialize database
 * enviroment, accept user input and execute SQL commands
 * @author Duc Vo
 *
 * @see "Seattle University, CPSC 5300, Winter Quarter 2024"
 */
#include "sql_shell.h"
#include "sql_exec.h"
#include "heap_storage.h"
#include "benchmark.h"

#include <stdio.h>

#include <iostream>
#include <sstream>
#include <string>

using namespace std;
using namespace hsql;

bool SQLShell::initialized = false;

void SQLShell::init(const char *envHome) {
    if (this->initialized) {
        cerr << "(database environment already initialized) \n";
        return;
    };
	
	MDB_env *env;
	mdb_env_create(&env);
	mdb_env_set_mapsize(env, 1UL * 1024UL * 1024UL * 1024UL); // 1Gb
	mdb_env_set_maxdbs(env, 5);
	int status = mdb_env_open(env, envHome, 0, 0664); // unlike in BDB, we can't pass in DB_CREATE
	_MDB_ENV = env;

	if (status) {
		cerr << "(bad status code: " << status << " )\n";
		exit(1);
	}

    _MDB_ENV = env;
    initialize_schema_tables();
    this->initialized = true;
    printf("(running with database environment at %s)\n", envHome);
}

void SQLShell::run() {
    while (true) {
        printf("SQL> ");
        string query;
        getline(cin, query);
        if (query.length() == 0) continue;
        if (query == "quit") break;
        if (query == "benchmark") Benchmark::run();

        SQLParserResult *parser_result = new SQLParserResult();
		bool is_valid = SQLParser::parseSQLString(query, parser_result);

        SQLExec *sql_exec = new SQLExec();
        if (is_valid) {
            for (uint i = 0; i < parser_result->size(); ++i)
            {
                try {
                    // Now, execute it using SQLExec
                    QueryResult *query_result = sql_exec->execute(parser_result->getStatement(i));
                    std::cout << *query_result;
                    delete query_result;
                } catch (SQLExecError &e) {
                    cout << "Error: " << e.what() << endl;
                }
            }
        } else {
            printf("Invalid SQL: %s\n", query.c_str());
        }
        delete parser_result;
        delete sql_exec;
    }
}

