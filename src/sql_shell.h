/**
 * @file sql_shell.h - Implementation of SQL Shell to accept user input and
 * execute SQL commands
 * 
 * @author Duc Vo
 * @see "Seattle University, CPSC5300, Winter Quarter 2024"
 */
#pragma once
#include <hsql/SQLParser.h>
#include "heap_storage.h"

/**
 * Initialize database environment, accept user input and execute SQL commands
 * Support SELECT and CREATE statment
 * @class SQLShell - Implementation of SQL Shell
 */
class SQLShell {
   public:
    /**
     * Initialize the database environment with the given home directory
     * @param envHome  the home directory of the database
     */
    virtual void init(const char *envHome);

    /**
     *  Run SQL Shell to accept user input and execute SQL commands
     */
    virtual void run();

   private:

    static bool initialized;
};
