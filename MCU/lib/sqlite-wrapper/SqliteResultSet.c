#include "SqliteResultSet.h"

#define NO_VALUE_INDEX (-1)

static ResultSet *mapColumnNames(ResultSet *resultSet);
static inline char *getValueStrFromRsVecByName(ResultSet *resultSet, const char *columnName);
static inline char *getValueStrFromRsVecByIndex(ResultSet *resultSet, int columnIndex);

static inline int getIndexByColumnName(ResultSet *resultSet, const char *columnName);
static const char *resultSetGetColumnName(ResultSet *resultSet, int column);

static char *getColumnNameByIndex(ResultSet *resultSet, int columnIndex);
static int resultSetColumnCount(ResultSet *resultSet);


ResultSet *newSqliteResultSet(sqlite3 *db, sqlite3_stmt *stmt) {
    ResultSet *resultSet = malloc(sizeof(struct ResultSet));
    if (resultSet == NULL) return NULL;
    resultSet->db = db;
    resultSet->stmt = stmt;
    resultSet->columnMap = NULL;
    resultSet->valueVec = NULL;
    resultSet->valueIndex = -1;

    if (stmt == NULL) {
        return resultSet;
    }
    return mapColumnNames(resultSet);
}

bool nextResultSet(ResultSet *resultSet) {
    if (resultSet == NULL) return false;

    if (resultSet->stmt != NULL) {
        int rc = sqlite3_step(resultSet->stmt);
        if (rc == SQLITE_ROW) {
            return true;
        }
        sqlite3_finalize(resultSet->stmt);
        return false;
    }

    int valueVecLength = (int) getVectorSize(resultSet->valueVec);
    if (resultSet->valueVec != NULL && resultSet->valueIndex < valueVecLength - 1) {
        resultSet->valueIndex++;
        return true;
    }
    return false;
}

int rsGetInt(ResultSet *resultSet, const char *columnName) {
    if (resultSet->stmt != NULL) {
        return sqlite3_column_int(resultSet->stmt, getIndexByColumnName(resultSet, columnName));
    }
    return (int) strtoimax(getValueStrFromRsVecByName(resultSet, columnName), NULL, 10);
}

int64_t rsGetI64(ResultSet *resultSet, const char *columnName) {
    if (resultSet->stmt != NULL) {
        return sqlite3_column_int64(resultSet->stmt, getIndexByColumnName(resultSet, columnName));
    }
    return strtoimax(getValueStrFromRsVecByName(resultSet, columnName), NULL, 10);
}

const char *rsGetString(ResultSet *resultSet, const char *columnName) {
    if (resultSet->stmt != NULL) {
        return (const char *) sqlite3_column_text(resultSet->stmt, getIndexByColumnName(resultSet, columnName));
    }
    return getValueStrFromRsVecByName(resultSet, columnName);
}

double rsGetDouble(ResultSet *resultSet, const char *columnName) {
    if (resultSet->stmt != NULL) {
        return sqlite3_column_double(resultSet->stmt, getIndexByColumnName(resultSet, columnName));
    }
    return strtod(getValueStrFromRsVecByName(resultSet, columnName), NULL);
}

int rsGetIntByIndex(ResultSet *resultSet, int columnIndex) {
    if (resultSet->stmt != NULL) {
        return sqlite3_column_int(resultSet->stmt, columnIndex);
    }
    return (int) strtoimax(getValueStrFromRsVecByIndex(resultSet, columnIndex), NULL, 10);
}

int64_t rsGetI64ByIndex(ResultSet *resultSet, int columnIndex) {
    if (resultSet->stmt != NULL) {
        return sqlite3_column_int64(resultSet->stmt, columnIndex);
    }
    return strtoimax(getValueStrFromRsVecByIndex(resultSet, columnIndex), NULL, 10);
}

const char *rsGetStringByIndex(ResultSet *resultSet, int columnIndex) {
    if (resultSet->stmt != NULL) {
        return (const char *) sqlite3_column_text(resultSet->stmt, columnIndex);
    }
    return getValueStrFromRsVecByIndex(resultSet, columnIndex);
}

double rsGetDoubleByIndex(ResultSet *resultSet, int columnIndex) {
    if (resultSet->stmt != NULL) {
        return sqlite3_column_double(resultSet->stmt, columnIndex);
    }
    return strtod(getValueStrFromRsVecByIndex(resultSet, columnIndex), NULL);
}

DbValueType rsGetColumnType(ResultSet *resultSet, const char *columnName) {
    int columnIndex = (long) hashMapGet(resultSet->columnMap, columnName);
    int columnType = sqlite3_column_type(resultSet->stmt, columnIndex);
    switch (columnType) {
        case SQLITE_INTEGER:
            return DB_VALUE_INT;
        case SQLITE_TEXT:
            return DB_VALUE_TEXT;
        case SQLITE_FLOAT:
            return DB_VALUE_REAL;
        case SQLITE_BLOB:
            return DB_VALUE_BLOB;
        default:
            return DB_VALUE_NULL;
    }
}

void resultSetDelete(ResultSet *resultSet) {
    if (resultSet != NULL) {
        for (uint32_t i = 0; i < getVectorSize(resultSet->valueVec); i++) {
            HashMap valueMap = (HashMap) vectorGet(resultSet->valueVec, i);
            HashMapIterator iterator = getHashMapIterator(valueMap);
            while (hashMapHasNext(&iterator)) {
                free((char *) iterator.key);
                free((char *) iterator.value);
            }
            hashMapDelete(valueMap);
        }
        vectorDelete(resultSet->valueVec);
        hashMapDelete(resultSet->columnMap);
        free(resultSet);
    }
}

static ResultSet *mapColumnNames(ResultSet *resultSet) {
    HashMap columnMap = getHashMapInstance(resultSetColumnCount(resultSet) * 2);
    for (int i = 0; i < resultSetColumnCount(resultSet); i++) {
        const char *name = resultSetGetColumnName(resultSet, i);
        hashMapPut(columnMap, name, (MapValueType) i);
    }
    
    resultSet->columnMap = columnMap;
    return resultSet;
}

static inline char *getValueStrFromRsVecByName(ResultSet *resultSet, const char *columnName) {
    HashMap valueMap = vectorGet(resultSet->valueVec, resultSet->valueIndex);
    return (char *) hashMapGet(valueMap, columnName);
}

static inline char *getValueStrFromRsVecByIndex(ResultSet *resultSet, int columnIndex) {
    HashMap valueMap = vectorGet(resultSet->valueVec, resultSet->valueIndex);
    return (char *) hashMapGet(valueMap, getColumnNameByIndex(resultSet, columnIndex));
}

static inline int getIndexByColumnName(ResultSet *resultSet, const char *columnName) {
    MapEntry *entry = hashMapGetEntry(resultSet->columnMap, columnName);
    return entry != NULL ? (long) entry->value : NO_VALUE_INDEX;
}

static char *getColumnNameByIndex(ResultSet *resultSet, int columnIndex) {
    HashMapIterator iterator = getHashMapIterator(resultSet->columnMap);
    while (hashMapHasNext(&iterator)) {
        if ((int) iterator.value == columnIndex) {
            return (char *) iterator.key;
        }
    }
    return "";
}

static const char *resultSetGetColumnName(ResultSet *resultSet, int column) {
    return sqlite3_column_name(resultSet->stmt, column);
}

static int resultSetColumnCount(ResultSet *resultSet) {
    return sqlite3_column_count(resultSet->stmt);
}
