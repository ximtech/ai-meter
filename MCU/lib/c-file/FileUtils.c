#include "FileUtils.h"

#define NO_FILE_INFO (-1)
#define MULTIPLE_PATH_SEPARATORS FILE_NAME_SEPARATOR_STR FILE_NAME_SEPARATOR_STR

#if defined(_WIN32) || defined(_WIN64)
    #define FILE_NAME_SEPARATOR_TO_REPLACE "/"
#else
    #define FILE_NAME_SEPARATOR_TO_REPLACE "\\"
#endif

#if defined(_WIN32) || defined(_WIN64)
    #define PATH_SEPARATOR_TO_REPLACE ";"
#else
    #define PATH_SEPARATOR_TO_REPLACE ":"
#endif

static File fileBuffer[MAX_FILES_IN_DIR] = {0};

static File *normalizePath(File *file, const char *path);
static void listFilesInDir(File *directory, fileVector *vec, bool recursive, bool includeDirs);
static uint32_t concatEndSeparator(char *path, uint32_t length);
static uint32_t removeEndSeparator(char *path, uint32_t length);
static uint32_t readFileContents(const char *path, char *buffer, uint32_t length);
static void removeFilesInDir(fileVector *vec);
static void removeSubDirs(fileVector *vec);
static uint32_t removeSizeName(char *text);

File *newFile(File *file, const char *path) {
    if (file == NULL || path == NULL) {
        return NULL;
    }
    strcpy(file->path, path);
    return normalizePath(file, file->path);
}

File *newFileFromParent(File *file, File *parent, const char *child) {
    if (file == NULL || parent == NULL || child == NULL || parent->pathLength == 0) {
        return NULL;
    }

    BufferString *resultPath = NEW_STRING(PATH_MAX_LEN, parent->path);
    if (child[0] != '\\' && child[0] != '/') {
        resultPath->length = concatEndSeparator(resultPath->value, resultPath->length);
    }
    resultPath = concatChars(resultPath, child);
    if (resultPath == NULL) return NULL;
    return normalizePath(file, resultPath->value);
}

File *getParentFile(File *file, File *parent) {
    if (file == NULL || parent == NULL || parent->pathLength == 0) {
        return NULL;
    }

    strncpy(file->path, parent->path, parent->pathLength);
    file->pathLength = parent->pathLength;
    file->pathLength = removeEndSeparator(file->path, file->pathLength);
    int32_t indexOfSubPath = lastIndexOfCStr(file->path, FILE_NAME_SEPARATOR_STR);
    if (indexOfSubPath != -1) {
        file->path[indexOfSubPath] = '\0';
        file->pathLength = indexOfSubPath;
        return file;
    }

    memset(file->path, 0, parent->pathLength);   // parent dir not found
    file->pathLength = 0;
    return file;
}

bool createFile(File *file) {
    if (file == NULL) {
        return false;
    }

    file->file = fopen(file->path, "ab+");
    if (file->file == NULL) {
        return false;
    }
    fclose(file->file);
    return true;
}

bool renameFileTo(File *source, File *dest) {
    if (source == NULL || dest == NULL) {
        return false;
    }
    return rename(source->path, dest->path) == 0;
}

bool createFileDirs(File *file) {
    if (isFileExists(file)) {
        return true;
    }

    File parentDirs = {0};
    BufferString *parentPath = EMPTY_STRING(PATH_MAX_LEN);
    getParentName(file, parentPath);
    strncpy(parentDirs.path, parentPath->value, parentPath->length);
    parentDirs.pathLength = parentPath->length;
    concatEndSeparator(parentDirs.path, parentDirs.pathLength);
    return createSubDirs(&parentDirs);
}

bool createSubDirs(File *directory) {
    int32_t index = 1;
    while ((index = indexOfCStr(directory->path, FILE_NAME_SEPARATOR_STR, index)) > 0) {
        BufferString *parentDir = SUBSTRING_CSTR(PATH_MAX_LEN, directory->path, 0, index);
        if (MKDIR(parentDir->value) == -1 && errno != EEXIST) {
            return false;
        }

        index++;
    }
    return true;
}

bool isFileExists(File *file) {
    if (file == NULL) return false;
    file->file = fopen(file->path, "r");
    if (file->file == NULL) return false;
    fclose(file->file);
    return true;
}

bool isDirExists(File *directory) {
    if (directory == NULL) return false;
    directory->dir = opendir(directory->path);
    if (directory->dir == NULL) return false;
    closedir(directory->dir);
    return true;
}

bool isEmptyDir(File *dir) {
    bool isEmptyDir = true;
    dir->dir = opendir(dir->path);
    if (dir->dir == NULL) {
        return isEmptyDir;
    }

    struct dirent *inDirectory;
    while ((inDirectory = readdir(dir->dir)) != NULL) {
        if (strncmp(inDirectory->d_name, ".", 1) != 0 && strncmp(inDirectory->d_name, "..", 2) != 0) {  // On linux/Unix and windows we don't want current and parent directories
            isEmptyDir = false;
            break;
        }
    }
    closedir(dir->dir);
    return isEmptyDir;
}

bool isFile(File *file) {
    if (file == NULL || file->path[0] == '\0') {
        return false;
    }
    struct stat fileInfo;
    if (stat(file->path, &fileInfo) == NO_FILE_INFO) {
        return false;
    }
    return S_ISREG(fileInfo.st_mode) == true;
}

bool isDirectory(File *directory) {
    if (directory == NULL || directory->path[0] == '\0') {
        return false;
    }
    struct stat dirInfo;
    if (stat(directory->path, &dirInfo) == NO_FILE_INFO) {
        return false;
    }
    return S_ISDIR(dirInfo.st_mode) == true;
}

uint64_t getFileSize(File *file) {
    if (file == NULL || (file->file = fopen(file->path, "rb")) == NULL) {
        return 0;
    }
    fseek(file->file, 0, SEEK_END);
    uint32_t fileSize = ftell(file->file);
    fclose(file->file);
    return fileSize;
}

BufferString *getFileName(File *file, BufferString *result) {
    int32_t index = lastIndexOfCStr(file->path, FILE_NAME_SEPARATOR_STR);
    if (index != -1) {
        substringCStrFrom(file->path, result, index + 1);
        return result;
    }
    return substringCStrFrom(file->path, result, 0);
}

BufferString *getParentName(File *file, BufferString *result) {
    int32_t index = lastIndexOfCStr(file->path, FILE_NAME_SEPARATOR_STR);
    if (index != -1) {
        substringCStrFromTo(file->path, result, 0, index);
        return result;
    }
    return substringCStrFrom(file->path, result, 0);
}

void listFiles(File *directory, fileVector *vec, bool recursive) {
    listFilesInDir(directory, vec, recursive, false);
}

void listFilesAndDirs(File *directory, fileVector *vec, bool recursive) {
    listFilesInDir(directory, vec, recursive, true);
}

void cleanDirectory(File *directory) {
    fileVector *vec = NEW_VECTOR_BUFF(File, file,  fileBuffer, MAX_FILES_IN_DIR);
    listFilesInDir(directory, vec, true, true);
    removeFilesInDir(vec);  // remove all files first
    removeSubDirs(vec);     // then remove directories

    if (isfileVecNotEmpty(vec)) {   // check that all deleted, if not remove next time
        removeSubDirs(vec);
    }
}

bool deleteDirectory(File *dir) {
    if (!isDirectory(dir) || !isDirExists(dir)) {
        return false;
    }
    cleanDirectory(dir);
    return rmdir(dir->path) == 0;
}

bool copyFile(File *srcFile, File *destFile) {
    if (!isFileExists(srcFile)) {
        return false;
    }

    if (destFile == NULL || strcmp(srcFile->path, destFile->path) == 0) {
        return false;
    }

    if (!isFileExists(destFile)) {
        if (!createFileDirs(destFile) || !createFile(destFile)) {
            return false;
        }
    }

    srcFile->file = fopen(srcFile->path, "r");
    if (srcFile->file == NULL) {
        return false;
    }

    destFile->file = fopen(destFile->path, "w");
    if (destFile->file == NULL) {
        fclose(srcFile->file);
        return false;
    }

    int dataChar;
    while ((dataChar = fgetc(srcFile->file)) != EOF) {
        fputc(dataChar, destFile->file);
    }

    fclose(srcFile->file);
    fclose(destFile->file);
    return true;
}

bool copyDirectory(File *srcDir, File *destDir) {
    if (strcmp(srcDir->path, destDir->path) == 0) {
        return false;       // cannot copy directory to a subdirectory of itself
    }

    if (!isDirExists(srcDir) || !isDirExists(destDir)) {
        return false;
    }

    fileVector *vec = NEW_VECTOR_BUFF(File, file, fileBuffer, MAX_FILES_IN_DIR);
    listFilesInDir(srcDir, vec, true, true);
    for (uint32_t i = 0; i < fileVecSize(vec); i++) {
        File srcFile = fileVecGet(vec, i);
        BufferString *subPath = SUBSTRING_CSTR_AFTER(PATH_MAX_LEN, srcFile.path, srcDir->path);
        File *copiedFile = FILE_OF(destDir, subPath->value);

        if (isDirectory(&srcFile)) {
            if (MKDIR(copiedFile->path) == -1 && errno != EEXIST) {
                return false;
            }
            continue;
        }

        if (!createFile(copiedFile)) {
            return false;
        }
    }

    return true;
}

bool moveFileToDir(File *srcFile, File *destDir) {
    if (!isFileExists(srcFile)) {
        return false;
    }

    if (!isDirExists(destDir)) {
        createSubDirs(destDir);
        if (MKDIR(destDir->path) != 0) {
            return false;
        }
    }

    BufferString *fileName = EMPTY_STRING(PATH_MAX_LEN);
    getFileName(srcFile, fileName);
    File destFile = {0};
    newFileFromParent(&destFile, destDir, fileName->value);
    return copyFile(srcFile, &destFile) && remove(srcFile->path) == 0;
}

bool moveDirToDir(File *srcDir, File *destDir) {
    if (!isDirExists(destDir)) {
        createSubDirs(destDir);
        if (MKDIR(destDir->path) != 0) {
            return false;
        }
    }

    return copyDirectory(srcDir, destDir) && deleteDirectory(srcDir);
}

uint32_t readFileToBuffer(File *file, char *buffer, uint32_t length) {
    return file != NULL && file->pathLength > 0 ? readFileContents(file->path, buffer, length) : 0;
}

uint32_t readFileToString(File *file, BufferString *str) {
    str->length += readFileToBuffer(file, str->value, str->capacity - str->length);
    return str->length;
}

uint32_t writeCharsToFile(File *file, const char *data, uint32_t length, bool append) {
    if (!isFileExists(file)) return 0;

    file->file = fopen(file->path, append ? "a" : "wb");
    if (file->file == NULL) {
        return 0;
    }

    length = fwrite(data, sizeof(char), length, file->file);
    fclose(file->file);
    return length;
}

uint32_t writeStringToFile(File *file, BufferString *str, bool append) {
    return writeCharsToFile(file, str->value, str->length, append);
}

BufferString *byteCountToDisplaySize(uint64_t bytes, BufferString *result) {
    if (bytes == 0 || result == NULL) return result;

    char *sizeUnits;
    if ((bytes / ONE_TB) > 0) {
        sizeUnits = " TB";
        bytes /= ONE_TB;

    } else if ((bytes / ONE_GB) > 0) {
        sizeUnits = " GB";
        bytes /= ONE_GB;

    } else if ((bytes / ONE_MB) > 0) {
        sizeUnits = " MB";
        bytes /= ONE_MB;

    }  else if ((bytes / ONE_KB) > 0) {
        sizeUnits = " KB";
        bytes /= ONE_KB;

    } else {
        sizeUnits = " bytes";
    }

    uInt64ToString(result, bytes);
    return concatChars(result, sizeUnits);
}

uint64_t displaySizeToBytes(const char *sizeStr) {
    if (sizeStr == NULL) return 0;

    BufferString *inputSize = NEW_STRING_32(sizeStr);
    trimAll(inputSize);
    toUpperCase(inputSize);

    if (containsStr(inputSize, "TB")) {
        removeSizeName(inputSize->value);
        return strtol(inputSize->value, NULL, 10) * ONE_TB;

    } else if (containsStr(inputSize, "GB")) {
        removeSizeName(inputSize->value);
        return strtol(inputSize->value, NULL, 10) * ONE_GB;

    } else if (containsStr(inputSize, "MB")) {
        removeSizeName(inputSize->value);
        return strtol(inputSize->value, NULL, 10) * ONE_MB;

    } else if (containsStr(inputSize, "KB")) {
        removeSizeName(inputSize->value);
        return strtol(inputSize->value, NULL, 10) * ONE_KB;

    } else {
        return strtol(inputSize->value, NULL, 10);
    }
}

uint32_t fileChecksumCRC32(File *file, char *buffer, uint32_t length) {
    if (!isFileExists(file)) return 0;
    uint32_t dataLength = readFileContents(file->path, buffer, length);
    return generateCRC32(buffer, dataLength);
}

uint16_t fileChecksumCRC16(File *file, char *buffer, uint32_t length) {
    if (!isFileExists(file)) return 0;
    uint32_t dataLength = readFileContents(file->path, buffer, length);
    return generateCRC16(buffer, dataLength);
}

static File *normalizePath(File *file, const char *path) {
    BufferString *pathStr = NEW_STRING(PATH_MAX_LEN, path);
    replaceFirstOccurrence(pathStr, PATH_SEPARATOR_TO_REPLACE, PATH_SEPARATOR_STR);
    replaceAllOccurrences(pathStr, FILE_NAME_SEPARATOR_TO_REPLACE, FILE_NAME_SEPARATOR_STR);
    replaceAllOccurrences(pathStr, MULTIPLE_PATH_SEPARATORS, FILE_NAME_SEPARATOR_STR);
    pathStr->length = removeEndSeparator(pathStr->value, pathStr->length);
    strcpy(file->path, pathStr->value);
    file->pathLength = pathStr->length;
    return file;
}

static void listFilesInDir(File *directory, fileVector *vec, bool recursive, bool includeDirs) {
    if (!isDirectory(directory) || !isDirExists(directory)) {
        return;
    }

    directory->dir = opendir(directory->path);
    if (directory->dir == NULL) {
        return;
    }

    struct dirent *inDirectory;
    while ((inDirectory = readdir(directory->dir)) != NULL) {
        if (strncmp(inDirectory->d_name, ".", 1) == 0 || strncmp(inDirectory->d_name, "..", 2) == 0) {  // On linux/Unix and windows we don't want current and parent directories
            continue;
        }

        File tmpFile = {0};
        strncpy(tmpFile.path, directory->path, directory->pathLength);
        tmpFile.pathLength = directory->pathLength;
        tmpFile.pathLength = concatEndSeparator(tmpFile.path, tmpFile.pathLength);

        uint32_t dirNameLength = strlen(inDirectory->d_name);
        strcat(tmpFile.path, inDirectory->d_name);
        tmpFile.pathLength += dirNameLength;
        if (includeDirs || isFile(&tmpFile)) {
            if (vec->size >= vec->capacity) {
                closedir(directory->dir);
                return;
            }

            File *file = &vec->items[vec->size];
            strncpy(file->path, tmpFile.path, tmpFile.pathLength);
            file->pathLength = tmpFile.pathLength;
            file->path[file->pathLength] = '\0';
            vec->size++;
        }

        if (recursive && isDirectory(&tmpFile)) {
            listFilesInDir(&tmpFile, vec, recursive, includeDirs);
        }
    }
    closedir(directory->dir);
}

static uint32_t concatEndSeparator(char *path, uint32_t length) {
    if (path[length - 1] != FILE_NAME_SEPARATOR_CHAR) {
        strcat(path, FILE_NAME_SEPARATOR_STR);
        length++;
    }
    return length;
}

static uint32_t removeEndSeparator(char *path, uint32_t length) {
    if (path[length - 1] == FILE_NAME_SEPARATOR_CHAR) {
        path[length - 1] = '\0';
        length--;
    }
    return length;
}

static uint32_t readFileContents(const char *path, char *buffer, uint32_t length) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) return 0;

    fseek(file, 0, SEEK_END);
    uint32_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    fileSize = fileSize >= length ? length : fileSize;

    fread(buffer, fileSize, 1, file);
    fclose(file);
    buffer[fileSize] = '\0';
    return fileSize;
}

static void removeFilesInDir(fileVector *vec) {
    for (uint32_t i = 0; i < fileVecSize(vec);) {
        File file = fileVecGet(vec, i);
        if (isFile(&file)) {
            remove(file.path);
            fileVecRemoveAt(vec, i);
            continue;
        }

        i++;
    }
}

static void removeSubDirs(fileVector *vec) {
    for (uint32_t i = 0; i < fileVecSize(vec);) {
        File dir = fileVecGet(vec, i);
        if (rmdir(dir.path) == 0) {
            fileVecRemoveAt(vec, i);
            continue;
        }

        i++;
    }
}

static uint32_t removeSizeName(char *text) {
    uint32_t length = 0;
    while(*text != '\0') {
        if (!isdigit((int) *text)) {
            *text = '\0';
            break;
        }
        text++;
        length++;
    }
    return length;
}
