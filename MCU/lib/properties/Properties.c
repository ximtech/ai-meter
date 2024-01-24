#include "Properties.h"

#define PROPERTY_FILE_EXTENSION ".properties"
#define IS_NOT_MULTILINE(textStart, textEnd) ((getLineEndBackslashCount(textStart, textEnd) % 2) == 0)
#define BACKSLASH_AND_ESCAPED_CHAR 2

static const char *const PROPERTY_STATUS_MSG_TABLE[] = {
        [CONFIG_PROP_OK] = "OK",
        [CONFIG_PROP_ERROR_INVALID_FILE_TYPE] = "Invalid config properties file extension. Only '.properties' files allowed",
        [CONFIG_PROP_ERROR_FILE_NOT_FOUND] = "Provided file for config properties does not exist",
        [CONFIG_PROP_ERROR_CREATE_FILE] = "Failed to create config property file",
        [CONFIG_PROP_ERROR_OPEN_FILE] = "Failed to open config property file",
        [CONFIG_PROP_ERROR_MEMORY_ALLOC] = "Memory allocation error",
        [CONFIG_PROP_ERROR_READ_FILE] = "Config properties file read error",
        [CONFIG_PROP_ERROR_MEMORY_ALLOC_KEY] = "Config property key memory allocation error",
        [CONFIG_PROP_ERROR_MEMORY_ALLOC_VALUE] = "Config property value memory allocation error",
};

static void parsePropertyBuffer(Properties *properties, char *dataBuffer);
static uint32_t trimLeadingWhitespaces(char *text);
static char *findNewLineChar(char *text);
static uint8_t getLineEndBackslashCount(const char *textStart, char *textEnd);
static char *resolveMultiline(char *textLine);
static void handlePropertyLine(Properties *properties, char *textLine);
static char *findMultilineIfPresent(char *textLine, uint8_t *separatorLength);
static char *trimLineEnd(char *string);
static bool savePropertyKeyValue(Properties *properties, char *key, char *value);
static char *splitValueByDelimiter(char *textLine);
static char *trimSpacesAndQuotes(char *string);
static void removePropertyEntry(Properties *properties, MapEntry *entry);


Properties *loadProperties(Properties *properties, const char *fileName) {
    if (properties == NULL) return NULL;

    BufferString *filePath = NEW_STRING(FILENAME_MAX, fileName);
    if (!isStrEndsWith(filePath, PROPERTY_FILE_EXTENSION)) {
        properties->status = CONFIG_PROP_ERROR_INVALID_FILE_TYPE;
        return properties;
    }

    File *propertyFile = NEW_FILE(fileName);
    if (!isFileExists(propertyFile)) {
        properties->status = CONFIG_PROP_ERROR_FILE_NOT_FOUND;
        return properties;
    }

    uint32_t fileSize = getFileSize(propertyFile);
    char *dataBuffer = malloc(sizeof(char) * (fileSize + 1));
    if (dataBuffer == NULL) {
        properties->status = CONFIG_PROP_ERROR_MEMORY_ALLOC;
        return properties;
    }

    uint32_t dataLength = readFileToBuffer(propertyFile, dataBuffer, fileSize);
    if (dataLength != fileSize) {
        properties->status = CONFIG_PROP_ERROR_READ_FILE;
        free(dataBuffer);
        return properties;
    }

    initProperties(properties);
    if (properties->status != CONFIG_PROP_OK) {
        free(dataBuffer);
        return properties;
    }

    parsePropertyBuffer(properties, dataBuffer);
    free(dataBuffer);
    return properties;
}

Properties *loadPropertiesBuffer(Properties *properties, char *dataBuffer) {
    if (properties == NULL) return NULL;

    properties->map = getHashMapInstance(PROPERTIES_INITIAL_CAPACITY);
    if (properties->map == NULL) {
        deleteConfigProperties(properties);
        properties->status = CONFIG_PROP_ERROR_MEMORY_ALLOC;
        return properties;
    }

    parsePropertyBuffer(properties, dataBuffer);
    return properties;
}

Properties *initProperties(Properties *properties) {
    if (properties->map == NULL) {  // can be already created
        properties->map = getHashMapInstance(PROPERTIES_INITIAL_CAPACITY);
        if (properties->map == NULL) {
            deleteConfigProperties(properties);
            properties->status = CONFIG_PROP_ERROR_MEMORY_ALLOC;
            return properties;
        }
    }
    properties->status = CONFIG_PROP_OK;
    return properties;
}

void storeProperties(Properties *properties, const char *fileName) {
    if (properties == NULL || properties->map == NULL) return;

    if (fileName == NULL || strstr(fileName, PROPERTY_FILE_EXTENSION) == NULL) {
        properties->status = CONFIG_PROP_ERROR_INVALID_FILE_TYPE;
        return;
    }

    File *propertyFile = NEW_FILE(fileName);
    if (propertyFile == NULL || !createFile(propertyFile)) {
        properties->status = CONFIG_PROP_ERROR_CREATE_FILE;
        return;
    }

    propertyFile->file = fopen(propertyFile->path, "w");  // also clear all contents
    if (propertyFile->file == NULL) {
        properties->status = CONFIG_PROP_ERROR_OPEN_FILE;
        return;
    }

    HashMapIterator iterator = getHashMapIterator(properties->map);
    while (hashMapHasNext(&iterator)) {
        if (iterator.value == NULL) {
            fprintf(propertyFile->file, "%s\n", iterator.key);
            continue;
        }

        fprintf(propertyFile->file, "%s=%s\n", iterator.key, (char *) iterator.value);
    }
    fclose(propertyFile->file);
}

const char *propStatusToString(PropertiesStatus status) {
    return status <= CONFIG_PROP_ERROR_READ_FILE ? PROPERTY_STATUS_MSG_TABLE[status] : "Unknown";
}

void deleteConfigProperties(Properties *properties) {
    if (properties != NULL && properties->map != NULL) {
        HashMapIterator iterator = getHashMapIterator(properties->map);
        while (hashMapHasNext(&iterator)) {
            free((char *) iterator.key);
            free(iterator.value);
        }
        hashMapDelete(properties->map);
    }
}

void propertiesToString(Properties *properties, char *buffer, int32_t length) {
    int32_t lengthWritten;
    HashMapIterator iterator = getHashMapIterator(properties->map);
    while (hashMapHasNext(&iterator) && length > 0) {
        if (iterator.value == NULL) {
            lengthWritten = snprintf(buffer, length, "[%s]\n", (char *) iterator.key);
            buffer += lengthWritten;
            length -= lengthWritten;
            continue;
        }
        lengthWritten = snprintf(buffer, length, "[%s]=[%s]\n", iterator.key, (char *) iterator.value);
        buffer += lengthWritten;
        length -= lengthWritten;
    }
}

bool putProperty(Properties *properties, char *key, char *value) {
    if (key == NULL) return false;
    MapEntry *entry = hashMapGetEntry(properties->map, key);
    if (entry != NULL) {
        removePropertyEntry(properties, entry);
    }
    return savePropertyKeyValue(properties, key, value);
}

void propertiesRemove(Properties *properties, char *key) {
    MapEntry *entry = hashMapGetEntry(properties->map, key);
    if (entry != NULL) {
        removePropertyEntry(properties, entry);
    }
}

void propertiesPutAll(Properties *from, Properties *to) {
    HashMapIterator iterator = getHashMapIterator(from->map);
    while (hashMapHasNext(&iterator)) {
        putProperty(to, (char *) iterator.key, iterator.value);
    }
}

static void parsePropertyBuffer(Properties *properties, char *dataBuffer) {
    for (char *text = dataBuffer; *text != '\0'; text++) {
        text += trimLeadingWhitespaces(text);
        if (*text == '#' || *text == '!') {
            text = strchr(text, '\n');   // skip comments line
            if (text == NULL) {
                break;  // no new lines after comments, exit the loop
            }
            continue;   // move to the next line
        }

        char *textLine = text;
        char *lineEnd = findNewLineChar(text);
        if (lineEnd == NULL) {  // No new line is found, end of data
            handlePropertyLine(properties, textLine);  // no line end '\n' char found, then handle all remaining string
            break;
        }

        *lineEnd = '\0';    // terminate line
        uint32_t lineLength = strlen(textLine);
        handlePropertyLine(properties, textLine);
        if (properties->status != CONFIG_PROP_OK) {
            break;
        }
        text += lineLength; // move to next line
    }
}

static uint32_t trimLeadingWhitespaces(char *text) {
    uint32_t skippedChars = 0;
    while (isspace((int) *text)) {
        text++;
        skippedChars++;
    }
    return skippedChars;
}

static char *findNewLineChar(char *text) {
    char *textPointer = text;
    while ((textPointer = strchr(textPointer, '\n')) != NULL) {
        if (IS_NOT_MULTILINE(text, textPointer)) {    // if no backslash or even count
            return textPointer;
        }
        textPointer++;
    }
    return textPointer;
}

static uint8_t getLineEndBackslashCount(const char *textStart, char *textEnd) {
    char *textPointer = textEnd;
    uint32_t lineLength = textEnd - textStart;

    uint8_t backslashCount = 0;
    for (; lineLength > 0 && *textPointer != '\0'; textPointer--, lineLength--) {   // count backslashes
        if (*textPointer == '\n' || *textPointer == '\r') { // skip '\r', '\n' and move to next char
            continue;
        }

        if (*textPointer != '\\') { // if not backslash no need to continue
            break;
        }

        backslashCount++;
    }

    return backslashCount;
}

static void handlePropertyLine(Properties *properties, char *textLine) {
    resolveMultiline(textLine);    // check for multiline value and resolve to one line
    trimLineEnd(textLine);

    char *key = textLine;
    char *value = splitValueByDelimiter(textLine);
    savePropertyKeyValue(properties, trimSpacesAndQuotes(key), trimSpacesAndQuotes(value));
}

static char *resolveMultiline(char *textLine) {
    char *textPointer;
    uint8_t separatorLength = 0;
    while ((textPointer = findMultilineIfPresent(textLine, &separatorLength)) != NULL) {
        uint32_t targetLength = 0;
        char *target = textPointer + separatorLength;  // Skip multiline separator
        char charToRemove = *target;
        while (charToRemove != '\0' && isspace((int) charToRemove)) { // skip whitespaces
            targetLength++;
            charToRemove = target[targetLength];
        }

        targetLength += separatorLength;
        uint32_t tailLength = strlen(textPointer + targetLength);
        memmove(textPointer, textPointer + targetLength, tailLength + 1);   // remove all whitespaces
    }
    return textLine;
}

static char *findMultilineIfPresent(char *textLine, uint8_t *separatorLength) {
    char *textPointer = strstr(textLine, "\n");
    if (textPointer == NULL) return NULL;

    uint32_t lineLength = textPointer - textLine;
    uint8_t backslashCount = 0;
    for (; lineLength > 0 && *textPointer != '\0'; textPointer--, lineLength--) {   // count backslashes
        if (*textPointer == '\n' || *textPointer == '\r') { // skip '\r', '\n' and move to next char
            *separatorLength += 1;
            continue;
        }

        if (*textPointer != '\\') { // if not backslash no need to continue
            break;
        }

        backslashCount++;
        *separatorLength += 1;
    }

    bool isOddNumberOfBacklashes = backslashCount % 2 != 0;
    if (isOddNumberOfBacklashes) {
        textPointer++;  // move to line separator start
        return textPointer;
    }

    return NULL;
}

static char *trimLineEnd(char *string) {    // Trim trailing spaces and backslashes
    char *stringEnd = string + strlen(string) - 1;
    while (stringEnd > string && (isspace((int) *stringEnd) || *stringEnd == '\\')) {
        stringEnd--;
    }

    stringEnd[1] = '\0'; // Write new null terminator character
    return string;
}

static bool savePropertyKeyValue(Properties *properties, char *key, char *value) {
    uint32_t keyLength = strlen(key);
    if (keyLength == 0) {
        return false;
    }

    char *propertyKey = malloc(sizeof(char) * (keyLength + 1));
    if (propertyKey == NULL) {
        properties->status = CONFIG_PROP_ERROR_MEMORY_ALLOC_KEY;
        return false;
    }
    strcpy(propertyKey, key);

    char *propertyValue = NULL;
    uint32_t valueLength = (value != NULL) ? strlen(value) : 0;
    if (valueLength > 0) {
        propertyValue = malloc(sizeof(char) * (valueLength + 1));
        if (propertyValue == NULL) {
            free(propertyKey);
            properties->status = CONFIG_PROP_ERROR_MEMORY_ALLOC_VALUE;
            return false;
        }
        strcpy(propertyValue, value);
    }

    hashMapPut(properties->map, propertyKey, propertyValue);
    properties->status = CONFIG_PROP_OK;
    return true;
}

static char *splitValueByDelimiter(char *textLine) {
    static const char DELIMITERS[] = {'=', ':', ' '};   // whitespace have the lowest priority

    char *textPointer = textLine;
    for (uint32_t i = 0; i < ARRAY_SIZE(DELIMITERS); i++) {
        char keyValueDelimiter = DELIMITERS[i];

        while (*textPointer != '\0') {
            if (*textPointer == '\\') {
                textPointer += BACKSLASH_AND_ESCAPED_CHAR; // skip slash and escaped char
                continue;
            }

            if (*textPointer == keyValueDelimiter) {
                *textPointer = '\0';
                return (textPointer + 1);
            }

            textPointer++;
        }

        textPointer = textLine;
    }
    return NULL;
}

static char *trimSpacesAndQuotes(char *string) {
    if (string == NULL) return NULL;

    while (isspace((int) *string) || *string == '"') {// Trim leading space and quotes
        string++;
    }

    if (*string == '\0') { // All spaces?
        return string;
    }

    // Trim trailing space and quotes
    char *stringEnd = string + strlen(string) - 1;
    while (stringEnd > string && (isspace((int) *stringEnd) || *stringEnd == '"')) {
        stringEnd--;
    }

    stringEnd[1] = '\0';    // Write new null terminator character
    return string;
}

static void removePropertyEntry(Properties *properties, MapEntry *entry) {
    char *entryKey = (char *) entry->key;
    char *entryValue = entry->value;
    hashMapRemoveEntry(properties->map, entry);
    free(entryKey);
    free(entryValue);
}
