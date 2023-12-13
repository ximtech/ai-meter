#pragma once

#include "SqliteResultSet.h"


sqlite3 *sqliteDbInit(const char* dbName);

ResultSet *executeQuery(sqlite3 *db, const char *sql, str_DbValueMap *queryParams);
int executeUpdate(sqlite3 *db, const char *sql, str_DbValueMap *queryParams);

ResultSet *executeCallbackQuery(sqlite3 *db, const char *sql, str_DbValueMap *queryParams);
int executeCallbackUpdate(sqlite3 *db, const char *sql, str_DbValueMap *queryParams);

bool idDbColumnExists(sqlite3 *db, const char *table, const char *columnName);
Vector getDbTableColumnNames(sqlite3 *db, const char *table);

void deleteDbColumnNameVector(Vector columns);
void sqliteDbClose(sqlite3 *db);




