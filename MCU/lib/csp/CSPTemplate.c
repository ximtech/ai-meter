#include "CSPTemplate.h"
#include "CSPTokener.h"

#define CSP_FILE_EXTENSION ".csp"
#define CSP_FILE_EXTENSION_LENGTH (sizeof(CSP_FILE_EXTENSION) - 1)

#define TAG_VECTOR_INIT_SIZE 16
#define ATTRIBUTE_NOT_FOUND (-1)

#define IS_EMPTY_STR(string) ((string) == NULL || (string)[0] == '\0')

typedef struct HtmlAttribute {
    char *name;
    char *value;
} HtmlAttribute;

CREATE_CUSTOM_COMPARATOR(attr, HtmlAttribute, one, two, strcmp(one.name, two.name));
CREATE_VECTOR_TYPE(HtmlAttribute, attr, attrComparator);

static const char *TAG = "CSP Template";

static bool isEndsWithCsp(const char *fileName, uint32_t length);
static CspTemplate *initCspTemplate(CspTemplate *cspTemplate, char *templateBuffer, size_t templateLength);
static CspTemplate *parseHtmlTemplate(CspTemplate *cspTemplate);
static inline void moveToNextChar(CspTemplate *cspTemplate);
static inline bool haveValidPreviousBranching(CspTemplate *cspTemplate);

static void parseIfTag(CspTemplate *cspTemplate, CspTagKind kind);
static void parseVarTag(CspTemplate *cspTemplate);
static void parseLoopTag(CspTemplate *cspTemplate);
static void parseRenderTag(CspTemplate *cspTemplate);
static uint32_t parseParamValue(CspTemplate *cspTemplate);

static void parseTagAttributes(CspTemplate *cspTemplate, attrVector *vector);
static uint32_t findHtmlTagEnd(CspTemplate *cspTemplate, char *htmlTagString);
static char *trimLeadingSpacesAndQuotes(char *string);
static char *trimEndSpacesAndQuotes(char *string);
static char *trimSpacesAndQuotes(char *string);

static CspTagNode *newVarTagNode(CspTemplate *cspTemplate, CspChunk *varCodeChunk, const char *varName);
static CspTagNode *newLoopTagNode(CspTemplate *cspTemplate, char *arrayName, const char *varName, const char *statusParam);
static CspTagNode *newCspTagNode(CspTemplate *cspTemplate, CspTagKind kind);
static inline void formatCspTemplateError(CspTemplate *cspTemplate, const char *message);
static void deleteCspTemplateData(CspTemplate *cspTemplate);
static void calculateTemplateTotalLength(CspTemplate *cspTemplate);


CspTemplate *newCspTemplate(const char *fileName) {
    CspTemplate *cspTemplate = calloc(1, sizeof(struct CspTemplate));
    if (cspTemplate == NULL) return NULL;

    CspReport *report = newCspReport(!IS_EMPTY_STR(fileName) ? fileName : "Unknown");
    if (report == NULL) {
        free(cspTemplate);
        return NULL;
    }
    cspTemplate->report = report;
    cspTemplate->report->errorMessage = NULL;

    size_t fileNameLength = fileName != NULL ? strnlen(fileName, PATH_MAX_LEN) : 0;
    if (fileNameLength == 0) {
        formatCspError(cspTemplate->report, "Template file name is empty");
        return cspTemplate;
    }

    if (!isEndsWithCsp(fileName, fileNameLength)) {
        formatCspError(cspTemplate->report, "Invalid template file extension. Only [" CSP_FILE_EXTENSION "] supported");
        return cspTemplate;
    }

    File *templateFile = malloc(sizeof(struct File));
    cspTemplate->templateFile = newFile(templateFile, fileName);
    if (!isFileExists(cspTemplate->templateFile)) {
        formatCspError(cspTemplate->report, "Can't open template file");
        deleteCspTemplateData(cspTemplate);
        return cspTemplate;
    }

    size_t templateSize = getFileSize(cspTemplate->templateFile);
    if (getFileSize(cspTemplate->templateFile) == 0) {
        formatCspError(cspTemplate->report, "Empty template file");
        deleteCspTemplateData(cspTemplate);
        return cspTemplate;
    }

    char *templateBuffer = malloc(sizeof(char) * templateSize + 1);
    if (templateBuffer == NULL) {
        formatCspError(cspTemplate->report, "Memory allocation failed for content");
        deleteCspTemplateData(cspTemplate);
        return cspTemplate;
    }

    size_t length = readFileToBuffer(cspTemplate->templateFile, templateBuffer, templateSize);
    if (length != templateSize) {
        formatCspError(cspTemplate->report, "Error while template file read");
        deleteCspTemplateData(cspTemplate);
        return cspTemplate;
    }

    return initCspTemplate(cspTemplate, templateBuffer, length);
}

bool isCspTemplateOk(CspTemplate *cspTemplate) {
    return cspTemplate != NULL && cspTemplate->report != NULL && CSP_HAS_NO_ERROR(cspTemplate->report);
}

char *cspTemplateErrorMessage(CspTemplate *cspTemplate) {
    return cspTemplate != NULL && cspTemplate->report != NULL ? cspTemplate->report->errorMessage : NULL;
}

uint32_t cspTemplateErrorOnLine(CspTemplate *cspTemplate) {
    return cspTemplate != NULL && cspTemplate->report != NULL ? cspTemplate->report->lineNumber : 0;
}

void deleteCspTemplate(CspTemplate *cspTemplate) {
    if (cspTemplate != NULL) {
        deleteCspTemplateData(cspTemplate);
        deleteCspReport(cspTemplate->report);
        free(cspTemplate);
    }
}

static bool isEndsWithCsp(const char *fileName, uint32_t length) {
    if (length < CSP_FILE_EXTENSION_LENGTH) return false;
    for (uint32_t i = CSP_FILE_EXTENSION_LENGTH; i > 0; i--, length--) {
        if (fileName[length - 1] != CSP_FILE_EXTENSION[i - 1]) {
            return false;
        }
    }
    return true;
}

static CspTemplate *initCspTemplate(CspTemplate *cspTemplate, char *templateBuffer, size_t templateLength) {
    cspTemplate->contents = templateBuffer;
    cspTemplate->report->lineNumber = 1;
    cspTemplate->length = templateLength;
    cspTemplate->remainingLength = templateLength;
    cspTemplate->nextKind = cspTemplate->contents;
    cspTemplate->includeCount = CSP_MAX_NESTED_INCLUDES;
    cspTemplate->tagVector = getVectorInstance(TAG_VECTOR_INIT_SIZE);
    if (cspTemplate->tagVector == NULL) {
        formatCspError(cspTemplate->report, "Memory allocation for tag vector for template: [%s]", cspTemplate->templateFile->path);
        deleteCspTemplateData(cspTemplate);
        return cspTemplate;
    }

    parseHtmlTemplate(cspTemplate);
    free(cspTemplate->contents);
    cspTemplate->contents = NULL;
    cspTemplate->nextKind = NULL;
    calculateTemplateTotalLength(cspTemplate);

    if (isVectorEmpty(cspTemplate->tagVector)) {    // if template do not contain dynamic data then vector can be deleted to save some space
        vectorDelete(cspTemplate->tagVector);
        cspTemplate->tagVector = NULL;
    }
    return cspTemplate;
}

static CspTemplate *parseHtmlTemplate(CspTemplate *cspTemplate) {
    uint32_t openedTagCount = 0;
    uint32_t closedTagCount = 0;

    while (*cspTemplate->nextKind != '\0' && CSP_HAS_NO_ERROR(cspTemplate->report)) {
        if (*cspTemplate->nextKind == '\n') {
            cspTemplate->report->lineNumber++;
            moveToNextChar(cspTemplate);
            continue;
        }

        if (*cspTemplate->nextKind == CSP_HTML_BACKTICK_QUOTE) {    // need to skip all in backtick quotes '`', because JS have string interpolation with same ${} parameters
            cspTemplate->nextKind++;
            char *backtickEnd = strchr(cspTemplate->nextKind, CSP_HTML_BACKTICK_QUOTE);
            if (backtickEnd == NULL) {
                formatCspTemplateError(cspTemplate, "Unclosed backtick quote: '`'");
                return cspTemplate;
            }

            uint32_t contentLength = (backtickEnd - cspTemplate->nextKind) + CSP_HTML_QUOTE_LENGTH;
            cspTemplate->remainingLength -= contentLength;
            cspTemplate->nextKind += contentLength;
            continue;
        }

        if (*cspTemplate->nextKind != CSP_HTML_TAG_START_CHAR && !isStartsWithCspParam(cspTemplate->nextKind)) {  // check for tag or parameter
            moveToNextChar(cspTemplate);
            continue;
        }

        if (cspTemplate->remainingLength > CSP_HTML_COMMENT_START_LENGTH && isStartsWithHtmlComment(cspTemplate->nextKind)) {  // skip comment
            char *commentEnd = strstr(cspTemplate->nextKind, CSP_HTML_COMMENT_END);
            if (commentEnd == NULL) {
                formatCspTemplateError(cspTemplate, "Unclosed Html comment tag");
                return cspTemplate;
            }

            uint32_t commentLength = (commentEnd - cspTemplate->nextKind) + CSP_HTML_COMMENT_END_LENGTH;
            cspTemplate->remainingLength -= commentLength;
            cspTemplate->nextKind += commentLength;
            continue;
        }

        if (cspTemplate->remainingLength > CSP_TARGET_TAG_LENGTH && isStartsWithCspOpenTag(cspTemplate->nextKind)) {
            if (isStartsWithCspIf(cspTemplate->nextKind)) {
                cspTemplate->nextKind += CSP_IF.length;
                parseIfTag(cspTemplate, CSP_IF.kind);
                openedTagCount++;

            } else if (isStartsWithCspVar(cspTemplate->nextKind )) {
                cspTemplate->nextKind += CSP_VAR.length;
                parseVarTag(cspTemplate);

            } else if (isStartsWithCspElseIf(cspTemplate->nextKind)) {
                cspTemplate->nextKind += CSP_ELSE_IF.length;
                if (!haveValidPreviousBranching(cspTemplate)) {
                    formatCspTemplateError(cspTemplate, "Invalid '" CSP_ELSE_IF_TAG_NAME "' branching");
                    break;
                }

                parseIfTag(cspTemplate, CSP_ELSE_IF.kind);
                openedTagCount++;

            } else if (isStartsWithCspElse(cspTemplate->nextKind)) {
                cspTemplate->nextKind += CSP_ELSE.length;
                if (!haveValidPreviousBranching(cspTemplate)) {
                    formatCspTemplateError(cspTemplate, "Invalid '" CSP_ELSE_TAG_NAME "' branching");
                    break;
                }

                CspTagNode *tagNode = newCspTagNode(cspTemplate, CSP_TAG_ELSE);
                if (tagNode == NULL) break;
                vectorAdd(cspTemplate->tagVector, tagNode);
                openedTagCount++;

            } else if (isStartsWithCspRender(cspTemplate->nextKind)) {
                cspTemplate->nextKind += CSP_RENDER.length;
                parseRenderTag(cspTemplate);

            } else if (isStartsWithCspLoop(cspTemplate->nextKind)) {
                cspTemplate->nextKind += CSP_LOOP.length;
                parseLoopTag(cspTemplate);
                openedTagCount++;

            } else {
                formatCspTemplateError(cspTemplate, "Unknown tag after [" CSP_TARGET_TAG" ]");
                return cspTemplate;
            }

        } else if (cspTemplate->remainingLength >= CSP_TARGET_END_TAG_LENGTH && isStartsWithCspCloseTag(cspTemplate->nextKind)) {
            if (isStartsWithCspEndIf(cspTemplate->nextKind)) {
                cspTemplate->nextKind += CSP_END_IF.length;
                CspTagNode *tagNode = newCspTagNode(cspTemplate, CSP_END_IF.kind);
                vectorAdd(cspTemplate->tagVector, tagNode);
                closedTagCount++;

            } else if (isStartsWithCspEndElseIf(cspTemplate->nextKind)) {
                cspTemplate->nextKind += CSP_END_ELSE_IF.length;
                CspTagNode *tagNode = newCspTagNode(cspTemplate, CSP_END_ELSE_IF.kind);
                vectorAdd(cspTemplate->tagVector, tagNode);
                closedTagCount++;

            } else if (isStartsWithCspEndElse(cspTemplate->nextKind)) {
                cspTemplate->nextKind += CSP_END_ELSE.length;
                CspTagNode *tagNode = newCspTagNode(cspTemplate, CSP_END_ELSE.kind);
                vectorAdd(cspTemplate->tagVector, tagNode);
                closedTagCount++;

            } else if (isStartsWithCspEndLoop(cspTemplate->nextKind)) {
                cspTemplate->nextKind += CSP_END_LOOP.length;
                CspTagNode *tagNode = newCspTagNode(cspTemplate, CSP_END_LOOP.kind);
                vectorAdd(cspTemplate->tagVector, tagNode);
                closedTagCount++;

            } else if (isStartsWithCspEndVar(cspTemplate->nextKind)) {
                cspTemplate->nextKind += CSP_END_VAR.length;
                CspTagNode *tagNode = newCspTagNode(cspTemplate, CSP_END_VAR.kind);
                vectorAdd(cspTemplate->tagVector, tagNode);
                closedTagCount++;

            } else {
                formatCspTemplateError(cspTemplate, "Unknown closing tag after [" CSP_TARGET_END_TAG "]");
                return cspTemplate;
            }

        } else if (cspTemplate->remainingLength >= CSP_PARAMETER_START_LENGTH && isStartsWithCspParam(cspTemplate->nextKind)) {
            uint32_t paramLength = parseParamValue(cspTemplate);
            if (paramLength == 0) break;
            cspTemplate->nextKind += paramLength;
            cspTemplate->remainingLength -= paramLength;
        }

        moveToNextChar(cspTemplate);
    }

    if (CSP_HAS_NO_ERROR(cspTemplate->report) && openedTagCount != closedTagCount) {
        formatCspParserError(cspTemplate->report, TAG, 0, "Template contains unclosed tags. Opened: [%d]. Closed: [%d]", openedTagCount, closedTagCount);
    }

    return cspTemplate;
}

static inline void moveToNextChar(CspTemplate *cspTemplate) {
    cspTemplate->nextKind++;
    cspTemplate->remainingLength--;
}

static inline bool haveValidPreviousBranching(CspTemplate *cspTemplate) {
    CspTagNode *previousNode = vectorGet(cspTemplate->tagVector, getVectorSize(cspTemplate->tagVector) - 1);
    return previousNode != NULL && (previousNode->kind == CSP_TAG_END_IF || previousNode->kind == CSP_TAG_END_ELSE_IF);
}

static void parseIfTag(CspTemplate *cspTemplate, CspTagKind kind) {
    attrVector *vector = NEW_VECTOR_4(HtmlAttribute, attr);
    parseTagAttributes(cspTemplate, vector);
    if (CSP_HAS_ERROR(cspTemplate->report)) {
        return;
    }

    int32_t attrIndex = attrVecIndexOf(vector, (HtmlAttribute){.name = "test"});
    if (attrIndex == ATTRIBUTE_NOT_FOUND) {
        formatCspTemplateError(cspTemplate, "Mandatory attribute for [if] tag: [test] not found");
        return;
    }

    HtmlAttribute attribute = attrVecGet(vector, attrIndex);
    lexTokenVector *tokens = NEW_VECTOR(CspLexerToken*, lexToken, CSP_TOKEN_MAX_COUNT);
    parseTemplateExpression(cspTemplate->report, tokens, attribute.value);
    if (CSP_HAS_ERROR(cspTemplate->report)) {
        return;
    }

    CspChunk *ifCodeChunk = cspCompile(cspTemplate->report, tokens);
    if (CSP_HAS_ERROR(cspTemplate->report)) return;

    CspTagNode *ifTagNode = newCspTagNode(cspTemplate, kind);
    if (ifTagNode == NULL) return;

    ifTagNode->valueCode = ifCodeChunk;
    vectorAdd(cspTemplate->tagVector, ifTagNode);
}

static void parseVarTag(CspTemplate *cspTemplate) {
    attrVector *vector = NEW_VECTOR_4(HtmlAttribute, attr);
    parseTagAttributes(cspTemplate, vector);
    if (CSP_HAS_ERROR(cspTemplate->report)) {
        return;
    }

    int32_t varNameIndex = attrVecIndexOf(vector, (HtmlAttribute){.name = "var"});
    if (varNameIndex == ATTRIBUTE_NOT_FOUND) {
        formatCspTemplateError(cspTemplate, "Mandatory attribute for [set] tag: [var] not found");
        return;
    }

    int32_t varValueIndex = attrVecIndexOf(vector, (HtmlAttribute){.name = "value"});
    if (varValueIndex == ATTRIBUTE_NOT_FOUND) {
        formatCspTemplateError(cspTemplate, "Mandatory attribute for [set] tag: [value] not found");
        return;
    }

    char *varName = attrVecGet(vector, varNameIndex).value;
    char *varValue = attrVecGet(vector, varValueIndex).value;
    lexTokenVector *tokens = NEW_VECTOR(CspLexerToken*, lexToken, CSP_TOKEN_MAX_COUNT);
    parseTemplateExpression(cspTemplate->report, tokens, varValue);
    if (CSP_HAS_ERROR(cspTemplate->report)) {
        return;
    }

    CspChunk *varCodeChunk = cspCompile(cspTemplate->report, tokens);
    if (CSP_HAS_ERROR(cspTemplate->report)) {
        return;
    }

    CspTagNode *tagNode = newVarTagNode(cspTemplate, varCodeChunk, varName);
    vectorAdd(cspTemplate->tagVector, tagNode);
}

static void parseLoopTag(CspTemplate *cspTemplate) {
    attrVector *vector = NEW_VECTOR_4(HtmlAttribute, attr);
    parseTagAttributes(cspTemplate, vector);
    if (CSP_HAS_ERROR(cspTemplate->report)) {
        return;
    }

    int32_t inIndex = attrVecIndexOf(vector, (HtmlAttribute){.name = "in"});
    if (inIndex == ATTRIBUTE_NOT_FOUND) {
        formatCspTemplateError(cspTemplate, "Mandatory attribute for [loop] tag: [in] not found");
        return;
    }

    int32_t varIndex = attrVecIndexOf(vector, (HtmlAttribute){.name = "var"});
    char *varName = varIndex != ATTRIBUTE_NOT_FOUND ? attrVecGet(vector, varIndex).value : "it";

    int32_t statusIndex = attrVecIndexOf(vector, (HtmlAttribute){.name = "status"});
    char *statusParam = statusIndex != ATTRIBUTE_NOT_FOUND ? attrVecGet(vector, statusIndex).value : NULL;

    HtmlAttribute inAttribute = attrVecGet(vector, inIndex);
    CspTagNode *tagNode = newLoopTagNode(cspTemplate, inAttribute.value, varName, statusParam);
    vectorAdd(cspTemplate->tagVector, tagNode);
}

static void parseRenderTag(CspTemplate *cspTemplate) {
    if (cspTemplate->includeCount == 0) {
        formatCspError(cspTemplate->report, "Too many template render includes > [%d]", CSP_MAX_NESTED_INCLUDES);
        return;
    }

    attrVector *vector = NEW_VECTOR_4(HtmlAttribute, attr);
    parseTagAttributes(cspTemplate, vector);
    if (CSP_HAS_ERROR(cspTemplate->report)) {
        return;
    }

    int32_t templateIndex = attrVecIndexOf(vector, (HtmlAttribute){.name = "template"});
    if (templateIndex == ATTRIBUTE_NOT_FOUND) {
        formatCspTemplateError(cspTemplate, "Mandatory attribute for [render] tag: [template] not found");
        return;
    }

    CspTemplate *renderTemplate = newCspTemplate(attrVecGet(vector, templateIndex).value);
    if (renderTemplate == NULL) {
        formatCspTemplateError(cspTemplate, "User-defined template return NULL");
        return;
    }
    renderTemplate->includeCount = cspTemplate->includeCount;

    if (renderTemplate->report->errorMessage != NULL) {
        formatCspTemplateError(cspTemplate, renderTemplate->report->errorMessage);
        deleteCspTemplate(renderTemplate);
        return;
    }

    CspTagNode *renderTagNode = newCspTagNode(cspTemplate, CSP_TAG_RENDER);
    renderTagNode->cspTemplate = renderTemplate;
    vectorAdd(cspTemplate->tagVector, renderTagNode);
}

static uint32_t parseParamValue(CspTemplate *cspTemplate) {
   char *paramStr = cspTemplate->nextKind;
   paramStr += CSP_PARAMETER_START_LENGTH; // skip param start

   char *paramEnd = paramStr;
   size_t remainingLength = cspTemplate->remainingLength;
   while (!isStartsWithCspParamEnd(paramEnd) && remainingLength > 0) { // find parameter end
       remainingLength--;
       paramEnd++;
   }

   if (!isStartsWithCspParamEnd(paramEnd) || remainingLength == 0) {
       formatCspTemplateError(cspTemplate, "Unclosed template parameter");
       return 0;
   }
   uint32_t paramLength = paramEnd - paramStr;
   paramStr[paramLength] = '\0'; // drop param end

    lexTokenVector *tokens = NEW_VECTOR(CspLexerToken*, lexToken, CSP_TOKEN_MAX_COUNT);
    parseTemplateExpression(cspTemplate->report, tokens, paramStr);
    if (CSP_HAS_ERROR(cspTemplate->report)) {
        return 0;
    }

    if (islexTokenVecEmpty(tokens)) {
        formatCspTemplateError(cspTemplate, "Empty template parameter");
        return 0;
    }

    CspChunk *paramCode = cspCompile(cspTemplate->report, tokens);
    if (CSP_HAS_ERROR(cspTemplate->report)) {
        return 0;
    }

    CspTagNode *paramTagNode = newCspTagNode(cspTemplate, CSP_TAG_PARAM);
    paramTagNode->valueCode = paramCode;
    vectorAdd(cspTemplate->tagVector, paramTagNode);
    return CSP_PARAMETER_START_LENGTH + paramLength + CSP_PARAMETER_END_LENGTH;
}

static void parseTagAttributes(CspTemplate *cspTemplate, attrVector *vector) {
    while (isspace((int) *cspTemplate->nextKind)) {  // skip starting whitespaces
        cspTemplate->nextKind++;
    }

    uint32_t tagLength = findHtmlTagEnd(cspTemplate, cspTemplate->nextKind);
    char *tagEndPtr = cspTemplate->nextKind + tagLength;  // find close tag
    if (CSP_HAS_ERROR(cspTemplate->report)) { // check for errors
        return;
    }

    char *attrPointer = cspTemplate->nextKind;
    char *sepPtr = strstr(attrPointer, "=");   // attribute name
    while (sepPtr != NULL && tagEndPtr - sepPtr > 0) {
        HtmlAttribute attr = {0};
        uint32_t nameLength = sepPtr - attrPointer;
        attrPointer[nameLength] = '\0';
        attr.name = trimSpacesAndQuotes(attrPointer); // set attribute name

        attrPointer += nameLength + sizeof("=\"") - 1;   // skip attribute name and separator
        attrPointer = trimLeadingSpacesAndQuotes(attrPointer);
        sepPtr = strstr(attrPointer, "\"");  // move to attribute value end
        if (sepPtr == NULL) {
            formatCspTemplateError(cspTemplate, "Unterminated attribute value");
            break;
        }

        uint32_t valueLength = sepPtr - attrPointer;
        attrPointer[valueLength] = '\0';
        if (isStartsWithCspParam(attrPointer) && isStartsWithCspParamEnd(attrPointer + (valueLength - 1))) {    // check that parameter value correctly formatted
            attrPointer[valueLength - 1] = '\0';   // remove end parenthesis
            attrPointer += CSP_PARAMETER_START_LENGTH;     // skip first '$' and '{'
        }
        attr.value = attrPointer;               // set value

        if (!attrVecAdd(vector, attr)) {    // check for vector overflow
            formatCspTemplateError(cspTemplate, "Too many tag attributes");
            break;
        }
        attrPointer += valueLength + 1;  // skip value

        sepPtr = strstr(attrPointer, "=\""); // move to next attribute
    }

    cspTemplate->nextKind += tagLength;    // skip all attributes line, its already processed
    cspTemplate->remainingLength -= tagLength;
}

static uint32_t findHtmlTagEnd(CspTemplate *cspTemplate, char *htmlTagString) {
    uint32_t tagLength = 0;
    while (*htmlTagString != CSP_HTML_TAG_END_CHAR && *htmlTagString != '\0' && tagLength < CSP_ATTR_STR_MAX_LENGTH) {

        if (isStartsWithCspParam(htmlTagString)) {
            while (!isStartsWithCspParamEnd(htmlTagString)) {
                if (*htmlTagString == '\0' || tagLength >= CSP_ATTR_STR_MAX_LENGTH) {
                    formatCspTemplateError(cspTemplate, "No closing '" CSP_PARAMETER_END "' found for parameter tag");
                    return tagLength;
                }
                tagLength++;
                htmlTagString++;
            }
        }
        tagLength++;
        htmlTagString++;
    }

    if (tagLength >= CSP_ATTR_STR_MAX_LENGTH) {
        formatCspTemplateError(cspTemplate, "Too long html parameter");
    }
    return tagLength;
}

static char *trimLeadingSpacesAndQuotes(char *string) {
    while (isspace((int) *string) || *string == '\"') {
        string++;
    }
    return string;
}

static char *trimEndSpacesAndQuotes(char *string) {
    // Trim trailing space
    char * stringEnd = string + strlen(string) - 1;
    while (stringEnd > string && (isspace((int) *stringEnd) || *stringEnd == '\"')) {
        stringEnd--;
    }
    // Write new null terminator character
    stringEnd[1] = '\0';
    return string;
}

static char *trimSpacesAndQuotes(char *string) {
    string = trimLeadingSpacesAndQuotes(string);
    return trimEndSpacesAndQuotes(string);
}

static CspTagNode *newVarTagNode(CspTemplate *cspTemplate, CspChunk *varCodeChunk, const char *varName) {
    CspTagNode *tagNode = newCspTagNode(cspTemplate, CSP_TAG_SET);
    if (CSP_HAS_ERROR(cspTemplate->report)) {
        return NULL;
    }

    CspVarTag *varTag = malloc(sizeof(struct CspVarTag));
    if (varTag == NULL) {
        free(tagNode);
        formatCspTemplateError(cspTemplate, "Memory allocation fail for [CspVarTag] struct");
        return NULL;
    }

    char *varNameCopy = malloc(sizeof(char) * strlen(varName) + 1);
    if (varNameCopy == NULL) {
        free(tagNode);
        free(varTag);
        formatCspTemplateError(cspTemplate, "Loop [var] parameter memory allocate fail");
        return NULL;
    }
    strcpy(varNameCopy, varName);


    tagNode->varTag = varTag;
    tagNode->varTag->varCode = varCodeChunk;
    tagNode->varTag->varName = varNameCopy;
    return tagNode;
}

static CspTagNode *newLoopTagNode(CspTemplate *cspTemplate, char *arrayName, const char *varName, const char *statusParam) {
    if (varName == NULL) {
        formatCspTemplateError(cspTemplate, "Mandatory loop [var] parameter is NULL");
        return NULL;
    }

    CspTagNode *tagNode = newCspTagNode(cspTemplate, CSP_TAG_LOOP);
    if (CSP_HAS_ERROR(cspTemplate->report)) {
        return NULL;
    }

    CspLoopTag *loopTag = malloc(sizeof(struct CspLoopTag));
    if (loopTag == NULL) {
        free(tagNode);
        formatCspTemplateError(cspTemplate, "Memory allocation fail for [CspLoopTag] struct");
        return NULL;
    }

    char *arrayNameCopy = malloc(sizeof(char) * strlen(arrayName) + 1);
    if (arrayNameCopy == NULL) {
        free(tagNode);
        free(loopTag);
        formatCspTemplateError(cspTemplate, "Loop [var] parameter memory allocate fail");
        return NULL;
    }
    strcpy(arrayNameCopy, arrayName);

    char *varNameCopy = malloc(sizeof(char) * strlen(varName) + 1);
    if (varNameCopy == NULL) {
        free(tagNode);
        free(loopTag);
        free(arrayNameCopy);
        formatCspTemplateError(cspTemplate, "Loop [var] parameter memory allocate fail");
        return NULL;
    }
    strcpy(varNameCopy, varName);

    char *indexParam = NULL;
    if (statusParam != NULL) {
        indexParam = malloc(sizeof(char) * strlen(statusParam) + 1);
        if (indexParam == NULL) {
            free(tagNode);
            free(loopTag);
            free(varNameCopy);
            free(arrayNameCopy);
            formatCspTemplateError(cspTemplate, "Loop [status] parameter memory allocate fail");
            return NULL;
        }
        strcpy(indexParam, statusParam);
    }

    loopTag->arrayName = arrayNameCopy;
    loopTag->varName = varNameCopy;
    loopTag->statusParam = indexParam;
    tagNode->loopTag = loopTag;
    return tagNode;
}

static CspTagNode *newCspTagNode(CspTemplate *cspTemplate, CspTagKind kind) {
    CspTagNode *tagNode = malloc(sizeof(struct CspTagNode));
    if (tagNode == NULL) {
        formatCspTemplateError(cspTemplate, "Memory allocation fail for [CspTagNode] struct");
        return NULL;
    }
    tagNode->kind = kind;
    return tagNode;
}

static inline void formatCspTemplateError(CspTemplate *cspTemplate, const char *message) {
    formatCspParserError(cspTemplate->report, TAG, cspTemplate->length - cspTemplate->remainingLength, message);
}

static void deleteCspTemplateData(CspTemplate *cspTemplate) {
    if (cspTemplate != NULL) {
        free(cspTemplate->templateFile);
        free(cspTemplate->contents);

        for (uint32_t i = 0; i < getVectorSize(cspTemplate->tagVector); i++) {// release all tags in vector
            CspTagNode *tagNode = vectorGet(cspTemplate->tagVector, i);
            switch (tagNode->kind) {
                case CSP_TAG_SET:
                    free(tagNode->varTag->varName);
                    cspChunkDelete(tagNode->varTag->varCode);
                    free(tagNode->varTag);
                    break;
                case CSP_TAG_IF:
                case CSP_TAG_ELSE_IF:
                case CSP_TAG_PARAM:
                    cspChunkDelete(tagNode->valueCode);
                    break;
                case CSP_TAG_RENDER:
                    deleteCspTemplate(tagNode->cspTemplate);
                    break;
                case CSP_TAG_LOOP:
                    free(tagNode->loopTag->varName);
                    free(tagNode->loopTag->statusParam);
                    free(tagNode->loopTag->arrayName);
                    free(tagNode->loopTag);
                    break;
                case CSP_TAG_ELSE:
                case CSP_TAG_END_IF:
                case CSP_TAG_END_ELSE_IF:
                case CSP_TAG_END_ELSE:
                case CSP_TAG_END_LOOP:
                case CSP_TAG_SET_END:
                    break;
            }

            free(tagNode);
        }
        vectorDelete(cspTemplate->tagVector);

        cspTemplate->templateFile = NULL;
        cspTemplate->contents = NULL;
        cspTemplate->tagVector = NULL;
        cspTemplate->remainingLength = 0;
    }
}

static void calculateTemplateTotalLength(CspTemplate *cspTemplate) {
    for (uint32_t i = 0; i < getVectorSize(cspTemplate->tagVector); i++) {
        CspTagNode *tagNode = vectorGet(cspTemplate->tagVector, i);
        if (tagNode->kind == CSP_TAG_RENDER) {
            cspTemplate->length += tagNode->cspTemplate->length;
        }
    }
}
