#ifndef COASTLINE_LAUNCHER_INI_H
#define COASTLINE_LAUNCHER_INI_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** The indexed lines read from a launcher INI file. */
typedef struct LauncherStringList {
    int count;    /*!< The number of lines. */
    int *indices; /*!< The source index of each line. */
    char **lines; /*!< The text of each line. */
} LauncherStringList;

/** The current line and indexed contents of a launcher INI file. */
typedef struct LauncherIniFile {
    LauncherStringList *list; /*!< The indexed lines of the file. */
    int currentLine;          /*!< The current line index. */
} LauncherIniFile;

/** A cursor over sections in a launcher INI file. */
typedef struct LauncherIniSectionIterator {
    LauncherIniFile *file; /*!< The file being searched. */
    int sectionIndex;      /*!< The current section index. */
} LauncherIniSectionIterator;

/**
 * Allocate a section iterator and search for an exact section header.
 *
 * @return An iterator with sectionIndex set to -1 when no section matches.
 * @ghidraAddress BRally: 0x00401130
 * @ghidraAddress SetVideo: 0x004014d0
 */
LauncherIniSectionIterator *FindIniSection(LauncherIniFile *file, const char *sectionName);
/**
 * Free a section iterator without freeing its INI file.
 *
 * @ghidraAddress BRally: 0x004011c0
 * @ghidraAddress SetVideo: 0x00401600
 */
void FreeIniSectionIterator(LauncherIniSectionIterator *iterator);
/**
 * Read an INI file into indexed lines, treating semicolons as comments.
 *
 * @return The parsed file with no section selected.
 * @ghidraAddress BRally: 0x00401250
 * @ghidraAddress SetVideo: 0x004016d0
 */
LauncherIniFile *ReadIniFile(const char *path);
/**
 * Free the parsed line storage and file record.
 *
 * @ghidraAddress BRally: 0x004012a0
 * @ghidraAddress SetVideo: 0x00401720
 */
void FreeIniFile(LauncherIniFile *file);
/**
 * Search the selected section for @p key without regard to case.
 *
 * @return Its value in a shared line buffer, or NULL when the section or key is absent.
 * @ghidraAddress BRally: 0x00401ae0
 * @ghidraAddress SetVideo: 0x004023b0
 */
char *GetIniSectionValue(LauncherIniSectionIterator *iterator, const char *key);

/** Allocate an iterator positioned at the first section, or at -1 when none exists.
 * @ghidraAddress SetVideo: 0x00401560 */
LauncherIniSectionIterator *FindFirstIniSection(LauncherIniFile *file);
/** Advance @p iterator to the next section, or set its index to -1 at the end.
 * @return The same iterator, or NULL for an invalid iterator or file.
 * @ghidraAddress SetVideo: 0x004015b0 */
LauncherIniSectionIterator *FindNextIniSection(LauncherIniSectionIterator *iterator);
/** Count section headers in @p file.
 * @ghidraAddress SetVideo: 0x00401610 */
int CountIniSections(LauncherIniFile *file);
/** Look up the section header at zero-based @p index.
 * @return The header in @p file, or NULL when the index has no section.
 * @ghidraAddress SetVideo: 0x00402ce0 */
char *GetIniSectionHeaderByIndex(int index, LauncherIniFile *file);

/**
 * Return the header at @p iterator, or NULL when its section index is invalid.
 *
 * @ghidraAddress BRally: 0x004011d0
 * @ghidraAddress SetVideo: 0x00401650
 */
char *GetIniSectionName(LauncherIniSectionIterator *iterator);
/**
 * Set the file's current line to the iterator's section header.
 *
 * @return The selected file, or NULL when the iterator has no section.
 * @ghidraAddress BRally: 0x00401200
 * @ghidraAddress SetVideo: 0x00401680
 */
LauncherIniFile *SelectIniSection(LauncherIniSectionIterator *iterator);
/**
 * Advance within the selected section without crossing into the next section.
 *
 * @return The next line, or NULL at the end of the section.
 * @ghidraAddress BRally: 0x00401220
 * @ghidraAddress SetVideo: 0x004016a0
 */
char *GetNextIniSectionLine(LauncherIniFile *file);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_LAUNCHER_INI_H */
