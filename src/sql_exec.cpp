/**
 * @file SQLExec.cpp - implementation of SQLExec class
 * @author Kevin Lundeen
 * @see "Seattle University, CPSC5300, Winter Quarter 2024"
 */
#include "sql_exec.h"

using namespace std;
using namespace hsql;

// define static data
Tables *SQLExec::tables = nullptr;

// make query result be printable
ostream &operator<<(ostream &out, const QueryResult &qres) {
    if (qres.column_names != nullptr) {
        for (auto const &column_name: *qres.column_names)
            out << column_name << " ";
        out << endl << "+";
        for (unsigned int i = 0; i < qres.column_names->size(); i++)
            out << "----------+";
        out << endl;
        for (auto const &row: *qres.rows) {
            for (auto const &column_name: *qres.column_names) {
                Value value = row->at(column_name);
                switch (value.data_type) {
                    case ColumnAttribute::INT:  out << value.n;                 break;
                    case ColumnAttribute::TEXT: out << "\"" << value.s << "\""; break;
                    default:                    out << "???";
                }
                out << " ";
            }
            out << endl;
        }
    }
    out << qres.message << "\n";
    return out;
}

QueryResult::~QueryResult() {
    delete column_names;
    delete column_attributes;
    delete rows;
}


QueryResult *SQLExec::execute(const SQLStatement *statement) {
    // FIXED: initialize _tables table, if not yet present
    if (!tables) {
        tables = new Tables();
        tables->open();
    }
    try {
        switch (statement->type()) {
            case kStmtCreate:   return create((const CreateStatement *) statement);
            case kStmtDrop:     return drop((const DropStatement *) statement);
            case kStmtShow:     return show((const ShowStatement *) statement);
            default:            return new QueryResult("not implemented");
        }
    } catch (DbRelationError &e) {
        throw SQLExecError(string("DbRelationError: ") + e.what());
    }
}

void
SQLExec::column_definition(const ColumnDefinition *col, Identifier &column_name, ColumnAttribute &column_attribute) {
    column_name = std::string(col->name);
    switch (col->type.data_type) {
        case DataType::INT:     column_attribute.set_data_type(ColumnAttribute::INT);   break;
        case DataType::TEXT:    column_attribute.set_data_type(ColumnAttribute::TEXT);  break;
        default:                        throw SQLExecError("Unknown column type");
    }
}

QueryResult *SQLExec::create(const CreateStatement *statement) {

    // Add table to _tables
    string table_name = string(statement->tableName);
    ValueDict table_record = {{"table_name", Value(table_name)}};
    Handle table_handle;
    try {
        table_handle = tables->insert(&table_record);
    } catch (DbRelationError &e) {
        throw e;
    }

    // Add columns to _columns
    DbRelation &columns_table = tables->get_table(Columns::TABLE_NAME);
    Handles *column_handles = new Handles();
    try {
        int loop_count = 0;
        for (auto const &column : *statement->columns) {
            string type;
            if (column->type == DataType::INT)
                type = "INT";
            else if (column->type == DataType::TEXT) {
                type = "TEXT";
            } else {
                throw DbRelationError("Bad type");
            }

            ValueDict row;
            row["table_name"] = Value(table_name);
            row["column_name"] = Value(column->name);
            row["data_type"] = Value(type);
            Handle column_handle = columns_table.insert(&row);
            column_handles->push_back(column_handle);
        }
    } catch (DbRelationError &e) {
        // Something prevented adding columns to _columns,
        // must remove table, as well as columns we did add.
        tables->del(table_handle); // Deleting the wrong table?

        // for (auto const &handle : *column_handles) {
        //     column_table.del(handle);
        // }

        throw e;
    }

    // Create file for table
    DbRelation &table = tables->get_table(table_name);
    table.create();

    return new QueryResult("created " + table_name);
}

// DROP ...
QueryResult *SQLExec::drop(const DropStatement *statement) {
    string table_name = statement->name;
    if (table_name == "_tables" || table_name == "_columns") {
        throw SQLExecError("SQLExecError: Cannot drop a schema table");
    }

    ValueDict where;
    where["table_name"] = table_name;
    Handles *handles = tables->select(&where);

    if (handles->size() == 0) {
        delete handles;
        throw SQLExecError("SQLExecError: Table does not exist");
    }

    // remove from _tables schema
    tables->del((*handles)[0]);

    // remove from _columns schema
    DbRelation &columns_table = tables->get_table(Columns::TABLE_NAME);
    where.clear();
    where["table_name"] = Value(table_name);
    handles = columns_table.select(&where);
    for (Handle handle : *handles) {
        columns_table.del(handle);
    }
    delete handles;

    // drop the table
    DbRelation &table = tables->get_table(table_name);
    table.drop();
    string message = "dropped " + table_name;
    return new QueryResult(message);
}

QueryResult *SQLExec::show(const ShowStatement *statement) {
    switch (statement->type) {
        case ShowType::kShowTables:    return show_tables();
        case ShowType::kShowColumns:   return show_columns(statement);
        default:                                    return new QueryResult("Cannot show unknown entity type!");
    }
}

QueryResult *SQLExec::show_tables() {
    ColumnNames *names = new ColumnNames();
    ColumnAttributes *attribs = new ColumnAttributes();
    tables->get_columns(Tables::TABLE_NAME, *names, *attribs);

    ValueDicts *rows = new ValueDicts();
    Handles *handles = tables->select();
    for (Handle &handle : *handles) {
        ValueDict *row = tables->project(handle);
        if ((*row)["table_name"].s == "_tables" || (*row)["table_name"].s == "_columns") {
            continue;
        }
        rows->push_back(row);
    }
    string message = "successfully returned " + std::to_string(rows->size()) + " rows";

    delete handles;
    return new QueryResult(names, attribs, rows, message);
}

QueryResult *SQLExec::show_columns(const ShowStatement *statement) {

    ColumnNames *names = new ColumnNames();
    ColumnAttributes *attribs = new ColumnAttributes();
    tables->get_columns(Columns::TABLE_NAME, *names, *attribs);

    ValueDict target_table;
    target_table["table_name"] = Value(statement->name);

    DbRelation &col_table = SQLExec::tables->get_table(Columns::TABLE_NAME);
    Handles *col_handles = col_table.select(&target_table);

    ValueDicts *rows = new ValueDicts();
    for (auto const &handle : *col_handles)
    {
        ValueDict *row = col_table.project(handle, names);
        rows->push_back(row);
    }

    string message = "successfully returned " + std::to_string(rows->size()) + " rows";


    return new QueryResult(names, attribs, rows, message);
}
