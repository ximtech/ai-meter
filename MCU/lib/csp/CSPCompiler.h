#pragma once

#include "CSPValue.h"

//#define CSP_DEBUG_TRACE_EXECUTION

typedef enum CspOpCode {
    CSP_OP_VARIABLE,
    CSP_OP_CONSTANT,
    CSP_OP_NEGATE,
    CSP_OP_ADD,
    CSP_OP_SUBTRACT,
    CSP_OP_MULTIPLY,
    CSP_OP_DIVIDE,
    CSP_OP_POWER,
    CSP_OP_REMINDER,
    CSP_OP_NOT,
    CSP_OP_EQUAL,
    CSP_OP_NOT_EQUAL,
    CSP_OP_GREATER,
    CSP_OP_GREATER_EQUAL,
    CSP_OP_LESS,
    CSP_OP_LESS_EQUAL,
    CSP_OP_JUMP_IF_FALSE,
    CSP_OP_JUMP,
    CSP_OP_POP,
} CspOpCode;

typedef struct CspChunk {
    uint8_t *code;
    uint8_t capacity;
    uint8_t size;
    CspValVector *constants;
} CspChunk;


CspChunk *cspCompile(CspReport *report, lexTokenVector *tokens);

uint8_t cspChunkGet(CspChunk *chunk, uint8_t index);
uint8_t getCspChunkSize(CspChunk *chunk);
void cspChunkDelete(CspChunk *chunk);

#ifdef CSP_DEBUG_TRACE_EXECUTION
int disassembleInstruction(CspChunk *chunk, int offset);
void printCspValue(CspValue value);
#endif