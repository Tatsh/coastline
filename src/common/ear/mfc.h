#ifndef COASTLINE_EAR_MFC_H
#define COASTLINE_EAR_MFC_H

#include <cstddef>

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <windows.h>
#else
#include "common/types.h"
#endif

class CArchive;
class CDumpContext;
class CRuntimeClass;
class CPlex;

/**
 * Allocate uninitialised storage with the current new handler retried after allocation failure.
 *
 * @param size The requested byte count.
 * @return Storage released with operator delete, or null when allocation cannot succeed.
 */
void *MfcAllocateWithNewHandler(std::size_t size);

/** Polymorphic MFC compatibility base with inactive serialisation and diagnostic hooks. */
class CObject {
public:
    /** @return Null because runtime class metadata is unavailable. */
    virtual CRuntimeClass *GetRuntimeClass() const;
    /** The base destructor does not perform additional cleanup. */
    virtual ~CObject();
    /**
     * Do not serialise archive data.
     * @param archive The unused archive reference.
     */
    virtual void Serialize(CArchive &archive);
    /** Do not validate object state. */
    virtual void AssertValid() const;
    /**
     * Do not emit diagnostic output.
     * @param context The unused diagnostic context.
     */
    virtual void Dump(CDumpContext &context) const;
};

/** Opaque container position representation; instances must not be dereferenced. */
typedef struct __POSITION {
    /**
     * @typedef POSITION
     * Opaque map or list cursor, or null at the end of iteration.
     * Map iteration may use a non-null sentinel before its first association.
     */
} *POSITION;

/**
 * Hash map from 16-bit words to borrowed pointers, with pooled associations.
 * Pointer values are never deleted. Positions become invalid after their associations are removed
 * or all storage is released. Hash-table reinitialisation requires an empty map.
 */
class CMapWordToPtr : public CObject {
public:
    /**
     * Initialise an empty map with 17 buckets allocated on first insertion.
     * @param blockSize The positive number of associations allocated per pool block.
     */
    explicit CMapWordToPtr(int blockSize = 10);
    /** Release buckets and association pools without deleting pointer values. */
    ~CMapWordToPtr() override;
    /** @return The number of registered associations. */
    int GetCount() const;
    /** @return The configured bucket count, even before bucket allocation. */
    UINT GetHashTableSize() const;
    /** @return TRUE when the association count is zero; otherwise FALSE. */
    BOOL IsEmpty() const;
    /**
     * Look up a pointer without changing the output on a missing key.
     * @param key The word-sized key.
     * @param value The output borrowed pointer, possibly null for a registered null value.
     * @return TRUE when the key exists; otherwise FALSE.
     */
    BOOL Lookup(WORD key, void *&value) const;
    /**
     * Insert or replace a pointer without releasing the previous value.
     * @param key The word-sized key.
     * @param value The borrowed pointer, possibly null.
     */
    void SetAt(WORD key, void *value);
    /**
     * Remove an association and release all storage when the map becomes empty.
     * @param key The word-sized key.
     * @return TRUE when an association was removed; otherwise FALSE.
     */
    BOOL RemoveKey(WORD key);
    /** Release all associations and buckets without deleting pointer values. */
    void RemoveAll();
    /** @return A cursor before the first association, or null for an empty map. */
    POSITION GetStartPosition() const;
    /**
     * Retrieve an association in hash-bucket order and advance the cursor.
     * @param position A non-null cursor from this map, replaced with the next cursor or null.
     * @param key The output word-sized key.
     * @param value The output borrowed pointer, valid while the external object remains allocated.
     */
    void GetNextAssoc(POSITION &position, WORD &key, void *&value) const;
    /**
     * Replace bucket storage in an empty map without changing the association-pool configuration.
     * @param hashSize The positive bucket count.
     * @param allocate Whether to allocate buckets immediately rather than on first insertion.
     */
    void InitHashTable(UINT hashSize, BOOL allocate = TRUE);

private:
    class Association;
    Association **hashTable;
    UINT hashTableSize;
    int count;
    Association *freeAssociations;
    CPlex *blocks;
    int blockSize;
};

/**
 * Doubly linked list of 16-bit words with pooled nodes and MFC position semantics.
 * Positions and references remain valid until their nodes are removed or all storage is released.
 */
class CWordList : public CObject {
public:
    /**
     * Initialise an empty list with node allocation deferred until insertion.
     * @param blockSize The positive number of nodes allocated per pool block.
     */
    explicit CWordList(int blockSize = 10);
    /** Release all nodes and pool blocks. */
    ~CWordList() override;
    /** @return TRUE when the list is empty; otherwise FALSE. */
    BOOL IsEmpty() const;
    /** @return The number of list nodes. */
    int GetCount() const;
    /** @return A mutable reference to the first value; the list must be nonempty. */
    WORD &GetHead();
    /** @return The first node position, or null when the list is empty. */
    POSITION GetHeadPosition() const;
    /**
     * Retrieve a node value and advance towards the tail.
     * @param position A non-null position from this list, replaced with the next position or null.
     * @return A mutable reference to the retrieved node value.
     */
    WORD &GetNext(POSITION &position);
    /**
     * Insert a value at the head.
     * @param value The word copied into the new node.
     * @return The new node position.
     */
    POSITION AddHead(WORD value);
    /**
     * Insert a value at the tail.
     * @param value The word copied into the new node.
     * @return The new node position.
     */
    POSITION AddTail(WORD value);
    /**
     * Insert a value before a position, or at the head for a null position.
     * @param position A position from this list, or null.
     * @param value The word copied into the new node.
     * @return The new node position.
     */
    POSITION InsertBefore(POSITION position, WORD value);
    /**
     * Insert a value after a position, or at the tail for a null position.
     * @param position A position from this list, or null.
     * @param value The word copied into the new node.
     * @return The new node position.
     */
    POSITION InsertAfter(POSITION position, WORD value);
    /**
     * Remove a node and release all pool blocks when the list becomes empty.
     * @param position A non-null position identifying a current node in this list.
     */
    void RemoveAt(POSITION position);
    /** Remove every node and release all pool blocks. */
    void RemoveAll();

private:
    struct Node;
    Node *head;
    Node *tail;
    int count;
    Node *freeNodes;
    CPlex *blocks;
    int blockSize;
};

#if TGR_PLATFORM == TGR_PLATFORM_WIN
class CFileException;

/** Windows file wrapper with automatic closure and unchecked byte writes. */
class CFile : public CObject {
public:
    /** Bit flags accepted by Open(); combinations select access and creation behaviour. */
    enum OpenFlags : UINT {
        /** Request write access rather than the default read access. */
        modeWrite = 0x0001,
        /** Create a new file or truncate an existing file. */
        modeCreate = 0x1000,
        /** Binary-mode compatibility flag; Windows byte operations require no conversion. */
        typeBinary = 0x8000,
    };

    /** Initialise a wrapper without an open file. */
    CFile();
    /** Close a file successfully opened through this wrapper. */
    ~CFile() override;
    /**
     * Open a Windows file with shared read and write access.
     * The wrapper must not already have an open file; its previous handle is not closed here.
     * @param fileName The non-null, terminated ANSI path.
     * @param openFlags The OpenFlags combination.
     * @param error The unused optional error object; failure does not populate it.
     * @return TRUE on success, with automatic closure enabled; FALSE on failure.
     */
    virtual BOOL Open(LPCSTR fileName, UINT openFlags, CFileException *error = nullptr);
    /**
     * Attempt a synchronous write without reporting failure or a partial write.
     * @param buffer Readable storage of at least count bytes for an open writable file.
     * @param count The byte count to write.
     */
    virtual void Write(const void *buffer, UINT count);
    /** Close the current file if open and reset the wrapper, discarding closure errors. */
    virtual void Close();

private:
    UINT file;
    BOOL closeOnDelete;
#if defined(ENABLE_PATCHES) || !defined(_MSC_VER) || _MSC_VER > 1200
    // The unused field preserves the MFC class layout.
    [[maybe_unused]] char *fileName;
#else
    char *fileName;
#endif
};
#endif

/**
 * Hash map from pointer keys to borrowed pointer values, with pooled associations.
 * Keys are compared as pointer values without dereferencing; neither keys nor values are deleted.
 * Hash-table reinitialisation requires an empty map.
 */
class CMapPtrToPtr : public CObject {
public:
    /**
     * Initialise an empty map with 17 buckets allocated on first insertion.
     * @param blockSize The positive number of associations allocated per pool block.
     */
    explicit CMapPtrToPtr(int blockSize = 10);
    /** Release buckets and association pools without deleting key or value objects. */
    ~CMapPtrToPtr() override;
    /** @return The number of registered associations. */
    int GetCount() const;
    /** @return The configured bucket count, even before bucket allocation. */
    UINT GetHashTableSize() const;
    /**
     * Insert or replace a pointer without releasing the previous value.
     * @param key The pointer-valued key, possibly null.
     * @param value The borrowed pointer value, possibly null.
     */
    void SetAt(void *key, void *value);
    /**
     * Remove an association and release all storage when the map becomes empty.
     * @param key The pointer-valued key supplied at insertion.
     * @return TRUE when an association was removed; otherwise FALSE.
     */
    BOOL RemoveKey(void *key);
    /**
     * Replace bucket storage in an empty map without changing the association-pool configuration.
     * @param hashSize The positive bucket count.
     * @param allocate Whether to allocate buckets immediately rather than on first insertion.
     */
    void InitHashTable(UINT hashSize, BOOL allocate = TRUE);

private:
    class Association;
    Association **hashTable;
    UINT hashTableSize;
    int count;
    Association *freeAssociations;
    CPlex *blocks;
    int blockSize;
};

#endif /* COASTLINE_EAR_MFC_H */
