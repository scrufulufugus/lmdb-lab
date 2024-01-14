#include "SQLParser.h"
#include <string>
#include <iostream>

class SQLRunner
{
private:
    std::vector<std::string> tokens;

    void parse_create(const hsql::CreateStatement *statement);
    void parse_select(const hsql::SelectStatement *statement);
    void parse_datatype(const hsql::ColumnDefinition::DataType type, std::string end = "");
    void parse_join_type(const hsql::JoinType type);
    void parse_expr(hsql::Expr *expr, std::string end = "");
    void parse_table(hsql::TableRef *ref);

public:
    SQLRunner();
    ~SQLRunner();

    void execute(hsql::SQLParserResult const *result);
    void print();
};