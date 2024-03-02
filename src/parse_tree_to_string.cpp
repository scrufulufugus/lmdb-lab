/**
 * @file parse_tree_to_string.cpp - SQL unparsing class implementation
 * @author Kevin Lundeen
 * @see "Seattle University, CPSC5300, Winter Quarter 2024"
 */
#include "parse_tree_to_string.h"

using namespace std;
using namespace hsql;

const vector <string> parse_tree_to_string::reserved_words = {"COLUMNS", "SHOW", "TABLES", "ADD", "ALL", "ALLOCATE",
                                                           "ALTER", "AND", "ANY", "ARE", "ARRAY", "AS", "ASENSITIVE",
                                                           "ASYMMETRIC", "AT", "ATOMIC", "AUTHORIZATION", "BEGIN",
                                                           "BETWEEN", "BIGINT", "BINARY", "BLOB", "BOOLEAN", "BOTH",
                                                           "BY", "CALL", "CALLED", "CASCADED", "CASE", "CAST", "CHAR",
                                                           "CHARACTER", "CHECK", "CLOB", "CLOSE", "COLLATE", "COLUMN",
                                                           "COMMIT", "CONNECT", "CONSTRAINT", "CONTINUE",
                                                           "CORRESPONDING", "CREATE", "CROSS", "CUBE", "CURRENT",
                                                           "CURRENT_DATE", "CURRENT_DEFAULT_TRANSFORM_GROUP",
                                                           "CURRENT_PATH", "CURRENT_ROLE", "CURRENT_TIME",
                                                           "CURRENT_TIMESTAMP", "CURRENT_TRANSFORM_GROUP_FOR_TYPE",
                                                           "CURRENT_USER", "CURSOR", "CYCLE", "DATE", "DAY",
                                                           "DEALLOCATE", "DEC", "DECIMAL", "DECLARE", "DEFAULT",
                                                           "DELETE", "DEREF", "DESCRIBE", "DETERMINISTIC", "DISCONNECT",
                                                           "DISTINCT", "DOUBLE", "DROP", "DYNAMIC", "EACH", "ELEMENT",
                                                           "ELSE", "END", "END-EXEC", "ESCAPE", "EXCEPT", "EXEC",
                                                           "EXECUTE", "EXISTS", "EXTERNAL", "FALSE", "FETCH", "FILTER",
                                                           "FLOAT", "FOR", "FOREIGN", "FREE", "FROM", "FULL",
                                                           "FUNCTION", "GET", "GLOBAL", "GRANT", "GROUP", "GROUPING",
                                                           "HAVING", "HOLD", "HOUR", "IDENTITY", "IMMEDIATE", "IN",
                                                           "INDICATOR", "INNER", "INOUT", "INPUT", "INSENSITIVE",
                                                           "INSERT", "INT", "INTEGER", "INTERSECT", "INTERVAL", "INTO",
                                                           "IS", "ISOLATION", "JOIN", "LANGUAGE", "LARGE", "LATERAL",
                                                           "LEADING", "LEFT", "LIKE", "LOCAL", "LOCALTIME",
                                                           "LOCALTIMESTAMP", "MATCH", "MEMBER", "MERGE", "METHOD",
                                                           "MINUTE", "MODIFIES", "MODULE", "MONTH", "MULTISET",
                                                           "NATIONAL", "NATURAL", "NCHAR", "NCLOB", "NEW", "NO", "NONE",
                                                           "NOT", "NULL", "NUMERIC", "OF", "OLD", "ON", "ONLY", "OPEN",
                                                           "OR", "ORDER", "OUT", "OUTER", "OUTPUT", "OVER", "OVERLAPS",
                                                           "PARAMETER", "PARTITION", "PRECISION", "PREPARE", "PRIMARY",
                                                           "PROCEDURE", "RANGE", "READS", "REAL", "RECURSIVE", "REF",
                                                           "REFERENCES", "REFERENCING", "REGR_AVGX", "REGR_AVGY",
                                                           "REGR_COUNT", "REGR_INTERCEPT", "REGR_R2", "REGR_SLOPE",
                                                           "REGR_SXX", "REGR_SXY", "REGR_SYY", "RELEASE", "RESULT",
                                                           "RETURN", "RETURNS", "REVOKE", "RIGHT", "ROLLBACK", "ROLLUP",
                                                           "ROW", "ROWS", "SAVEPOINT", "SCROLL", "SEARCH", "SECOND",
                                                           "SELECT", "SENSITIVE", "SESSION_USER", "SET", "SIMILAR",
                                                           "SMALLINT", "SOME", "SPECIFIC", "SPECIFICTYPE", "SQL",
                                                           "SQLEXCEPTION", "SQLSTATE", "SQLWARNING", "START", "STATIC",
                                                           "SUBMULTISET", "SYMMETRIC", "SYSTEM", "SYSTEM_USER", "TABLE",
                                                           "THEN", "TIME", "TIMESTAMP", "TIMEZONE_HOUR",
                                                           "TIMEZONE_MINUTE", "TO", "TRAILING", "TRANSLATION", "TREAT",
                                                           "TRIGGER", "TRUE", "UESCAPE", "UNION", "UNIQUE", "UNKNOWN",
                                                           "UNNEST", "UPDATE", "UPPER", "USER", "USING", "VALUE",
                                                           "VALUES", "VAR_POP", "VAR_SAMP", "VARCHAR", "VARYING",
                                                           "WHEN", "WHENEVER", "WHERE", "WIDTH_BUCKET", "WINDOW",
                                                           "WITH", "WITHIN", "WITHOUT", "YEAR"};

bool parse_tree_to_string::is_reserved_word(string candidate) {
    for (auto const &word: reserved_words)
        if (candidate == word)
            return true;
    return false;
}

string parse_tree_to_string::operator_expression(const Expr *expr) {
    if (expr == NULL)
        return "null";

    string ret;
    if (expr->opType == OperatorType::kOpNot)
        ret += "NOT ";
    ret += expression(expr->expr) + " ";
    switch (expr->opType) {
        case OperatorType::kOpAnd:
            ret += "AND";
            break;
        case OperatorType::kOpOr:
            ret += "OR";
            break;
        default:
            ret += "???";
            break;
    }
    if (expr->expr2 != NULL)
        ret += " " + expression(expr->expr2);
    return ret;
}

string parse_tree_to_string::expression(const Expr *expr) {
    string ret;
    switch (expr->type) {
		case ExprType::kExprStar:
            ret += "*";
            break;
        case ExprType::kExprColumnRef:
            if (expr->table != NULL)
                ret += string(expr->table) + ".";
        case ExprType::kExprLiteralString:
            ret += expr->name;
            break;
        case ExprType::kExprLiteralFloat:
            ret += to_string(expr->fval);
            break;
        case ExprType::kExprLiteralInt:
            ret += to_string(expr->ival);
            break;
        case ExprType::kExprFunctionRef:
            ret += string(expr->name) + "?" + expr->expr->name;
            break;
        case ExprType::kExprOperator:
            ret += operator_expression(expr);
            break;
        default:
            ret += "???";
            break;
    }
    if (expr->alias != NULL)
        ret += string(" AS ") + expr->alias;
    return ret;
}

string parse_tree_to_string::table_ref(const TableRef *table) {
    string ret;
    switch (table->type) {
		case TableRefType::kTableSelect:
            ret += "kTableSelect FIXME"; // FIXME
            break;
        case TableRefType::kTableName:
            ret += table->name;
            if (table->alias != NULL)
                ret += string(" AS ") + table->alias->name;
            break;
        case TableRefType::kTableJoin:
            ret += table_ref(table->join->left);
            switch (table->join->type) {
				case JoinType::kJoinCross:
                case JoinType::kJoinInner:
                    ret += " JOIN ";
                    break;
                case JoinType::kJoinLeft:
                    ret += " LEFT JOIN ";
                    break;
                case JoinType::kJoinRight:
                    ret += " RIGHT JOIN ";
                    break;
                case JoinType::kJoinNatural:
                    ret += " NATURAL JOIN ";
                    break;
            }
            ret += table_ref(table->join->right);
            if (table->join->condition != NULL)
                ret += " ON " + expression(table->join->condition);
            break;
        case TableRefType::kTableCrossProduct:
            bool doComma = false;
            for (TableRef *tbl : *table->list) {
                if (doComma)
                    ret += ", ";
                ret += table_ref(tbl);
                doComma = true;
            }
            break;
    }
    return ret;
}

string parse_tree_to_string::column_definition(const ColumnDefinition *col) {
    string ret(col->name);
    switch (col->type.data_type) {
        case DataType::DOUBLE:
            ret += " DOUBLE";
            break;
        case DataType::INT:
            ret += " INT";
            break;
        case DataType::TEXT:
            ret += " TEXT";
            break;
        default:
            ret += " ...";
            break;
    }
    return ret;
}

string parse_tree_to_string::select(const SelectStatement *stmt) {
    string ret("SELECT ");
    bool doComma = false;
    for (Expr *expr : *stmt->selectList) {
        if (doComma)
            ret += ", ";
        ret += expression(expr);
        doComma = true;
    }
    ret += " FROM " + table_ref(stmt->fromTable);
    if (stmt->whereClause != NULL)
        ret += " WHERE " + expression(stmt->whereClause);
    return ret;
}

string parse_tree_to_string::insert(const InsertStatement *stmt) {
    return "INSERT ...";
}

string parse_tree_to_string::create(const CreateStatement *stmt) {
    string ret("CREATE ");
    if (stmt->type != CreateType::kCreateTable)
        return ret + "...";
    ret += "TABLE ";
    if (stmt->ifNotExists)
        ret += "IF NOT EXISTS ";
    ret += string(stmt->tableName) + " (";
    bool doComma = false;
    for (ColumnDefinition *col : *stmt->columns) {
        if (doComma)
            ret += ", ";
        ret += column_definition(col);
        doComma = true;
    }
    ret += ")";
    return ret;
}

string parse_tree_to_string::drop(const DropStatement *stmt) {
    string ret("DROP ");
    switch (stmt->type) {
        case DropType::kDropTable:
            ret += "TABLE ";
            break;
        default:
            ret += "? ";
    }
    ret += stmt->name;
    return ret;
}

string parse_tree_to_string::show(const ShowStatement *stmt) {
    string ret("SHOW ");
    switch (stmt->type) {
        case ShowType::kShowTables:
            ret += "TABLES";
            break;
        case ShowType::kShowColumns:
            ret += string("COLUMNS FROM ") + stmt->name;
            break;
        default:
            ret += "?what?";
            break;
    }
    return ret;
}

string parse_tree_to_string::statement(const SQLStatement *stmt) {
    switch (stmt->type()) {
		case StatementType::kStmtSelect:
            return select((const SelectStatement *) stmt);
        case StatementType::kStmtInsert:
            return insert((const InsertStatement *) stmt);
        case StatementType::kStmtCreate:
            return create((const CreateStatement *) stmt);
        case StatementType::kStmtDrop:
            return drop((const DropStatement *) stmt);
        case StatementType::kStmtShow:
            return show((const ShowStatement *) stmt);

        case StatementType::kStmtError:
        case StatementType::kStmtImport:
        case StatementType::kStmtUpdate:
        case StatementType::kStmtDelete:
        case StatementType::kStmtPrepare:
        case StatementType::kStmtExecute:
        case StatementType::kStmtExport:
        case StatementType::kStmtRename:
        case StatementType::kStmtAlter:
        default:
            return "Not implemented";
    }
}


