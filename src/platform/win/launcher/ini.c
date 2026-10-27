#include "ini.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

// @ghidraAddress BRally: 0x00403218
static int g_bDebugTrace;
// @ghidraAddress BRally: 0x0040308c
static char g_cListCommentChar = '#';
// @ghidraAddress BRally: 0x00403220
static FILE *g_apIncludeFileStack[32];
// @ghidraAddress BRally: 0x004032a0
static unsigned g_dwIncludeFileDepth;
// @ghidraAddress BRally: 0x004037b0
static char g_szIniLineBuffer[1024];

// @ghidraAddress BRally: 0x00401000
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

int FileExists(const char *path) {
    if (g_bDebugTrace) {
        char message[1024];
        sprintf(message, "CHK_FileExists(%s)\n", path);
        OutputDebugStringA(message);
    }
    FILE *file = fopen(path, "rb");
    if (!file) {
        return 0;
    }
    fclose(file);
    return 1;
}

void *AllocateMemoryChecked(size_t bytes, const char *description) {
    if (!bytes) {
        return nullptr;
    }
    void *memory = malloc(bytes);
    if (!memory) {
        char message[1024];
        sprintf(
            message, "CHK_AllocateMemory(): Out of memory: couldn't allocate %s\n", description);
        OutputDebugStringA(message);
        exit(1);
    }
    return memory;
}

LauncherIniSectionIterator *FindIniSection(LauncherIniFile *file, const char *sectionName) {
    LauncherIniSectionIterator *iterator =
        AllocateMemoryChecked(sizeof(*iterator), "SetSubstituteDir():pins");
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

// @ghidraAddress BRally: 0x004011d0
static char *GetIniSectionName(LauncherIniSectionIterator *iterator) {
    if (iterator && iterator->file && iterator->sectionIndex != -1) {
        return iterator->file->list->lines[iterator->sectionIndex];
    }
    return nullptr;
}

// @ghidraAddress BRally: 0x00401200
static LauncherIniFile *SelectIniSection(LauncherIniSectionIterator *iterator) {
    if (iterator && iterator->file && iterator->sectionIndex != -1) {
        iterator->file->currentLine = iterator->sectionIndex;
        return iterator->file;
    }
    return nullptr;
}

// @ghidraAddress BRally: 0x00401220
static char *GetNextIniSectionLine(LauncherIniFile *file) {
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

// @ghidraAddress BRally: 0x004014a0
static void ResetIncludeFileStack(void) {
    g_dwIncludeFileDepth = 0;
}

// @ghidraAddress BRally: 0x00401660
static int IsIncludeFileStackEmpty(void) {
    return g_dwIncludeFileDepth == 0;
}

// @ghidraAddress BRally: 0x00401670
static void PushIncludeFile(FILE *file) {
    g_apIncludeFileStack[g_dwIncludeFileDepth++] = file;
}

// @ghidraAddress BRally: 0x00401690
static FILE *PopIncludeFile(void) {
    return g_apIncludeFileStack[--g_dwIncludeFileDepth];
}

// @ghidraAddress BRally: 0x004016b0
static char GetListCommentChar(void) {
    return g_cListCommentChar;
}

// @ghidraAddress BRally: 0x004016c0
static void SetListCommentChar(char comment) {
    g_cListCommentChar = comment;
}

// @ghidraAddress BRally: 0x004014b0
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

// @ghidraAddress BRally: 0x004012c0
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
    LauncherStringList *list = AllocateMemoryChecked(sizeof(*list), "PRJ");
    list->count = count;
    list->indices = AllocateMemoryChecked(count * 4, "rgiObj");
    list->lines = AllocateMemoryChecked(list->count * 4, "rgszObj");
    file = fopen(path, "rt");
    if (!file) {
        printf("ReadList: error opening file %s.\n", path);
        exit(1);
    }
    for (int index = 0; index < list->count;) {
        next = ReadNextListLine(buffer, 32767, file);
        if (strlen(buffer) > 1) {
            char *newline = strrchr(buffer, '\n');
            if (newline) {
                *newline = '\0';
            }
            list->lines[index] = AllocateMemoryChecked(strlen(buffer) + 1, "ReadList():szObji");
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
    LauncherIniFile *file = AllocateMemoryChecked(sizeof(*file), "ReadINI():pini");
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
        strcpy(g_szIniLineBuffer, line);
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
