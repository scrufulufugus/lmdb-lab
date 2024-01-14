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

void parse_statement(hsql::SQLParserResult const *result, std::vector<std::string> &tokens);
void parse_create(const hsql::CreateStatement *statement, std::vector<std::string> &tokens);
void parse_select(const hsql::SelectStatement *statement, std::vector<std::string> &tokens);
void parse_datatype(const hsql::ColumnDefinition::DataType type, std::vector<std::string> &tokens, std::string end = "");
void parse_join_type(const hsql::JoinType type, std::vector<std::string> &tokens);
void parse_expr(hsql::Expr *expr, std::vector<std::string> &tokens, std::string end = "");
void parse_table(hsql::TableRef *ref, std::vector<std::string> &tokens);
void print_tokens(std::vector<std::string> &tokens);

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
				std::vector<std::string> tokens;
				parse_statement(result, tokens);
				print_tokens(tokens);
				delete result;
			}
			else
			{
				std::cout << "Invalid SQL: " << input << "\n";
				delete result;
			}
		}
	}
	std::cout << "exited out of program\n"
			  << std::endl;

	return EXIT_SUCCESS;
}

void parse_statement(hsql::SQLParserResult const *result, std::vector<std::string> &tokens)
{
	const hsql::SQLStatement *statement = static_cast<const hsql::SQLStatement *>(result->getStatement(0));
	printStatementInfo(statement);
	switch (statement->type())
	{
	case hsql::kStmtSelect:
		parse_select((const hsql::SelectStatement *)statement, tokens);
		break;
	case hsql::kStmtCreate:
		parse_create((const hsql::CreateStatement *)statement, tokens);
		break;
	default:
		break;
	}
	return;
}

void parse_create(const hsql::CreateStatement *statement, std::vector<std::string> &tokens)
{
	tokens.push_back("CREATE");
	tokens.push_back("TABLE");
	tokens.push_back("(");
	for (hsql::ColumnDefinition *def : *statement->columns)
	{
		tokens.push_back(def->name);
		if (def != statement->columns->back())
		{
			parse_datatype(def->type, tokens, ",");
		}
		else
		{
			parse_datatype(def->type, tokens);
		}
	}

	tokens.push_back(")");
	return;
}

void parse_select(const hsql::SelectStatement *statement, std::vector<std::string> &tokens)
{
	tokens.push_back("SELECT");
	for (hsql::Expr *expr : *statement->selectList)
	{
		if (expr != statement->selectList->back())
		{
			parse_expr(expr, tokens, ",");
		}
		else
		{
			parse_expr(expr, tokens);
		}
	}

	if (statement->fromTable)
	{
		tokens.push_back("FROM");
		parse_table(statement->fromTable, tokens);
	}

	if (statement->whereClause)
	{
		tokens.push_back("WHERE");
		parse_expr(statement->whereClause, tokens);
	}
	return;
}

void parse_datatype(const hsql::ColumnDefinition::DataType type, std::vector<std::string> &tokens, std::string end)
{
	std::string result = "";
	switch (type)
	{
	case hsql::ColumnDefinition::DataType::INT:
		result = "INT";
		break;
	case hsql::ColumnDefinition::DataType::DOUBLE:
		result = "DOUBLE";
		break;
	case hsql::ColumnDefinition::DataType::TEXT:
		result = "TEXT";
		break;
	default:
		result = "UNKNOWN";
		break;
	}
	tokens.push_back(result);
	tokens.back() += end;
	return;
}

void parse_table(hsql::TableRef *ref, std::vector<std::string> &tokens)
{
	switch (ref->type)
	{
	case hsql::TableRefType::kTableCrossProduct:
		std::reverse(ref->list->begin(), ref->list->end());
		for (hsql::TableRef *nex_ref : *ref->list)
		{
			std::string token = "";
			if (nex_ref->alias)
			{
				std::string name(nex_ref->name);
				std::string alias(nex_ref->alias);
				token += name + " AS " + alias;
			}
			else
			{
				std::string name(nex_ref->name);
				token += name;
			}

			if (nex_ref != ref->list->back())
			{
				token += ",";
			}
			tokens.push_back(token);
		}
		break;
	case hsql::TableRefType::kTableJoin:
		parse_table(ref->join->left, tokens);
		parse_join_type(ref->join->type, tokens);
		parse_table(ref->join->right, tokens);
		tokens.push_back("ON");
		parse_expr(ref->join->condition, tokens);
		break;
	case hsql::TableRefType::kTableSelect:
		parse_select(ref->select, tokens);
	case hsql::TableRefType::kTableName:
		if (ref->alias)
		{
			tokens.push_back(ref->name);
			tokens.push_back("AS");
			tokens.push_back(ref->alias);
		}
		else
		{
			tokens.push_back(ref->name);
		}
	default:
		break;
	}
	return;
}

void parse_expr(hsql::Expr *expr, std::vector<std::string> &tokens, std::string end)
{
	switch (expr->type)
	{
	case hsql::ExprType::kExprStar:
		tokens.push_back("*");
		break;
	case hsql::ExprType::kExprColumnRef:
	{
		std::string column = "";
		if (expr->table)
		{
			std::string table(expr->table);
			std::string name(expr->name);
			column += table + "." + name;
		}
		else
		{
			std::string name(expr->name);
			column += name;
		}
		tokens.push_back(column);
		break;
	}
	case hsql::ExprType::kExprLiteralInt:
		tokens.push_back(std::to_string(expr->ival));
		break;
	case hsql::ExprType::kExprOperator:
	{
		parse_expr(expr->expr, tokens);
		std::string op(1, expr->opChar);
		tokens.push_back(op);
		if (expr->expr2)
		{
			parse_expr(expr->expr2, tokens);
		}
		else if (expr->exprList)
		{
			for (hsql::Expr *e : *expr->exprList)
				parse_expr(e, tokens);
		}
		break;
	}
	default:
		tokens.push_back("MISSING_TYPE>");
		tokens.push_back(std::to_string(expr->type));
		break;
	}
	tokens.back() += end;
	return;
}

void parse_join_type(const hsql::JoinType type, std::vector<std::string> &tokens)
{
	switch (type)
	{
	case hsql::JoinType::kJoinLeft:
		tokens.push_back("LEFT");
		break;
	case hsql::JoinType::kJoinRight:
		tokens.push_back("RIGHT");
		break;
	default:
		break;
	}
	tokens.push_back("JOIN");
	return;
}

void print_tokens(std::vector<std::string> &tokens)
{
	for (std::string &token : tokens)
	{
		std::cout << token << " ";
	}
	std::cout << "\n";
}