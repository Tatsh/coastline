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

class CMapWordToPtr : public CObject {
public:
    BOOL Lookup(WORD key, void *&value) const;
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

class CMapPtrToPtr : public CObject {
public:
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
