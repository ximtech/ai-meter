#include "CSPReport.h"

#define CSP_PARSE_ERROR_MESSAGE_LENGTH 128

CspReport *newCspReport(const char *templateName) {
    CspReport *report = malloc(sizeof(struct CspReport));
    if (report == NULL) return NULL;
    report->errorMessage = NULL;
    report->lineNumber = 0;
    report->templateName = templateName;
    return report;
}

void formatCspError(CspReport *report, const char *format, ...) {
    if (report->errorMessage == NULL) {
        report->errorMessage = malloc(sizeof(char) * CSP_ERROR_MESSAGE_LENGTH);
        if (report->errorMessage == NULL) return;
    }
    va_list argp;
    va_start(argp, format);
    memset(report->errorMessage, 0, CSP_ERROR_MESSAGE_LENGTH);
    uint32_t length = sprintf(report->errorMessage, "CSP Error [%s]: ", report->templateName);
    vsnprintf(report->errorMessage + length, CSP_ERROR_MESSAGE_LENGTH, format, argp);
    va_end(argp);
}

void formatCspParserError(CspReport *report, const char *tag, uint32_t errorAt, const char *format, ...) {
    va_list argp;
    va_start(argp, format);
    char buffer[CSP_PARSE_ERROR_MESSAGE_LENGTH];
    uint32_t length = sprintf(buffer, "[%s] - [line: %" PRIu32 "]. Error at %" PRIu32 ": ", tag, report->lineNumber, errorAt);
    vsnprintf(buffer + length, CSP_PARSE_ERROR_MESSAGE_LENGTH, format, argp);
    va_end(argp);
    formatCspError(report, buffer);
}

void deleteCspReport(CspReport *report) {
    if (report != NULL) {
        free(report->errorMessage);
        free(report);
    }
}
