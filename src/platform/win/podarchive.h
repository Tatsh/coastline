#pragma once

/**
 * @file
 * The archives of the game, the pod files that store its data.
 */

#include <cstdio>

/** The size of the name of an entry of a pod file. */
#define POD_ENTRY_NAME_SIZE 64
/** The size of the path of a pod file. */
#define POD_PATH_SIZE 0x400
/** The most entries the writer of a pod file collects. */
#define POD_WRITER_ENTRY_COUNT 4096
/** The version a pod file the game writes records. */
#define POD_FILE_VERSION 500

/** The header at the start of a pod file. */
struct PodHeader {
    char szSignature[4];    /*!< "POD" in the first three bytes. */
    int nVersion;           /*!< The version of the file. */
    int nEntryCount;        /*!< The number of entries of the directory. */
    long dwDirectoryOffset; /*!< The offset of the directory in the file. */
};

/** An entry of the directory of a pod file. */
struct PodEntry {
    long dwOffset;                    /*!< The offset of the data in the file. */
    unsigned dwSize;                  /*!< The size of the data. */
    unsigned char bAttrA;             /*!< The first attribute the writer records. */
    unsigned char bAttrB;             /*!< The second attribute the writer records. */
    char abUnknownA[2];               /* +0x0a */
    char szName[POD_ENTRY_NAME_SIZE]; /*!< The name, in capitals. */
};

/** The files a pod archive reads and writes. Every routine that fails ends the game. */
class PodFileHelper {
public:
    /**
     * Copy the part of a path after its last backslash.
     *
     * @param pszPath The path.
     * @param pszName Receives the name.
     * @ghidraAddress BRD3D: 0x10008b90
     * @ghidraAddress BRGlide: 0x10008d70
     */
    void ExtractFileName(const char *pszPath, char *pszName);

    /**
     * Create a file to write.
     *
     * @param pszPath The path of the file.
     * @return The file.
     * @ghidraAddress BRD3D: 0x10008be0
     * @ghidraAddress BRGlide: 0x10008dc0
     */
    FILE *FileOpenBinaryWriteOrFatal(const char *pszPath);

    /**
     * Open a file to read.
     *
     * @param pszPath The path of the file.
     * @return The file.
     * @ghidraAddress BRD3D: 0x10008c20
     * @ghidraAddress BRGlide: 0x10008e10
     */
    FILE *FileOpenBinaryReadOrFatal(const char *pszPath);

    /**
     * Read an exact number of bytes from a file.
     *
     * @param pFile The file.
     * @param pBuffer Receives the bytes.
     * @param dwSize The number of bytes.
     * @ghidraAddress BRD3D: 0x10008c60
     * @ghidraAddress BRGlide: 0x10008e60
     */
    void FileReadExactOrFatal(FILE *pFile, void *pBuffer, unsigned dwSize);

    /**
     * Write an exact number of bytes to a file. Its failure reports a failure to read.
     *
     * @param pFile The file.
     * @param pBuffer The bytes.
     * @param dwSize The number of bytes.
     * @ghidraAddress BRD3D: 0x10008c90
     * @ghidraAddress BRGlide: 0x10008e90
     */
    void FileWriteChecked(FILE *pFile, const void *pBuffer, unsigned dwSize);
};

/**
 * Format a message of an error that is not shown, and end the game.
 *
 * @param pszFormat The format of the message.
 * @ghidraAddress BRD3D: 0x10008cc0
 * @ghidraAddress BRGlide: 0x10008ec0
 */
void FatalErrorWithMessage(const char *pszFormat, ...);

/** A pod file the game reads, and the writer of one. */
class PodArchive {
private:
    void ClearPodArchive();

    PodFileHelper helper;       /*!< The routines of the files. */
    PodHeader header;           /*!< The header of the file. */
    PodEntry *pEntries;         /*!< The directory, from new. */
    FILE *pFile;                /*!< The file. */
    char szPath[POD_PATH_SIZE]; /*!< The path of the file. */
    unsigned dwDirectorySize;   /*!< The size of the directory in bytes. */

public:
    /**
     * An archive without a file.
     *
     * @ghidraAddress BRD3D: 0x10008580
     * @ghidraAddress BRGlide: 0x10008760
     */
    PodArchive();

    /**
     * Do nothing.
     *
     * @ghidraAddress BRD3D: 0x100085e0
     * @ghidraAddress BRGlide: 0x100087c0
     */
    virtual ~PodArchive();

private:
    /**
     * Make the name of an entry from a path: the name of the file, in capitals, padded with zeros
     * to #POD_ENTRY_NAME_SIZE bytes. A longer name ends the game.
     *
     * @param pszPath The path.
     * @param pszName Receives the name, #POD_ENTRY_NAME_SIZE bytes.
     * @ghidraAddress BRD3D: 0x100085f0
     * @ghidraAddress BRGlide: 0x100087d0
     */
    virtual void CleanupName(const char *pszPath, char *pszName);

    /**
     * Find the entry of a path.
     *
     * @param pszPath The path.
     * @return The index of the entry, or -1.
     * @ghidraAddress BRD3D: 0x10008670
     * @ghidraAddress BRGlide: 0x10008850
     */
    virtual int FindPodIndexByName(const char *pszPath);

    /**
     * Find the entry of a path, which must exist.
     *
     * @param pszPath The path.
     * @return The index of the entry.
     * @ghidraAddress BRD3D: 0x10008750
     * @ghidraAddress BRGlide: 0x10008930
     */
    virtual int GetPodIndexByName(const char *pszPath);

    /**
     * The size of the data of an entry.
     *
     * @param nIndex The index of the entry.
     * @return The size.
     * @ghidraAddress BRD3D: 0x10008780
     * @ghidraAddress BRGlide: 0x10008960
     */
    virtual unsigned GetPodLength(unsigned nIndex);

    /**
     * Read the data of an entry.
     *
     * @param nIndex The index of the entry.
     * @param pBuffer Receives the data.
     * @ghidraAddress BRD3D: 0x100087b0
     * @ghidraAddress BRGlide: 0x10008990
     */
    virtual void ReadPodBytesFromFile(unsigned nIndex, void *pBuffer);

    /**
     * Read the data of the entry of a path.
     *
     * @param pszPath The path.
     * @param pBuffer Receives the data.
     * @return pBuffer.
     * @ghidraAddress BRD3D: 0x100088b0
     * @ghidraAddress BRGlide: 0x10008a90
     */
    virtual void *LoadPodByName(const char *pszPath, void *pBuffer);

    /**
     * Read the data of an entry.
     *
     * @param nIndex The index of the entry.
     * @param pBuffer Receives the data.
     * @return pBuffer.
     * @ghidraAddress BRD3D: 0x10008850
     * @ghidraAddress BRGlide: 0x10008a30
     */
    virtual void *LoadPodEntryByIndex(unsigned nIndex, void *pBuffer);

    /**
     * Read the data of the entry of a path into memory from new.
     *
     * @param pszPath The path.
     * @return The data.
     * @ghidraAddress BRD3D: 0x10008890
     * @ghidraAddress BRGlide: 0x10008a70
     */
    virtual void *AllocAndLoadPodByName(const char *pszPath);

    /**
     * Read the data of an entry into memory from new.
     *
     * @param nIndex The index of the entry.
     * @return The data.
     * @ghidraAddress BRD3D: 0x10008810
     * @ghidraAddress BRGlide: 0x100089f0
     */
    virtual void *AllocAndLoadPodByIndex(unsigned nIndex);

    /**
     * Open the file of szPath and read its header and directory. A file that is not a pod file
     * ends the game.
     *
     * @ghidraAddress BRD3D: 0x100088d0
     * @ghidraAddress BRGlide: 0x10008ab0
     */
public:
    virtual void OpenPodFile();

private:
    /**
     * Create a pod file to write, after the space of its header, and clear the entries of the
     * writer.
     *
     * @param pszPath The path of the file.
     * @ghidraAddress BRD3D: 0x100089c0
     * @ghidraAddress BRGlide: 0x10008ba0
     */
    virtual void CreatePodFile(const char *pszPath);

    /**
     * Write the data of an entry to the pod file being written.
     *
     * @param pszPath The path the name of the entry comes from.
     * @param pData The data.
     * @param dwSize The size of the data.
     * @param bAttrA The first attribute of the entry.
     * @param bAttrB The second attribute of the entry.
     * @ghidraAddress BRD3D: 0x10008a00
     * @ghidraAddress BRGlide: 0x10008be0
     */
    virtual void
    AddPodEntry(const char *pszPath, const void *pData, unsigned dwSize, char bAttrA, char bAttrB);

    /**
     * Write the directory and the header of the pod file being written, and close it.
     *
     * @ghidraAddress BRD3D: 0x10008aa0
     * @ghidraAddress BRGlide: 0x10008c80
     */
    virtual void FinalizePodFile();

    /**
     * Close the file and free the directory.
     *
     * @ghidraAddress BRD3D: 0x10008970
     * @ghidraAddress BRGlide: 0x10008b50
     */
public:
    void ClosePodFile();

    /**
     * Set the path of the file of the archive.
     *
     * @param pszPath The path, or null to keep the path.
     * @ghidraAddress BRD3D: 0x10008b40
     * @ghidraAddress BRGlide: 0x10008d20
     */
    void SetFilePath(const char *pszPath);
};

/**
 * The archive of the data of the game.
 *
 * @ghidraAddress BRD3D: 0x10a99780
 * @ghidraAddress BRGlide: 0x10ac0810
 */
extern PodArchive g_BossRallyPodArchive;
