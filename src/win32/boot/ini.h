#ifndef COASTLINE_LAUNCHER_INI_H
#define COASTLINE_LAUNCHER_INI_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LauncherStringList {
    int count;
    int *indices;
    char **lines;
} LauncherStringList;

typedef struct LauncherIniFile {
    LauncherStringList *list;
    int currentLine;
} LauncherIniFile;

typedef struct LauncherIniSectionIterator {
    LauncherIniFile *file;
    int sectionIndex;
} LauncherIniSectionIterator;

/**
 * @ghidraAddress BRally: 0x00401130
 * @ghidraAddress SetVideo: 0x004014d0
 */
LauncherIniSectionIterator *FindIniSection(LauncherIniFile *file, const char *sectionName);
/**
 * @ghidraAddress BRally: 0x004011c0
 * @ghidraAddress SetVideo: 0x00401600
 */
void FreeIniSectionIterator(LauncherIniSectionIterator *iterator);
/**
 * @ghidraAddress BRally: 0x00401250
 * @ghidraAddress SetVideo: 0x004016d0
 */
LauncherIniFile *ReadIniFile(const char *path);
/**
 * @ghidraAddress BRally: 0x004012a0
 * @ghidraAddress SetVideo: 0x00401720
 */
void FreeIniFile(LauncherIniFile *file);
/**
 * @ghidraAddress BRally: 0x00401ae0
 * @ghidraAddress SetVideo: 0x004023b0
 */
char *GetIniSectionValue(LauncherIniSectionIterator *iterator, const char *key);

/** @ghidraAddress SetVideo: 0x00401560 */
LauncherIniSectionIterator *FindFirstIniSection(LauncherIniFile *file);
/** @ghidraAddress SetVideo: 0x004015b0 */
LauncherIniSectionIterator *FindNextIniSection(LauncherIniSectionIterator *iterator);
/** @ghidraAddress SetVideo: 0x00401610 */
int CountIniSections(LauncherIniFile *file);
/** @ghidraAddress SetVideo: 0x00402ce0 */
char *GetIniSectionHeaderByIndex(int index, LauncherIniFile *file);

/**
 * @ghidraAddress BRally: 0x004011d0
 * @ghidraAddress SetVideo: 0x00401650
 */
char *GetIniSectionName(LauncherIniSectionIterator *iterator);
/**
 * @ghidraAddress BRally: 0x00401200
 * @ghidraAddress SetVideo: 0x00401680
 */
LauncherIniFile *SelectIniSection(LauncherIniSectionIterator *iterator);
/**
 * @ghidraAddress BRally: 0x00401220
 * @ghidraAddress SetVideo: 0x004016a0
 */
char *GetNextIniSectionLine(LauncherIniFile *file);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_LAUNCHER_INI_H */
