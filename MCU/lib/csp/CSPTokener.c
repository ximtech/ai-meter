#include "CSPTokener.h"

#if CSP_FLOAT_TYPE == float
#define STR_TO_FLOAT strtof
#else
#define STR_TO_FLOAT strtod
#endif

#define WRITE_TOKENER_ERROR(processor, msg) formatCspParserError((processor)->report, TAG, (processor)->current - 1, (msg))
#define WRITE_TOKENER_ERROR_PARAMS(processor, msg, ...) formatCspParserError((processor)->report, TAG, (processor)->current - 1, (msg), __VA_ARGS__)

typedef struct LexerProcessor {
    CspReport *report;
    char *source;
    uint32_t sourceLength;
    lexTokenVector *lexVector;
    uint32_t start;
    uint32_t current;
} LexerProcessor;

static const char *TAG = "Lexer Token";

static inline LexerProcessor createLexerProcessor(CspReport *report, lexTokenVector *vec, const char *expression, uint32_t length);

static inline bool isAtExpEnd(LexerProcessor *processor);
static inline char advanceExpChar(LexerProcessor *processor);
static inline bool isMatchNext(LexerProcessor *processor, char expected);
static inline char currentExpChar(LexerProcessor *processor);
static inline char nextExpChar(LexerProcessor *processor);
static inline CspLexerToken *getPreviousToken(LexerProcessor *processor);
static uint32_t skipExpCharsWhileStopChar(LexerProcessor *processor, char stopChar);

static inline void handleTernaryOperator(LexerProcessor *processor);
static inline void handleExpDot(LexerProcessor *processor);
static inline void handleExpSlash(LexerProcessor *processor);
static inline void handleExpOr(LexerProcessor *processor);
static inline void handleExpAnd(LexerProcessor *processor);
static inline void handleExpIdentifiers(LexerProcessor *processor, char tokenChar);
static void handleExpStringLiteral(LexerProcessor *processor, char quote);
static void handleExpNumericValue(LexerProcessor *processor);
static void handleExpVariable(LexerProcessor *processor);

static void addExpTokenType(LexerProcessor *processor, CspExprType type);
static void addExpToken(LexerProcessor *processor, CspExprType type, void *tokenValue);
static void *saveExpValue(LexerProcessor *processor, CspExprType type, char *value);


void parseTemplateExpression(CspReport *report, lexTokenVector *tokens, char *expression) {
    LexerProcessor processor = createLexerProcessor(report, tokens, expression, strlen(expression));
    while (!isAtExpEnd(&processor) && CSP_HAS_NO_ERROR(report)) {
        processor.start = processor.current;
        char tokenChar = advanceExpChar(&processor);
        switch (tokenChar) {
            case '(':
                addExpTokenType(&processor, CSP_EXP_LEFT_PAREN);
                break;
            case ')':
                addExpTokenType(&processor, CSP_EXP_RIGHT_PAREN);
                break;
            case '[':
                addExpTokenType(&processor, CSP_EXP_LEFT_BRACKET);
                break;
            case ']':
                addExpTokenType(&processor, CSP_EXP_RIGHT_BRACKET);
                break;
            case '-':
                addExpTokenType(&processor, CSP_EXP_MINUS);
                break;
            case '+':
                addExpTokenType(&processor, CSP_EXP_PLUS);
                break;
            case '*':
                addExpTokenType(&processor, isMatchNext(&processor, '*') ? CSP_EXP_POWER : CSP_EXP_MULTIPLY);
                break;
            case '%':
                addExpTokenType(&processor, CSP_EXP_PERCENT);
                break;
            case '?':
                handleTernaryOperator(&processor);
                break;
            case '.':
                handleExpDot(&processor);
                break;
            case ':':
                addExpTokenType(&processor, CSP_EXP_COLON);
                break;
            case ',':
                addExpTokenType(&processor, CSP_EXP_COMMA);
                break;
            case '!':
                addExpTokenType(&processor, isMatchNext(&processor, '=') ? CSP_EXP_NOT_EQUAL : CSP_EXP_NOT);
                break;
            case '=':
                addExpTokenType(&processor, isMatchNext(&processor, '=') ? CSP_EXP_EQUAL_EQUAL : CSP_EXP_EQUAL);
                break;
            case '<':
                addExpTokenType(&processor, isMatchNext(&processor, '=') ? CSP_EXP_LESS_EQUAL : CSP_EXP_LESS_THAN);
                break;
            case '>':
                addExpTokenType(&processor, isMatchNext(&processor, '=') ? CSP_EXP_GREATER_EQUAL : CSP_EXP_GREATER_THAN);
                break;
            case '/':
                handleExpSlash(&processor);
                break;
            case '|':
                handleExpOr(&processor);
                break;
            case '&':
                handleExpAnd(&processor);
                break;
            case ' ':  // Ignore whitespaces
            case '\r':
            case '\t':
                break;
            case '\n':
                processor.report->lineNumber++;
                break;
            case '"':
                handleExpStringLiteral(&processor, '"');
                break;
            case '\'':
                handleExpStringLiteral(&processor, '\'');
                break;
            default:
                handleExpIdentifiers(&processor, tokenChar);
                break;
        }
    }
}

void deleteLexerTokens(lexTokenVector *vec) {
    for (uint32_t i = 0; i < lexTokenVecSize(vec); i++) {   // Null safe
        CspLexerToken *token = lexTokenVecGet(vec, i);
        deleteLexerToken(token);
    }
}

void deleteLexerToken(CspLexerToken *token) {
    if (token != NULL) {
        free(token->value);
        free(token);
    }
}

static inline LexerProcessor createLexerProcessor(CspReport *report, lexTokenVector *vec, const char *expression, uint32_t length) {
    LexerProcessor processor = {
            .report = report,
            .source = (char *) expression,
            .sourceLength = length,
            .lexVector = vec,
            .start = 0,
            .current = 0};
    return processor;
}

static inline bool isAtExpEnd(LexerProcessor *processor) {
    return processor->current >= processor->sourceLength;
}

static inline char advanceExpChar(LexerProcessor *processor) {
    return processor->source[processor->current++];
}

static inline bool isMatchNext(LexerProcessor *processor, char expected) {
    if (isAtExpEnd(processor) || processor->source[processor->current] != expected) {
        return false;
    }
    processor->current++;
    return true;
}

static inline char currentExpChar(LexerProcessor *processor) {
    if (isAtExpEnd(processor)) return '\0';
    return processor->source[processor->current];
}

static inline char nextExpChar(LexerProcessor *processor) {
    if ((processor->current + 1) >= processor->sourceLength) {
        return '\0';
    }
    return processor->source[processor->current + 1];
}

static inline CspLexerToken *getPreviousToken(LexerProcessor *processor) {
    return islexTokenVecNotEmpty(processor->lexVector) ? lexTokenVecGet(processor->lexVector, lexTokenVecSize(processor->lexVector) - 1) : NULL;
}

static uint32_t skipExpCharsWhileStopChar(LexerProcessor *processor, char stopChar) {
    uint32_t length = 0;
    char currentChar = currentExpChar(processor);
    while (currentChar != stopChar && currentChar != '\0') {
        advanceExpChar(processor);
        currentChar = currentExpChar(processor);
        length++;
    }
    return length;
}

static inline void handleTernaryOperator(LexerProcessor *processor) {
    if (isMatchNext(processor, ':')) {
        addExpTokenType(processor, CSP_EXP_ELVIS);
        advanceExpChar(processor);
        return;
    }

    char *semicolonPointer = strstr(processor->source + processor->current, ":");
    uint32_t semicolonPosition = semicolonPointer != NULL ? semicolonPointer - processor->source : 0;
    if (semicolonPosition == 0 || semicolonPosition == processor->sourceLength - 1) {   // not found or at the end of expression
        WRITE_TOKENER_ERROR(processor, "Unterminated ternary expression");
        return;
    }

    advanceExpChar(processor);
    addExpTokenType(processor, CSP_EXP_TERNARY);
}

static inline void handleExpDot(LexerProcessor *processor) {
    if (isMatchNext(processor, '.') && isMatchNext(processor, '.')) {
        addExpTokenType(processor, CSP_EXP_RANGE);
        return;
    }
    WRITE_TOKENER_ERROR(processor, "Unsupported dot operator, only range expression supported");
}

static inline void handleExpSlash(LexerProcessor *processor) {
    if (isMatchNext(processor, '/')) {
        skipExpCharsWhileStopChar(processor, '\n'); // A comment goes until the end of the line.
        return;
    }
    addExpToken(processor, CSP_EXP_SLASH, "/");
}

static inline void handleExpOr(LexerProcessor *processor) {
    if (isMatchNext(processor, '|')) {
        addExpToken(processor, CSP_EXP_LOGICAL_OR, "||");
        return;
    }
    WRITE_TOKENER_ERROR(processor, "Unsupported binary OR operators only Logical OR is supported");
}

static inline void handleExpAnd(LexerProcessor *processor) {
    if (isMatchNext(processor, '&')) {
        addExpToken(processor, CSP_EXP_LOGICAL_AND, "&&");
        return;
    }
    WRITE_TOKENER_ERROR(processor, "Unsupported binary AND operators only Logical AND is supported");
}

static inline void handleExpIdentifiers(LexerProcessor *processor, char tokenChar) {
    if (isdigit((int) tokenChar)) {
        handleExpNumericValue(processor);

    } else if (isalpha((int) tokenChar) || tokenChar == '_') {
        handleExpVariable(processor);

    } else {
        WRITE_TOKENER_ERROR_PARAMS(processor, "Unexpected lexer character: [%c]", tokenChar);
    }
}

static void handleExpStringLiteral(LexerProcessor *processor, char quote) {
    char *stringStart = processor->source + processor->current;
    uint32_t literalLength = skipExpCharsWhileStopChar(processor, quote);

    if (isAtExpEnd(processor)) {
        WRITE_TOKENER_ERROR(processor, "Unterminated string");
        return;
    }

    advanceExpChar(processor);
    BufferString *strValue = SUBSTRING_CSTR(CSP_TOKEN_STRING_LENGTH, stringStart, 0, literalLength);
    addExpToken(processor, CSP_EXP_STRING, strValue->value);
}

static void handleExpNumericValue(LexerProcessor *processor) {
    char *numberPointer = processor->source + (processor->current - 1);
    uint32_t numberStartPosition = processor->current - 1;
    while (isdigit((int) currentExpChar(processor))) {
        advanceExpChar(processor);
    }

    bool hasFractionalPart = currentExpChar(processor) == '.';
    if (hasFractionalPart && isdigit((int) nextExpChar(processor))) {    // Look for a fractional part
        advanceExpChar(processor); // Skip "."
    } else {
        hasFractionalPart = false;
    }

    while (isdigit((int) currentExpChar(processor))) {
        advanceExpChar(processor);
    }

    BufferString *numberStr = SUBSTRING_CSTR(32, numberPointer, 0, processor->current - numberStartPosition);
    if (hasFractionalPart) {
        addExpToken(processor, CSP_EXP_NUMBER_FLOAT, numberStr->value);
        return;
    }

    addExpToken(processor, CSP_EXP_NUMBER_INT, numberStr->value);
}

static void handleExpVariable(LexerProcessor *processor) {
    char *variableStart = processor->source + (processor->current - 1);
    uint32_t variableStartPosition = processor->current - 1;
    while (isalnum((int) currentExpChar(processor)) || currentExpChar(processor) == '_' || currentExpChar(processor) == '.') {
        advanceExpChar(processor);
    }

    BufferString *variableStr = SUBSTRING_CSTR(CSP_TOKEN_VAR_NAME_LENGTH, variableStart, 0, processor->current - variableStartPosition);
    if (strncasecmp(variableStr->value, "null", 4) == 0) {
        addExpToken(processor, CSP_EXP_NULL, variableStr->value);
        return;

    } else if (strncasecmp(variableStr->value, "true", 4) == 0) {
        addExpToken(processor, CSP_EXP_BOOL_TRUE, variableStr->value);
        return;

    } else if (strncasecmp(variableStr->value, "false", 5) == 0) {
        addExpToken(processor, CSP_EXP_BOOL_FALSE, variableStr->value);
        return;
    }

    CspLexerToken *previousToken = getPreviousToken(processor);
    if (previousToken != NULL && (previousToken->type >= CSP_EXP_STRING && previousToken->type <= CSP_EXP_BOOL_FALSE)) {
        WRITE_TOKENER_ERROR(processor, "Invalid variable name identifier");
        return;
    }

    addExpToken(processor, CSP_EXP_VARIABLE, variableStr->value);
}

static void addExpTokenType(LexerProcessor *processor, CspExprType type) {
    addExpToken(processor, type, NULL);
}

static void addExpToken(LexerProcessor *processor, CspExprType type, void *tokenValue) {
    CspLexerToken *token = malloc(sizeof(struct CspLexerToken));
    void *value = tokenValue != NULL ? saveExpValue(processor, type, tokenValue) : NULL;
    if (token == NULL || (tokenValue != NULL && value == NULL)) {
        WRITE_TOKENER_ERROR(processor, "Memory allocation fail for 'CspLexerToken' struct");
        free(token);
        return;
    }

    token->type = type;
    token->value = value;
    token->position = processor->current - 1;
    if (!lexTokenVecAdd(processor->lexVector, token)) {
        WRITE_TOKENER_ERROR_PARAMS(processor,"Token vector overflow on line: [%d]", processor->report->lineNumber);
        deleteLexerToken(token);
    }
}

static void *saveExpValue(LexerProcessor *processor, CspExprType type, char *value) {
    if (type != CSP_EXP_NUMBER_INT && type != CSP_EXP_NUMBER_FLOAT) {   // String literals, NULL, variable name
        char *tokenValue = malloc(sizeof(char) * strlen(value) + 1);
        if (tokenValue == NULL) {
            WRITE_TOKENER_ERROR_PARAMS(processor, "Memory allocation fail for field: [%s] of type 'char *'", value);
            return NULL;
        }
        strcpy(tokenValue, value);
        return tokenValue;

    } else if (type == CSP_EXP_NUMBER_INT) {
        CSP_INT_TYPE *tokenValue = malloc(sizeof(CSP_INT_TYPE));
        if (tokenValue == NULL) {
            WRITE_TOKENER_ERROR_PARAMS(processor, "Memory allocation fail for field: [%s] of type 'int'", value);
            return NULL;
        }

        int64_t tmpVal;
        StringToI64Status status = cStrToInt64(value, &tmpVal, 10);
        if (status != STR_TO_I64_SUCCESS) {
            WRITE_TOKENER_ERROR_PARAMS(processor, "Invalid 'int' value for field: [%s]", value);
            free(tokenValue);
            return NULL;
        }

        *tokenValue = (CSP_INT_TYPE) tmpVal;
        return tokenValue;

    } else {
        CSP_FLOAT_TYPE *tokenValue = malloc(sizeof(CSP_FLOAT_TYPE));
        if (tokenValue == NULL) {
            WRITE_TOKENER_ERROR_PARAMS(processor, "Memory allocation fail for field: [%s] of floating type", value);
            return NULL;
        }

        char *endPointer = NULL;
        *tokenValue = STR_TO_FLOAT(value, &endPointer);
        // no digits found, or underflow occurred, or overflow occurred
        if ((value == endPointer) || (errno == ERANGE && *tokenValue == DBL_MIN) || (errno == ERANGE && *tokenValue == DBL_MAX)) {
            WRITE_TOKENER_ERROR_PARAMS(processor, "Invalid floating point value: [%s]", value);
            free(tokenValue);
            return NULL;
        }
        return tokenValue;
    }
}
