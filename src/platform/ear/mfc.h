#ifndef COASTLINE_EAR_MFC_H
#define COASTLINE_EAR_MFC_H

#include <cstddef>
#include <type_traits>

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

private:
    class Association;
    Association **hashTable;
    UINT hashTableSize;
    int count;
    Association *freeAssociations;
    CPlex *blocks;
    int blockSize;
};

template <typename Key, typename ArgumentKey, typename Value, typename ArgumentValue>
    requires(std::is_integral_v<Key> && std::is_pointer_v<Value>)
class CMap : public CObject {
public:
    int GetCount() const;
    UINT GetHashTableSize() const;
    void SetAt(ArgumentKey key, ArgumentValue value);
    BOOL RemoveKey(ArgumentKey key);

private:
    class Association;
    Association **hashTable;
    UINT hashTableSize;
    int count;
    Association *freeAssociations;
    CPlex *blocks;
    int blockSize;
};

#endif
