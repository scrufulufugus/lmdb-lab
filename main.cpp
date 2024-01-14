// Will Alicar
// CPSC 5300 WQ24
// Verano - milestone 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// DB dependencies
#include "db_cxx.h"
#include "SQLParser.h" // parser
#include "sqlhelper.h" // printing utils

// This is example code from /usr/local/db6/example

// CREATE A DIRECTORY IN YOUR HOME DIR ~/cpsc5300/data before running this
const char *HOME = "cpsc5300/data";
const char *EXAMPLE = "example.db";
const unsigned int BLOCK_SZ = 4096;

void print_statement(hsql::SQLParserResult const *result);
void print_create(const hsql::CreateStatement *statement);
void print_select(const hsql::SelectStatement *statement);
void print_datatype(const hsql::ColumnDefinition::DataType type);
// default arguments should be in prototype, not definition
void print_expr(hsql::Expr *expr, std::string end = "");
void print_table(hsql::TableRef *ref);

int main(void)
{
	// TODO: might want to remove this
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
				print_statement(result);
				delete result;
			}
			else
			{
				fprintf(stderr, "Given string is not a valid SQL query.\n");
				fprintf(stderr, "%s (L%d:%d)\n",
						result->errorMsg(),
						result->errorLine(),
						result->errorColumn());
				delete result;
			}
		}
	}
	std::cout << "exited out of program\n"
			  << std::endl;

	return EXIT_SUCCESS;
}

void print_statement(hsql::SQLParserResult const *result)
{
	const hsql::SQLStatement *statement = static_cast<const hsql::SQLStatement *>(result->getStatement(0));

	switch (statement->type())
	{
	case hsql::kStmtSelect:
		print_select((const hsql::SelectStatement *)statement);
		break;
	case hsql::kStmtCreate:
		print_create((const hsql::CreateStatement *)statement);
		break;
	default:
		break;
	}
	std::cout << "\n";
	return;
}

void print_create(const hsql::CreateStatement *statement)
{
	std::cout << "CREATE TABLE " << statement->tableName << " (";

	for (hsql::ColumnDefinition *def : *statement->columns)
	{
		std::cout << def->name << " ";
		print_datatype(def->type);
		if (def != statement->columns->back())
		{
			std::cout << ", ";
		}
	}

	std::cout << ")" << std::endl;
	return;
}

void print_select(const hsql::SelectStatement *statement)
{
	hsql::printStatementInfo(statement);
	std::cout << "SELECT ";
	// selectList
	for (hsql::Expr *expr : *statement->selectList)
	{
		if (expr != statement->selectList->back())
		{
			print_expr(expr, ",");
		}
		else
		{
			print_expr(expr);
		}
	}
	// fromTable
	if (statement->fromTable)
	{
		std::cout << "FROM ";
		print_table(statement->fromTable);
	}
	// where clause, search conditions
	if (statement->whereClause)
	{	
		std::cout << "WHERE ";
		print_expr(statement->whereClause);
	}
	return;
}

void print_datatype(const hsql::ColumnDefinition::DataType type)
{
	switch (type)
	{
	case hsql::ColumnDefinition::DataType::INT:
		std::cout << "INT";
		break;
	case hsql::ColumnDefinition::DataType::DOUBLE:
		std::cout << "DOUBLE";
		break;
	case hsql::ColumnDefinition::DataType::TEXT:
		std::cout << "TEXT";
		break;
	default:
		std::cout << "UNKNOWN";
		break;
	}
	return;
}

void print_table(hsql::TableRef *ref)
{
	switch (ref->type)
	{
	case hsql::TableRefType::kTableCrossProduct:
		for (hsql::TableRef *nex_ref : *ref->list)
		{
			if (nex_ref->alias)
			{
				std::cout << nex_ref->name << " AS " << nex_ref->alias;
			}
			else
			{
				std::cout << ref->name;
			}

			if (nex_ref != ref->list->back())
			{
				std::cout << ", ";
			}
			else
			{
				std::cout << " ";
			}
		}
		break;
	case hsql::TableRefType::kTableJoin:
		std::cout << "LEFT ";
		print_table(ref->join->left);
		std::cout << "RIGHT ";
		print_table(ref->join->right);
		std::cout << "ON ";	
		print_expr(ref->join->condition);
		break;
	case hsql::TableRefType::kTableSelect:
		print_select(ref->select);
	case hsql::TableRefType::kTableName:
		if (ref->alias)
		{
			std::cout << ref->name << " AS " << ref->alias;
		}
		else
		{
			std::cout << ref->name;
		}
	default:
		break;
	}
	std::cout << " ";
	return;
}

void print_expr(hsql::Expr *expr, std::string end)
{
	switch (expr->type)
	{
	case hsql::ExprType::kExprStar:
		std::cout << "*";
		break;
	case hsql::ExprType::kExprColumnRef:
		if (expr->table)
		{
			std::cout << expr->table << "." << expr->name;
		}
		else
		{
			std::cout << expr->name;
		}
		break;
	case hsql::ExprType::kExprLiteralInt:
		std::cout << expr->ival;
		break;
	case hsql::ExprType::kExprOperator:
		print_expr(expr->expr);
		std::cout << expr->opChar << " ";
		if (expr->expr2)
		{
			print_expr(expr->expr2);
		}
		else if (expr->exprList)
		{
			for (hsql::Expr *e : *expr->exprList)
				print_expr(e);
		}
		break;
	default:
		std::cout << expr->type;
		std::cout << "?";
		break;
	}
	std::cout << end << " ";
	return;
}