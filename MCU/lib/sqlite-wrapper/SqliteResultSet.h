#pragma once

#include "SqliteQuery.h"

typedef struct ResultSet {
    sqlite3 *db;    // sqlite3* db is used to print errmsg
    sqlite3_stmt *stmt;
    HashMap columnMap;
    int valueIndex;
    Vector valueVec;
} ResultSet;


// Create a cursor
ResultSet *newSqliteResultSet(sqlite3 *db, sqlite3_stmt *stmt);
bool nextResultSet(ResultSet *resultSet);

int rsGetInt(ResultSet *resultSet, const char *columnName);
int64_t rsGetI64(ResultSet *resultSet, const char *columnName);
const char *rsGetString(ResultSet *resultSet, const char *columnName);
double rsGetDouble(ResultSet *resultSet, const char *columnName);

int rsGetIntByIndex(ResultSet *resultSet, int columnIndex);
int64_t rsGetI64ByIndex(ResultSet *resultSet, int columnIndex);
const char *rsGetStringByIndex(ResultSet *resultSet, int columnIndex);
double rsGetDoubleByIndex(ResultSet *resultSet, int columnIndex);

DbValueType rsGetColumnType(ResultSet *resultSet, const char *columnName);
void resultSetDelete(ResultSet *resultSet);