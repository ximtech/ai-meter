#include "CSPInterpreter.h"

#if CSP_FLOAT_TYPE == float
    #define CSP_POW powf
    #define CSP_FMOD fmodf
#else
    #define CSP_POW pow
    #define CSP_FMOD fmod
#endif

#define CSP_BYTE_OFFSET 2
#define CSP_NEXT_BYTES_TO_SHORT(processor, index) ((processor)->chunk->code[(index) + 1] << 8 | (processor)->chunk->code[(index) + 2])

#define NULL_STR "NULL"
#define NULL_STR_LEN (sizeof("NULL") - 1)

#define WRITE_INTERPRETER_ERROR(processor, msg) formatCspParserError((processor)->report, TAG, 0, (msg));
#define WRITE_INTERPRETER_ERROR_PARAMS(processor, msg, ...) formatCspParserError((processor)->report, TAG, 0, (msg), __VA_ARGS__);

typedef struct CspStack {
    CspValue stack[CSP_STACK_MAX_VALUES];
    CspValue *stackTop;
} CspStack;

typedef struct ByteCodeProcessor {
    CspReport *report;
    CspObjectMap *paramObj;
    CspChunk *chunk;
    CspStack *valueStack;
} ByteCodeProcessor;

static const char *TAG = "Byte Code Interpreter";
static char stringBuffer[CSP_TOKEN_STRING_LENGTH] = {0};

static void runCspChunk(ByteCodeProcessor *processor);
static void initByteCodeProcessor(ByteCodeProcessor *processor, CspReport *report, CspChunk* chunk, CspObjectMap *mapObj);
static void evaluateConstant(ByteCodeProcessor *processor, uint32_t constantIndex);
static void evaluateVariable(ByteCodeProcessor *processor, uint32_t variableIndex);
static CspValue getVariableFromParams(ByteCodeProcessor *processor, const char *name);

static void evaluateUnaryExp(ByteCodeProcessor *processor);
static void evaluateBinaryPlus(ByteCodeProcessor *processor);
static void evaluateBinaryMinus(ByteCodeProcessor *processor);
static void evaluateBinaryMultiplication(ByteCodeProcessor *processor);
static void evaluateBinaryDivision(ByteCodeProcessor *processor);
static void evaluateBinaryPow(ByteCodeProcessor *processor);
static void evaluateBinaryReminder(ByteCodeProcessor *processor);

static bool isBinaryEqualExpr(ByteCodeProcessor *processor);
static bool isBinaryExprGreater(ByteCodeProcessor *processor);
static bool isBinaryExprGreaterEqual(ByteCodeProcessor *processor);
static bool isBinaryExprLess(ByteCodeProcessor *processor);
static bool isBinaryExprLessEqual(ByteCodeProcessor *processor);
static bool isUnaryTruthyExpr(ByteCodeProcessor *processor);
static bool isLiteralTruthyExp(CspValue value);
static CspValue multiplyString(CspObjectString *str, uint16_t count);

static void stringifyResult(ByteCodeProcessor *processor, CspValue value, CspTableString *resultStr);
static BufferString * removeTrailingZeroes(BufferString *decimal, BufferString *result);
static void arrayObjectToString(ByteCodeProcessor *processor, CspValue value, CspTableString *resultStr);
static void mapObjectToString(ByteCodeProcessor *processor, CspValue value, CspTableString *resultStr);
static void handleDivisionByZero(ByteCodeProcessor *processor);

static void resetStack(CspStack *stack);
static void push(CspStack *stack, CspValue value);
static CspValue pop(CspStack *stack);
static CspValue peek(CspStack *stack);

void interpretCspChunk(CspReport *report, CspChunk* chunk, CspTableString *resultStr, CspObjectMap *paramMap) {
    ByteCodeProcessor processor = {0};
    initByteCodeProcessor(&processor, report, chunk, paramMap);
    runCspChunk(&processor);
    stringifyResult(&processor, pop(processor.valueStack), resultStr);
}

bool isTruthyCspExp(CspReport *report, CspChunk* chunk, CspObjectMap *paramMap) {
    ByteCodeProcessor processor = {0};
    initByteCodeProcessor(&processor, report, chunk, paramMap);
    runCspChunk(&processor);
    bool isExpressionTrue = isLiteralTruthyExp(pop(processor.valueStack));
    return isExpressionTrue;
}

CspValue evaluateToCspValue(CspReport *report, CspChunk* chunk, CspObjectMap *paramMap) {
    ByteCodeProcessor processor = {0};
    initByteCodeProcessor(&processor, report, chunk, paramMap);
    runCspChunk(&processor);
    CspValue result = pop(processor.valueStack);
    return result;
}

static void initByteCodeProcessor(ByteCodeProcessor *processor, CspReport *report, CspChunk* chunk, CspObjectMap *mapObj) {
    static CspStack valueStack = {0};
    resetStack(&valueStack);
    createdObjects = NULL;  // compiled objects will be stored in chunk value vector
    processor->report = report;
    processor->paramObj = mapObj;
    processor->chunk = chunk;
    processor->valueStack = &valueStack;
}

static void runCspChunk(ByteCodeProcessor *processor) {
    for (uint32_t i = 0; i < getCspChunkSize(processor->chunk) && CSP_HAS_NO_ERROR(processor->report); i++) {
        #ifdef CSP_DEBUG_TRACE_EXECUTION
        disassembleInstruction(processor->chunk, (int) i);
        #endif

        CspOpCode instruction = processor->chunk->code[i];
        switch (instruction) {
            case CSP_OP_VARIABLE:
                evaluateVariable(processor, processor->chunk->code[++i]);
                break;
            case CSP_OP_CONSTANT:
                evaluateConstant(processor, processor->chunk->code[++i]);
                break;
            case CSP_OP_ADD:
                evaluateBinaryPlus(processor);
                break;
            case CSP_OP_SUBTRACT:
                evaluateBinaryMinus(processor);
                break;
            case CSP_OP_MULTIPLY:
                evaluateBinaryMultiplication(processor);
                break;
            case CSP_OP_DIVIDE:
                evaluateBinaryDivision(processor);
                break;
            case CSP_OP_NEGATE:
                evaluateUnaryExp(processor);
                break;
            case CSP_OP_POWER:
                evaluateBinaryPow(processor);
                break;
            case CSP_OP_REMINDER:
                evaluateBinaryReminder(processor);
                break;
            case CSP_OP_NOT:
                push(processor->valueStack, CSP_BOOL_VALUE(isUnaryTruthyExpr(processor)));
                break;
            case CSP_OP_EQUAL:
                push(processor->valueStack, CSP_BOOL_VALUE(isBinaryEqualExpr(processor)));
                break;
            case CSP_OP_NOT_EQUAL:
                push(processor->valueStack, CSP_BOOL_VALUE(!isBinaryEqualExpr(processor)));
                break;
            case CSP_OP_GREATER:
                push(processor->valueStack, CSP_BOOL_VALUE(isBinaryExprGreater(processor)));
                break;
            case CSP_OP_GREATER_EQUAL:
                push(processor->valueStack, CSP_BOOL_VALUE(isBinaryExprGreaterEqual(processor)));
                break;
            case CSP_OP_LESS:
                push(processor->valueStack, CSP_BOOL_VALUE(isBinaryExprLess(processor)));
                break;
            case CSP_OP_LESS_EQUAL:
                push(processor->valueStack, CSP_BOOL_VALUE(isBinaryExprLessEqual(processor)));
                break;
            case CSP_OP_JUMP_IF_FALSE: {
                uint16_t offset = CSP_NEXT_BYTES_TO_SHORT(processor, i);
                if (!isLiteralTruthyExp(peek(processor->valueStack))) {
                    i += offset;
                }
                i += CSP_BYTE_OFFSET; // skip jump offset instructions
                break;
            }
            case CSP_OP_JUMP: {
                uint16_t offset = CSP_NEXT_BYTES_TO_SHORT(processor, i);
                i += offset + CSP_BYTE_OFFSET;
                break;
            }
            case CSP_OP_POP:
                pop(processor->valueStack);
                break;
            default:
                WRITE_INTERPRETER_ERROR_PARAMS(processor, "Unsupported byte code instruction: [%d]", instruction)
                return;
        }

    }
}

static void evaluateConstant(ByteCodeProcessor *processor, uint32_t constantIndex) {
    CspValue constant = cspValVecGet(processor->chunk->constants, constantIndex);
    push(processor->valueStack, constant);
    #ifdef CSP_DEBUG_TRACE_EXECUTION
    printCspValue(constant);
    printf("\n");
    #endif
}

static void evaluateVariable(ByteCodeProcessor *processor, uint32_t variableIndex) {
    CspValue variable = cspValVecGet(processor->chunk->constants, variableIndex);
    CspValue paramValue = getVariableFromParams(processor, AS_CSP_VAR_NAME(variable));
    push(processor->valueStack, paramValue);
    #ifdef CSP_DEBUG_TRACE_EXECUTION
    printCspValue(variable);
    printf("\n");
    #endif
}

static CspValue getVariableFromParams(ByteCodeProcessor *processor, const char *name) {
    BufferString *paramKey = NEW_STRING_BUFF(stringBuffer, name);

    if (containsStr(paramKey, ".")) {
        BufferString *singleKey = EMPTY_STRING(64);
        StringIterator iter = getStringSplitIterator(paramKey, ".");

        CspValue value = CSP_NULL_VALUE();
        CspHashMap *paramMap = processor->paramObj->map;
        while (hasNextSplitToken(&iter, singleKey)) {
            value = cspMapGet(paramMap, singleKey->value);
            if (IS_CSP_NULL(value)) {
                break;
            }

            if (!IS_CSP_MAP(value)) {
                return value;
            }

            paramMap = AS_CSP_MAP(value)->map;
        }
        return value;
    }

    return cspMapGet(processor->paramObj->map, paramKey->value);
}

static void evaluateUnaryExp(ByteCodeProcessor *processor) {
    CspValue *value = processor->valueStack->stackTop - 1;
    switch (value->type) {
        case CSP_VAL_VARIABLE:
        case CSP_VAL_OBJECT:
        case CSP_VAL_NULL:
            break;
        case CSP_VAL_NUMBER_INT:
            value->as.numInt = -value->as.numInt;
            break;
        case CSP_VAL_NUMBER_FLOAT:
            value->as.numFloat = -value->as.numFloat;
            break;
        case CSP_VAL_BOOL_TRUE:
            value->type = CSP_VAL_BOOL_FALSE;
            break;
        case CSP_VAL_BOOL_FALSE:
            value->type = CSP_VAL_BOOL_TRUE;
            break;
    }
}

static void evaluateBinaryPlus(ByteCodeProcessor *processor) {
    CspValue right = pop(processor->valueStack);
    CspValue left = pop(processor->valueStack);

    if (IS_CSP_FLOAT(left) && IS_CSP_FLOAT(right)) {
        push(processor->valueStack, CSP_FLOAT_VALUE(AS_CSP_FLOAT(left) + AS_CSP_FLOAT(right)));

    } else if (IS_CSP_FLOAT(left) && IS_CSP_BOOL(right)) {
        push(processor->valueStack, CSP_FLOAT_VALUE(AS_CSP_FLOAT(left) + (IS_CSP_BOOL_TRUE(right) ? 1 : 0)));

    } else if (IS_CSP_BOOL(left) && IS_CSP_FLOAT(right)) {
        push(processor->valueStack, CSP_FLOAT_VALUE(AS_CSP_INT(right) + (IS_CSP_BOOL_TRUE(left) ? 1 : 0)));

    } else if (IS_CSP_FLOAT(left) && IS_CSP_INT(right)) {
        push(processor->valueStack, CSP_FLOAT_VALUE(AS_CSP_FLOAT(left) + AS_CSP_INT(right)));

    } else if (IS_CSP_INT(left) && IS_CSP_FLOAT(right)) {
        push(processor->valueStack, CSP_FLOAT_VALUE(AS_CSP_INT(left) + AS_CSP_FLOAT(right)));

    } else if (IS_CSP_INT(left) && IS_CSP_INT(right)) {
        push(processor->valueStack, CSP_INT_VALUE(AS_CSP_INT(left) + AS_CSP_INT(right)));

    } else if (IS_CSP_INT(left) && IS_CSP_BOOL(right)) {
        push(processor->valueStack, CSP_INT_VALUE(AS_CSP_INT(left) + (IS_CSP_BOOL_TRUE(right) ? 1 : 0)));

    } else if (IS_CSP_BOOL(left) && IS_CSP_INT(right)) {
        push(processor->valueStack, CSP_INT_VALUE(AS_CSP_INT(right) + (IS_CSP_BOOL_TRUE(left) ? 1 : 0)));

    } else if (IS_CSP_STRING(left) && IS_CSP_STRING(right)) {
        CspObjectString *leftStr = AS_CSP_STRING(left);
        CspObjectString *rightStr = AS_CSP_STRING(right);
        push(processor->valueStack, CSP_STR_CONCAT_OBJECTS(leftStr, rightStr));

    } else if (IS_CSP_STRING(left) && IS_CSP_INT(right)) {
        CspObjectString *leftStr = AS_CSP_STRING(left);
        BufferString *number = INT64_TO_STRING(AS_CSP_INT(right));
        push(processor->valueStack, CSP_STR_VALUE_CONCAT(leftStr->chars, number->value, leftStr->length + number->length));

    } else if (IS_CSP_INT(left) && IS_CSP_STRING(right)) {
        CspObjectString *rightStr = AS_CSP_STRING(right);
        BufferString *number = INT64_TO_STRING(AS_CSP_INT(left));
        push(processor->valueStack, CSP_STR_VALUE_CONCAT(number->value, rightStr->chars, rightStr->length + number->length));

    } else if (IS_CSP_STRING(left) && IS_CSP_FLOAT(right)) {
        CspObjectString *leftStr = AS_CSP_STRING(left);
        BufferString *decimal = STRING_FORMAT_32("%f", AS_CSP_FLOAT(right));
        push(processor->valueStack, CSP_STR_VALUE_CONCAT(leftStr->chars, decimal->value, leftStr->length + decimal->length));

    } else if (IS_CSP_FLOAT(left) && IS_CSP_STRING(right)) {
        CspObjectString *rightStr = AS_CSP_STRING(right);
        BufferString *decimal = STRING_FORMAT_32("%f", AS_CSP_FLOAT(left));
        push(processor->valueStack, CSP_STR_VALUE_CONCAT(decimal->value, rightStr->chars, rightStr->length + decimal->length));

    } else if (IS_CSP_STRING(left) && IS_CSP_NULL(right)) {
        CspObjectString *leftStr = AS_CSP_STRING(left);
        push(processor->valueStack, CSP_STR_VALUE_CONCAT(leftStr->chars, NULL_STR, leftStr->length + NULL_STR_LEN));

    } else if (IS_CSP_NULL(left) && IS_CSP_STRING(right)) {
        CspObjectString *rightStr = AS_CSP_STRING(right);
        push(processor->valueStack, CSP_STR_VALUE_CONCAT(NULL_STR, rightStr->chars, rightStr->length + NULL_STR_LEN));

    } else if (IS_CSP_NULL(left) && IS_CSP_NULL(right)) {
        push(processor->valueStack, CSP_STR_VALUE_CONCAT(NULL_STR, NULL_STR, (NULL_STR_LEN) + (NULL_STR_LEN)));

    } else if (IS_CSP_NULL(left) || IS_CSP_NULL(right)) {
        push(processor->valueStack, CSP_NULL_VALUE());

    } else if (IS_CSP_BOOL(left) && IS_CSP_BOOL(right)) {
        push(processor->valueStack, CSP_INT_VALUE((IS_CSP_BOOL_TRUE(left) ? 1 : 0) + (IS_CSP_BOOL_TRUE(right) ? 1 : 0)));

    } else if (IS_CSP_ARRAY(left)) {
        cspAddValToArray(AS_CSP_ARRAY(left), right);
        push(processor->valueStack, left);

    } else {
        WRITE_INTERPRETER_ERROR(processor, "Sum invalid operands");
        push(processor->valueStack, CSP_NULL_VALUE());
    }
}

static void evaluateBinaryMinus(ByteCodeProcessor *processor) {
    CspValue right = pop(processor->valueStack);
    CspValue left = pop(processor->valueStack);

    if (IS_CSP_FLOAT(left) && IS_CSP_FLOAT(right)) {
        push(processor->valueStack, CSP_FLOAT_VALUE(AS_CSP_FLOAT(left) - AS_CSP_FLOAT(right)));

    } else if (IS_CSP_FLOAT(left) && IS_CSP_INT(right)) {
        push(processor->valueStack, CSP_FLOAT_VALUE(AS_CSP_FLOAT(left) - AS_CSP_INT(right)));

    } else if (IS_CSP_INT(left) && IS_CSP_FLOAT(right)) {
        push(processor->valueStack, CSP_FLOAT_VALUE(AS_CSP_INT(left) - AS_CSP_FLOAT(right)));

    } else if (IS_CSP_INT(left) && IS_CSP_INT(right)) {
        push(processor->valueStack, CSP_INT_VALUE(AS_CSP_INT(left) - AS_CSP_INT(right)));

    } else if (IS_CSP_NULL(left) || IS_CSP_NULL(right)) {
        push(processor->valueStack, CSP_NULL_VALUE());

    } else if (IS_CSP_STRING(left)) {
        push(processor->valueStack, left);

    } else {
        WRITE_INTERPRETER_ERROR(processor, "Subtraction invalid operands");
        push(processor->valueStack, CSP_NULL_VALUE());
    }
}

static void evaluateBinaryMultiplication(ByteCodeProcessor *processor) {
    CspValue right = pop(processor->valueStack);
    CspValue left = pop(processor->valueStack);

    if (IS_CSP_FLOAT(left) && IS_CSP_FLOAT(right)) {
        push(processor->valueStack, CSP_FLOAT_VALUE(AS_CSP_FLOAT(left) * AS_CSP_FLOAT(right)));

    } else if (IS_CSP_FLOAT(left) && IS_CSP_INT(right)) {
        push(processor->valueStack, CSP_FLOAT_VALUE(AS_CSP_FLOAT(left) * AS_CSP_INT(right)));

    } else if (IS_CSP_INT(left) && IS_CSP_FLOAT(right)) {
        push(processor->valueStack, CSP_FLOAT_VALUE(AS_CSP_INT(left) * AS_CSP_FLOAT(right)));

    } else if (IS_CSP_INT(left) && IS_CSP_INT(right)) {
        push(processor->valueStack, CSP_INT_VALUE(AS_CSP_INT(left) * AS_CSP_INT(right)));

    } else if (IS_CSP_INT(left) && IS_CSP_BOOL(right)) {
        push(processor->valueStack, CSP_INT_VALUE(AS_CSP_INT(left) * (IS_CSP_BOOL_TRUE(right) ? 1 : 0)));

    } else if (IS_CSP_BOOL(left) && IS_CSP_INT(right)) {
        push(processor->valueStack, CSP_INT_VALUE(AS_CSP_INT(right) * (IS_CSP_BOOL_TRUE(left) ? 1 : 0)));

    } else if (IS_CSP_FLOAT(left) && IS_CSP_BOOL(right)) {
        push(processor->valueStack, CSP_FLOAT_VALUE(AS_CSP_FLOAT(left) * (IS_CSP_BOOL_TRUE(right) ? 1 : 0)));

    } else if (IS_CSP_BOOL(left) && IS_CSP_FLOAT(right)) {
        push(processor->valueStack, CSP_FLOAT_VALUE(AS_CSP_FLOAT(right) * (IS_CSP_BOOL_TRUE(left) ? 1 : 0)));

    } else if (IS_CSP_STRING(left) && IS_CSP_INT(right)) {
        CspObjectString *leftStr = AS_CSP_STRING(left);
        CSP_INT_TYPE count = AS_CSP_INT(right);
        push(processor->valueStack, multiplyString(leftStr, count));

    } else if (IS_CSP_STRING(left) && IS_CSP_FLOAT(right)) {
        CspObjectString *leftStr = AS_CSP_STRING(left);
        CSP_INT_TYPE count = (CSP_INT_TYPE) AS_CSP_FLOAT(right);
        push(processor->valueStack, multiplyString(leftStr, count));

    } else if (IS_CSP_NULL(left) || IS_CSP_NULL(right)) {
        push(processor->valueStack, CSP_NULL_VALUE());

    } else {
        WRITE_INTERPRETER_ERROR(processor, "Multiplication invalid operands");
        push(processor->valueStack, CSP_NULL_VALUE());
    }
}

static void evaluateBinaryDivision(ByteCodeProcessor *processor) {
    CspValue right = pop(processor->valueStack);
    CspValue left = pop(processor->valueStack);

    if (IS_CSP_FLOAT(left) && IS_CSP_FLOAT(right)) {
        if (AS_CSP_FLOAT(right) == 0) {
            handleDivisionByZero(processor);
            return;
        }
        push(processor->valueStack, CSP_FLOAT_VALUE(AS_CSP_FLOAT(left) / AS_CSP_FLOAT(right)));

    } else if (IS_CSP_FLOAT(left) && IS_CSP_INT(right)) {
        if (AS_CSP_INT(right) == 0) {
            handleDivisionByZero(processor);
            return;
        }
        push(processor->valueStack, CSP_FLOAT_VALUE(AS_CSP_FLOAT(left) / (CSP_FLOAT_TYPE) AS_CSP_INT(right)));

    } else if (IS_CSP_INT(left) && IS_CSP_FLOAT(right)) {
        if (AS_CSP_FLOAT(right) == 0) {
            handleDivisionByZero(processor);
            return;
        }
        push(processor->valueStack, CSP_FLOAT_VALUE((CSP_FLOAT_TYPE) AS_CSP_INT(left) / AS_CSP_FLOAT(right)));

    } else if (IS_CSP_INT(left) && IS_CSP_INT(right)) {
        if (AS_CSP_INT(right) == 0) {
            handleDivisionByZero(processor);
            return;
        }
        push(processor->valueStack, CSP_INT_VALUE(AS_CSP_INT(left) / AS_CSP_INT(right)));

    } else if (IS_CSP_FLOAT(left) && IS_CSP_BOOL(right)) {
        if (IS_CSP_BOOL_FALSE(right)) {
            handleDivisionByZero(processor);
            return;
        }
        push(processor->valueStack, CSP_FLOAT_VALUE(AS_CSP_FLOAT(left)));

    } else if (IS_CSP_BOOL(left) && IS_CSP_FLOAT(right)) {
        if (AS_CSP_FLOAT(right) == 0) {
            handleDivisionByZero(processor);
            return;
        }
        push(processor->valueStack, CSP_FLOAT_VALUE((IS_CSP_BOOL_TRUE(left) ? 1 : 0) / AS_CSP_FLOAT(right)));

    } else if (IS_CSP_INT(left) && IS_CSP_BOOL(right)) {
        if (IS_CSP_BOOL_FALSE(right)) {
            handleDivisionByZero(processor);
            return;
        }
        push(processor->valueStack, CSP_INT_VALUE(AS_CSP_INT(left) / (IS_CSP_BOOL_TRUE(right) ? 1 : 0)));

    } else if (IS_CSP_BOOL(left) && IS_CSP_INT(right)) {
        if (IS_CSP_INT(right) == 0) {
            handleDivisionByZero(processor);
            return;
        }
        push(processor->valueStack, CSP_FLOAT_VALUE((IS_CSP_BOOL_TRUE(left) ? 1 : 0) / (CSP_FLOAT_TYPE) AS_CSP_INT(right)));

    } else if (IS_CSP_NULL(left) || IS_CSP_NULL(right)) {
        push(processor->valueStack, CSP_NULL_VALUE());

    } else {
        WRITE_INTERPRETER_ERROR(processor, "Division invalid operands");
        push(processor->valueStack, CSP_NULL_VALUE());
    }
}

static void evaluateBinaryPow(ByteCodeProcessor *processor) {
    CspValue right = pop(processor->valueStack);
    CspValue left = pop(processor->valueStack);

    if (IS_CSP_FLOAT(left) && IS_CSP_FLOAT(right)) {
        push(processor->valueStack, CSP_FLOAT_VALUE(CSP_POW(AS_CSP_FLOAT(left), AS_CSP_FLOAT(right))));

    } else if (IS_CSP_FLOAT(left) && IS_CSP_INT(right)) {
        push(processor->valueStack, CSP_FLOAT_VALUE(CSP_POW(AS_CSP_FLOAT(left), (CSP_FLOAT_TYPE) AS_CSP_INT(right))));

    } else if (IS_CSP_INT(left) && IS_CSP_FLOAT(right)) {
        push(processor->valueStack, CSP_FLOAT_VALUE(CSP_POW((CSP_FLOAT_TYPE) AS_CSP_INT(left), AS_CSP_FLOAT(right))));

    } else if (IS_CSP_INT(left) && IS_CSP_INT(right)) {
        push(processor->valueStack, CSP_INT_VALUE((CSP_INT_TYPE) (CSP_POW((CSP_FLOAT_TYPE) AS_CSP_INT(left), AS_CSP_INT(right)))));

    } else if (IS_CSP_NULL(left) || IS_CSP_NULL(right)) {
        push(processor->valueStack, CSP_NULL_VALUE());

    } else {
        WRITE_INTERPRETER_ERROR(processor, "Pow invalid operands");
        push(processor->valueStack, CSP_NULL_VALUE());
    }
}

static void evaluateBinaryReminder(ByteCodeProcessor *processor) {
    CspValue right = pop(processor->valueStack);
    CspValue left = pop(processor->valueStack);

    if (IS_CSP_FLOAT(left) && IS_CSP_FLOAT(right)) {
        push(processor->valueStack, CSP_FLOAT_VALUE(CSP_FMOD(AS_CSP_FLOAT(left), AS_CSP_FLOAT(right))));

    } else if (IS_CSP_FLOAT(left) && IS_CSP_INT(right)) {
        push(processor->valueStack, CSP_FLOAT_VALUE(CSP_FMOD(AS_CSP_FLOAT(left), (CSP_FLOAT_TYPE) AS_CSP_INT(right))));

    } else if (IS_CSP_INT(left) && IS_CSP_FLOAT(right)) {
        push(processor->valueStack, CSP_FLOAT_VALUE(CSP_FMOD((CSP_FLOAT_TYPE) AS_CSP_INT(left), AS_CSP_FLOAT(right))));

    } else if (IS_CSP_INT(left) && IS_CSP_INT(right)) {
        push(processor->valueStack, CSP_INT_VALUE(AS_CSP_INT(left) % AS_CSP_INT(right)));

    } else if (IS_CSP_NULL(left) || IS_CSP_NULL(right)) {
        push(processor->valueStack, CSP_NULL_VALUE());

    } else {
        WRITE_INTERPRETER_ERROR(processor, "Reminder invalid operands");
        push(processor->valueStack, CSP_NULL_VALUE());
    }
}

static bool isBinaryEqualExpr(ByteCodeProcessor *processor) {
    CspValue right = pop(processor->valueStack);
    CspValue left = pop(processor->valueStack);

    if (IS_CSP_FLOAT(left) && IS_CSP_FLOAT(right)) {
        return AS_CSP_FLOAT(left) == AS_CSP_FLOAT(right);

    } else if (IS_CSP_FLOAT(left) && IS_CSP_INT(right)) {
        return AS_CSP_FLOAT(left) == (double) AS_CSP_INT(right);

    } else if (IS_CSP_INT(left) && IS_CSP_FLOAT(right)) {
        return (double) AS_CSP_INT(left) == AS_CSP_FLOAT(right);

    } else if (IS_CSP_INT(left) && IS_CSP_INT(right)) {
        return AS_CSP_INT(left) == AS_CSP_INT(right);

    } else if (IS_CSP_NULL(left) && IS_CSP_NULL(right)) {
        return true;

    } else if (IS_CSP_STRING(left) && IS_CSP_STRING(right)) {
        return AS_CSP_STRING(left)->length == AS_CSP_STRING(right)->length &&
               strcmp(AS_CSP_CSTRING(left), AS_CSP_CSTRING(right)) == 0;

    } else if (IS_CSP_INT(left) && IS_CSP_STRING(right)) {
        BufferString *intStr = INT64_TO_STRING(AS_CSP_INT(left));
        return intStr->length == AS_CSP_STRING(right)->length &&
               strcmp(intStr->value, AS_CSP_CSTRING(right)) == 0;

    } else if (IS_CSP_STRING(left) && IS_CSP_INT(right)) {
        BufferString *intStr = INT64_TO_STRING(AS_CSP_INT(right));
        return intStr->length == AS_CSP_STRING(left)->length &&
               strcmp(intStr->value, AS_CSP_CSTRING(left)) == 0;

    } else if (IS_CSP_FLOAT(left) && IS_CSP_STRING(right)) {
        BufferString *decimal = STRING_FORMAT_32("%f", AS_CSP_FLOAT(left));
        BufferString *result = removeTrailingZeroes(decimal, EMPTY_STRING(32));
        return strcmp(result->value, AS_CSP_CSTRING(right)) == 0;
        
    } else if (IS_CSP_STRING(left) && IS_CSP_FLOAT(right)) {
        BufferString *decimal = STRING_FORMAT_32("%f", AS_CSP_FLOAT(right));
        BufferString *result = removeTrailingZeroes(decimal, EMPTY_STRING(32));
        return strcmp(result->value, AS_CSP_CSTRING(left)) == 0;
        
    } else if (IS_CSP_NULL(left) || IS_CSP_NULL(right)) {
        return false;

    } else if (IS_CSP_BOOL(left)) {
        if (IS_CSP_FLOAT(right)) {
            return (IS_CSP_BOOL_TRUE(left) && AS_CSP_FLOAT(right) != 0.0) ||
                   (IS_CSP_BOOL_FALSE(left) && AS_CSP_FLOAT(right) == 0.0);

        } else if (IS_CSP_INT(right)) {
            return (IS_CSP_BOOL_TRUE(left) && AS_CSP_INT(right) != 0) ||
                   (IS_CSP_BOOL_FALSE(left) && AS_CSP_INT(right) == 0);

        } else if (IS_CSP_STRING(right)) {
            return strcmp(IS_CSP_BOOL_TRUE(left) ? "true" : "false", AS_CSP_CSTRING(right)) == 0;

        } else if (IS_CSP_BOOL(right)) {
            return (IS_CSP_BOOL_TRUE(left) && IS_CSP_BOOL_TRUE(right)) ||
                   (IS_CSP_BOOL_FALSE(left) && IS_CSP_BOOL_FALSE(right));
        }

    } else if (IS_CSP_BOOL(right)) {
        if (IS_CSP_FLOAT(left)) {
            return (IS_CSP_BOOL_TRUE(right) && AS_CSP_FLOAT(left) != 0.0) ||
                   (IS_CSP_BOOL_FALSE(right) && AS_CSP_FLOAT(left) == 0.0);

        } else if (IS_CSP_INT(left)) {
            return (IS_CSP_BOOL_TRUE(right) && AS_CSP_INT(left) != 0) ||
                   (IS_CSP_BOOL_FALSE(right) && AS_CSP_INT(left) == 0);

        } else if (IS_CSP_STRING(left)) {
            return strcmp(IS_CSP_BOOL_TRUE(right) ? "true" : "false", AS_CSP_CSTRING(left)) == 0;

        } else if (IS_CSP_BOOL(left)) {
            return (IS_CSP_BOOL_TRUE(left) && IS_CSP_BOOL_TRUE(right)) ||
                   (IS_CSP_BOOL_FALSE(left) && IS_CSP_BOOL_FALSE(right));
        }
    }

    WRITE_INTERPRETER_ERROR(processor, "Equals evaluation invalid operands");
    return false;
}

static bool isBinaryExprGreater(ByteCodeProcessor *processor) {
    CspValue right = pop(processor->valueStack);
    CspValue left = pop(processor->valueStack);

    if (IS_CSP_FLOAT(left) && IS_CSP_FLOAT(right)) {
        return AS_CSP_FLOAT(left) > AS_CSP_FLOAT(right);

    } else if (IS_CSP_FLOAT(left) && IS_CSP_INT(right)) {
        return AS_CSP_FLOAT(left) > (double) AS_CSP_INT(right);

    } else if (IS_CSP_INT(left) && IS_CSP_FLOAT(right)) {
        return (double) AS_CSP_INT(left) > AS_CSP_FLOAT(right);

    } else if (IS_CSP_INT(left) && IS_CSP_INT(right)) {
        return AS_CSP_INT(left) > AS_CSP_INT(right);

    } else if (IS_CSP_STRING(left) && IS_CSP_STRING(right)) {
        return strcmp(AS_CSP_CSTRING(left), AS_CSP_CSTRING(right)) > 0;

    } else if (IS_CSP_NULL(left) || IS_CSP_NULL(right)) {
        return false;

    } else if (IS_CSP_BOOL(left) && IS_CSP_BOOL(right)) {
        return (IS_CSP_BOOL_TRUE(left) ? 1 : 0) > (IS_CSP_BOOL_TRUE(right) ? 1 : 0);
    }

    WRITE_INTERPRETER_ERROR(processor, "Greater evaluation invalid operands");
    return false;
}

static bool isBinaryExprGreaterEqual(ByteCodeProcessor *processor) {
    CspValue right = pop(processor->valueStack);
    CspValue left = pop(processor->valueStack);

    if (IS_CSP_FLOAT(left) && IS_CSP_FLOAT(right)) {
        return AS_CSP_FLOAT(left) >= AS_CSP_FLOAT(right);

    } else if (IS_CSP_FLOAT(left) && IS_CSP_INT(right)) {
        return AS_CSP_FLOAT(left) >= (double) AS_CSP_INT(right);

    } else if (IS_CSP_INT(left) && IS_CSP_FLOAT(right)) {
        return (double) AS_CSP_INT(left) >= AS_CSP_FLOAT(right);

    } else if (IS_CSP_INT(left) && IS_CSP_INT(right)) {
        return AS_CSP_INT(left) >= AS_CSP_INT(right);

    } else if (IS_CSP_STRING(left) && IS_CSP_STRING(right)) {
        return strcmp(AS_CSP_CSTRING(left), AS_CSP_CSTRING(right)) >= 0;

    } else if (IS_CSP_NULL(left) && IS_CSP_NULL(right)) {
        return true;

    } else if (IS_CSP_NULL(left) || IS_CSP_NULL(right)) {
        return false;

    } else if (IS_CSP_BOOL(left) && IS_CSP_BOOL(right)) {
        return (IS_CSP_BOOL_TRUE(left) ? 1 : 0) >= (IS_CSP_BOOL_TRUE(right) ? 1 : 0);
    }

    WRITE_INTERPRETER_ERROR(processor, "Greater equal evaluation invalid operands");
    return false;
}

static bool isBinaryExprLess(ByteCodeProcessor *processor) {
    CspValue right = pop(processor->valueStack);
    CspValue left = pop(processor->valueStack);

    if (IS_CSP_FLOAT(left) && IS_CSP_FLOAT(right)) {
        return AS_CSP_FLOAT(left) < AS_CSP_FLOAT(right);

    } else if (IS_CSP_FLOAT(left) && IS_CSP_INT(right)) {
        return AS_CSP_FLOAT(left) < (double) AS_CSP_INT(right);

    } else if (IS_CSP_INT(left) && IS_CSP_FLOAT(right)) {
        return (double) AS_CSP_INT(left) < AS_CSP_FLOAT(right);

    } else if (IS_CSP_INT(left) && IS_CSP_INT(right)) {
        return AS_CSP_INT(left) < AS_CSP_INT(right);

    } else if (IS_CSP_STRING(left) && IS_CSP_STRING(right)) {
        return strcmp(AS_CSP_CSTRING(left), AS_CSP_CSTRING(right)) < 0;

    }  else if (IS_CSP_NULL(left) || IS_CSP_NULL(right)) {
        return false;

    } else if (IS_CSP_BOOL(left) && IS_CSP_BOOL(right)) {
        return (IS_CSP_BOOL_TRUE(left) ? 1 : 0) < (IS_CSP_BOOL_TRUE(right) ? 1 : 0);
    }

    WRITE_INTERPRETER_ERROR(processor, "Less evaluation invalid operands");
    return false;
}

static bool isBinaryExprLessEqual(ByteCodeProcessor *processor) {
    CspValue right = pop(processor->valueStack);
    CspValue left = pop(processor->valueStack);

    if (IS_CSP_FLOAT(left) && IS_CSP_FLOAT(right)) {
        return AS_CSP_FLOAT(left) <= AS_CSP_FLOAT(right);

    } else if (IS_CSP_FLOAT(left) && IS_CSP_INT(right)) {
        return AS_CSP_FLOAT(left) <= (double) AS_CSP_INT(right);

    } else if (IS_CSP_INT(left) && IS_CSP_FLOAT(right)) {
        return (double) AS_CSP_INT(left) <= AS_CSP_FLOAT(right);

    } else if (IS_CSP_INT(left) && IS_CSP_INT(right)) {
        return AS_CSP_INT(left) <= AS_CSP_INT(right);

    } else if (IS_CSP_STRING(left) && IS_CSP_STRING(right)) {
        return strcmp(AS_CSP_CSTRING(left), AS_CSP_CSTRING(right)) <= 0;

    }  else if (IS_CSP_NULL(left) || IS_CSP_NULL(right)) {
        return false;

    } else if (IS_CSP_BOOL(left) && IS_CSP_BOOL(right)) {
        return (IS_CSP_BOOL_TRUE(left) ? 1 : 0) <= (IS_CSP_BOOL_TRUE(right) ? 1 : 0);
    }

    WRITE_INTERPRETER_ERROR(processor, "Less equal evaluation invalid operands");
    return false;
}

static bool isUnaryTruthyExpr(ByteCodeProcessor *processor) {
    CspValue right = pop(processor->valueStack);

    if (IS_CSP_BOOL_FALSE(right)) {
        return true;

    } else if (IS_CSP_BOOL_TRUE(right) || IS_CSP_NULL(right)) {  // 	the negation of true is false
        return false;

    } else if (IS_CSP_STRING(right)) { // non-empty string evaluating to true, empty string evaluating to false
        return (AS_CSP_STRING(right)->length > 0) ? false : true;
    }

    WRITE_INTERPRETER_ERROR(processor, "Invalid 'Not operator' -> '!' unary operator type");
    return false;
}

static bool isLiteralTruthyExp(CspValue value) {
    if (IS_CSP_BOOL_TRUE(value)) {
        return true;

    } else if (IS_CSP_STRING(value)) {
        return AS_CSP_STRING(value)->length > 0;

    } else if (IS_CSP_FLOAT(value)) {
        return AS_CSP_FLOAT(value) != 0;

    } else if (IS_CSP_INT(value)) {
        return AS_CSP_INT(value) != 0;
    }

    return false;
}

static CspValue multiplyString(CspObjectString *str, uint16_t count) {
    uint16_t totalLength = str->length * count;
    CspValue value = CSP_STR_VALUE_CONCAT("", "", totalLength);
    while (count > 0) {
        strcat(AS_CSP_STRING(value)->chars, str->chars);
        count--;
    }
    return value;
}

static void stringifyResult(ByteCodeProcessor *processor, CspValue value, CspTableString *resultStr) {
    if (IS_CSP_INT(value)) {
        BufferString *number = INT64_TO_STRING(AS_CSP_INT(value));
        tableStringAdd(resultStr, number->value, number->length);

    } else if (IS_CSP_FLOAT(value)) {
        BufferString *decimal = STRING_FORMAT_32("%f", AS_CSP_FLOAT(value));
        BufferString *result = removeTrailingZeroes(decimal, EMPTY_STRING(32));
        tableStringAdd(resultStr, result->value, result->length);

    } else if (IS_CSP_NULL(value)) {
        tableStringAdd(resultStr, NULL_STR, NULL_STR_LEN);

    } else if (IS_CSP_BOOL_TRUE(value)) {
        tableStringAdd(resultStr, "true", sizeof("true") - 1);

    } else if (IS_CSP_BOOL_FALSE(value)) {
        tableStringAdd(resultStr, "false", sizeof("false") - 1);

    } else if (IS_CSP_STRING(value)) {
        tableStringAdd(resultStr, AS_CSP_CSTRING(value), AS_CSP_STRING(value)->length);

    } else if (IS_CSP_VARIABLE(value)) {
        CspValue paramValue = getVariableFromParams(processor, AS_CSP_VAR_NAME(value));
        stringifyResult(processor, paramValue, resultStr);

    } else if (IS_CSP_ARRAY(value)) {
        arrayObjectToString(processor, value, resultStr);

    } else if (IS_CSP_MAP(value)) {
        mapObjectToString(processor, value, resultStr);

    } else {
        WRITE_INTERPRETER_ERROR_PARAMS(processor, "Can't convert value type to string: [%d]", value.type);
    }
}

static BufferString *removeTrailingZeroes(BufferString *decimal, BufferString *result) {
    int32_t index = indexOfChar(decimal, '.', 0);
    if (index < 0) return result;
    index++;    // skip dot
    if (charAt(decimal, index) == '0') {    // if there is no decimal part, then trim until one zero
        return substringFromTo(decimal, result, 0, index + 1);
    }

    uint32_t length = decimal->length - 1;
    while (length > 0 && decimal->value[length] == '0') {   // move from string back while is zero char
        length--;
    }
    return substringFromTo(decimal, result, 0, length + 1);
}

static void arrayObjectToString(ByteCodeProcessor *processor, CspValue value, CspTableString *resultStr) {
    CspObjectArray *arrayObject = AS_CSP_ARRAY(value);
    tableStringAddChar(resultStr, '[');
    for (uint32_t i = 0; i < cspValVecSize(arrayObject->vec); i++) {
        CspValue arrayValue = cspValVecGet(arrayObject->vec, i);
        stringifyResult(processor, arrayValue, resultStr);

        if (i < cspValVecSize(arrayObject->vec) - 1) {
            tableStringAdd(resultStr, ", ", sizeof(", ") - 1);
        }
    }
    tableStringAddChar(resultStr, ']');
}

static void mapObjectToString(ByteCodeProcessor *processor, CspValue value, CspTableString *resultStr) {
    CspObjectMap *mapObject = AS_CSP_MAP(value);
    if (getCspMapSize(mapObject->map) == 0) {
        tableStringAdd(resultStr, "[:]", sizeof("[:]") - 1);
        return;
    }

    tableStringAddChar(resultStr, '[');
    uint32_t index = 0;
    uint32_t itemsCount = 0;
    while (index < mapObject->map->capacity) {
        CspMapEntry mapEntry = mapObject->map->entries[index];
        if (mapEntry.key != NULL) {
            tableStringAdd(resultStr, mapEntry.key, strlen(mapEntry.key));
            tableStringAddChar(resultStr, ':');
            stringifyResult(processor, mapEntry.value, resultStr);

            if (itemsCount < getCspMapSize(mapObject->map) - 1) {
                tableStringAdd(resultStr, ", ", sizeof(", ") - 1);
            }
            itemsCount++;
        }
        index++;
    }
    tableStringAddChar(resultStr, ']');
}

static void handleDivisionByZero(ByteCodeProcessor *processor) {
    WRITE_INTERPRETER_ERROR(processor, "Division by zero");
    push(processor->valueStack, CSP_NULL_VALUE());
}

static void resetStack(CspStack *stack) {
    stack->stackTop = stack->stack;
}

static void push(CspStack *stack, CspValue value) {
    *stack->stackTop = value;
    stack->stackTop++;
}

static CspValue pop(CspStack *stack) {
    stack->stackTop--;
    return *stack->stackTop;
}

static CspValue peek(CspStack *stack) {
    return *(stack->stackTop - 1);
}

