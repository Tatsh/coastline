// The containers of MFC that EAR uses. The original links MFC; these follow the algorithms of
// MFC 4.2, on which the order the maps give their associations depends.

#include "ear/mfc.h"

#include <cstdint>
#include <cstdlib>
#include <new>

#include "ear/registry.h"

// A block of associations or nodes, allocated together and freed together.
class CPlex {
public:
    CPlex *next;

    void *Data() {
        return this + 1;
    }

    static CPlex *Create(CPlex *&head, unsigned max, std::size_t elementSize) {
        auto *plex =
            static_cast<CPlex *>(MfcAllocateWithNewHandler(sizeof(CPlex) + max * elementSize));
        plex->next = head;
        head = plex;
        return plex;
    }

    void FreeDataChain() {
        CPlex *plex = this;
        while (plex != nullptr) {
            CPlex *following = plex->next;
            std::free(plex);
            plex = following;
        }
    }
};

namespace {

// GetNextAssoc() starts from this position, before the first association.
POSITION const BEFORE_START_POSITION = reinterpret_cast<POSITION>(-1L);

// The bucket of a key, as the HashKey() of MFC gives it for a word, a pointer, or a double word.
UINT HashKey(std::uintptr_t key) {
    return static_cast<UINT>(key) >> 4;
}

std::uintptr_t KeyBits(WORD key) {
    return key;
}

std::uintptr_t KeyBits(DWORD key) {
    return key;
}

std::uintptr_t KeyBits(void *key) {
    return reinterpret_cast<std::uintptr_t>(key);
}

// The members every map of MFC has, which the routines below share.
template <typename Association>
struct MapState {
    Association **&hashTable;
    UINT &hashTableSize;
    int &count;
    Association *&freeAssociations;
    CPlex *&blocks;
    int blockSize;

    void InitHashTable(UINT hashSize, BOOL allocate) {
        delete[] hashTable;
        hashTable = nullptr;
        if (allocate) {
            hashTable = new Association *[hashSize]();
        }
        hashTableSize = hashSize;
    }

    void RemoveAll() {
        delete[] hashTable;
        hashTable = nullptr;
        count = 0;
        freeAssociations = nullptr;
        if (blocks != nullptr) {
            blocks->FreeDataChain();
        }
        blocks = nullptr;
    }

    template <typename Key>
    Association *Find(Key key) const {
        if (hashTable == nullptr) {
            return nullptr;
        }
        for (Association *association = hashTable[HashKey(KeyBits(key)) % hashTableSize];
             association != nullptr;
             association = association->next) {
            if (association->key == key) {
                return association;
            }
        }
        return nullptr;
    }

    // The association of a key, added at the head of its bucket when the map has none.
    template <typename Key>
    Association *FindOrAdd(Key key) {
        if (Association *association = Find(key)) {
            return association;
        }
        if (hashTable == nullptr) {
            InitHashTable(hashTableSize, TRUE);
        }
        if (freeAssociations == nullptr) {
            CPlex *block = CPlex::Create(blocks, blockSize, sizeof(Association));
            auto *free = static_cast<Association *>(block->Data()) + blockSize - 1;
            for (int i = blockSize - 1; i >= 0; i--, free--) {
                free->next = freeAssociations;
                freeAssociations = free;
            }
        }
        Association *association = freeAssociations;
        freeAssociations = freeAssociations->next;
        count++;
        association->key = key;
        UINT bucket = HashKey(KeyBits(key)) % hashTableSize;
        association->next = hashTable[bucket];
        hashTable[bucket] = association;
        return association;
    }

    template <typename Key>
    BOOL RemoveKey(Key key) {
        if (hashTable == nullptr) {
            return FALSE;
        }
        Association **previous = &hashTable[HashKey(KeyBits(key)) % hashTableSize];
        for (Association *association = *previous; association != nullptr;
             association = association->next) {
            if (association->key == key) {
                *previous = association->next;
                association->next = freeAssociations;
                freeAssociations = association;
                // The last association removed frees the table and the blocks.
                if (--count == 0) {
                    RemoveAll();
                }
                return TRUE;
            }
            previous = &association->next;
        }
        return FALSE;
    }
};

} // namespace

void *MfcAllocateWithNewHandler(std::size_t size) {
    for (;;) {
        if (void *memory = std::malloc(size)) {
            return memory;
        }
        std::new_handler handler = std::get_new_handler();
        if (handler == nullptr) {
            return nullptr;
        }
        handler();
    }
}

CRuntimeClass *CObject::GetRuntimeClass() const {
    // EAR never asks a container for its class.
    return nullptr;
}

CObject::~CObject() {
}

void CObject::Serialize(CArchive &archive) {
}

void CObject::AssertValid() const {
}

void CObject::Dump(CDumpContext &context) const {
}

class CMapWordToPtr::Association {
public:
    Association *next;
    WORD key;
    void *value;
};

CMapWordToPtr::CMapWordToPtr(int blockSize)
    : hashTable(nullptr), hashTableSize(17), count(0), freeAssociations(nullptr), blocks(nullptr),
      blockSize(blockSize) {
}

CMapWordToPtr::~CMapWordToPtr() {
    RemoveAll();
}

int CMapWordToPtr::GetCount() const {
    return count;
}

UINT CMapWordToPtr::GetHashTableSize() const {
    return hashTableSize;
}

BOOL CMapWordToPtr::IsEmpty() const {
    return count == 0;
}

void CMapWordToPtr::InitHashTable(UINT hashSize, BOOL allocate) {
    MapState<Association>{hashTable, hashTableSize, count, freeAssociations, blocks, blockSize}
        .InitHashTable(hashSize, allocate);
}

void CMapWordToPtr::RemoveAll() {
    MapState<Association>{hashTable, hashTableSize, count, freeAssociations, blocks, blockSize}
        .RemoveAll();
}

BOOL CMapWordToPtr::Lookup(WORD key, void *&value) const {
    auto *self = const_cast<CMapWordToPtr *>(this);
    Association *association = MapState<Association>{self->hashTable,
                                                     self->hashTableSize,
                                                     self->count,
                                                     self->freeAssociations,
                                                     self->blocks,
                                                     blockSize}
                                   .Find(key);
    if (association == nullptr) {
        return FALSE;
    }
    value = association->value;
    return TRUE;
}

void CMapWordToPtr::SetAt(WORD key, void *value) {
    MapState<Association>{hashTable, hashTableSize, count, freeAssociations, blocks, blockSize}
        .FindOrAdd(key)
        ->value = value;
}

BOOL CMapWordToPtr::RemoveKey(WORD key) {
    return MapState<Association>{
        hashTable, hashTableSize, count, freeAssociations, blocks, blockSize}
        .RemoveKey(key);
}

POSITION CMapWordToPtr::GetStartPosition() const {
    return count == 0 ? nullptr : BEFORE_START_POSITION;
}

void CMapWordToPtr::GetNextAssoc(POSITION &position, WORD &key, void *&value) const {
    auto *association = reinterpret_cast<Association *>(position);
    if (position == BEFORE_START_POSITION) {
        for (UINT bucket = 0; bucket < hashTableSize; bucket++) {
            if ((association = hashTable[bucket]) != nullptr) {
                break;
            }
        }
    }
    Association *following = association->next;
    if (following == nullptr) {
        for (UINT bucket = HashKey(association->key) % hashTableSize + 1; bucket < hashTableSize;
             bucket++) {
            if ((following = hashTable[bucket]) != nullptr) {
                break;
            }
        }
    }
    position = reinterpret_cast<POSITION>(following);
    key = association->key;
    value = association->value;
}

class CMapPtrToPtr::Association {
public:
    Association *next;
    void *key;
    void *value;
};

CMapPtrToPtr::CMapPtrToPtr(int blockSize)
    : hashTable(nullptr), hashTableSize(17), count(0), freeAssociations(nullptr), blocks(nullptr),
      blockSize(blockSize) {
}

CMapPtrToPtr::~CMapPtrToPtr() {
    MapState<Association>{hashTable, hashTableSize, count, freeAssociations, blocks, blockSize}
        .RemoveAll();
}

int CMapPtrToPtr::GetCount() const {
    return count;
}

UINT CMapPtrToPtr::GetHashTableSize() const {
    return hashTableSize;
}

void CMapPtrToPtr::InitHashTable(UINT hashSize, BOOL allocate) {
    MapState<Association>{hashTable, hashTableSize, count, freeAssociations, blocks, blockSize}
        .InitHashTable(hashSize, allocate);
}

void CMapPtrToPtr::SetAt(void *key, void *value) {
    MapState<Association>{hashTable, hashTableSize, count, freeAssociations, blocks, blockSize}
        .FindOrAdd(key)
        ->value = value;
}

BOOL CMapPtrToPtr::RemoveKey(void *key) {
    return MapState<Association>{
        hashTable, hashTableSize, count, freeAssociations, blocks, blockSize}
        .RemoveKey(key);
}

class EarActiveEventRegistry::Association {
public:
    Association *next;
    DWORD key;
    WORD value;
};

EarActiveEventRegistry::EarActiveEventRegistry(int blockSize)
    : hashTable(nullptr), hashTableSize(17), count(0), freeAssociations(nullptr), blocks(nullptr),
      blockSize(blockSize) {
}

EarActiveEventRegistry::~EarActiveEventRegistry() {
    MapState<Association>{hashTable, hashTableSize, count, freeAssociations, blocks, blockSize}
        .RemoveAll();
}

BOOL EarActiveEventRegistry::Lookup(DWORD key, WORD &value) const {
    auto *self = const_cast<EarActiveEventRegistry *>(this);
    Association *association = MapState<Association>{self->hashTable,
                                                     self->hashTableSize,
                                                     self->count,
                                                     self->freeAssociations,
                                                     self->blocks,
                                                     blockSize}
                                   .Find(key);
    if (association == nullptr) {
        return FALSE;
    }
    value = association->value;
    return TRUE;
}

void EarActiveEventRegistry::SetAt(DWORD key, WORD value) {
    MapState<Association>{hashTable, hashTableSize, count, freeAssociations, blocks, blockSize}
        .FindOrAdd(key)
        ->value = value;
}

BOOL EarActiveEventRegistry::RemoveKey(DWORD key) {
    return MapState<Association>{
        hashTable, hashTableSize, count, freeAssociations, blocks, blockSize}
        .RemoveKey(key);
}

struct CWordList::Node {
    Node *next;
    Node *previous;
    WORD data;
};

CWordList::CWordList(int blockSize)
    : head(nullptr), tail(nullptr), count(0), freeNodes(nullptr), blocks(nullptr),
      blockSize(blockSize) {
}

CWordList::~CWordList() {
    RemoveAll();
}

BOOL CWordList::IsEmpty() const {
    return count == 0;
}

int CWordList::GetCount() const {
    return count;
}

WORD &CWordList::GetHead() {
    return head->data;
}

POSITION CWordList::GetHeadPosition() const {
    return reinterpret_cast<POSITION>(head);
}

WORD &CWordList::GetNext(POSITION &position) {
    auto *node = reinterpret_cast<Node *>(position);
    position = reinterpret_cast<POSITION>(node->next);
    return node->data;
}

namespace {

// A node from the free list of a list, refilled a block at a time as CList::NewNode() does.
template <typename Node>
Node *
NewNode(Node *&freeNodes, CPlex *&blocks, int blockSize, int &count, Node *previous, Node *next) {
    if (freeNodes == nullptr) {
        CPlex *block = CPlex::Create(blocks, blockSize, sizeof(Node));
        auto *free = static_cast<Node *>(block->Data()) + blockSize - 1;
        for (int i = blockSize - 1; i >= 0; i--, free--) {
            free->next = freeNodes;
            freeNodes = free;
        }
    }
    Node *node = freeNodes;
    freeNodes = freeNodes->next;
    node->previous = previous;
    node->next = next;
    node->data = 0;
    count++;
    return node;
}

} // namespace

POSITION CWordList::AddHead(WORD value) {
    Node *node = NewNode(freeNodes, blocks, blockSize, count, static_cast<Node *>(nullptr), head);
    node->data = value;
    if (head != nullptr) {
        head->previous = node;
    } else {
        tail = node;
    }
    head = node;
    return reinterpret_cast<POSITION>(node);
}

POSITION CWordList::AddTail(WORD value) {
    Node *node = NewNode(freeNodes, blocks, blockSize, count, tail, static_cast<Node *>(nullptr));
    node->data = value;
    if (tail != nullptr) {
        tail->next = node;
    } else {
        head = node;
    }
    tail = node;
    return reinterpret_cast<POSITION>(node);
}

POSITION CWordList::InsertBefore(POSITION position, WORD value) {
    if (position == nullptr) {
        return AddHead(value);
    }
    auto *old = reinterpret_cast<Node *>(position);
    Node *node = NewNode(freeNodes, blocks, blockSize, count, old->previous, old);
    node->data = value;
    if (old->previous != nullptr) {
        old->previous->next = node;
    } else {
        head = node;
    }
    old->previous = node;
    return reinterpret_cast<POSITION>(node);
}

POSITION CWordList::InsertAfter(POSITION position, WORD value) {
    if (position == nullptr) {
        return AddTail(value);
    }
    auto *old = reinterpret_cast<Node *>(position);
    Node *node = NewNode(freeNodes, blocks, blockSize, count, old, old->next);
    node->data = value;
    if (old->next != nullptr) {
        old->next->previous = node;
    } else {
        tail = node;
    }
    old->next = node;
    return reinterpret_cast<POSITION>(node);
}

void CWordList::RemoveAt(POSITION position) {
    auto *old = reinterpret_cast<Node *>(position);
    if (old == head) {
        head = old->next;
    } else {
        old->previous->next = old->next;
    }
    if (old == tail) {
        tail = old->previous;
    } else {
        old->next->previous = old->previous;
    }
    old->next = freeNodes;
    freeNodes = old;
    // The last node removed frees the blocks.
    if (--count == 0) {
        RemoveAll();
    }
}

void CWordList::RemoveAll() {
    count = 0;
    head = tail = freeNodes = nullptr;
    if (blocks != nullptr) {
        blocks->FreeDataChain();
    }
    blocks = nullptr;
}
