#pragma once

#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <errno.h>
#include <time.h>

#define CRC16_USE_LOOKUP_TABLE
#define CRC32_USE_LOOKUP_TABLE

#include "CRC.h"
#include "BufferString.h"
#include "BufferVector.h"

#define ONE_KB 1024ULL
#define ONE_MB (ONE_KB * ONE_KB)
#define ONE_GB (ONE_KB * ONE_MB)
#define ONE_TB (ONE_KB * ONE_GB)

#if defined(_WIN32) || defined(_WIN64)
    #define FILE_NAME_SEPARATOR_STR "\\"
    #define FILE_NAME_SEPARATOR_CHAR '\\'
    #define PATH_SEPARATOR_STR ":"
    #define MKDIR(PATH) mkdir(PATH)
#else
    #include <unistd.h>
    #define FILE_NAME_SEPARATOR_STR "/"
    #define FILE_NAME_SEPARATOR_CHAR '/'
    #define PATH_SEPARATOR_STR ";"
    #define MKDIR(PATH) mkdir(PATH, S_IRWXU | S_IRWXG | S_IRWXO)    // Read + Write + Execute: S_IRWXU (User), S_IRWXG (Group), S_IRWXO (Others)
#endif

#ifndef PATH_MAX_LEN
    #define PATH_MAX_LEN PATH_MAX
#endif

#ifndef MAX_FILES_IN_DIR
    #define MAX_FILES_IN_DIR 256
#endif

typedef struct File {
    FILE *file;
    DIR *dir;
    uint32_t pathLength;
    char path[PATH_MAX_LEN];
} File;

typedef File file;
CREATE_CUSTOM_COMPARATOR(filePath, File, one, two, strcmp(one.path, two.path));
CREATE_VECTOR_TYPE(File, file, filePathComparator);


#define NEW_FILE(path) newFile(&(File){0}, path)
#define FILE_OF(parentFile, childPath) newFileFromParent(&(File){0}, parentFile, childPath)
#define PARENT_FILE(parentFile) getParentFile(&(File){0}, parentFile)


File *newFile(File *file, const char *path);
File *newFileFromParent(File *file, File *parent, const char *child);
File *getParentFile(File *file, File *parent);

bool createFile(File *file);
bool createFileDirs(File *file);
bool createSubDirs(File *directory);
bool renameFileTo(File *source, File *dest);

bool isFileExists(File *file);
bool isDirExists(File *directory);
bool isEmptyDir(File *dir);

bool isFile(File *file);
bool isDirectory(File *directory);
uint64_t getFileSize(File *file);

BufferString *getFileName(File *file, BufferString *result);
BufferString *getParentName(File *file, BufferString *result);

void listFiles(File *directory, fileVector *vec, bool recursive);
void listFilesAndDirs(File *directory, fileVector *vec, bool recursive);

void cleanDirectory(File *directory);
bool deleteDirectory(File *dir);

bool copyFile(File *srcFile, File *destFile);
bool copyDirectory(File *srcDir, File *destDir);

bool moveFileToDir(File *srcFile, File *destDir);
bool moveDirToDir(File *srcDir, File *destDir);

uint32_t readFileToBuffer(File *file, char *buffer, uint32_t length);
uint32_t readFileToString(File *file, BufferString *str);

uint32_t writeCharsToFile(File *file, const char *data, uint32_t length, bool append);
uint32_t writeStringToFile(File *file, BufferString *str, bool append);

BufferString *byteCountToDisplaySize(uint64_t bytes, BufferString *result);
uint64_t displaySizeToBytes(const char *sizeStr);

uint32_t fileChecksumCRC32(File *file, char *buffer, uint32_t length);
uint16_t fileChecksumCRC16(File *file, char *buffer, uint32_t length);