#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define CSP_HTML_TAG_START_CHAR '<'
#define CSP_HTML_TAG_END_CHAR '>'
#define CSP_HTML_BACKTICK_QUOTE '`'
#define CSP_HTML_QUOTE_LENGTH 1

#define CSP_HTML_COMMENT_START "<!--"
#define CSP_HTML_COMMENT_END "-->"
#define CSP_HTML_COMMENT_START_LENGTH (sizeof(CSP_HTML_COMMENT_START) - 1)
#define CSP_HTML_COMMENT_END_LENGTH (sizeof(CSP_HTML_COMMENT_END) - 1)

#define CSP_TARGET_TAG "<csp:"
#define CSP_TARGET_TAG_LENGTH (sizeof(CSP_TARGET_TAG) - 1)
#define CSP_TARGET_END_TAG "</csp:"
#define CSP_TARGET_END_TAG_LENGTH (sizeof(CSP_TARGET_END_TAG) - 1)

#define CSP_PARAMETER_START "${"
#define CSP_PARAMETER_END "}"
#define CSP_PARAMETER_START_LENGTH (sizeof(CSP_PARAMETER_START) - 1)
#define CSP_PARAMETER_END_LENGTH (sizeof(CSP_PARAMETER_END) - 1)

#define CSP_SET_TAG_NAME CSP_TARGET_TAG "set"
#define CSP_IF_TAG_NAME CSP_TARGET_TAG "if"
#define CSP_ELSE_IF_TAG_NAME CSP_TARGET_TAG "elseif"
#define CSP_ELSE_TAG_NAME CSP_TARGET_TAG "else"
#define CSP_RENDER_TAG_NAME CSP_TARGET_TAG "render"
#define CSP_LOOP_TAG_NAME CSP_TARGET_TAG "loop"

#define CSP_END_IF_TAG_NAME CSP_TARGET_END_TAG "if"
#define CSP_END_ELSE_IF_TAG_NAME CSP_TARGET_END_TAG "elseif"
#define CSP_END_ELSE_TAG_NAME CSP_TARGET_END_TAG "else"
#define CSP_END_LOOP_TAG_NAME CSP_TARGET_END_TAG "loop"
#define CSP_END_VAR_TAG_NAME CSP_TARGET_END_TAG "var"

#define CSP_ATTR_STR_MAX_LENGTH CSP_TOKEN_STRING_LENGTH

typedef enum CspTagKind {
    CSP_TAG_SET,        // <ct:set>
    CSP_TAG_SET_END,    // </ct:set>, optional
    CSP_TAG_IF,         // <ct:if>
    CSP_TAG_ELSE_IF,    // <ct:elseif>
    CSP_TAG_ELSE,       // <ct:else>
    CSP_TAG_END_IF,     // </ct:if>
    CSP_TAG_END_ELSE_IF,// </ct:elseif>
    CSP_TAG_END_ELSE,   // </ct:else>
    CSP_TAG_RENDER,     // <ct:render>
    CSP_TAG_LOOP,       // <ct:loop>
    CSP_TAG_END_LOOP,   // </ct:loop>
    CSP_TAG_PARAM,      // any other value enclosed in ${...}
} CspTagKind;

typedef struct CspTagKindData {
    CspTagKind kind;
    const char *name;
    uint8_t length;
} CspTagKindData;

static const CspTagKindData CSP_IF = {.kind = CSP_TAG_IF, .name = CSP_IF_TAG_NAME, .length = sizeof(CSP_IF_TAG_NAME) - 1};
static const CspTagKindData CSP_ELSE_IF = {.kind = CSP_TAG_ELSE_IF, .name = CSP_ELSE_IF_TAG_NAME, .length = sizeof(CSP_ELSE_IF_TAG_NAME) - 1};
static const CspTagKindData CSP_ELSE = {.kind = CSP_TAG_ELSE, .name = CSP_ELSE_TAG_NAME, .length = sizeof(CSP_ELSE_TAG_NAME) - 1};
static const CspTagKindData CSP_VAR = {.kind = CSP_TAG_SET, .name = CSP_SET_TAG_NAME, .length = sizeof(CSP_SET_TAG_NAME) - 1};
static const CspTagKindData CSP_RENDER = {.kind = CSP_TAG_RENDER, .name = CSP_RENDER_TAG_NAME, .length = sizeof(CSP_RENDER_TAG_NAME) - 1};
static const CspTagKindData CSP_LOOP = {.kind = CSP_TAG_LOOP, .name = CSP_LOOP_TAG_NAME, .length = sizeof(CSP_LOOP_TAG_NAME) - 1};
static const CspTagKindData CSP_END_IF = {.kind = CSP_TAG_END_IF, .name = CSP_END_IF_TAG_NAME, .length = sizeof(CSP_END_IF_TAG_NAME) - 1};
static const CspTagKindData CSP_END_ELSE_IF = {.kind = CSP_TAG_END_ELSE_IF, .name = CSP_END_ELSE_IF_TAG_NAME, .length = sizeof(CSP_END_ELSE_IF_TAG_NAME) - 1};
static const CspTagKindData CSP_END_ELSE = {.kind = CSP_TAG_END_ELSE, .name = CSP_END_ELSE_TAG_NAME, .length = sizeof(CSP_END_ELSE_TAG_NAME) - 1};
static const CspTagKindData CSP_END_LOOP = {.kind = CSP_TAG_END_LOOP, .name = CSP_END_LOOP_TAG_NAME, .length = sizeof(CSP_END_LOOP_TAG_NAME) - 1};
static const CspTagKindData CSP_END_VAR = {.kind = CSP_TAG_SET_END, .name = CSP_END_VAR_TAG_NAME, .length = sizeof(CSP_END_VAR_TAG_NAME) - 1};

static inline bool isStartsWithHtmlTag(const char *htmlString) {
    return *htmlString == CSP_HTML_TAG_START_CHAR;
}

static inline bool isStartsWithHtmlComment(const char *htmlString) {
    return strncmp(htmlString, CSP_HTML_COMMENT_START, CSP_HTML_COMMENT_START_LENGTH) == 0;
}

static inline bool isStartsWithCspOpenTag(const char *htmlString) {
    return strncasecmp(htmlString, CSP_TARGET_TAG, CSP_TARGET_TAG_LENGTH) == 0;
}

static inline bool isStartsWithCspCloseTag(const char *htmlString) {
    return strncasecmp(htmlString, CSP_TARGET_END_TAG, CSP_TARGET_END_TAG_LENGTH) == 0;
}

static inline bool isStartsWithCspIf(const char *htmlString) {
    return strncasecmp(htmlString, CSP_IF.name, CSP_IF.length) == 0;
}

static inline bool isStartsWithCspElseIf(const char *htmlString) {
    return strncasecmp(htmlString, CSP_ELSE_IF.name, CSP_ELSE_IF.length) == 0;
}

static inline bool isStartsWithCspElse(const char *htmlString) {
    return strncasecmp(htmlString, CSP_ELSE.name, CSP_ELSE.length) == 0;
}

static inline bool isStartsWithCspVar(const char *htmlString) {
    return strncasecmp(htmlString, CSP_VAR.name, CSP_VAR.length) == 0;
}

static inline bool isStartsWithCspRender(const char *htmlString) {
    return strncasecmp(htmlString, CSP_RENDER.name, CSP_RENDER.length) == 0;
}

static inline bool isStartsWithCspLoop(const char *htmlString) {
    return strncasecmp(htmlString, CSP_LOOP.name, CSP_LOOP.length) == 0;
}

static inline bool isStartsWithCspEndIf(const char *htmlString) {
    return strncasecmp(htmlString, CSP_END_IF.name, CSP_END_IF.length) == 0;
}

static inline bool isStartsWithCspEndElseIf(const char *htmlString) {
    return strncasecmp(htmlString, CSP_END_ELSE_IF.name, CSP_END_ELSE_IF.length) == 0;
}

static inline bool isStartsWithCspEndElse(const char *htmlString) {
    return strncasecmp(htmlString, CSP_END_ELSE.name, CSP_END_ELSE.length) == 0;
}

static inline bool isStartsWithCspEndLoop(const char *htmlString) {
    return strncasecmp(htmlString, CSP_END_LOOP.name, CSP_END_LOOP.length) == 0;
}

static inline bool isStartsWithCspEndVar(const char *htmlString) {
    return strncasecmp(htmlString, CSP_END_VAR.name, CSP_END_VAR.length) == 0;
}

static inline bool isStartsWithCspParam(const char *htmlString) {
    return htmlString[0] == CSP_PARAMETER_START[0] && htmlString[1] == CSP_PARAMETER_START[1];
}

static inline bool isStartsWithCspParamEnd(const char *htmlString) {
    return htmlString[0] == CSP_PARAMETER_END[0];
}
