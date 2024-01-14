#include "SQLRunner.h"

// Public

SQLRunner::SQLRunner() { return; };
SQLRunner::~SQLRunner() { return; };

void SQLRunner::execute(hsql::SQLParserResult const *result)
{
	const hsql::SQLStatement *statement = static_cast<const hsql::SQLStatement *>(result->getStatement(0));
	switch (statement->type())
	{
	case hsql::kStmtSelect:
		parse_select((const hsql::SelectStatement *)statement);
		break;
	case hsql::kStmtCreate:
		parse_create((const hsql::CreateStatement *)statement);
		break;
	default:
		break;
	}
	return;
}

void SQLRunner::print()
{
	for (std::string &token : tokens)
	{
		std::cout << token << " ";
	}
	std::cout << "\n";
}

// Private

void SQLRunner::parse_create(const hsql::CreateStatement *statement)
{
	tokens.push_back("CREATE");
	tokens.push_back("TABLE");
	tokens.push_back("(");
	for (hsql::ColumnDefinition *def : *statement->columns)
	{
		tokens.push_back(def->name);
		if (def != statement->columns->back())
		{
			parse_datatype(def->type, ",");
		}
		else
		{
			parse_datatype(def->type);
		}
	}

	tokens.push_back(")");
	return;
}

void SQLRunner::parse_select(const hsql::SelectStatement *statement)
{
	tokens.push_back("SELECT");
	for (hsql::Expr *expr : *statement->selectList)
	{
		if (expr != statement->selectList->back())
		{
			parse_expr(expr, ",");
		}
		else
		{
			parse_expr(expr);
		}
	}

	if (statement->fromTable)
	{
		tokens.push_back("FROM");
		parse_table(statement->fromTable);
	}

	if (statement->whereClause)
	{
		tokens.push_back("WHERE");
		parse_expr(statement->whereClause);
	}
	return;
}

void SQLRunner::parse_datatype(const hsql::ColumnDefinition::DataType type, std::string end)
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

void SQLRunner::parse_table(hsql::TableRef *ref)
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
		parse_table(ref->join->left);
		parse_join_type(ref->join->type);
		parse_table(ref->join->right);
		tokens.push_back("ON");
		parse_expr(ref->join->condition);
		break;
	case hsql::TableRefType::kTableSelect:
		parse_select(ref->select);
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

void SQLRunner::parse_expr(hsql::Expr *expr, std::string end)
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
		parse_expr(expr->expr);
		std::string op(1, expr->opChar);
		tokens.push_back(op);
		if (expr->expr2)
		{
			parse_expr(expr->expr2);
		}
		else if (expr->exprList)
		{
			for (hsql::Expr *e : *expr->exprList)
				parse_expr(e);
		}
		break;
	}
	default:
		tokens.push_back("MISSING_TYPE =>");
		tokens.push_back(std::to_string(expr->type));
		break;
	}
	tokens.back() += end;
	return;
}

void SQLRunner::parse_join_type(const hsql::JoinType type)
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
