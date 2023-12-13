#pragma once

#include "inttypes.h"

#include "BufferString.h"

#define CSP_ERROR_MESSAGE_LENGTH 256

#define CSP_HAS_ERROR(report) ((report)->errorMessage != NULL)
#define CSP_HAS_NO_ERROR(report) ((report)->errorMessage == NULL)

typedef struct CspReport {
    char *errorMessage;
    uint32_t lineNumber;
    const char *templateName;
} CspReport;


CspReport *newCspReport(const char *templateName);

void formatCspError(CspReport *report, const char *format, ...);
void formatCspParserError(CspReport *report, const char *tag, uint32_t errorAt, const char *format, ...);

void deleteCspReport(CspReport *report);
