# color-tiles
![bad](https://github.com/aliakseis/color-tiles/assets/11851670/d6879995-b862-4457-806f-699fbf4ccbc0)

# "A party descends into an existential nightmare!"

# Tile Flood Puzzle Solver

A high-performance C++ solver for **color flood / tile conquest puzzles**. The algorithm analyzes a colored grid, compresses connected regions into a graph, and searches for an optimal or near-optimal sequence of color changes required to capture the entire board.

The implementation combines:

* Connected-component extraction
* Union-Find (Disjoint Set Union) region merging
* Compact bitset-based state representation
* Graph-based board abstraction
* Beam-search style exploration
* Duplicate-state elimination through hashing

The result is a solver capable of evaluating thousands of candidate board states while keeping memory usage extremely low.

---

## Overview

Instead of operating directly on board cells, the solver first converts the board into a graph of contiguous color regions.

Each region becomes a graph node:

* Nodes represent connected areas of the same color.
* Edges connect adjacent regions of different colors.
* The top-left region becomes the starting territory.

The solver repeatedly selects new colors and expands the currently owned territory until all regions have been absorbed.

---

## Algorithm

### 1. Region Extraction

The board is scanned and neighboring cells of identical color are merged using a Union-Find structure with path compression.

This produces:

* Unique region identifiers
* Region colors
* Region adjacency information

The total number of regions is limited to:

```cpp
32 * 6 = 192
```

which allows an extremely compact bitset representation.

---

### 2. Graph Construction

Each connected region becomes an `Item`:

```cpp
struct Item
{
    int color;
    int id;
    IdSet neighbours[6];
};
```

For every neighboring region pair:

```cpp
regionA.Link(regionB);
```

the solver records adjacency grouped by target color.

This allows instant lookup of all neighboring regions reachable through a specific color choice.

---

### 3. Compact State Representation

The `IdSet` class stores region membership using six 32-bit integers:

```cpp
int m_data[6];
```

Capabilities include:

* O(1) insertion
* Fast union operations
* Fast difference operations
* Efficient iteration over set bits
* Population counting
* Hash generation

A state containing up to 192 regions occupies only 24 bytes of raw bit storage.

---

### 4. Search State

Each candidate solution is represented by a `Course`:

```cpp
struct Course
{
    IdSet items;
    IdSet covered[6];
    Moves moves;
};
```

Where:

* `items` = currently captured regions
* `covered[color]` = cached regions already processed for a color
* `moves` = sequence of color selections

---

### 5. Beam Search

The solver does not perform exhaustive search.

Instead it maintains only the strongest candidate states:

```cpp
vector<Course*> courses;
```

For every move:

1. Generate successors for all colors.
2. Expand territory.
3. Remove duplicate states.
4. Keep only the best `numCourses` candidates.

This acts as a beam search where board coverage is the primary heuristic.

---

### 6. Duplicate Elimination

A custom hash table prevents revisiting identical region sets:

```cpp
unsigned int hash()
{
    return m_data[0] ^
           m_data[1] ^
           m_data[2] ^
           m_data[3] ^
           m_data[4] ^
           m_data[5];
}
```

States are compared using exact bitset equality.

This significantly reduces branching and search cost.

---

## Performance Techniques

### Bit-Scanning Iteration

The solver uses a custom Bit Scan Forward implementation:

```cpp
unsigned int BSF(unsigned int v)
```

allowing iteration over active regions without scanning all possible IDs.

---

### Population Counting

Set cardinality is computed using a SWAR-style bit counting algorithm:

```cpp
v = v - ((v >> 1) & 0x55555555);
v = (v & 0x33333333) +
    ((v >> 2) & 0x33333333);
```

avoiding per-bit loops.

---

### Memory Reuse

Candidate states are allocated from a fixed-size pool:

```cpp
Course* pCoursePool =
    new Course[numCourses * 2];
```

This minimizes:

* Dynamic allocations
* Heap fragmentation
* Allocation overhead

during search.

---

## Complexity

### Preprocessing

Region extraction:

```
O(N² α(N))
```

where:

* `N × N` = board size
* `α` = inverse Ackermann function from Union-Find

Effectively close to linear.

### Search

Depends on:

* Number of regions
* Beam width (`numCourses`)
* Maximum move depth

Approximate upper bound:

```
O(depth × beamWidth × colors)
```

with:

```
colors = 6
```

making the search practical even for large boards.

---

## Key Data Structures

| Structure         | Purpose                                     |
| ----------------- | ------------------------------------------- |
| `IdSet`           | Compact bitset representation of region IDs |
| `Item`            | Graph node representing a connected region  |
| `Moves`           | Fixed-size move sequence storage            |
| `Course`          | Search state                                |
| Union-Find arrays | Region extraction and merging               |
| Hash table        | Duplicate-state detection                   |

---

## Features

* Connected-component compression
* Graph-based board abstraction
* Union-Find region merging
* Compact 192-bit state representation
* Beam-search optimization
* Duplicate-state elimination
* Memory-pool allocation
* Fast bit-level iteration
* Low memory footprint
* Suitable for Flood-It / Color Flood style games

---

## Return Value

```cpp
vector<int> DoSolve(Board& originalBoard,
                    int numCourses);
```

Returns a sequence of color indices representing the moves required to flood the entire board.

Example:

```cpp
[3, 1, 5, 2, 4, 0]
```

Each number corresponds to a board color selected at that step.

---

## Design Summary

This solver transforms a color-flood puzzle into a graph search problem. By combining region compression, bitset-based state encoding, hashing, and beam search, it efficiently explores large solution spaces while maintaining very low memory consumption. The implementation is heavily optimized toward speed and is designed to solve Flood-It style puzzles in a fraction of the time required by naive cell-based approaches.
