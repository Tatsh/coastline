#include "ini.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "win32/fileio.h"

/**
 * @ghidraAddress BRally: 0x0040308c
 * @ghidraAddress SetVideo: 0x0040b1ec
 */
static char g_cListCommentChar = '#';
/**
 * @ghidraAddress BRally: 0x00403220
 * @ghidraAddress SetVideo: 0x0040e008
 */
static FILE *g_apIncludeFileStack[32];
/**
 * @ghidraAddress BRally: 0x004032a0
 * @ghidraAddress SetVideo: 0x0040e088
 */
static unsigned g_dwIncludeFileDepth;
/**
 * @ghidraAddress BRally: 0x004037b0
 * @ghidraAddress SetVideo: 0x0040e598
 */
static char g_szIniLineBuffer[1024];

/**
 * @ghidraAddress BRally: 0x00401000
 * @ghidraAddress SetVideo: 0x00401000
 */
static void FreeStringList(LauncherStringList *list) {
    if (list->count) {
        for (int index = 0; index < list->count; ++index) {
            free(list->lines[index]);
        }
        free(list->lines);
        free(list->indices);
    }
    // The original does not free the list record.
}

LauncherIniSectionIterator *FindIniSection(LauncherIniFile *file, const char *sectionName) {
    LauncherIniSectionIterator *iterator =
        CHK_AllocateMemory(sizeof(*iterator), "SetSubstituteDir():pins");
    iterator->file = file;
    iterator->sectionIndex = -1;
    if (file) {
        int count = file->list->count;
        for (int index = 0; index < count; ++index) {
            if (!strcmp(file->list->lines[index], sectionName)) {
                iterator->sectionIndex = index;
                break;
            }
        }
    }
    return iterator;
}

void FreeIniSectionIterator(LauncherIniSectionIterator *iterator) {
    free(iterator);
}

LauncherIniSectionIterator *FindFirstIniSection(LauncherIniFile *file) {
    LauncherIniSectionIterator *iterator =
        CHK_AllocateMemory(sizeof(*iterator), "SetSubstituteDir():pins");
    iterator->file = file;
    iterator->sectionIndex = -1;
    if (file) {
        int count = file->list->count;
        for (int index = 0; index < count; index++) {
            if (file->list->lines[index][0] == '[') {
                iterator->sectionIndex = index;
                break;
            }
        }
    }
    return iterator;
}

LauncherIniSectionIterator *FindNextIniSection(LauncherIniSectionIterator *iterator) {
    if (!iterator || !iterator->file) {
        return nullptr;
    }
    LauncherStringList *list = iterator->file->list;
    int index = (int)((unsigned)iterator->sectionIndex + 1u);
    for (; index < list->count; index++) {
        if (list->lines[index][0] == '[') {
            iterator->sectionIndex = index;
            return iterator;
        }
    }
    iterator->sectionIndex = -1;
    return iterator;
}

int CountIniSections(LauncherIniFile *file) {
    int count = 0;
    LauncherIniSectionIterator *iterator = FindFirstIniSection(file);
    while (iterator->sectionIndex != -1) {
        count++;
        iterator = FindNextIniSection(iterator);
    }
    FreeIniSectionIterator(iterator);
    return count;
}

char *GetIniSectionName(LauncherIniSectionIterator *iterator) {
    if (iterator && iterator->file && iterator->sectionIndex != -1) {
        return iterator->file->list->lines[iterator->sectionIndex];
    }
    return nullptr;
}

char *GetIniSectionHeaderByIndex(int index, LauncherIniFile *file) {
    LauncherIniSectionIterator *iterator = FindFirstIniSection(file);
    for (; index > 0; index--) {
        iterator = FindNextIniSection(iterator);
    }
    char *name = GetIniSectionName(iterator);
    FreeIniSectionIterator(iterator);
    return name;
}

LauncherIniFile *SelectIniSection(LauncherIniSectionIterator *iterator) {
    if (iterator && iterator->file && iterator->sectionIndex != -1) {
        iterator->file->currentLine = iterator->sectionIndex;
        return iterator->file;
    }
    return nullptr;
}

char *GetNextIniSectionLine(LauncherIniFile *file) {
    if (file && file->currentLine >= 0) {
        ++file->currentLine;
        if (file->currentLine < file->list->count) {
            char *line = file->list->lines[file->currentLine];
            if (line[0] != '[') {
                return line;
            }
        }
        file->currentLine = -1;
    }
    return nullptr;
}

/**
 * @ghidraAddress BRally: 0x004014a0
 * @ghidraAddress SetVideo: 0x00401910
 */
static void ResetIncludeFileStack(void) {
    g_dwIncludeFileDepth = 0;
}

/**
 * @ghidraAddress BRally: 0x00401660
 * @ghidraAddress SetVideo: 0x00401ac0
 */
static int IsIncludeFileStackEmpty(void) {
    return g_dwIncludeFileDepth == 0;
}

/**
 * @ghidraAddress BRally: 0x00401670
 * @ghidraAddress SetVideo: 0x00401ad0
 */
static void PushIncludeFile(FILE *file) {
#ifdef ENABLE_PATCHES
    if (g_dwIncludeFileDepth >= sizeof(g_apIncludeFileStack) / sizeof(*g_apIncludeFileStack)) {
        printf("ReadList: too many nested #include files.\n");
        exit(1);
    }
#endif
    g_apIncludeFileStack[g_dwIncludeFileDepth++] = file;
}

/**
 * @ghidraAddress BRally: 0x00401690
 * @ghidraAddress SetVideo: 0x00401af0
 */
static FILE *PopIncludeFile(void) {
    return g_apIncludeFileStack[--g_dwIncludeFileDepth];
}

/**
 * @ghidraAddress BRally: 0x004016b0
 * @ghidraAddress SetVideo: 0x00401b10
 */
static char GetListCommentChar(void) {
    return g_cListCommentChar;
}

/**
 * @ghidraAddress BRally: 0x004016c0
 * @ghidraAddress SetVideo: 0x00401b20
 */
static void SetListCommentChar(char comment) {
    g_cListCommentChar = comment;
}

/**
 * @ghidraAddress BRally: 0x004014b0
 * @ghidraAddress SetVideo: 0x00401920
 */
static FILE *ReadNextListLine(char *buffer, int capacity, FILE *file) {
    char includePath[32768];
    for (;;) {
        if (!fgets(buffer, capacity, file)) {
            if (feof(file)) {
                if (IsIncludeFileStackEmpty()) {
                    return nullptr;
                }
                fclose(file);
                file = PopIncludeFile();
                continue;
            }
            printf("ReadList: error reading file.\n");
            exit(1);
        }
        if (!strncmp(buffer, "#include", 8)) {
            char *start = strchr(buffer, '"');
            char terminator = '"';
            if (!start) {
                start = strchr(buffer, '<');
                terminator = '>';
            }
            if (start) {
                strcpy(includePath, start + 1);
                char *end = strrchr(includePath, terminator);
                if (end) {
                    *end = '\0';
                }
            } else {
                strcpy(includePath, buffer + 8);
            }
            PushIncludeFile(file);
            file = fopen(includePath, "rt");
            if (!file) {
                printf("ReadList: error opening #include file %s.\n", includePath);
                exit(1);
            }
            continue;
        }
        if (buffer[0] != g_cListCommentChar) {
            return file;
        }
    }
}

/**
 * @ghidraAddress BRally: 0x004012c0
 * @ghidraAddress SetVideo: 0x00401740
 */
static LauncherStringList *ReadStringListFile(const char *path) {
    char buffer[32768];
    int count = 0;
    ResetIncludeFileStack();
    FILE *file = fopen(path, "rt");
    if (!file) {
        printf("ReadList: error opening file %s.\n", path);
        exit(1);
    }
    FILE *next;
    while ((next = ReadNextListLine(buffer, 32767, file))) {
        file = next;
        if (strlen(buffer) > 1) {
            ++count;
        }
    }
    fclose(file);
    LauncherStringList *list = CHK_AllocateMemory(sizeof(*list), "PRJ");
    list->count = count;
    list->indices = CHK_AllocateMemory(count * 4, "rgiObj");
    list->lines = CHK_AllocateMemory(list->count * 4, "rgszObj");
    file = fopen(path, "rt");
    if (!file) {
        printf("ReadList: error opening file %s.\n", path);
        exit(1);
    }
    for (int index = 0; index < list->count;) {
        next = ReadNextListLine(buffer, 32767, file);
#ifdef ENABLE_PATCHES
        if (!next) {
            // The file ended before the lines the first pass counted, and keeps those read.
            list->count = index;
            break;
        }
#endif
        if (strlen(buffer) > 1) {
            char *newline = strrchr(buffer, '\n');
            if (newline) {
                *newline = '\0';
            }
            list->lines[index] = CHK_AllocateMemory(strlen(buffer) + 1, "ReadList():szObji");
            strcpy(list->lines[index], buffer);
            list->indices[index] = index;
            ++index;
        }
        file = next;
    }
    fclose(file);
    return list;
}

LauncherIniFile *ReadIniFile(const char *path) {
    LauncherIniFile *file = CHK_AllocateMemory(sizeof(*file), "ReadINI():pini");
    char comment = GetListCommentChar();
    SetListCommentChar(';');
    file->list = ReadStringListFile(path);
    SetListCommentChar(comment);
    file->currentLine = -1;
    return file;
}

void FreeIniFile(LauncherIniFile *file) {
    FreeStringList(file->list);
    free(file);
}

char *GetIniSectionValue(LauncherIniSectionIterator *iterator, const char *key) {
    LauncherIniFile *file = SelectIniSection(iterator);
    if (!file) {
        return nullptr;
    }
    char *line;
    while ((line = GetNextIniSectionLine(file))) {
#ifdef ENABLE_PATCHES
        size_t length = strnlen(line, sizeof(g_szIniLineBuffer) - 1);
        memcpy(g_szIniLineBuffer, line, length);
        g_szIniLineBuffer[length] = '\0';
#else
        strcpy(g_szIniLineBuffer, line);
#endif
        char *separator = strchr(g_szIniLineBuffer, '=');
        if (!separator) {
            printf("Unable to parse %s in section %s.\n",
                   g_szIniLineBuffer,
                   GetIniSectionName(iterator));
            exit(1);
        }
        *separator = '\0';
        if (!_stricmp(g_szIniLineBuffer, key)) {
            return separator + 1;
        }
    }
    return nullptr;
}
