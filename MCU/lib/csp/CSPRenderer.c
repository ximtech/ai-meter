#include "CSPRenderer.h"

#define IS_END_OF_CSP_TEMPLATE(renderer) (*((renderer)->templateText) == '\0')

static const char *TAG = "CSP Renderer";

static CspRenderer *initRendererParams(CspRenderer *renderer, CspTemplate *cspTemplate, CspObjectMap *paramMap, CspTableString *tableStr);
static CspTableString *renderTemplate(CspRenderer *renderer);
static inline void moveToNextChar(CspRenderer *renderer);

static void renderBranchingTag(CspRenderer *renderer, CspTagKind kind);
static void renderLoopTag(CspRenderer *renderer);
static void renderVarTag(CspRenderer *renderer);
static void renderRenderTag(CspRenderer *renderer);
static void collapseNextBranchExecution(CspRenderer *renderer);
static void skipToNextBranchExecution(CspRenderer *renderer);
static uint32_t skipLoopInnerBlock(CspRenderer *renderer, char *htmlTagString);
static inline bool hasNextCspBranching(CspRenderer *renderer);

static CspTagNode *getNextTagNode(CspRenderer *renderer, CspTagKind kind);
static void skipUntilTagEnd(CspRenderer *renderer);
static void skipUntilParamEnd(CspRenderer *renderer);
static void skipUntilCommentEnd(CspRenderer *renderer);
static void skipNextWhitespaces(CspRenderer *renderer);

static uint32_t getBacktickQuoteContentLength(CspRenderer *renderer);
static uint32_t getHtmlCommentLength(CspRenderer *renderer);
static uint32_t getHtmlTagLength(char *htmlTagString);
static uint32_t getTemplateParamLength(char *htmlTagString);
static inline void formatCspRendererError(CspRenderer *renderer, const char *message);
static inline void moveFilePointer(CspRenderer *renderer, uint32_t length);


CspRenderer *initCspRenderer(CspRenderer *renderer, CspTemplate *cspTemplate, CspObjectMap *paramMap) {
    if (renderer == NULL || !isCspTemplateOk(cspTemplate)) {
        return NULL;
    }

    uint32_t templateSize = (uint32_t) (cspTemplate->length * CSP_BUFFER_SIZE_MULTIPLIER);
    CspTableString *tableStr = newTableString(templateSize);
    if (tableStr == NULL) {
        formatCspError(cspTemplate->report, "[%s] - Memory allocation fail for [CspTableString] for size: [%d]", TAG, templateSize);
        return NULL;
    }
    return initRendererParams(renderer, cspTemplate, paramMap, tableStr);
}

CspTableString *renderCspTemplate(CspRenderer *renderer) {
    if (renderer == NULL || !isCspTemplateOk(renderer->cspTemplate)) {
        return NULL;
    }

    char *templateText = CSP_STRING_MALLOC(renderer->cspTemplate->length * sizeof(char) + 1);
    if (templateText == NULL) {
        formatCspError(renderer->cspTemplate->report, "[%s] - Memory allocate fail for tmp file buffer", TAG);
        return NULL;
    }

    renderer->length = readFileToBuffer(renderer->cspTemplate->templateFile, templateText, renderer->cspTemplate->length);
    if (renderer->length == 0) {
        formatCspError(renderer->cspTemplate->report, "[%s] - File read error", TAG);
        CSP_STRING_FREE(templateText);
        renderer->templateText = NULL;
        return NULL;
    }
    renderer->templateText = templateText;

    CspTableString *resultStr = renderTemplate(renderer);
    CSP_STRING_FREE(templateText);
    renderer->templateText = NULL;
    return resultStr;
}

void deleteCspRenderer(CspRenderer *renderer) {
    if (renderer != NULL) {
        freeCspObjects();
        deleteTableString(renderer->tableStr);
    }
}

static CspRenderer *initRendererParams(CspRenderer *renderer, CspTemplate *cspTemplate, CspObjectMap *paramMap, CspTableString *tableStr) {
    renderer->cspTemplate = cspTemplate;
    renderer->paramMap = paramMap;
    renderer->tableStr = tableStr;
    renderer->tagIndex = 0;
    renderer->lineNumber = 1;
    renderer->length = 0;
    return renderer;
}

static CspTableString *renderTemplate(CspRenderer *renderer) {
    while (CSP_HAS_NO_ERROR(renderer->cspTemplate->report) && !IS_END_OF_CSP_TEMPLATE(renderer)) {

        if (*renderer->templateText == '\n') {
            renderer->lineNumber++;
            tableStringAddChar(renderer->tableStr, *renderer->templateText);
            moveToNextChar(renderer);
            continue;

        } else if (*renderer->templateText == CSP_HTML_BACKTICK_QUOTE) {
            uint32_t contentLength = getBacktickQuoteContentLength(renderer);
            tableStringAdd(renderer->tableStr, renderer->templateText, contentLength);
            moveFilePointer(renderer, contentLength);
            continue;

        } else if (!isStartsWithHtmlTag(renderer->templateText) && !isStartsWithCspParam(renderer->templateText)) {
            tableStringAddChar(renderer->tableStr, *renderer->templateText);
            moveToNextChar(renderer);
            continue;

        } else if (isStartsWithHtmlComment(renderer->templateText)) {  // write comments as is
            uint32_t commentLength = getHtmlCommentLength(renderer);
            tableStringAdd(renderer->tableStr, renderer->templateText, commentLength);
            moveFilePointer(renderer, commentLength);
            continue;

        } else if (isStartsWithCspOpenTag(renderer->templateText)) {
            if (isStartsWithCspIf(renderer->templateText)) {
                renderBranchingTag(renderer, CSP_TAG_IF);

            } else if (isStartsWithCspElseIf(renderer->templateText)) {
                renderBranchingTag(renderer, CSP_TAG_ELSE_IF);

            } else if (isStartsWithCspElse(renderer->templateText)) {
                renderer->tagIndex++;
                skipUntilTagEnd(renderer);
                skipNextWhitespaces(renderer);

            } else if (isStartsWithCspLoop(renderer->templateText)) {
                skipUntilTagEnd(renderer);
                skipNextWhitespaces(renderer);
                renderLoopTag(renderer);

            } else if (isStartsWithCspVar(renderer->templateText)) {
                skipUntilTagEnd(renderer);
                skipNextWhitespaces(renderer);
                renderVarTag(renderer);

            } else if (isStartsWithCspRender(renderer->templateText)) {
                skipUntilTagEnd(renderer);
                skipNextWhitespaces(renderer);
                renderRenderTag(renderer);
            }

        } else if (isStartsWithCspCloseTag(renderer->templateText)) {
            if (isStartsWithCspEndIf(renderer->templateText) || isStartsWithCspEndElseIf(renderer->templateText)) {
                skipUntilTagEnd(renderer);
                renderer->tagIndex++;
                return renderer->tableStr;

            } else if (isStartsWithCspEndLoop(renderer->templateText)) {
                return renderer->tableStr;

            }
            renderer->tagIndex++;
            skipUntilTagEnd(renderer);
            skipNextWhitespaces(renderer);

        } else if (isStartsWithCspParam(renderer->templateText)) {
            CspTagNode *paramTagNode = getNextTagNode(renderer, CSP_TAG_PARAM);
            if (paramTagNode == NULL) break;
            interpretCspChunk(renderer->cspTemplate->report, paramTagNode->valueCode, renderer->tableStr, renderer->paramMap);
            skipUntilParamEnd(renderer);

        } else {
            tableStringAddChar(renderer->tableStr, *renderer->templateText);
            moveToNextChar(renderer);
        }
    }

    return renderer->tableStr;
}

static inline void moveToNextChar(CspRenderer *renderer) {
    renderer->templateText++;
    renderer->length--;
}

static void renderBranchingTag(CspRenderer *renderer, CspTagKind kind) {
    CspTagNode *tagNode = getNextTagNode(renderer, kind);
    if (tagNode == NULL) return;

    bool isTruthyExpr = isTruthyCspExp(renderer->cspTemplate->report, tagNode->valueCode, renderer->paramMap);
    if (isTruthyExpr) { // if 'true' expand tag
        skipUntilTagEnd(renderer);
        skipNextWhitespaces(renderer);
        renderTemplate(renderer);  // render inner block

        if (hasNextCspBranching(renderer)) {
            collapseNextBranchExecution(renderer); // collapse all other branching if present
        }
        skipNextWhitespaces(renderer);
        return;
    }

    skipUntilTagEnd(renderer);
    skipNextWhitespaces(renderer);
    skipToNextBranchExecution(renderer); // skip entire if block with all nested blocks, then check if else or else statements
    skipNextWhitespaces(renderer);
}

static void renderLoopTag(CspRenderer *renderer) {
    CspTagNode *tagNode = getNextTagNode(renderer, CSP_TAG_LOOP);
    if (tagNode == NULL) return;

    CspLoopTag *loopTag = tagNode->loopTag;
    CspValue loopValue = cspMapGet(renderer->paramMap->map, loopTag->arrayName);
    if (IS_CSP_NULL(loopValue)) {
        formatCspParserError(renderer->cspTemplate->report, TAG, renderer->lineNumber, "Array parameter not found: [%s]", loopTag->arrayName);
        return;
    }

    if (!IS_CSP_ARRAY(loopValue)) {
        formatCspRendererError(renderer, "[" CSP_LOOP_TAG_NAME "] - Unsupported value type. Only arrays is allowed");
        moveFilePointer(renderer, skipLoopInnerBlock(renderer, renderer->templateText));
        skipNextWhitespaces(renderer);
        return;
    }

    if (loopTag->statusParam != NULL) {
        cspAddIntToMap(renderer->paramMap, loopTag->statusParam, 0);
    }

    uint32_t beforeLoopTagIndex = renderer->tagIndex;   // save tag index counter to refresh it after each iteration
    char *previousTextPointer = renderer->templateText;
    uint32_t fileCharCounter = renderer->length;

    CspValVector *paramVec = AS_CSP_ARRAY(loopValue)->vec;
    if (isCspValVecEmpty(paramVec)) {
        moveFilePointer(renderer, skipLoopInnerBlock(renderer, renderer->templateText));
        skipNextWhitespaces(renderer);
        return;
    }

    for (uint32_t i = 0; i < cspValVecSize(paramVec); i++) {
        CspValue arrayElement = cspValVecGet(paramVec, i);
        renderer->tagIndex = beforeLoopTagIndex;
        renderer->templateText = previousTextPointer;
        renderer->length = fileCharCounter;
        cspMapPut(renderer->paramMap->map, loopTag->varName, arrayElement);
        renderTemplate(renderer);

        if (loopTag->statusParam != NULL) {
            CspValue counterValue = cspMapGet(renderer->paramMap->map, loopTag->statusParam);
            AS_CSP_INT(counterValue) += 1;
            cspMapPut(renderer->paramMap->map, loopTag->statusParam, counterValue);
        }
    }

    renderer->tagIndex++;   // skip loop closing tag
    skipUntilTagEnd(renderer);  // remove tag
    skipNextWhitespaces(renderer);
}

static void renderVarTag(CspRenderer *renderer) {
    CspTagNode *tagNode = getNextTagNode(renderer, CSP_TAG_SET);
    if (tagNode == NULL) return;

    CspVarTag *varTag = tagNode->varTag;
    CspValue varValue = evaluateToCspValue(renderer->cspTemplate->report, varTag->varCode, renderer->paramMap);
    cspMapPut(renderer->paramMap->map, varTag->varName, varValue);
}

static void renderRenderTag(CspRenderer *renderer) {
    CspTagNode *renderTagNode = getNextTagNode(renderer, CSP_TAG_RENDER);
    if (renderTagNode == NULL) return;
    CspRenderer *newRenderer = initRendererParams(&(CspRenderer){0}, renderTagNode->cspTemplate, renderer->paramMap, renderer->tableStr);
    renderCspTemplate(newRenderer);
}

static void collapseNextBranchExecution(CspRenderer *renderer) {
    while (!IS_END_OF_CSP_TEMPLATE(renderer)) { // resolve condition branching
        if (!isStartsWithHtmlTag(renderer->templateText) && !isStartsWithCspParam(renderer->templateText)) {
            moveToNextChar(renderer);
            continue;
        }

        if (isStartsWithHtmlComment(renderer->templateText)) {
            skipUntilCommentEnd(renderer);

        } else if (isStartsWithCspElseIf(renderer->templateText) || isStartsWithCspElse(renderer->templateText)) {
            skipUntilTagEnd(renderer);  // skip current else if tag end
            skipNextWhitespaces(renderer);
            renderer->tagIndex++;   // skip current 'else if' tag node

        } else if (isStartsWithCspEndIf(renderer->templateText) || isStartsWithCspEndElseIf(renderer->templateText)) {
            skipUntilTagEnd(renderer);  // skip current if tag end
            skipNextWhitespaces(renderer);
            renderer->tagIndex++;
            if (!hasNextCspBranching(renderer)) { // check that 'if' block can contain 'else if' or 'else' statements, need also to skip them
                break;  // single if block
            }

        } else if (isStartsWithCspEndElse(renderer->templateText)) {
            skipUntilTagEnd(renderer);
            renderer->tagIndex++;
            break;  // 'else' is the last endpoint

        } else if (isStartsWithCspParam(renderer->templateText)) {
            skipUntilParamEnd(renderer);
            renderer->tagIndex++;

        } else {
            moveToNextChar(renderer);
        }
    }
}

static void skipToNextBranchExecution(CspRenderer *renderer) {
    int32_t ifTagCounter = 1;   // entered here from 'if' or 'else if' tag, so starting from 1

    while (!IS_END_OF_CSP_TEMPLATE(renderer)) {

        if (!isStartsWithHtmlTag(renderer->templateText) && !isStartsWithCspParam(renderer->templateText)) {
            moveToNextChar(renderer);
            continue;
        }

        if (isStartsWithHtmlComment(renderer->templateText)) {
            skipUntilCommentEnd(renderer);

        } else if (isStartsWithCspIf(renderer->templateText)) {
            skipUntilTagEnd(renderer);
            renderer->tagIndex++;   // skip value of precalculated tag value
            ifTagCounter++;

        } else if (isStartsWithCspElseIf(renderer->templateText)) {
            if (ifTagCounter == 0 && hasNextCspBranching(renderer)) {
                renderBranchingTag(renderer, CSP_TAG_ELSE_IF);
                break;
            }
            ifTagCounter++;
            skipUntilTagEnd(renderer);
            renderer->tagIndex++;

        } else if (isStartsWithCspElse(renderer->templateText)) {
            skipUntilTagEnd(renderer);
            skipNextWhitespaces(renderer);
            renderer->tagIndex++;
            if (ifTagCounter == 0) {
                renderTemplate(renderer);  // render else block
                break;
            }

        } else if (isStartsWithCspEndIf(renderer->templateText)) {
            skipUntilTagEnd(renderer);
            renderer->tagIndex++;
            ifTagCounter--;
            if (ifTagCounter == 0 && !hasNextCspBranching(renderer)) {
                break;
            }

        } else if (isStartsWithCspEndElseIf(renderer->templateText)) {
            skipUntilTagEnd(renderer);
            renderer->tagIndex++;
            ifTagCounter--;
        }

        else if (isStartsWithCspEndElse(renderer->templateText)) {
            skipUntilTagEnd(renderer);
            renderer->tagIndex++;
        }

        else if (isStartsWithCspParam(renderer->templateText)) {
            skipUntilParamEnd(renderer);
            renderer->tagIndex++;

        } else {
            moveToNextChar(renderer);
        }
    }
}

static uint32_t skipLoopInnerBlock(CspRenderer *renderer, char *htmlTagString) {
    uint32_t blockLength = 0;
    while (!isStartsWithCspEndLoop(htmlTagString) && *htmlTagString != '\0') {
        if (isStartsWithHtmlComment(htmlTagString)) {
            skipUntilCommentEnd(renderer);
        }

        if (isStartsWithCspParam(htmlTagString) || isStartsWithCspOpenTag(htmlTagString) || isStartsWithCspCloseTag(htmlTagString)) {
            renderer->tagIndex++;   // skip inner param
        }

        if (isStartsWithCspLoop(htmlTagString)) {
            renderer->tagIndex++;
            uint32_t loopStartTagLength = getHtmlTagLength(htmlTagString);
            htmlTagString += loopStartTagLength;
            uint32_t innerBlockLength = skipLoopInnerBlock(renderer, htmlTagString);

            htmlTagString += innerBlockLength;
            blockLength += (loopStartTagLength + innerBlockLength); // nested block length from tag start to tag end
            continue;
        }

        blockLength++;
        htmlTagString++;
    }

    renderer->tagIndex++;   // skip loop closing tag
    return blockLength + getHtmlTagLength(htmlTagString);   // + length of closing tag
}

static inline bool hasNextCspBranching(CspRenderer *renderer) {
    CspTagNode *nextTagNode = vectorGet(renderer->cspTemplate->tagVector, renderer->tagIndex);
    return nextTagNode != NULL && (nextTagNode->kind == CSP_TAG_ELSE_IF || nextTagNode->kind == CSP_TAG_ELSE);
}

static CspTagNode *getNextTagNode(CspRenderer *renderer, CspTagKind kind) {
    CspTagNode *tagNode = vectorGet(renderer->cspTemplate->tagVector, renderer->tagIndex);
    if (tagNode == NULL || tagNode->kind != kind) {
        formatCspRendererError(renderer, "Unexpected [CspTagNode] type for requested value");
        return NULL;
    }
    renderer->tagIndex++;
    return tagNode;
}

static void skipUntilTagEnd(CspRenderer *renderer) {
    uint32_t tagLength = getHtmlTagLength(renderer->templateText);
    moveFilePointer(renderer, tagLength);
}

static void skipUntilParamEnd(CspRenderer *renderer) {
    uint32_t paramLength = getTemplateParamLength(renderer->templateText);
    moveFilePointer(renderer, paramLength);
}

static void skipUntilCommentEnd(CspRenderer *renderer) {
    uint32_t commentLength = getHtmlCommentLength(renderer);
    moveFilePointer(renderer, commentLength);
}

static void skipNextWhitespaces(CspRenderer *renderer) {
    char templateChar = *renderer->templateText;
    while (isspace((int) templateChar)) {
        moveToNextChar(renderer);
        templateChar = *renderer->templateText;
    }
}

static uint32_t getBacktickQuoteContentLength(CspRenderer *renderer) {
    char *backtickEnd = strchr(renderer->templateText + CSP_HTML_QUOTE_LENGTH, CSP_HTML_BACKTICK_QUOTE); // skip opening '`'
    return (backtickEnd - renderer->templateText) + (CSP_HTML_QUOTE_LENGTH * 2);  // '`'...'`'
}

static uint32_t getHtmlCommentLength(CspRenderer *renderer) {
    char *commentEnd = strstr(renderer->templateText, CSP_HTML_COMMENT_END);
    return (commentEnd - renderer->templateText) + CSP_HTML_COMMENT_END_LENGTH;
}

static uint32_t getHtmlTagLength(char *htmlTagString) {
    uint32_t tagLength = 0;
    while (*htmlTagString != CSP_HTML_TAG_END_CHAR && *htmlTagString != '\0') {

        if (isStartsWithCspParam(htmlTagString)) {
            while (!isStartsWithCspParamEnd(htmlTagString)) {
                tagLength++;
                htmlTagString++;
            }
        }
        tagLength++;
        htmlTagString++;
    }

    return tagLength > 0 ? tagLength + 1 : tagLength; // skip also closing tag char '>'
}

static uint32_t getTemplateParamLength(char *htmlTagString) {
    uint32_t paramLength = 0;
    while (!isStartsWithCspParamEnd(htmlTagString) && *htmlTagString != '\0') {
        paramLength++;
        htmlTagString++;
    }
    return paramLength > 0 ? paramLength + CSP_PARAMETER_END_LENGTH : paramLength; // skip also closing param
}

static inline void formatCspRendererError(CspRenderer *renderer, const char *message) {
    formatCspParserError(renderer->cspTemplate->report, TAG, renderer->lineNumber, message);
}

static inline void moveFilePointer(CspRenderer *renderer, uint32_t length) {
    renderer->templateText += length;
    renderer->length -= length;
}
