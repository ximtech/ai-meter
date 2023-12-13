#pragma once

#include "SqliteParameter.h"

#ifndef DEFAULT_SQLITE_QUERY_STRING_SIZE
    #define DEFAULT_SQLITE_QUERY_STRING_SIZE 128
#endif

#ifndef SQLITE_QUERY_FORMAT_STRING_SIZE
    #define SQLITE_QUERY_FORMAT_STRING_SIZE 512
#endif

typedef struct QueryString {
    char *value;
    uint32_t size;
    uint32_t capacity;
} QueryString;


QueryString *newQueryString();
QueryString *newQueryStringWithSize(uint32_t capacity);

QueryString *queryStringAppend(QueryString *str, const char* value, uint32_t valueLength);
QueryString *queryStringAppendChar(QueryString *str, char charValue);

QueryString *queryStringOf(const char* format, ...);
QueryString *namedQueryString(const char* sql, str_DbValueMap *queryParams);

const char *queryStringGetValue(QueryString *str);
void deleteQueryString(QueryString *str);
