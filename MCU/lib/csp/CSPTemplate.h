#pragma once

#include "FileUtils.h"
#include "Vector.h"
#include "HashMap.h"
#include "CSPCompiler.h"
#include "CSPDataUtils.h"

#ifndef CSP_MAX_NESTED_INCLUDES
#define CSP_MAX_NESTED_INCLUDES 10
#endif

typedef struct CspTemplate {
    CspReport *report;
    File *templateFile;           // name of template file
    char *contents;               // contents of template file
    size_t length;
    size_t remainingLength;
    char *nextKind;
    Vector tagVector;
    uint8_t includeCount;
} CspTemplate;

typedef struct CspVarTag {	// global scope
    CspChunk *varCode;
    char *varName;
} CspVarTag;

typedef struct CspLoopTag {	// local scope for vars
    char *arrayName;
    char *varName;
    char *statusParam;
} CspLoopTag;

typedef struct CspTagNode {
    CspTagKind kind;
    union {
        CspVarTag *varTag;
        CspLoopTag *loopTag;
        CspChunk *valueCode;    // if tag or param -> ${some.param}
        CspTemplate *cspTemplate;   // render tag
    };
} CspTagNode;


CspTemplate *newCspTemplate(const char *fileName);

bool isCspTemplateOk(CspTemplate *cspTemplate);
char *cspTemplateErrorMessage(CspTemplate *aTemplate);
uint32_t cspTemplateErrorOnLine(CspTemplate *aTemplate);

void deleteCspTemplate(CspTemplate *aTemplate);
