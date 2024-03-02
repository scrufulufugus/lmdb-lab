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

        SQLParserResult *parser_result = SQLParser::parseSQLString(query);
        SQLExec *sql_exec = new SQLExec();

        if (parser_result->isValid()) {
            for (uint i = 0; i < parser_result->size(); ++i)
            {
                // First, echo the command entered by the user
                std::cout << this->echo(parser_result->getStatement(i)) << std::endl;

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

string SQLShell::echo(const hsql::SQLStatement *stmt) {
    stringstream ss;
    if (stmt->type() == kStmtSelect) {
        ss << "SELECT ";
        int count = ((SelectStatement *)stmt)->selectList->size();
        for (Expr *expr : *((SelectStatement *)stmt)->selectList) {
            this->printExpression(expr, ss);
            if (--count) {
                ss << ",";
            }
            ss << " ";
        }

        ss << "FROM ";

        this->printTableRefInfo(((SelectStatement *)stmt)->fromTable, ss);

        if (((SelectStatement *)stmt)->whereClause != NULL) {
            ss << " WHERE ";
            this->printExpression(((SelectStatement *)stmt)->whereClause, ss);
        }
    }

    if (stmt->type() == kStmtCreate) {
        ss << "CREATE TABLE " << ((CreateStatement *)stmt)->tableName;

        ss << " (";
        int count = ((CreateStatement *)stmt)->columns->size();
        for (auto col : *((CreateStatement *)stmt)->columns) {
            ss << this->columnDefinitionToString(col);
            if (--count) {
                ss << ", ";
            }
        }
        ss << ")";
    }

    if (stmt->type() == kStmtShow) {
        ss << "SHOW ";
        ShowStatement::EntityType type = ((ShowStatement *)stmt)->type;
        switch(type) {
            case ShowStatement::EntityType::kTables:
                ss << "TABLES";
                break;
            case ShowStatement::EntityType::kColumns:
                ss << "COLUMNS FROM " << ((ShowStatement *)stmt)->tableName;
            default:
                break;
        }
    }

    return ss.str();
}


void SQLShell::printExpression(Expr *expr, stringstream &ss) {
    switch (expr->type) {
        case kExprStar:
            ss << "*";
            break;
        case kExprColumnRef:
            if (expr->hasTable()) {
                ss << expr->table << ".";
            }
            ss << expr->name;
            break;
        case kExprLiteralInt:
            ss << expr->ival;
            break;
        case kExprLiteralString:
            ss << expr->name;
            break;
        case kExprOperator:
            this->printExpression(expr->expr, ss);
            switch (expr->opType) {
                case Expr::SIMPLE_OP:
                    ss << " " << expr->opChar << " ";
                    break;
                default:
                    ss << expr->opType;
                    break;
            }
            if (expr->expr2 != NULL) this->printExpression(expr->expr2, ss);
            break;
        default:
            fprintf(stderr, "Unrecognized expression type %d\n", expr->type);
            return;
    }
}

void SQLShell::printTableRefInfo(TableRef *table, stringstream &ss) {
    switch (table->type) {
        case kTableSelect:
            break;
        case kTableName:
            ss << table->name;
            break;
        case kTableJoin:
            this->printTableRefInfo(table->join->left, ss);
            if (table->join->type == kJoinLeft) {
                ss << " LEFT";
            }
            ss << " JOIN ";
            this->printTableRefInfo(table->join->right, ss);
            ss << " ON ";
            printExpression(table->join->condition, ss);
            break;
        case kTableCrossProduct:
            int count = table->list->size();
            for (TableRef *tbl : *table->list) {
                this->printTableRefInfo(tbl, ss);
                if (--count) {
                    ss << ", ";
                }
            }
            break;
    }
    if (table->alias != NULL) {
        ss << " AS " << table->alias;
    }
}

// remove?
string SQLShell::columnDefinitionToString(const ColumnDefinition *col) {
    string ret(col->name);
    switch (col->type) {
        case ColumnDefinition::DOUBLE:
            ret += " DOUBLE";
            break;
        case ColumnDefinition::INT:
            ret += " INT";
            break;
        case ColumnDefinition::TEXT:
            ret += " TEXT";
            break;
        default:
            ret += " ...";
            break;
    }
    return ret;
}

void SQLShell::testParseSQLQuery(string query, string expected) {
    SQLParserResult *result = SQLParser::parseSQLString(query);
    if (!result->isValid()) {
        printf("SQL> %s\n", query.c_str());
        printf("invalid SQL: %s\n", query.c_str());
    } else {
        SQLExec *sql_exec = new SQLExec();
        for (uint i = 0; i < result->size(); ++i) {
            QueryResult *query_result = sql_exec->execute(result->getStatement(i));
            printf("SQL> %s\n", query.c_str());
            printf(">>>> %s\n", query_result->get_message().c_str());
            if (query_result->get_message() != expected) printf("TEST FAILED\n");
        }
    }
    delete result;
}

// Tests are broken for now
void SQLShell::testSQLParser() {
    string query;
    string expected;
    printf("Testing SQLShell needs to be implemented with new tests!\n");
}
