#ifndef COASTLINE_EAR_MFC_H
#define COASTLINE_EAR_MFC_H

#include <cstddef>

#include <windows.h>

class CArchive;
class CDumpContext;
class CRuntimeClass;
class CPlex;

void *MfcAllocateWithNewHandler(std::size_t size);

class CObject {
public:
    virtual CRuntimeClass *GetRuntimeClass() const;
    virtual ~CObject();
    virtual void Serialize(CArchive &archive);
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext &context) const;
};

typedef struct __POSITION {
} *POSITION;

class CMapWordToPtr : public CObject {
public:
    explicit CMapWordToPtr(int blockSize = 10);
    ~CMapWordToPtr() override;
    int GetCount() const;
    UINT GetHashTableSize() const;
    BOOL IsEmpty() const;
    BOOL Lookup(WORD key, void *&value) const;
    void SetAt(WORD key, void *value);
    BOOL RemoveKey(WORD key);
    void RemoveAll();
    POSITION GetStartPosition() const;
    void GetNextAssoc(POSITION &position, WORD &key, void *&value) const;
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

/** A list of words, as the CList template of MFC instantiates it. */
class CWordList : public CObject {
public:
    explicit CWordList(int blockSize = 10);
    ~CWordList() override;
    BOOL IsEmpty() const;
    int GetCount() const;
    WORD &GetHead();
    POSITION GetHeadPosition() const;
    WORD &GetNext(POSITION &position);
    POSITION AddHead(WORD value);
    POSITION AddTail(WORD value);
    POSITION InsertBefore(POSITION position, WORD value);
    POSITION InsertAfter(POSITION position, WORD value);
    void RemoveAt(POSITION position);
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

class CFileException;

class CFile : public CObject {
public:
    enum OpenFlags : UINT {
        modeWrite = 0x0001,
        modeCreate = 0x1000,
        typeBinary = 0x8000,
    };

    CFile();
    ~CFile() override;
    virtual BOOL Open(LPCSTR fileName, UINT openFlags, CFileException *error = nullptr);
    virtual void Write(const void *buffer, UINT count);
    virtual void Close();

private:
    UINT file;
    BOOL closeOnDelete;
#ifdef ENABLE_PATCHES
    // The name keeps the layout of the class of MFC, and no routine reads it.
    [[maybe_unused]] char *fileName;
#else
    char *fileName;
#endif /* ENABLE_PATCHES */
};

class CMapPtrToPtr : public CObject {
public:
    explicit CMapPtrToPtr(int blockSize = 10);
    ~CMapPtrToPtr() override;
    int GetCount() const;
    UINT GetHashTableSize() const;
    void SetAt(void *key, void *value);
    BOOL RemoveKey(void *key);
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
