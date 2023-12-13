#pragma once

#include "CSPTemplate.h"
#include "CSPInterpreter.h"

#define CSP_BUFFER_SIZE_MULTIPLIER 1.5

typedef struct CspRenderer {
    CspTemplate *cspTemplate;
    CspTableString *tableStr;
    CspObjectMap *paramMap;
    uint32_t tagIndex;
    uint32_t lineNumber;
    char *templateText;
    uint32_t length;
    FILE *file;
} CspRenderer;


#define NEW_CSP_RENDERER(cspTemplate, params) initCspRenderer(&(CspRenderer){0}, cspTemplate, params)

CspRenderer *initCspRenderer(CspRenderer *renderer, CspTemplate *cspTemplate, CspObjectMap *paramMap);
CspTableString *renderCspTemplate(CspRenderer *renderer);

void deleteCspRenderer(CspRenderer *renderer);