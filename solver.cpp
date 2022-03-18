// tiles.cpp : Defines the entry point for the console application.
//

#include "solver.h"

#include <string>
#include <set>
#include <algorithm>
#include <memory>

#include <cassert>

#include <memory.h>

using std::string;
using std::vector;
using std::for_each;



///////////////////////////////////////////////////////////////

namespace {

unsigned int BSF(unsigned int v)
{
    unsigned int c;     // c will be the number of zero bits on the right,
    // so if v is 1101000 (base 2), then c will be 3
    // NOTE: if 0 == v, then c = 31.
    if (v & 0x1)
    {
        // special case for odd v (assumed to happen half of the time)
        c = 0;
    }
    else
    {
        c = 1;
        if ((v & 0xffff) == 0)
        {
            v >>= 16;
            c += 16;
        }
        if ((v & 0xff) == 0)
        {
            v >>= 8;
            c += 8;
        }
        if ((v & 0xf) == 0)
        {
            v >>= 4;
            c += 4;
        }
        if ((v & 0x3) == 0)
        {
            v >>= 2;
            c += 2;
        }
        c -= v & 0x1;
    }
    return c;
}

class IdSet
{
public:

    struct iterator_end
    {
    };

    class iterator
    {
    public:
        explicit iterator(const int* pData)
            : m_buffer(0), m_index(-1), m_pData(pData)
        {
            operator ++();
        }

        bool operator != (const iterator_end& /*unused*/) const
        {
            return m_index < 6;
        }
        void operator ++()
        {
            while (0 == m_buffer)
            {
                if (m_index >= 6) {
                    return;
                }
                m_buffer = m_pData[++m_index];
            }
            m_value = m_index * 32 + BSF((unsigned int)m_buffer);
            m_buffer &= m_buffer - 1;
        }
        int operator *()
        {
            return m_value;
        }

    private:
        int m_buffer, m_index, m_value;
        const int* m_pData;
    };


    IdSet()
    {
        m_data[0] = m_data[1] = m_data[2] = m_data[3] = m_data[4] = m_data[5] = 0;
    }

    IdSet(const IdSet& min, const IdSet& sub)
    {
        m_size = 0;
        for (int i = 6; --i >= 0;)
        {
            if ((m_data[i] = min.m_data[i] & ~sub.m_data[i]) != 0) {
                m_size = -1;
            }
        }
    }

    void insert(int id)
    {
        assert(id >= 0);
        assert(id < 32 * 6);
        m_data[id >> 5] |= 1 << (id & 31);
        m_size = -1;
    }

    void insert(const IdSet& other)
    {
        if (0 != other.m_size)
        {
            m_data[0] |= other.m_data[0];
            m_data[1] |= other.m_data[1];
            m_data[2] |= other.m_data[2];
            m_data[3] |= other.m_data[3];
            m_data[4] |= other.m_data[4];
            m_data[5] |= other.m_data[5];
            m_size = (0 == m_size) ? other.m_size : -1;
        }
    }

    bool empty() const
    {
        assert((0 != m_size) == (m_data[0] || m_data[1] || m_data[2] || m_data[3] || m_data[4] || m_data[5]));
        return 0 == m_size;
    }

    int size() const
    {
        assert((0 != m_size) == (m_data[0] || m_data[1] || m_data[2] || m_data[3] || m_data[4] || m_data[5]));
        if (-1 == m_size)
        {
            m_size = 0;
            for (int i = 6; --i >= 0;)
            {
                unsigned int v = m_data[i];
                if (v != 0)
                {
                    v = v - ((v >> 1) & 0x55555555);                    // reuse input as temporary
                    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);     // temp
                    m_size += ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count
                }
            }
        }
        return m_size;
    }


    iterator begin()
    {
        return iterator(m_data);
    }
    static iterator_end end()
    {
        return {};
    }

    unsigned int hash()
    {
        return m_data[0] ^ m_data[1] ^ m_data[2]
            ^ m_data[3] ^ m_data[4] ^ m_data[5];
    }

    bool operator == (const IdSet& other) const
    {
        return m_data[0] == other.m_data[0]
            && m_data[1] == other.m_data[1]
            && m_data[2] == other.m_data[2]
            && m_data[3] == other.m_data[3]
            && m_data[4] == other.m_data[4]
            && m_data[5] == other.m_data[5];
    }
    bool operator != (const IdSet& other) const
    {
        return !(*this == other);
    }

private:
    int m_data[6];
    mutable int m_size{ 0 };
};

struct Item
{
    Item(int color_, int id_) : color(color_), id(id_)
    {
        assert(color >= 0 && color < 6);
    }

    IdSet neighbours[6];
    const int color;
    const int id;

    void Link(Item* pOther)
    {
        assert(pOther != nullptr);
        assert(pOther->color != color);
        neighbours[pOther->color].insert(pOther->id);
        pOther->neighbours[color].insert(id);
    }
};


class Moves
{
public:
    Moves() {}
    void push_back(int color)
    {
        assert(color >= 0 && color < 6);
        assert(m_cnt < sizeof(m_moves) / sizeof(m_moves[0]));
        m_moves[m_cnt++] = (char)color;
    }
    char* rbegin()
    {
        return m_moves + m_cnt - 1;
    }
    operator vector<int>() const
    {
        return { m_moves, m_moves + m_cnt };
    }
private:
    int m_cnt{ 0 };
    char m_moves[28];
};

struct Course
{
    IdSet items;
    IdSet covered[6];
    Moves moves;

    Course* pNext;
    Course** ppPrevNext;
};

struct IsCourseGreater
{
    bool operator() (const Course* x, const Course* y) const
    {
        return x->items.size() > y->items.size();
    }
};

int GetRootId(const int* indices, int id)
{
    while (indices[id] != id) {
        id = indices[id];
    }
    return id;
}

bool Find(Course* pOld, const IdSet& newItems)
{
    for (
        ; pOld != nullptr
        ; pOld = pOld->pNext)
    {
        if (pOld->items == newItems) {
            return true;
        }
    }
    return false;
}

} // namespace

vector<int> DoSolve(Board& originalBoard, int numCourses)
{
    // extract contiguous areas
    // https://github.com/caisah/Sedgewick-algorithms-in-c-exercises-and-examples/blob/master/01-introduction/examples/prog_1.3-path_compression_by_halving.c
    int id[DIM * DIM];
    int sz[DIM * DIM];
    int indicesBoard[DIM][DIM];

    for (int i = 0; i < DIM * DIM; ++i)
    {
        id[i] = i;
        sz[i] = 1;
    }

    indicesBoard[0][0] = 0;

    int areaIndex = 1;

    for (int i = 1; i < DIM; ++i) {
        if (originalBoard[0][i] != originalBoard[0][i - 1]) {
            indicesBoard[0][i] = areaIndex++;
        }
        else {
            indicesBoard[0][i] = indicesBoard[0][i - 1];
        }
    }

    for (int i = 1; i < DIM; ++i) {
        if (originalBoard[i][0] != originalBoard[i - 1][0]) {
            indicesBoard[i][0] = areaIndex++;
        }
        else {
            indicesBoard[i][0] = indicesBoard[i - 1][0];
        }
    }


    for (int i = 1; i < DIM; ++i) {
        for (int j = 1; j < DIM; ++j) {
            if (originalBoard[i][j] == originalBoard[i - 1][j])
            {
                if (originalBoard[i][j] == originalBoard[i][j - 1])
                {
                    // merge
                    int p = indicesBoard[i - 1][j];
                    int q = indicesBoard[i][j - 1];
                    for (; p != id[p]; p = id[p]) {
                        id[p] = id[id[p]];
                    }
                    for (; q != id[q]; q = id[q]) {
                        id[q] = id[id[q]];
                    }
                    if (p != q)
                    {
                        if (sz[p] < sz[q])
                        {
                            id[p] = q; sz[q] += sz[p];
                        }
                        else { id[q] = p; sz[p] += sz[q]; }
                    }
                }
                indicesBoard[i][j] = indicesBoard[i - 1][j];
            }
            else
            {
                if (originalBoard[i][j] == originalBoard[i][j - 1]) {
                    indicesBoard[i][j] = indicesBoard[i][j - 1];
                }
                else {
                    indicesBoard[i][j] = areaIndex++;
                }
            }
        }
    }

    assert(areaIndex <= 32 * 6);

    Item* items[DIM * DIM]{};

    Item* pRoot = new Item(originalBoard[0][0], GetRootId(id, 0));
    items[GetRootId(id, 0)] = pRoot;


    for (int i = 0; i < DIM; ++i) {
        for (int j = 1; j < DIM; ++j)
        {
            if (originalBoard[i][j] != originalBoard[i][j - 1])
            {
                int idx = GetRootId(id, indicesBoard[i][j]);
                if (nullptr == items[idx])
                {
                    items[idx] = new Item(originalBoard[i][j], idx);
                }
                items[idx]->Link(items[GetRootId(id, indicesBoard[i][j - 1])]);
            }
            else {
                assert(GetRootId(id, indicesBoard[i][j]) == GetRootId(id, indicesBoard[i][j - 1]));
            }

            if (originalBoard[j][i] != originalBoard[j - 1][i])
            {
                int idx = GetRootId(id, indicesBoard[j][i]);
                if (nullptr == items[idx])
                {
                    items[idx] = new Item(originalBoard[j][i], idx);
                }
                items[idx]->Link(items[GetRootId(id, indicesBoard[j - 1][i])]);
            }
            else {
                assert(GetRootId(id, indicesBoard[j][i]) == GetRootId(id, indicesBoard[j - 1][i]));
            }

        }
    }


    for (int i = areaIndex; --i >= 0; )
    {
        assert((id[i] == i) == (items[i] != nullptr));
    }

    auto* pCoursePool = new Course[numCourses * 2];

    vector<Course*> courses;

    courses.push_back(pCoursePool);
    pCoursePool->items.insert(pRoot->id);
    pCoursePool->moves.push_back(originalBoard[0][0]);

    int nMerged = 0;
    for (int i = areaIndex; --i >= 0; ) {
        if (id[i] != i)
        {
            ++nMerged;
        }
        else
        {
            pCoursePool->covered[items[i]->color].insert(i);
        }
    }

    int coursePoolIdx = 1;

    const int hashSize = numCourses * 2 + 1;

    auto** hashTable = new Course*[hashSize];

    for (int nMove = 0; nMove < 25; ++nMove)
    {
        memset(hashTable, 0, sizeof(Course*) * hashSize);

        vector<Course*> newCourses;
        newCourses.reserve(numCourses);
        for (const auto pCourse : courses)
        {
            const int lastColor = *(pCourse->moves.rbegin());

            for (int nColor = 0; nColor < 6; ++nColor)
            {
                if (lastColor == nColor) {
                    continue;
                }

                IdSet actual(pCourse->items, pCourse->covered[nColor]);
                IdSet newItems;

                for (int it : actual)
                {
                    Item* pItem = items[it];
                    assert(nColor != pItem->color);
                    newItems.insert(pItem->neighbours[nColor]);
                }

                if (newItems.empty()) {
                    continue;
                }

                newItems.insert(pCourse->items);

                if (newCourses.size() >= numCourses
                    && newItems.size() <= (*newCourses.begin())->items.size()) {
                    continue;
                }

                if (newItems.size() == areaIndex - nMerged)
                {
                    for_each(items, items + areaIndex, std::default_delete<Item>());
                    vector<int> result = pCourse->moves;
                    result.push_back(nColor);
                    delete[] pCoursePool;
                    delete[] hashTable;
                    return result;
                }

                const int hashIdx = newItems.hash() % hashSize;

                if (Find(hashTable[hashIdx], newItems)) {
                    continue;
                }

                Course* pNewCourse;
                if (newCourses.size() < numCourses)
                {
                    pNewCourse = pCoursePool + coursePoolIdx;
                    if (++coursePoolIdx >= numCourses * 2) {
                        coursePoolIdx = 0;
                    }

                    newCourses.push_back(pNewCourse);
                }
                else
                {
                    pNewCourse = *newCourses.begin();
                    pop_heap(newCourses.begin(), newCourses.end(), IsCourseGreater());
                    assert(pNewCourse == *newCourses.rbegin());

                    if (pNewCourse->pNext != nullptr) {
                        pNewCourse->pNext->ppPrevNext = pNewCourse->ppPrevNext;
                    }
                    *pNewCourse->ppPrevNext = pNewCourse->pNext;

                }

                pNewCourse->items = newItems;
                for (int i = 0; i < 6; ++i) {
                    pNewCourse->covered[i] = pCourse->covered[i];
                }

                pNewCourse->covered[nColor].insert(pNewCourse->items);

                pNewCourse->moves = pCourse->moves;
                pNewCourse->moves.push_back(nColor);

                pNewCourse->pNext = hashTable[hashIdx];
                pNewCourse->ppPrevNext = &hashTable[hashIdx];
                if (hashTable[hashIdx] != nullptr) {
                    hashTable[hashIdx]->ppPrevNext = &pNewCourse->pNext;
                }
                hashTable[hashIdx] = pNewCourse;

                push_heap(newCourses.begin(), newCourses.end(), IsCourseGreater());
            }
        }

        courses.swap(newCourses);
    }

    delete[] hashTable;

    for_each(items, items + areaIndex, std::default_delete<Item>());
    vector<int> result = (*courses.rbegin())->moves;
    delete[] pCoursePool;
    return result;
}

