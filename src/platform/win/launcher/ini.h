#ifndef COASTLINE_LAUNCHER_INI_H
#define COASTLINE_LAUNCHER_INI_H

#include <stddef.h>

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

/** @ghidraAddress BRally: 0x00401050 */
int FileExists(const char *path);
/** @ghidraAddress BRally: 0x004010c0 */
void *AllocateMemoryChecked(size_t bytes, const char *description);
/** @ghidraAddress BRally: 0x00401130 */
LauncherIniSectionIterator *FindIniSection(LauncherIniFile *file, const char *sectionName);
/** @ghidraAddress BRally: 0x004011c0 */
void FreeIniSectionIterator(LauncherIniSectionIterator *iterator);
/** @ghidraAddress BRally: 0x00401250 */
LauncherIniFile *ReadIniFile(const char *path);
/** @ghidraAddress BRally: 0x004012a0 */
void FreeIniFile(LauncherIniFile *file);
/** @ghidraAddress BRally: 0x00401ae0 */
char *GetIniSectionValue(LauncherIniSectionIterator *iterator, const char *key);

#endif
