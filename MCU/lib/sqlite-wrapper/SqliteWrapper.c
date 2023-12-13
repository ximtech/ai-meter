#include "SqliteWrapper.h"


static sqlite3_stmt *tryExecuteStep(sqlite3 *db, const char *sql);
static int tryExecuteUpdate(sqlite3 *db, const char *sql);
static int sqliteValueMapperCallback(void *userData, int valueCount, char **values, char **tableColumnNames);


sqlite3 *sqliteDbInit(const char* dbName) {
    sqlite3 *db;
    sqlite3_initialize();
    sqlite3_open(dbName, &db);
    return db;
}

ResultSet *executeQuery(sqlite3 *db, const char *sql, str_DbValueMap *queryParams) {
    QueryString *query = namedQueryString(sql, queryParams);
    sqlite3_stmt *stmt = tryExecuteStep(db, query->value);
    ResultSet *resultSet = stmt != NULL ? newSqliteResultSet(db, stmt) : NULL;
    deleteQueryString(query);
    return resultSet;
}

int executeUpdate(sqlite3 *db, const char *sql, str_DbValueMap *queryParams) {
    QueryString *query = namedQueryString(sql, queryParams);
    int rc = tryExecuteUpdate(db, query->value);
    deleteQueryString(query);
    return rc;
}

ResultSet *executeCallbackQuery(sqlite3 *db, const char *sql, str_DbValueMap *queryParams) {
    QueryString *query = namedQueryString(sql, queryParams);
    char *errorMessage = NULL;
    ResultSet *rs = newSqliteResultSet(db, NULL);
    int rc = sqlite3_exec(db, query->value, sqliteValueMapperCallback, (void *) rs, &errorMessage);

    if (rc != SQLITE_OK) {
        sqlite3_free(errorMessage);
        deleteQueryString(query);
        resultSetDelete(rs);
        return NULL;
    }

    deleteQueryString(query);
    return rs;
}

int executeCallbackUpdate(sqlite3 *db, const char *sql, str_DbValueMap *queryParams) {
    QueryString *query = namedQueryString(sql, queryParams);
    char *errorMessage = NULL;
    int rc = sqlite3_exec(db, query->value, NULL, NULL, &errorMessage);
    sqlite3_free(errorMessage);
    deleteQueryString(query);
    return rc;
}

bool idDbColumnExists(sqlite3 *db, const char *table, const char *columnName) {
    QueryString *sql = queryStringOf("PRAGMA table_info(%s)", table);
    char **result = NULL;
    int row = 0;
    int col = 0;
    char *err = NULL;
    sqlite3_get_table(db, sql->value, &result, &row, &col, &err);

    int idx = col;
    bool exist = false;
    for (int i = 0; i < row; i++) {
        if (strcmp(result[idx + 1], columnName) == 0) {
            exist = true;
            break;
        }

        idx += col;
    }

    sqlite3_free_table(result);
    deleteQueryString(sql);
    return exist;
}

Vector getDbTableColumnNames(sqlite3 *db, const char *table) {
    QueryString *sql = queryStringOf("PRAGMA table_info(%s)", table);
    char **result = NULL;
    int row = 0;
    int column = 0;
    char *errmsg = NULL;
    sqlite3_get_table(db, sql->value, &result, &row, &column, &errmsg);

    Vector columns = getVectorInstance(row);
    int idx = column;
    for (int i = 0; i < row; i++) {
        const char *columnName = result[idx + 1];
        vectorAdd(columns, strdup(columnName));
        idx += column;
    }

    sqlite3_free_table(result);
    deleteQueryString(sql);
    return columns;
}

void deleteDbColumnNameVector(Vector columns) {
    for (int i = 0; i < getVectorSize(columns); i++) {
        free(vectorGet(columns, i));
    }
    vectorDelete(columns);
}

void sqliteDbClose(sqlite3 *db) {
    if (db != NULL) {
        sqlite3_close(db);
    }
}

static sqlite3_stmt *tryExecuteStep(sqlite3 *db, const char *sql) {
    sqlite3_stmt *stmt = NULL;
    sqlite3_prepare_v2(db, sql, (int) strlen(sql), &stmt, NULL);
    return stmt;
}

static int tryExecuteUpdate(sqlite3 *db, const char *sql) {
    sqlite3_stmt *stmt = NULL;
    sqlite3_prepare_v2(db, sql, (int) strlen(sql), &stmt, NULL);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc != SQLITE_DONE ? rc : SQLITE_OK;
}

static int sqliteValueMapperCallback(void *userData, int valueCount, char **values, char **tableColumnNames) {
    ResultSet *rs = (ResultSet *) userData;
    initSingletonHashMap(&rs->columnMap, valueCount * 2);
    initSingletonVector(&rs->valueVec, 128);
    HashMap valueMap = getHashMapInstance(valueCount);

    for (int i = 0; i < valueCount; i++) {
        char *columnName = strdup(tableColumnNames[i]);
        hashMapPut(valueMap, columnName, (MapValueType) strdup(values[i]));
        hashMapPut(rs->columnMap, columnName, (MapValueType) i);
    }
    vectorAdd(rs->valueVec, valueMap);
    return SQLITE_OK;
}
