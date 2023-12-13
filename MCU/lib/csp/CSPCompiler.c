#include "CSPCompiler.h"

#define CHUNK_INITIAL_CAPACITY_MULTIPLIER 4

#define LOWEST_PRECEDENCE_LEVEL PRECEDENCE_TERNARY
#define WRITE_COMPILE_ERROR(processor, msg) formatCspParserError((processor)->report, TAG, (processor)->tokenIndex, (msg))

typedef struct CspCompilerProcessor {
    CspReport *report;
    lexTokenVector *tokens;
    CspChunk *chunk;
    uint32_t tokenIndex;
} CspCompilerProcessor;

typedef enum CSPPrecedence {
    PRECEDENCE_NONE,
    PRECEDENCE_TERNARY,     // ?:
    PRECEDENCE_OR,          // or
    PRECEDENCE_AND,         // and
    PRECEDENCE_EQUALITY,    // == !=
    PRECEDENCE_COMPARISON,  // < > <= >=
    PRECEDENCE_TERM,        // + -
    PRECEDENCE_FACTOR,      // * / **
    PRECEDENCE_UNARY,       // ! -
} CSPPrecedence;

typedef void (*CspParseFn)(CspCompilerProcessor *processor);

typedef struct CspParseRule {
    CspParseFn prefix;
    CspParseFn infix;
    CSPPrecedence precedence;
} CspParseRule;

typedef enum CspCollectionType {
    CSP_COLLECTION_UNKNOWN,
    CSP_COLLECTION_ARRAY,
    CSP_COLLECTION_MAP,
} CspCollectionType;

static const char *TAG = "CSP Compiler";

static void compileBinaryExp(CspCompilerProcessor *processor);
static void compileUnaryExp(CspCompilerProcessor *processor);
static void compileLiteralExp(CspCompilerProcessor *processor);
static void compileCollectionsExp(CspCompilerProcessor *processor);
static void compileGroupingExp(CspCompilerProcessor *processor);
static void compileAndExp(CspCompilerProcessor *processor);
static void compileOrExp(CspCompilerProcessor *processor);
static void compileTernaryExp(CspCompilerProcessor *processor);
static void compileElvisExp(CspCompilerProcessor *processor);

static const CspParseRule PARSE_RULES[] = {
        [CSP_EXP_LEFT_PAREN]    = {compileGroupingExp, NULL, PRECEDENCE_NONE},
        [CSP_EXP_RIGHT_PAREN]   = {NULL, NULL, PRECEDENCE_NONE},
        [CSP_EXP_COLON]         = {NULL, NULL, PRECEDENCE_NONE},

        [CSP_EXP_MINUS]         = {compileUnaryExp, compileBinaryExp, PRECEDENCE_TERM},
        [CSP_EXP_NOT]           = {compileUnaryExp, NULL, PRECEDENCE_NONE},

        [CSP_EXP_PLUS]          = {compileUnaryExp, compileBinaryExp, PRECEDENCE_TERM},
        [CSP_EXP_SLASH]         = {NULL, compileBinaryExp, PRECEDENCE_FACTOR},
        [CSP_EXP_MULTIPLY]      = {NULL, compileBinaryExp, PRECEDENCE_FACTOR},
        [CSP_EXP_PERCENT]       = {NULL, compileBinaryExp, PRECEDENCE_FACTOR},
        [CSP_EXP_POWER]         = {NULL, compileBinaryExp, PRECEDENCE_FACTOR},
        [CSP_EXP_GREATER_THAN]  = {NULL, compileBinaryExp, PRECEDENCE_COMPARISON},
        [CSP_EXP_GREATER_EQUAL] = {NULL, compileBinaryExp, PRECEDENCE_COMPARISON},
        [CSP_EXP_NOT_EQUAL]     = {NULL, compileBinaryExp, PRECEDENCE_EQUALITY},
        [CSP_EXP_EQUAL]         = {NULL, compileBinaryExp, PRECEDENCE_EQUALITY},
        [CSP_EXP_EQUAL_EQUAL]   = {NULL, compileBinaryExp, PRECEDENCE_EQUALITY},
        [CSP_EXP_LESS_THAN]     = {NULL, compileBinaryExp, PRECEDENCE_COMPARISON},
        [CSP_EXP_LESS_EQUAL]    = {NULL, compileBinaryExp, PRECEDENCE_COMPARISON},

        [CSP_EXP_STRING]        = {compileLiteralExp, NULL, PRECEDENCE_NONE},
        [CSP_EXP_NUMBER_INT]    = {compileLiteralExp, NULL, PRECEDENCE_NONE},
        [CSP_EXP_NUMBER_FLOAT]  = {compileLiteralExp, NULL, PRECEDENCE_NONE},
        [CSP_EXP_VARIABLE]      = {compileLiteralExp, NULL, PRECEDENCE_NONE},
        [CSP_EXP_NULL]          = {compileLiteralExp, NULL, PRECEDENCE_NONE},
        [CSP_EXP_BOOL_TRUE]     = {compileLiteralExp, NULL, PRECEDENCE_NONE},
        [CSP_EXP_BOOL_FALSE]    = {compileLiteralExp, NULL, PRECEDENCE_NONE},
        [CSP_EXP_LEFT_BRACKET]  = {compileCollectionsExp, NULL, PRECEDENCE_NONE},

        [CSP_EXP_LOGICAL_AND]   = {NULL, compileAndExp, PRECEDENCE_AND},
        [CSP_EXP_LOGICAL_OR]    = {NULL, compileOrExp, PRECEDENCE_OR},

        [CSP_EXP_TERNARY]       = {NULL, compileTernaryExp, PRECEDENCE_TERNARY},
        [CSP_EXP_ELVIS]         = {NULL, compileElvisExp, PRECEDENCE_TERNARY},
};

static void expression(CspCompilerProcessor *processor);
static void parsePrecedence(CspCompilerProcessor *processor, CSPPrecedence precedence);

static inline CspLexerToken *advanceToNextToken(CspCompilerProcessor *processor);
static inline CspLexerToken *getPreviousToken(CspCompilerProcessor *processor);
static inline CspLexerToken *getCurrentToken(CspCompilerProcessor *processor);
static void skipTokensUntil(CspCompilerProcessor *processor, CspExprType type);

static inline bool isAtTokensEnd(CspCompilerProcessor *processor);
static inline CspLexerToken *consumeToken(CspCompilerProcessor *processor, CspExprType type);
static inline bool isTokenTypeEquals(CspCompilerProcessor *processor, CspExprType type);
static inline const CspParseRule *getParseRule(CspExprType type);
static uint32_t addJumpInstruction(CspCompilerProcessor *processor, uint8_t instruction);
static void patchJump(CspCompilerProcessor *processor, uint32_t offset);

static CspChunk *newCspChunk(uint16_t capacity);
static bool cspChunkAdd(CspChunk *chunk, uint8_t code);
static bool cspChunkAddConstant(CspChunk *chunk, CspValue value);
static bool cspChunkAddVariable(CspChunk *chunk, CspValue value);
static bool doubleCspChunkCapacity(CspChunk *chunk);
static bool adjustCspChunkCapacity(CspChunk *chunk, uint32_t newCapacity);

static CspValue getLiteralValueFromToken(CspCompilerProcessor *processor, CspLexerToken *token);
static bool isCollectionHasEnd(CspCompilerProcessor *processor);
static CspCollectionType detectCollectionType(CspCompilerProcessor *processor);
static void handleCollectionArray(CspCompilerProcessor *processor);
static bool handleArrayRange(CspCompilerProcessor *processor, CspValue arrayObject);
static void handleCollectionMap(CspCompilerProcessor *processor);

#ifdef CSP_DEBUG_TRACE_EXECUTION
static int simpleInstruction(const char* name, int offset);
static int constantInstruction(const char* name, CspChunk *chunk, int offset);
static int jumpInstruction(const char* name, int sign, CspChunk* chunk, int offset);
static void printObject(CspValue value);
#endif


CspChunk *cspCompile(CspReport *report, lexTokenVector *tokens) {
    CspChunk *chunk = newCspChunk(lexTokenVecSize(tokens) * CHUNK_INITIAL_CAPACITY_MULTIPLIER);
    CspCompilerProcessor processor = {.report = report, .tokens = tokens, .chunk = chunk};
    if (processor.chunk == NULL) {
        WRITE_COMPILE_ERROR(&processor, "Error creating chunk");
        return NULL;
    }

    expression(&processor);
    if (chunk->capacity > chunk->size) {    // fit to size
        adjustCspChunkCapacity(chunk, chunk->size);
    }

    cspValVecFitToSize(chunk->constants);
    deleteLexerTokens(tokens);
    return chunk;
}

uint8_t cspChunkGet(CspChunk *chunk, uint8_t index) {
    return (chunk != NULL && index < chunk->size) ? chunk->code[index] : 0;
}

uint8_t getCspChunkSize(CspChunk *chunk) {
    return chunk != NULL ? chunk->size : 0;
}

void cspChunkDelete(CspChunk *chunk) {
    if (chunk != NULL) {
        cspValVecDelete(chunk->constants);
        free(chunk->code);
        free(chunk);
    }
}

#ifdef CSP_DEBUG_TRACE_EXECUTION
int disassembleInstruction(CspChunk *chunk, int offset) {
    CspOpCode instruction = cspChunkGet(chunk, offset);
    switch (instruction) {
        case CSP_OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        case CSP_OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case CSP_OP_ADD:
            return constantInstruction("OP_ADD", chunk, offset);
        case CSP_OP_SUBTRACT:
            return simpleInstruction("OP_SUBTRACT", offset);
        case CSP_OP_MULTIPLY:
            return simpleInstruction("OP_MULTIPLY", offset);
        case CSP_OP_DIVIDE:
            return simpleInstruction("OP_DIVIDE", offset);
        case CSP_OP_POWER:
            return simpleInstruction("OP_POWER", offset);
        case CSP_OP_REMINDER:
            return simpleInstruction("OP_REMINDER", offset);
        case CSP_OP_NOT:
            return simpleInstruction("OP_NOT", offset);
        case CSP_OP_EQUAL:
            return simpleInstruction("OP_EQUAL", offset);
        case CSP_OP_NOT_EQUAL:
            return simpleInstruction("OP_NOT_EQUAL", offset);
        case CSP_OP_GREATER:
            return simpleInstruction("OP_GREATER", offset);
        case CSP_OP_GREATER_EQUAL:
            return simpleInstruction("OP_GREATER_EQUAL", offset);
        case CSP_OP_LESS:
            return simpleInstruction("OP_LESS", offset);
        case CSP_OP_LESS_EQUAL:
            return simpleInstruction("OP_LESS_EQUAL", offset);
        case CSP_OP_JUMP_IF_FALSE:
            return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
        case CSP_OP_JUMP:
            return jumpInstruction("OP_JUMP", 1, chunk, offset);
        case CSP_OP_POP:
            return simpleInstruction("OP_POP", offset);
        case CSP_OP_VARIABLE:
            return simpleInstruction("OP_VARIABLE", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}

void printCspValue(CspValue value) {
    switch (value.type) {
        case CSP_VAL_NUMBER_INT:
            printf("%d", value.as.numInt);
            break;
        case CSP_VAL_NUMBER_FLOAT:
            printf("%g", value.as.numFloat);
            break;
        case CSP_VAL_NULL:
            printf("NULL");
            break;
        case CSP_VAL_BOOL_TRUE:
            printf("TRUE");
            break;
        case CSP_VAL_BOOL_FALSE:
            printf("FALSE");
            break;
        case CSP_VAL_OBJECT:
            printObject(value);
            break;
        case CSP_VAL_VARIABLE:
            printf("%s", value.as.varName);
            break;
    }
}
#endif

static void expression(CspCompilerProcessor *processor) {
    parsePrecedence(processor, LOWEST_PRECEDENCE_LEVEL);
}

static void parsePrecedence(CspCompilerProcessor *processor, CSPPrecedence precedence) {
    advanceToNextToken(processor);
    CspLexerToken *token = getPreviousToken(processor);
    CspParseFn prefixRule = getParseRule(token->type)->prefix;
    if (prefixRule == NULL) {
        WRITE_COMPILE_ERROR(processor, "Expect expression");
        return;
    }
    prefixRule(processor);

    while (!isAtTokensEnd(processor) && precedence <= getParseRule(getCurrentToken(processor)->type)->precedence) {
        advanceToNextToken(processor);
        CspParseFn infixRule = getParseRule(getPreviousToken(processor)->type)->infix;
        infixRule(processor);
    }
}

static void compileBinaryExp(CspCompilerProcessor *processor) {
    CspLexerToken *operatorToken = getPreviousToken(processor);
    const CspParseRule *rule = getParseRule(operatorToken->type);
    parsePrecedence(processor, rule->precedence + 1);

    switch (operatorToken->type) {
        case CSP_EXP_PLUS:
            cspChunkAdd(processor->chunk, CSP_OP_ADD);
            break;
        case CSP_EXP_MINUS:
            cspChunkAdd(processor->chunk, CSP_OP_SUBTRACT);
            break;
        case CSP_EXP_MULTIPLY:
            cspChunkAdd(processor->chunk, CSP_OP_MULTIPLY);
            break;
        case CSP_EXP_SLASH:
            cspChunkAdd(processor->chunk, CSP_OP_DIVIDE);
            break;
        case CSP_EXP_PERCENT:
            cspChunkAdd(processor->chunk, CSP_OP_REMINDER);
            break;
        case CSP_EXP_POWER:
            cspChunkAdd(processor->chunk, CSP_OP_POWER);
            break;
        case CSP_EXP_GREATER_THAN:
            cspChunkAdd(processor->chunk, CSP_OP_GREATER);
            break;
        case CSP_EXP_GREATER_EQUAL:
            cspChunkAdd(processor->chunk, CSP_OP_GREATER_EQUAL);
            break;
        case CSP_EXP_LESS_THAN:
            cspChunkAdd(processor->chunk, CSP_OP_LESS);
            break;
        case CSP_EXP_LESS_EQUAL:
            cspChunkAdd(processor->chunk, CSP_OP_LESS_EQUAL);
            break;
        case CSP_EXP_NOT_EQUAL:
            cspChunkAdd(processor->chunk, CSP_OP_NOT_EQUAL);
            break;
        case CSP_EXP_EQUAL:
        case CSP_EXP_EQUAL_EQUAL:
            cspChunkAdd(processor->chunk, CSP_OP_EQUAL);
            break;
        default:
            WRITE_COMPILE_ERROR(processor, "Invalid binary token type");
            break;
    }
}

static void compileUnaryExp(CspCompilerProcessor *processor) {
    if (isAtTokensEnd(processor)) return;
    CspLexerToken *operatorToken = getPreviousToken(processor);
    parsePrecedence(processor, PRECEDENCE_UNARY);   // Compile the operand.

    switch (operatorToken->type) {
        case CSP_EXP_MINUS:
            cspChunkAdd(processor->chunk, CSP_OP_NEGATE);
            break;
        case CSP_EXP_NOT:
            cspChunkAdd(processor->chunk, CSP_OP_NOT);
            break;
        default:
            break;
    }
}

static void compileLiteralExp(CspCompilerProcessor *processor) {
    CspLexerToken *token = getPreviousToken(processor);
    CspValue value = getLiteralValueFromToken(processor, token);
    if (processor->report->errorMessage != NULL) return;

    switch (token->type) {
        case CSP_EXP_STRING:
        case CSP_EXP_NUMBER_INT:
        case CSP_EXP_NUMBER_FLOAT:
        case CSP_EXP_NULL:
        case CSP_EXP_BOOL_TRUE:
        case CSP_EXP_BOOL_FALSE:
            cspChunkAddConstant(processor->chunk, value);
            break;
        case CSP_EXP_VARIABLE:
            cspChunkAddVariable(processor->chunk, value);
            break;
        default:
            break;
    }
}

static void compileCollectionsExp(CspCompilerProcessor *processor) {
    if (!isCollectionHasEnd(processor)) {
        WRITE_COMPILE_ERROR(processor, "Unterminated collection. Missing end bracket ']'");
        return;
    }

    CspCollectionType type = detectCollectionType(processor);
    switch (type) {
        case CSP_COLLECTION_ARRAY:
            handleCollectionArray(processor);
            break;
        case CSP_COLLECTION_MAP:
            handleCollectionMap(processor);
            break;
        case CSP_COLLECTION_UNKNOWN:
            WRITE_COMPILE_ERROR(processor, "Unknown collection type");
            break;
    }
}

static void compileAndExp(CspCompilerProcessor *processor) {
    uint32_t endJump = addJumpInstruction(processor, CSP_OP_JUMP_IF_FALSE);
    parsePrecedence(processor, PRECEDENCE_AND);
    patchJump(processor, endJump);
}

static void compileOrExp(CspCompilerProcessor *processor) {
    uint32_t overJump = addJumpInstruction(processor, CSP_OP_JUMP_IF_FALSE);
    uint32_t endJump = addJumpInstruction(processor, CSP_OP_JUMP);
    patchJump(processor, overJump);

    parsePrecedence(processor, PRECEDENCE_OR);
    patchJump(processor, endJump);
}

static void compileTernaryExp(CspCompilerProcessor *processor) {
    uint32_t thenJump = addJumpInstruction(processor, CSP_OP_JUMP_IF_FALSE);
    cspChunkAdd(processor->chunk, CSP_OP_POP);
    expression(processor);

    CspLexerToken *token = consumeToken(processor, CSP_EXP_COLON);
    if (token == NULL) {
        WRITE_COMPILE_ERROR(processor, "Missing ':' in ternary expression.");
    }

    uint32_t elseJump = addJumpInstruction(processor, CSP_OP_JUMP);
    cspChunkAdd(processor->chunk, CSP_OP_POP);
    patchJump(processor, thenJump);

    expression(processor);
    patchJump(processor, elseJump);
}

static void compileElvisExp(CspCompilerProcessor *processor) {
    uint32_t thenJump = addJumpInstruction(processor, CSP_OP_JUMP_IF_FALSE);
    uint32_t elseJump = addJumpInstruction(processor, CSP_OP_JUMP);
    cspChunkAdd(processor->chunk, CSP_OP_POP);
    patchJump(processor, thenJump);

    expression(processor);
    patchJump(processor, elseJump);
}

static void compileGroupingExp(CspCompilerProcessor *processor) {
    expression(processor);
    CspLexerToken *token = consumeToken(processor, CSP_EXP_RIGHT_PAREN);
    if (token == NULL) {
        WRITE_COMPILE_ERROR(processor, "Expect ')' after expression.");
    }
}

static inline CspLexerToken *advanceToNextToken(CspCompilerProcessor *processor) {
    if (!isAtTokensEnd(processor)) {
        processor->tokenIndex++;
    }
    return getPreviousToken(processor);
}

static inline CspLexerToken *getPreviousToken(CspCompilerProcessor *processor) {
    return lexTokenVecGet(processor->tokens, processor->tokenIndex - 1);
}

static inline CspLexerToken *getCurrentToken(CspCompilerProcessor *processor) {
    return lexTokenVecGet(processor->tokens, processor->tokenIndex);
}

static inline bool isAtTokensEnd(CspCompilerProcessor *processor) {
    return lexTokenVecSize(processor->tokens) == processor->tokenIndex;
}

static inline CspLexerToken *consumeToken(CspCompilerProcessor *processor, CspExprType type) {
    return isTokenTypeEquals(processor, type) ? advanceToNextToken(processor) : NULL;
}

static inline bool isTokenTypeEquals(CspCompilerProcessor *processor, CspExprType type) {
    if (isAtTokensEnd(processor)) {
        return false;
    }
    return getCurrentToken(processor)->type == type;
}

static void skipTokensUntil(CspCompilerProcessor *processor, CspExprType type) {
    CspLexerToken *token = getCurrentToken(processor);
    while(!isAtTokensEnd(processor) && token->type != type) {
        token = advanceToNextToken(processor);
    }
}

static inline const CspParseRule *getParseRule(CspExprType type) {
    return &PARSE_RULES[type];
}

static uint32_t addJumpInstruction(CspCompilerProcessor *processor, uint8_t instruction) {
    cspChunkAdd(processor->chunk, instruction);
    cspChunkAdd(processor->chunk, 0xFF);
    cspChunkAdd(processor->chunk, 0xFF);
    return processor->chunk->size - 2;
}

static void patchJump(CspCompilerProcessor *processor, uint32_t offset) {
    uint32_t jump = processor->chunk->size - offset - 2;    // -2 to adjust for the bytecode for the jump offset itself
    if (jump > UINT8_MAX) {
        WRITE_COMPILE_ERROR(processor, "Too much code to jump over");
    }

    processor->chunk->code[offset] = (jump >> 8) & 0xFF;
    processor->chunk->code[offset + 1] = jump & 0xFF;
}

static CspChunk *newCspChunk(uint16_t capacity) {
    if (capacity < 1) return NULL;

    CspChunk *chunk = malloc(sizeof(struct CspChunk));
    if (chunk == NULL) return NULL;
    chunk->size = 0;
    chunk->capacity = capacity;
    chunk->code = calloc(chunk->capacity, sizeof(uint8_t));
    chunk->constants = newCspValVec(capacity);

    if (chunk->code == NULL || chunk->constants == NULL) {
        cspChunkDelete(chunk);
        return NULL;
    }
    return chunk;
}

static bool cspChunkAdd(CspChunk *chunk, uint8_t code) {
    if (chunk != NULL) {
        if (chunk->size >= chunk->capacity) {
            if (!doubleCspChunkCapacity(chunk)) return false;
        }
        chunk->code[chunk->size++] = code;
        return true;
    }
    return false;
}

static bool cspChunkAddConstant(CspChunk *chunk, CspValue value) {
    if (chunk != NULL) {
        cspChunkAdd(chunk, CSP_OP_CONSTANT);
        cspValVecAdd(chunk->constants, value);
        return cspChunkAdd(chunk, cspValVecSize(chunk->constants) - 1);
    }
    return false;
}

static bool cspChunkAddVariable(CspChunk *chunk, CspValue value) {
    if (chunk != NULL) {
        cspChunkAdd(chunk, CSP_OP_VARIABLE);
        cspValVecAdd(chunk->constants, value);
        return cspChunkAdd(chunk, cspValVecSize(chunk->constants) - 1);
    }
    return false;
}

static bool doubleCspChunkCapacity(CspChunk *chunk) {
    uint32_t newCapacity = chunk->capacity * 2;
    if (newCapacity > UINT8_MAX) return false;
    return adjustCspChunkCapacity(chunk, newCapacity);
}

static bool adjustCspChunkCapacity(CspChunk *chunk, uint32_t newCapacity) {
    if (newCapacity < chunk->size) return false;
    uint8_t *newItemArray = malloc(sizeof(uint8_t) * newCapacity);
    if (newItemArray == NULL) return false;

    for (uint32_t i = 0; i < chunk->size; i++) {
        newItemArray[i] = chunk->code[i];
    }
    free(chunk->code);
    chunk->code = newItemArray;
    chunk->capacity = newCapacity;
    return true;
}

static CspValue getLiteralValueFromToken(CspCompilerProcessor *processor, CspLexerToken *token) {
    switch (token->type) {
        case CSP_EXP_STRING:
            return CSP_CONST_STR_VALUE(token->value);
        case CSP_EXP_NUMBER_INT:
            return CSP_INT_VALUE(*((CSP_INT_TYPE *) token->value));
        case CSP_EXP_NUMBER_FLOAT:
            return CSP_FLOAT_VALUE(*((CSP_FLOAT_TYPE *) token->value));
        case CSP_EXP_VARIABLE:
            return CSP_VAR_VALUE(token->value);
        case CSP_EXP_NULL:
            return CSP_NULL_VALUE();
        case CSP_EXP_BOOL_TRUE:
            return CSP_BOOL_TRUE_VALUE();
        case CSP_EXP_BOOL_FALSE:
            return CSP_BOOL_FALSE_VALUE();
        default:
            WRITE_COMPILE_ERROR(processor, "Invalid literal token type");
            return CSP_NULL_VALUE();
    }
}

static bool isCollectionHasEnd(CspCompilerProcessor *processor) {
    for (int32_t i = (int32_t) processor->tokenIndex; i < lexTokenVecSize(processor->tokens); i++) {
        CspLexerToken *token = lexTokenVecGet(processor->tokens, i);
        if (token->type == CSP_EXP_RIGHT_BRACKET) {
            return true;
        }
    }
    return false;
}

static CspCollectionType detectCollectionType(CspCompilerProcessor *processor) {
    for (uint32_t i = processor->tokenIndex; i < lexTokenVecSize(processor->tokens); i++) {
        CspLexerToken *token = lexTokenVecGet(processor->tokens, i);
        if (token->type == CSP_EXP_COLON) {
            return CSP_COLLECTION_MAP;

        } else if (token->type == CSP_EXP_COMMA || token->type == CSP_EXP_RIGHT_BRACKET) {
            return CSP_COLLECTION_ARRAY;

        }
    }
    return CSP_COLLECTION_UNKNOWN;
}

static void handleCollectionArray(CspCompilerProcessor *processor) {
    CspValue arrayObject = CSP_CONST_ARRAY_VALUE(16);
    CspLexerToken *token = advanceToNextToken(processor);

    while (token->type != CSP_EXP_RIGHT_BRACKET) {
        if (token->type != CSP_EXP_COMMA && token->type != CSP_EXP_LEFT_BRACKET) {
            CspLexerToken *nextToken = getCurrentToken(processor);

            if (nextToken != NULL && nextToken->type == CSP_EXP_RANGE) {
                if (!handleArrayRange(processor, arrayObject)) {
                    skipTokensUntil(processor, CSP_EXP_RIGHT_BRACKET);  // roll up to array end
                    deleteCspValue(arrayObject);
                    return;
                }
                token = getCurrentToken(processor);
                continue;
            }

            CspValue value = getLiteralValueFromToken(processor, token);
            if (!cspValVecAdd(AS_CSP_ARRAY(arrayObject)->vec, value)) {
                WRITE_COMPILE_ERROR(processor, "Error while adding value to array");
                skipTokensUntil(processor, CSP_EXP_RIGHT_BRACKET);  // roll up to array end
                deleteCspValue(arrayObject);
                return;
            }
        }
        token = advanceToNextToken(processor);
    }

    cspValVecFitToSize(AS_CSP_ARRAY(arrayObject)->vec);
    cspChunkAddConstant(processor->chunk, arrayObject);
}

static bool handleArrayRange(CspCompilerProcessor *processor, CspValue arrayObject) {
    CspValue rangeFromValue = getLiteralValueFromToken(processor, getPreviousToken(processor));
    if (!IS_CSP_INT(rangeFromValue)) {
        WRITE_COMPILE_ERROR(processor, "Invalid range from value! Only int type allowed");
        return false;
    }
    consumeToken(processor, CSP_EXP_RANGE); // skip range token

    CspValue rangeToValue = getLiteralValueFromToken(processor, getCurrentToken(processor));
    if (!IS_CSP_INT(rangeToValue)) {
        WRITE_COMPILE_ERROR(processor, "Invalid range to value! Only int type allowed");
        return false;
    }

    if (AS_CSP_INT(rangeFromValue) > AS_CSP_INT(rangeToValue)) {
        WRITE_COMPILE_ERROR(processor, "Invalid range! From value can't be greater than to value");
        return false;
    }

    for (CSP_INT_TYPE i = AS_CSP_INT(rangeFromValue); i <= AS_CSP_INT(rangeToValue); i++) {
        if (!cspValVecAdd(AS_CSP_ARRAY(arrayObject)->vec, CSP_INT_VALUE(i))) {
            WRITE_COMPILE_ERROR(processor, "Error while adding range value to array");
            return false;
        }
    }

    advanceToNextToken(processor);
    return true;
}

static void handleCollectionMap(CspCompilerProcessor *processor) {
    CspValue mapObject = CSP_CONST_MAP_VALUE(16);
    CspLexerToken *token = advanceToNextToken(processor);

    while (token->type != CSP_EXP_RIGHT_BRACKET) {
        if (token->type != CSP_EXP_COMMA && token->type != CSP_EXP_LEFT_BRACKET) {
            CspValue mapKey = getLiteralValueFromToken(processor, token);
            if (!IS_CSP_STRING(mapKey)) {
                WRITE_COMPILE_ERROR(processor, "Map Object: Only string as a key allowed");
                skipTokensUntil(processor, CSP_EXP_RIGHT_BRACKET);
                deleteCspValue(mapObject);
                return;
            }

            CspLexerToken *colonToken = consumeToken(processor, CSP_EXP_COLON);
            if (colonToken == NULL) {
                WRITE_COMPILE_ERROR(processor, "Map Object: Missing key value separator ':'");
                skipTokensUntil(processor, CSP_EXP_RIGHT_BRACKET);
                deleteCspValue(mapObject);
                return;
            }

            token = advanceToNextToken(processor);
            CspValue mapValue = getLiteralValueFromToken(processor, token);

            if (!cspMapPut(AS_CSP_MAP(mapObject)->map, AS_CSP_CSTRING(mapKey), mapValue)) {
                WRITE_COMPILE_ERROR(processor, "Map Object: Error while adding entry to map");
                skipTokensUntil(processor, CSP_EXP_RIGHT_BRACKET);
                deleteCspValue(mapObject);
                return;
            }
        }
        token = advanceToNextToken(processor);
    }

    cspChunkAddConstant(processor->chunk, mapObject);
}

#ifdef CSP_DEBUG_TRACE_EXECUTION
static int simpleInstruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

static int constantInstruction(const char* name, CspChunk *chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    printCspValue(chunk->constants->items[constant]);
    printf("'\n");
    return offset + 2;
}

static int jumpInstruction(const char* name, int sign, CspChunk* chunk, int offset) {
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

static void printObject(CspValue value) {
    switch (CSP_OBJECT_TYPE(value)) {
        case CSP_OBJ_STRING:
            printf("%s", AS_CSP_CSTRING(value));
            break;
        case CSP_OBJ_ARRAY:
            printf("Csp Array");
            break;
        case CSP_OBJ_MAP:
            printf("Csp Map");
            break;
    }
}
#endif
