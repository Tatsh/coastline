#include <cassert>
#include <new>
#include <set>
#include <stdexcept>

#include "common/clipplane.h"
#include "ear/mfc.cpp"

namespace {

struct Vertex {
    Vertex *pNext;
    float distance;
};

struct Polygon {
    Vertex *pHead;
    int nCount;
};

void TestClippingFailure() {
    for (int outside = 0; outside < 3; ++outside) {
        for (int failAt = 1; failAt <= 2; ++failAt) {
            Vertex vertices[5]{};
            for (int i = 0; i < 3; ++i) {
                vertices[i].pNext = &vertices[(i + 1) % 3];
                vertices[i].distance = i == outside ? -1.0f : 1.0f;
            }
            Polygon polygon{vertices, 3};
            int allocations = 0;
            std::set<Vertex *> recycled;
            ClipLinkedPolygonPlane(
                &polygon,
                [](Vertex *vertex) { return vertex->distance; },
                [&](Vertex *, Vertex *, float) -> Vertex * {
                    if (++allocations == failAt) {
                        return nullptr;
                    }
                    return &vertices[2 + allocations];
                },
                [&](Vertex *vertex) { assert(recycled.insert(vertex).second); });
            assert(polygon.pHead == nullptr && polygon.nCount == 0);
            assert(recycled.size() == static_cast<std::size_t>(3 + failAt - 1));
        }
    }
    Polygon empty{nullptr, 0};
    ClipLinkedPolygonPlane(
        &empty,
        [](Vertex *) { return 0.0f; },
        [](Vertex *, Vertex *, float) -> Vertex * { return nullptr; },
        [](Vertex *) { assert(false); });
}

void TestContainers() {
    CMapWordToPtr map(1);
    int first = 1, second = 2;
    map.SetAt(1, &first);
    map.SetAt(2, &second);
    POSITION position = map.GetStartPosition();
    std::set<WORD> keys;
    while (position != nullptr) {
        WORD key;
        void *value;
        map.GetNextAssoc(position, key, value);
        assert((key == 1 && value == &first) || (key == 2 && value == &second));
        keys.insert(key);
    }
    assert(keys.size() == 2);
    WORD key = 99;
    void *value = &first;
    map.GetNextAssoc(position, key, value);
    assert(position == nullptr && key == 0 && value == nullptr);
    map.RemoveAll();
    position = reinterpret_cast<POSITION>(-1L);
    map.GetNextAssoc(position, key, value);
    assert(position == nullptr && key == 0 && value == nullptr);
    try {
        map.InitHashTable(0);
        assert(false);
    } catch (const std::invalid_argument &) {
    }
    CMapWordToPtr invalid(0);
    try {
        invalid.SetAt(1, &first);
        assert(false);
    } catch (const std::invalid_argument &) {
    }
    CWordList list(0);
    try {
        list.AddHead(1);
        assert(false);
    } catch (const std::invalid_argument &) {
    }
    CWordList valid(1);
    valid.AddTail(1);
    valid.AddHead(2);
    position = valid.GetHeadPosition();
    assert(valid.GetNext(position) == 2);
    assert(valid.GetNext(position) == 1);
    assert(position == nullptr);
}

} // namespace

int main() {
    TestClippingFailure();
    TestContainers();
    return 0;
}
