# EmergencyGrid — Data Structures, Algorithms & Complexity Reference

This document serves as the comprehensive technical reference for the data structures and algorithms implemented in **EmergencyGrid**. It details theoretical definitions, concrete C++20 implementation mechanics, Big-O time/space complexity analysis, and real-world project applications.

---

## 1. Graph Representation & Dijkstra's Algorithm

### Concept & Mechanism
A city road network is mathematically modeled as a directed, weighted graph $G = (V, E)$, where vertices $V$ represent physical street intersections and edges $E$ represent road segments connecting them.

- **Adjacency List Storage**:
  Stored in [`backend/src/dsa/graph.cpp`](file:///b:/ADS%20Project/backend/src/dsa/graph.cpp) as an `std::unordered_map<std::string, std::vector<RoadSegment>>`. For an intersection $u$, its entry in the map contains a list of all outgoing road segments $(u, v)$.
- **Dynamic Weighting & Closures**:
  Each edge stores:
  $$\text{effectiveTravelTime} = \text{baseTravelTime} \times \text{congestionMultiplier}$$
  If `isClosed == true`, the edge weight is treated as $\infty$, causing the pathfinder to skip it completely. Negative weights are strictly rejected upon mutation.
- **Priority Queue Relaxation**:
  Implemented in [`backend/src/dsa/dijkstra.cpp`](file:///b:/ADS%20Project/backend/src/dsa/dijkstra.cpp). Uses a binary min-heap (`std::priority_queue<std::pair<double, std::string>, std::vector<...>, std::greater<...>>`) storing pairs of $(\text{tentativeDistance}, \text{nodeId})$.
- **Path Reconstruction**:
  Maintains a predecessor map `std::unordered_map<std::string, std::string> previousNode`. Once the target destination node is extracted from the min-heap, backtracking from destination to source reconstructs the exact sequence of intersections and road IDs.

### Project Use Case
Calculating optimal vehicle travel times for ambulances, fire trucks, and police cruisers responding from stations or scene locations across MetroGrid-16 under dynamic traffic congestion.

### Complexity Analysis
| Operation | Implementation Complexity | Notes |
| :--- | :--- | :--- |
| **Edge Relaxation** | $O(\log V)$ | Inserts updated tentative distance into binary min-heap. |
| **Extract Min Node** | $O(\log V)$ | Pops the node with smallest tentative distance. |
| **Total Algorithm Time** | $O((V + E) \log V)$ | Every vertex extracted at most once ($V \log V$); every edge relaxed at most once ($E \log V$). |
| **Space Complexity** | $O(V + E)$ | Adjacency list $O(V + E)$, tentative distances $O(V)$, predecessors $O(V)$, min-heap $O(V)$. |

> **Theoretical vs. Measured Distinction**:
> While a Fibonacci Heap achieves theoretical $O(E + V \log V)$, its high constant-factor overhead makes a binary min-heap significantly faster in practice for sparse road networks ($E \ll V^2$). On MetroGrid-16 ($V=16, E=26$), Dijkstra executes in $< 0.1\text{ ms}$.

---

## 2. Array-Backed Binary Max-Heap

### Concept & Mechanism
A custom array-backed complete binary tree that maintains the **Max-Heap Invariant**: every parent node has a priority strictly greater than or equal to that of its child nodes.

- **Contiguous Array Layout**:
  Implemented in [`backend/include/dsa/binary_heap.hpp`](file:///b:/ADS%20Project/backend/include/dsa/binary_heap.hpp) and [`backend/src/dsa/binary_heap.cpp`](file:///b:/ADS%20Project/backend/src/dsa/binary_heap.cpp) using `std::vector<Incident> heap_`. For any node at 0-based index $i$:
  $$\text{Parent Index} = \left\lfloor \frac{i - 1}{2} \right\rfloor, \quad \text{Left Child} = 2i + 1, \quad \text{Right Child} = 2i + 2$$
- **Multi-Tier Deterministic Priority Rule**:
  Implemented in `BinaryHeap::hasHigherPriority(Incident a, Incident b)`:
  1. **Severity (Primary)**: Critical (4) > High (3) > Medium (2) > Low (1).
  2. **Arrival Sequence (Secondary)**: Smaller `arrivalSequence` integer comes first (strict FIFO queueing among emergencies of identical severity).
  3. **Incident ID (Deterministic Tie-Breaker)**: Lexicographical order of `id` ensures 100% reproducible execution traces.
- **Sift-Up (`heapifyUp`)**:
  Used on insertion. Newly added element at the end of the vector is compared with its parent and repeatedly swapped upwards until the max-heap invariant is restored.
- **Sift-Down (`heapifyDown`)**:
  Used on `extractMax()`. The root is removed and replaced with the last element of the array. It is then repeatedly swapped with its highest-priority child until the invariant is restored.

### Project Use Case
Real-time emergency incident triage queueing. Ensures that an incoming life-threatening cardiac arrest (Critical 4) immediately preempts minor calls and moves to the top of the queue in $O(\log N)$ time.

### Complexity Analysis
| Operation | Average Case | Worst Case | Space | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **`peek()` (Top Priority)** | $O(1)$ | $O(1)$ | $O(1)$ | Immediate access to index 0. |
| **`insert()` (`heapifyUp`)** | $O(\log N)$ | $O(\log N)$ | $O(1)$ | Swaps up tree height $h = \lfloor \log_2 N \rfloor$. Average case is $O(1)$ for random data, but $O(\log N)$ worst-case. |
| **`extractMax()` (`heapifyDown`)** | $O(\log N)$ | $O(\log N)$ | $O(1)$ | Replaces root and bubbles down height $h$. |
| **Storage Space** | $O(N)$ | $O(N)$ | $O(N)$ | Contiguous vector storage with zero pointer overhead. |

---

## 3. Custom Self-Balancing AVL Tree

### Concept & Mechanism
An AVL tree (named after Adelson-Velsky and Landis) is a self-balancing binary search tree where the heights of the two child subtrees of any node differ by at most one.

- **Pointer-Based Node Hierarchy**:
  Implemented in [`backend/include/dsa/avl_tree.hpp`](file:///b:/ADS%20Project/backend/include/dsa/avl_tree.hpp) as a template class `AVLTree<Key, Value>`. Each node owns its children via `std::unique_ptr<AVLNode<Key, Value>>`, ensuring automatic memory reclamation without memory leaks.
- **Node Balance Factor ($BF$)**:
  $$BF(\text{node}) = \text{height}(\text{node.left}) - \text{height}(\text{node.right})$$
  Invariant: $|BF| \le 1 \implies BF \in \{-1, 0, +1\}$.
- **Four Rotation Primitives**:
  When an insertion or deletion causes $|BF| = 2$, balance is restored in $O(1)$ pointer manipulations:
  1. **LL (Single Right Rotation)**: Triggered when left child is heavy ($BF = +2$, left child $BF \ge 0$).
  2. **RR (Single Left Rotation)**: Triggered when right child is heavy ($BF = -2$, right child $BF \le 0$).
  3. **LR (Double Left-Right Rotation)**: Triggered when left child's right subtree is heavy ($BF = +2$, left child $BF < 0$). Left rotates child, then right rotates root.
  4. **RL (Double Right-Left Rotation)**: Triggered when right child's left subtree is heavy ($BF = -2$, right child $BF > 0$). Right rotates child, then left rotates root.
- **Inorder Traversal**:
  Recursively visits Left $\to$ Root $\to$ Right, yielding all stored records sorted in ascending lexicographical order by `incidentId`.

### Project Use Case
Chronological and lexical auditing of all emergency calls. Guarantees that even if incidents are created with monotonically increasing IDs (`INC-001`, `INC-002`, `INC-003`), the tree will never degenerate into an $O(N)$ linked list.

### Complexity Analysis
| Operation | Average Case | Worst Case | Space | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **Search / Lookup** | $O(\log N)$ | $O(\log N)$ | $O(1)$ | Maximum tree height $h < 1.44 \log_2(N + 2)$. |
| **Insert with Rebalance** | $O(\log N)$ | $O(\log N)$ | $O(\log N)$ | At most one single or double rotation restores balance after insert. |
| **Delete with Rebalance** | $O(\log N)$ | $O(\log N)$ | $O(\log N)$ | Up to $O(\log N)$ rotations can propagate along the path to the root. |
| **Inorder Traversal** | $O(N)$ | $O(N)$ | $O(\log N)$ | Visits all $N$ nodes in strictly sorted order. |

---

## 4. Custom Hash Table with Separate Chaining

### Concept & Mechanism
A hash table maps arbitrary key strings (`incidentId`) to values (`Incident`) via a hash function, aiming for constant-time $O(1)$ lookups.

- **64-bit FNV-1a Hash Algorithm**:
  Implemented in [`backend/include/dsa/hash_table.hpp`](file:///b:/ADS%20Project/backend/include/dsa/hash_table.hpp). A fast, non-cryptographic hash with excellent avalanche properties:
  $$\text{hash} = \text{OFFSET\_BASIS} \ (14695981039346656037\text{ULL})$$
  $$\forall \text{ byte } b: \quad \text{hash} = (\text{hash} \oplus b) \times \text{PRIME} \ (1099511628211\text{ULL})$$
- **Bucket Selection**:
  $$\text{bucketIndex} = \text{hash} \pmod{\text{bucketCount}}$$
- **Collision Resolution (Separate Chaining)**:
  Each bucket is an `std::vector<BucketEntry<Key, Value>>`. When multiple keys map to the same bucket index, they are appended to the bucket's collision chain.
- **Load Factor & Dynamic Doubling Rehash**:
  $$\text{Load Factor } \alpha = \frac{N}{\text{bucketCount}}$$
  The initial capacity is 16 buckets. When $\alpha \ge 0.75$ (e.g. inserting 13 elements into 16 buckets), `rehash()` automatically allocates $2 \times \text{bucketCount}$ (32 buckets) and redistributes all existing elements into new buckets according to $\text{hash} \pmod{32}$.

### Project Use Case
Authoritative incident registry. Enables $O(1)$ instant retrieval and status updates of incidents by ID when completing dispatches or updating active response statuses.

### Complexity Analysis
| Operation | Average Case | Worst Case | Space | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **Search / Lookup** | $O(1)$ | $O(N)$ | $O(1)$ | Worst case occurs if all $N$ keys hash into the same bucket. |
| **Insert** | $O(1)$ | $O(N)$ | $O(1)$ | Amortized $O(1)$ including periodic doubling rehash. |
| **Delete** | $O(1)$ | $O(N)$ | $O(1)$ | Unlinks key from collision chain. |
| **Dynamic Rehash** | $O(N)$ | $O(N)$ | $O(B + N)$ | Triggered when $\alpha \ge 0.75$; copies and re-indexes all elements. |

---

## 5. Multi-Agency Dispatch Decision Pipeline

### Concept & Mechanism
The dispatch engine in [`backend/src/simulation/dispatch_service.cpp`](file:///b:/ADS%20Project/backend/src/simulation/dispatch_service.cpp) solves a multi-attribute vehicle assignment problem without relying on generic external optimization packages.

```text
[Incident from Max-Heap]
          │
          ▼
[1. Candidate Discovery] ──► Filter fleet of size U by Agency & Capabilities
          │                  (Produces K eligible units)
          ▼
[2. Route Evaluation]   ──► Run Dijkstra from each candidate station to scene
          │                  (Calculates K shortest paths)
          ▼
[3. Optimal Selection]  ──► Select candidate with min(travelTimeMinutes)
          │                  (Tie-break by lexical unitId)
          ▼
[4. State Update]       ──► Incident -> ASSIGNED, Unit -> DISPATCHED, Heap purge
```

### Complexity Analysis
| Step | Complexity | Implementation Details |
| :--- | :--- | :--- |
| **Triage Extraction** | $O(\log N)$ | `BinaryHeap::extractMax()` or `peek()` to select top emergency. |
| **Candidate Filtering** | $O(U)$ | Scans all $U$ units in fleet; checks status and capability strings. |
| **Route Travel Times** | $O(K \cdot (V + E) \log V)$ | Executes $K$ independent Dijkstra computations for $K$ eligible units. |
| **Total Dispatch Time** | $O(\log N + U + K(V+E)\log V)$ | Governed by $K$ Dijkstra routing queries. |
| **Space Overhead** | $O(U + V + E)$ | Storage for candidate routes and dispatch records. |

---

## 6. Comprehensive Algorithmic Summary Table

| Data Structure / Component | Primary File | Core Operation | Average Time | Worst Case Time | Space | Primary Invariant / Guarantee |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Graph & Dijkstra** | `dijkstra.cpp` | Route Pathfinder | $O((V+E)\log V)$ | $O((V+E)\log V)$ | $O(V + E)$ | Non-negative edge weights; optimal travel times. |
| **Binary Max-Heap** | `binary_heap.cpp` | Incident Triage | $O(1)$ peek | $O(\log N)$ extract | $O(N)$ | Parent priority $\ge$ child; FIFO for equal severity. |
| **AVL Tree** | `avl_tree.hpp` | Chronological Index | $O(\log N)$ | $O(\log N)$ | $O(N)$ | Strict balance $|BF| \le 1$ via LL, RR, LR, RL rotations. |
| **Hash Table** | `hash_table.hpp` | Direct ID Registry | $O(1)$ | $O(N)$ | $O(B + N)$ | FNV-1a 64-bit; auto-doubling rehash at $\alpha \ge 0.75$. |
| **Dispatch Pipeline** | `dispatch_service.cpp` | Unit Allocation | $O(K(V+E)\log V)$ | $O(K(V+E)\log V)$ | $O(U + V + E)$ | Minimum travel time among eligible responders. |
