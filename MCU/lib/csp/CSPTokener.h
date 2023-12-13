#pragma once

#include <float.h>
#include "Vector.h"
#include "BufferVector.h"
#include "CSPReport.h"

#ifndef CSP_TOKEN_STRING_LENGTH
#define CSP_TOKEN_STRING_LENGTH 256
#endif

#ifndef CSP_TOKEN_VAR_NAME_LENGTH
#define CSP_TOKEN_VAR_NAME_LENGTH 64
#endif

#ifndef CSP_TOKEN_MAX_COUNT
#define CSP_TOKEN_MAX_COUNT 128
#endif

#ifndef CSP_INT_TYPE
#define CSP_INT_TYPE int32_t
#endif

#ifndef CSP_FLOAT_TYPE
#define CSP_FLOAT_TYPE float
#endif

typedef enum CspExprType {
    CSP_EXP_STRING,           // string literal -> "..."
    CSP_EXP_NUMBER_INT,       // const int value
    CSP_EXP_NUMBER_FLOAT,     // const float value
    CSP_EXP_VARIABLE,         // variable
    CSP_EXP_NULL,             // NULL
    CSP_EXP_BOOL_TRUE,        // true
    CSP_EXP_BOOL_FALSE,       // false

    CSP_EXP_LEFT_PAREN,         // (
    CSP_EXP_RIGHT_PAREN,        // )
    CSP_EXP_LEFT_BRACKET,       // [
    CSP_EXP_RIGHT_BRACKET,      // ]
    CSP_EXP_PLUS,               // +
    CSP_EXP_MINUS,              // -
    CSP_EXP_MULTIPLY,           // *
    CSP_EXP_SLASH,              // /
    CSP_EXP_PERCENT,            // %
    CSP_EXP_POWER,              // **
    CSP_EXP_COLON,              // :
    CSP_EXP_NOT,                // !
    CSP_EXP_EQUAL,              // =
    CSP_EXP_COMMA,              // ,

    CSP_EXP_EQUAL_EQUAL,        // ==
    CSP_EXP_NOT_EQUAL,          // !=
    CSP_EXP_GREATER_THAN,       // >
    CSP_EXP_GREATER_EQUAL,      // >=
    CSP_EXP_LESS_THAN,          // <
    CSP_EXP_LESS_EQUAL,         // <=
    CSP_EXP_LOGICAL_OR,         // ||
    CSP_EXP_LOGICAL_AND,        // &&
    CSP_EXP_TERNARY,            // 'if ? then : else'
    CSP_EXP_ELVIS,              // Shortening of the ternary operator. 'if ?: else'
    CSP_EXP_RANGE,              // '...', from-to range -> 0...5 -> 1,2,3,4,5
} CspExprType;

typedef struct CspLexerToken {
    CspExprType type;
    uint32_t position;
    void *value;
} CspLexerToken;

CREATE_CUSTOM_COMPARATOR(lexToken, CspLexerToken*, one, two, ((one->type) < (two->type)) ? (-1) : (((one->type) == (two->type)) ? 0 : 1));
CREATE_VECTOR_TYPE(CspLexerToken*, lexToken, lexTokenComparator);

void parseTemplateExpression(CspReport *report, lexTokenVector *tokens, char *expression);
void deleteLexerTokens(lexTokenVector *vec);
void deleteLexerToken(CspLexerToken *token);
