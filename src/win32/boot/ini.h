#ifndef COASTLINE_LAUNCHER_INI_H
#define COASTLINE_LAUNCHER_INI_H

/**
 * @file
 * The INI reader of the Boss Rally launcher and the display wizard.
 */

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
 * @param file The parsed file, or NULL for an iterator with no section.
 * @param sectionName The header line to match with case, brackets included, such as `[Video]`.
 * @return An iterator with sectionIndex set to -1 when no section matches.
 * @ghidraAddress BRally: 0x00401130
 * @ghidraAddress SetVideo: 0x004014d0
 */
LauncherIniSectionIterator *FindIniSection(LauncherIniFile *file, const char *sectionName);
/**
 * Free a section iterator without freeing its INI file.
 *
 * @param iterator The iterator to free.
 * @ghidraAddress BRally: 0x004011c0
 * @ghidraAddress SetVideo: 0x00401600
 */
void FreeIniSectionIterator(LauncherIniSectionIterator *iterator);
/**
 * Read an INI file into indexed lines, treating semicolons as comments. `#include` lines read
 * the named file in place, and lines of one character or fewer are skipped. The process exits
 * when a file cannot be opened or read.
 *
 * @param path The path of the file.
 * @return The parsed file with no section selected.
 * @ghidraAddress BRally: 0x00401250
 * @ghidraAddress SetVideo: 0x004016d0
 */
LauncherIniFile *ReadIniFile(const char *path);
/**
 * Free the parsed lines and the file record. The original does not free the list record.
 *
 * @param file The file read by ReadIniFile().
 * @ghidraAddress BRally: 0x004012a0
 * @ghidraAddress SetVideo: 0x00401720
 */
void FreeIniFile(LauncherIniFile *file);
/**
 * Search the selected section for @p key without regard to case. The process exits when a line
 * of the section has no `=`.
 *
 * @param iterator The section to search, selected as by SelectIniSection().
 * @param key The key before the `=` of a line.
 * @return Its value in a shared line buffer, or NULL when the section or key is absent.
 * @ghidraAddress BRally: 0x00401ae0
 * @ghidraAddress SetVideo: 0x004023b0
 */
char *GetIniSectionValue(LauncherIniSectionIterator *iterator, const char *key);

/** Allocate an iterator positioned at the first section, or at -1 when none exists.
 * @param file The parsed file, or NULL for an iterator with no section.
 * @return The new iterator, freed with FreeIniSectionIterator().
 * @ghidraAddress SetVideo: 0x00401560 */
LauncherIniSectionIterator *FindFirstIniSection(LauncherIniFile *file);
/** Advance @p iterator to the next section, or set its index to -1 at the end.
 * @param iterator The iterator to advance.
 * @return The same iterator, or NULL for an invalid iterator or file.
 * @ghidraAddress SetVideo: 0x004015b0 */
LauncherIniSectionIterator *FindNextIniSection(LauncherIniSectionIterator *iterator);
/** Count section headers in @p file, the lines that start with `[`.
 * @param file The parsed file.
 * @return The number of sections.
 * @ghidraAddress SetVideo: 0x00401610 */
int CountIniSections(LauncherIniFile *file);
/** Look up the section header at zero-based @p index.
 * @param index The zero-based position of the section.
 * @param file The parsed file.
 * @return The header in @p file, or NULL when the index has no section.
 * @ghidraAddress SetVideo: 0x00402ce0 */
char *GetIniSectionHeaderByIndex(int index, LauncherIniFile *file);

/**
 * Look up the header line of the section of an iterator.
 *
 * @param iterator The iterator, or NULL.
 * @return The header in the file of the iterator, or NULL when its section index is invalid.
 * @ghidraAddress BRally: 0x004011d0
 * @ghidraAddress SetVideo: 0x00401650
 */
char *GetIniSectionName(LauncherIniSectionIterator *iterator);
/**
 * Set the file's current line to the iterator's section header.
 *
 * @param iterator The iterator, or NULL.
 * @return The selected file, or NULL when the iterator has no section.
 * @ghidraAddress BRally: 0x00401200
 * @ghidraAddress SetVideo: 0x00401680
 */
LauncherIniFile *SelectIniSection(LauncherIniSectionIterator *iterator);
/**
 * Advance within the selected section without crossing into the next section. The current line
 * becomes -1 at the end of the section.
 *
 * @param file The file selected by SelectIniSection(), or NULL.
 * @return The next line, or NULL at the end of the section.
 * @ghidraAddress BRally: 0x00401220
 * @ghidraAddress SetVideo: 0x004016a0
 */
char *GetNextIniSectionLine(LauncherIniFile *file);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_LAUNCHER_INI_H */
