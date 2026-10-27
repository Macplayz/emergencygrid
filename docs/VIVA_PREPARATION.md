# EmergencyGrid — Advanced Data Structures Viva Examination Guide

This document is prepared specifically for college students defending **EmergencyGrid** in an **Advanced Data Structures & Algorithms (ADS)** practical examination or viva voce. It covers project objectives, architecture justification, 25+ essential viva questions with comprehensive answers, 10 deep technical design questions, a live demonstration script, and honest system limitations.

---

## 1. Project Pitch (30-Second Elevator Pitch)

> *"EmergencyGrid is a real-time multi-agency emergency response simulation system that demonstrates how classical data structures and graph algorithms solve real-world emergency dispatch challenges.
> The core algorithmic engine is implemented from scratch in ISO C++20 without external databases or high-level libraries. It utilizes a **Graph with Dijkstra's algorithm** for road routing, an array-backed **Binary Max-Heap** for incident priority triage, a self-balancing **AVL Tree** for ordered chronological auditing, and a custom **64-bit FNV-1a Hash Table** with separate chaining for direct incident lookups.
> A modern Next.js 16 frontend provides both an interactive **Operations Mode** for real-time dispatching and an educational **DSA Lab** that visualizes step-by-step execution traces emitted directly by the C++ engine."*

---

## 2. Core Architecture Justifications

### Why C++20 for the Backend and Next.js for the Frontend?
- **C++20**: Provides deterministic, high-performance memory management and zero runtime overhead. Writing the data structures in C++ allows us to manually manage pointers (`std::unique_ptr` in AVL trees), cache-friendly contiguous arrays (Binary Heap), and 64-bit bitwise hashing without garbage collection pauses.
- **Next.js 16 (App Router)**: Delivers an accessible, modern command-and-control user interface using React 19, SVG canvas rendering, and server-side API proxy routing, without cluttering the algorithmic core with UI code.

### Why Three Separate Indexes for Incidents?
| Index | Data Structure | Why It Is Necessary |
| :--- | :--- | :--- |
| **Priority Queue** | `BinaryHeap` (Max-Heap) | Instant $O(1)$ peek and $O(\log N)$ extraction of the most urgent emergency call based on severity and wait-time. |
| **Direct Registry** | `HashTable` (Chaining) | Instant $O(1)$ direct lookup, verification, and status updates by unique `incidentId`. |
| **Chronological Log** | `AVLTree` (Self-Balancing BST) | Strictly balanced $O(\log N)$ tree that enables $O(N)$ inorder sorted dumps and range queries without degrading to $O(N)$ linked list behavior. |

---

## 3. Top 25 Likely Viva Questions & Model Answers

### Question 1: Why use a Binary Max-Heap instead of simply sorting an array of incidents?
**Answer**:
Sorting an array takes $O(N \log N)$ time. When emergencies arrive dynamically in real time, sorting the entire array for every new arrival is computationally wasteful. A binary max-heap allows inserting a new emergency in $O(\log N)$ time, peeking at the highest-priority incident in $O(1)$ time, and extracting it in $O(\log N)$ time. Furthermore, an array-backed heap provides optimal contiguous cache locality with zero pointer overhead.

### Question 2: How does the Binary Max-Heap break ties when two incidents have equal severity?
**Answer**:
The heap uses a three-tier deterministic comparator in `BinaryHeap::hasHigherPriority`:
1. **Severity (Primary)**: Higher severity wins (Critical: 4 > High: 3 > Medium: 2 > Low: 1).
2. **Arrival Sequence (Secondary)**: Smaller `arrivalSequence` integer wins, enforcing strict First-In, First-Out (FIFO) ordering among emergencies of equal severity.
3. **Incident ID (Tertiary)**: Lexicographical order of `incidentId` serves as a deterministic tie-breaker, guaranteeing reproducible execution traces.

### Question 3: Why did you implement an AVL tree in addition to a Hash Table?
**Answer**:
A hash table is an unordered data structure; it cannot answer range queries (e.g. *"retrieve incidents reported between tick 10 and 50"*) or output records in sorted order without an external $O(N \log N)$ sort. The AVL tree maintains elements in strictly sorted binary search tree order, enabling $O(N)$ sorted inorder traversals and range queries in guaranteed $O(\log N)$ worst-case time.

### Question 4: Why choose an AVL tree over a standard Binary Search Tree (BST)?
**Answer**:
If incidents arrive with sequentially ordered keys (e.g. `INC-001`, `INC-002`, `INC-003`), a naive BST degenerates into a linear linked list of height $N$, causing search, insertion, and deletion times to degrade from $O(\log N)$ to $O(N)$. An AVL tree enforces the invariant that for every node, the height difference between left and right subtrees $|BF| \le 1$. If $|BF| = 2$, it executes constant-time $O(1)$ rotations, strictly bounding tree height to $h < 1.44 \log_2(N + 2)$.

### Question 5: What are the four AVL tree rotation types and when are they triggered?
**Answer**:
1. **LL (Single Right Rotation)**: Triggered when a node has balance factor $+2$ and its left child has balance factor $\ge 0$ (Left-heavy insertion into left subtree).
2. **RR (Single Left Rotation)**: Triggered when a node has balance factor $-2$ and its right child has balance factor $\le 0$ (Right-heavy insertion into right subtree).
3. **LR (Double Left-Right Rotation)**: Triggered when a node has balance factor $+2$ and its left child has balance factor $< 0$. It first left-rotates the left child, then right-rotates the unbalanced node.
4. **RL (Double Right-Left Rotation)**: Triggered when a node has balance factor $-2$ and its right child has balance factor $> 0$. It first right-rotates the right child, then left-rotates the unbalanced node.

### Question 6: What hash function is used in the custom Hash Table and why?
**Answer**:
We implemented the 64-bit **FNV-1a** (Fowler–Noll–Vo) non-cryptographic hash algorithm. It initializes with the 64-bit offset basis `14695981039346656037ULL` and iteratively XORs each byte of the string key before multiplying by the prime `1099511628211ULL`. FNV-1a was chosen because it has minimal instruction overhead, computes in $O(L)$ time for string length $L$, and provides excellent avalanche distribution across buckets.

### Question 7: How are hash collisions handled in your implementation?
**Answer**:
We use **separate chaining**. Each bucket in the hash table is an `std::vector` storing entries that hashed to the same bucket index ($hash \pmod{\text{bucketCount}}$). When collisions occur, new entries are appended to the bucket's collision chain.

### Question 8: What is the load factor $\alpha$ and when does the Hash Table rehash?
**Answer**:
Load factor is defined as $\alpha = \frac{N}{\text{bucketCount}}$, where $N$ is the number of stored elements and $\text{bucketCount}$ is the number of buckets. The table initializes with 16 buckets and a maximum load factor threshold of $0.75$. When an insertion causes $\alpha \ge 0.75$ (e.g. inserting 13 elements into 16 buckets), `rehash()` doubles bucket capacity to 32 and redistributes all existing elements.

### Question 9: What is the difference between average-case and worst-case hash table lookup?
**Answer**:
- **Average Case**: $O(1)$ constant time, assuming a uniform hash distribution where the average bucket chain length is bounded by $\alpha \le 0.75$.
- **Worst Case**: $O(N)$ linear time, which occurs if the hash function suffers pathological collisions and all $N$ keys map to the exact same bucket, degrading the chain into a linear search.

### Question 10: How does Dijkstra's algorithm work on your road network?
**Answer**:
Dijkstra's algorithm finds the shortest path between a source intersection and a destination on a graph with non-negative edge weights. It initializes all tentative distances to $\infty$ (0 at the source) and pushes $(0, \text{source})$ into a binary min-heap priority queue. In each iteration, it extracts the unvisited node $u$ with minimum tentative distance, explores all outgoing roads $(u, v)$, and relaxes the edge:
$$\text{if } d[u] + w(u, v) < d[v] \implies d[v] = d[u] + w(u, v)$$
It updates the min-heap and records $u$ as the predecessor of $v$. Once the destination node is extracted, the shortest path is reconstructed by following predecessors.

### Question 11: What is the time complexity of your Dijkstra implementation?
**Answer**:
$O((V + E) \log V)$ time, where $V$ is the number of intersections and $E$ is the number of road segments. Every node is extracted from the min-heap at most once ($V \log V$), and every edge is examined and potentially relaxed at most once ($E \log V$). The space complexity is $O(V + E)$ for adjacency list storage and predecessor/distance tracking.

### Question 12: Can your Dijkstra implementation handle negative edge weights?
**Answer**:
No. Dijkstra's greedy choice property assumes that once a node is extracted from the min-heap, its shortest path distance is permanently finalized. A negative edge weight could allow a previously settled node to be reached via a shorter path, invalidating the greedy invariant. Our engine explicitly validates road travel times and rejects negative edge weights with HTTP 400 Bad Request.

### Question 13: How are closed roads and traffic congestion represented in the graph?
**Answer**:
Each road segment stores a `congestionFactor` ($1.0\times\text{--}5.0\times$) and an `isClosed` boolean.
The effective travel time is computed dynamically as $\text{baseTravelTimeMinutes} \times \text{congestionFactor}$. If `isClosed` is true, the router treats the edge weight as $\infty$ and skips it during edge exploration, forcing Dijkstra to find an alternate route.

### Question 14: How does the dispatch engine select an emergency unit?
**Answer**:
The dispatch engine follows a 4-stage pipeline:
1. **Triage**: Inspects the highest-priority emergency call from the `BinaryHeap`.
2. **Candidate Discovery**: Filters the fleet for units that are `AVAILABLE`, match the incident's agency (`MEDICAL`, `FIRE`, `POLICE`), and possess all required capabilities (`ALS`, `HAZMAT`).
3. **Route Evaluation**: Runs Dijkstra from each candidate's stationed intersection to the scene to compute travel times.
4. **Selection**: Chooses the candidate unit with the minimum estimated travel time in minutes, resolving ties by unique `unitId`.

### Question 15: What happens if an incident is cancelled while still buried inside the Binary Max-Heap?
**Answer**:
EmergencyGrid uses **safe lazy invalidation**. Deleting an arbitrary node from the middle of an array-backed heap takes $O(N)$ linear time and disrupts cache locality. Instead, when an incident is cancelled, its authoritative status in the `HashTable` and `AVLTree` is immediately updated to `CANCELLED`. When the heap's root is later extracted, `purgeStaleTop()` checks the incident's authoritative status. If it is no longer `PENDING`, it is discarded via a `skip_stale_entry` trace event and the heap sinks the next element.

### Question 16: Why must educational DSA Lab demonstrations not modify live simulation state?
**Answer**:
If a student runs an educational demonstration in the lab (such as triggering an AVL tree double rotation or testing heap sift-downs), mutating the live simulation would corrupt the operational incident list and dispatch queue. EmergencyGrid isolates educational demonstrations by running them on ephemeral instances within local endpoint scopes, guaranteeing **zero side effects** on live Operations Mode data.

### Question 17: How does the frontend receive and replay algorithmic trace events?
**Answer**:
When an operation is executed with `enableTrace: true`, the C++ engine records structured `TraceEvent` objects into a `TraceCollector`. The backend returns these events in the HTTP response envelope. The frontend hook `useTracePlayer` loads this event list and maintains a step pointer. The student can scrub, step forward, or step backward, and the UI dynamically highlights the nodes, edges, or bucket indices referenced in that step.

### Question 18: What happens if the C++ backend is stopped while using the frontend?
**Answer**:
The Next.js client has a polling health probe (`/api/backend-status`) that queries the engine every 10–12 seconds. If the backend is unreachable, the UI displays an accessible yellow warning banner: *"C++ Engine Offline — Showing Cached State"*, and disables mutation buttons to prevent inconsistent client-side state.

### Question 19: What is the array indexing formula for a binary heap?
**Answer**:
For 0-based indexing:
- Parent of node at index $i$: $\lfloor (i - 1) / 2 \rfloor$
- Left child of node at index $i$: $2i + 1$
- Right child of node at index $i$: $2i + 2$

### Question 20: What is the maximum height of an AVL tree with $N$ nodes?
**Answer**:
The maximum height of an AVL tree is bounded by $h < 1.44 \log_2(N + 2) \approx 1.44 \log_2 N$. This is derived from the Fibonacci tree recurrence relation for minimal AVL trees, proving that AVL search is guaranteed to execute in $O(\log N)$ time.

### Question 21: Why is an adjacency list preferred over an adjacency matrix for MetroGrid-16?
**Answer**:
MetroGrid-16 has $V = 16$ vertices and $E = 26$ edges. It is a sparse planar graph where $E \ll V^2$ (a complete graph would have $V(V - 1) = 240$ directed edges). An adjacency matrix would consume $O(V^2) = 256$ entries and require iterating through all 16 vertices during edge exploration ($O(V)$). An adjacency list consumes $O(V + E)$ space and only visits existing neighbors, executing Dijkstra in $O((V + E) \log V)$ rather than $O(V^2)$.

### Question 22: What design pattern is used for managing the C++ backend lifecycle?
**Answer**:
The Singleton / RAII (Resource Acquisition Is Initialization) pattern. Dynamic allocations in the AVL tree use `std::unique_ptr` smart pointers to guarantee automatic cleanup without manual `delete` calls. Mutex locks (`std::lock_guard<std::mutex>`) guarantee thread-safe graph mutations and route recalculations.

### Question 23: How does the application prevent duplicate incident creation?
**Answer**:
Before inserting an incident, `IncidentService::createIncident()` queries the `HashTable` with the requested `incidentId`. If the key already exists, the function immediately aborts and returns an error envelope with HTTP `409 Conflict`, preserving data integrity across all three indexes.

### Question 24: What is the purpose of the Next.js API proxy (`/api/proxy/[...path]`)?
**Answer**:
The API proxy acts as a server-side reverse proxy. It forwards requests from the browser to `http://127.0.0.1:8080`, eliminating browser Cross-Origin Resource Sharing (CORS) errors, preventing mixed-content blocking, and allowing the backend URL to be configured in a single `.env.local` file.

### Question 25: Can an emergency vehicle be assigned to multiple active incidents simultaneously?
**Answer**:
No. The `UnitService` enforces strict lifecycle states: `AVAILABLE` $\to$ `DISPATCHED` $\to$ `RESPONDING` $\to$ `BUSY` $\to$ `AVAILABLE`. When a unit is selected for dispatch, its status immediately transitions to `DISPATCHED`. During subsequent candidate discovery, any unit whose status is not `AVAILABLE` is excluded with an explicit rejection reason.

---

## 4. 10 Deep Technical Questions on Implementation Decisions

### Technical Question 1: How does your AVL tree implementation handle memory management during rotations?
**Answer**:
Our AVL tree uses `std::unique_ptr<AVLNode<Key, Value>>` for child pointers. During an LL (Single Right) rotation on node $P$ with left child $L$:
1. $L$'s right child subtree ($T_2$) is moved to $P$'s left child pointer via `std::move`.
2. Node $P$ is moved into $L$'s right child pointer via `std::move`.
3. Node heights are recalculated: first for $P$ (the demoted parent), then for $L$ (the promoted root).
4. $L$ is returned as the new owning `std::unique_ptr`. Because ownership transfers cleanly through `std::move`, zero manual pointer frees are required and no dangling references are possible.

### Technical Question 2: Why not use `std::priority_queue` for the incident priority queue?
**Answer**:
`std::priority_queue` in the C++ standard library is a black-box container adapter. It does not provide access to underlying vector indices, does not expose parent-child structural relationships for UI visualization, and does not support instrumenting internal comparisons and swaps for algorithmic trace logging. Implementing our own `BinaryHeap` class gave us complete control over sift-up/down telemetry and array snapshot serialization.

### Technical Question 3: How does `purgeStaleTop()` in the Incident Service work?
**Answer**:
When an incident is assigned or cancelled, its entry in the max-heap becomes stale. Rather than performing an expensive $O(N)$ vector linear scan to find and remove it, we leave it in the heap. In `IncidentService::getNextIncident()`, a loop inspects the root element:
```cpp
while (!heap_.isEmpty()) {
    auto top = heap_.peek();
    auto authoritative = hashTable_.search(top->id);
    if (authoritative.has_value() && authoritative->status == IncidentStatus::PENDING) {
        return authoritative; // Found valid pending incident
    }
    // Discard stale entry and bubble down next element
    heap_.extractMax(&trace);
}
```
This amortizes the invalidation cost to $O(\log N)$ only when the stale entry reaches the top of the heap.

### Technical Question 4: How is tie-breaking guaranteed to be deterministic in Dijkstra routing?
**Answer**:
When two candidate routes have identical travel times, floating-point comparisons (`dist[u] + weight < dist[v]`) can vary based on compiler optimizations or CPU register precision. To guarantee 100% deterministic, reproducible behavior, our comparator enforces a secondary comparison: if travel times are equal within an epsilon tolerance ($\Delta < 10^{-6}$), the intersection ID string is compared lexicographically (`u < v`).

### Technical Question 5: What is the avalanche effect in FNV-1a hashing and why does it matter?
**Answer**:
The avalanche effect means that changing a single bit in the input key string (e.g. `INC-101` vs `INC-102`) causes a drastic, pseudo-random change in the output hash bits. Because incident IDs share common prefixes (`INC-`), a poor hash function (such as summing ASCII characters) would cause severe clustering in adjacent buckets. FNV-1a's combination of bitwise XOR and large prime multiplication scatters sequential keys evenly across all 16 buckets.

### Technical Question 6: What happens during `rehash()` in your custom Hash Table?
**Answer**:
When the load factor $\alpha \ge 0.75$, the `rehash()` function:
1. Doubles `bucketCount_` (e.g. from 16 to 32).
2. Allocates a temporary `std::vector<std::vector<BucketEntry>>` with 32 empty chains.
3. Iterates through all existing buckets and re-hashes every entry: $\text{newBucketIndex} = \text{hash}(\text{key}) \pmod{32}$.
4. Swaps the old bucket vector with the new bucket vector via `buckets_ = std::move(newBuckets)`.
This operation takes $O(N)$ linear time but occurs infrequently (at powers of two), giving insertions an amortized $O(1)$ time complexity.

### Technical Question 7: Why does your AVL tree return a copy of the value instead of a raw reference?
**Answer**:
In `AVLTree::search()`, returning an `std::optional<Value>` by value prevents returning dangling references to internal tree nodes that might be moved or deleted during subsequent tree balancing or rotations. For our `Incident` struct (which contains small strings and integers), copying is lightweight and memory-safe.

### Technical Question 8: How do you prevent thread-safety race conditions on the city road graph?
**Answer**:
In [`backend/src/api/server.cpp`](file:///b:/ADS%20Project/backend/src/api/server.cpp), all state-mutating HTTP handlers (updating road congestion, toggling closures, creating incidents, or executing dispatches) acquire an `std::lock_guard<std::mutex> lock(graphMutex_)`. This serializes concurrent requests and prevents one thread from reading an adjacency list while another thread is modifying road segment weights.

### Technical Question 9: Why does the Dijkstra router return the total node settlements and edge relaxations in its response?
**Answer**:
Returning `nodesSettled`, `nodesExplored`, and `relaxations` allows students and instructors to verify algorithmic efficiency. For example, when a road closure cuts off a direct highway path, the inspector proves that Dijkstra explored more nodes ($N_{\text{settled}}$ increases) and evaluated detour edges before finding the alternate shortest route.

### Technical Question 10: How does the trace player maintain step consistency when a user steps backward?
**Answer**:
Forward execution traces are recorded as a list of discrete `TraceEvent` objects. Reversing an arbitrary mutation in memory can be error-prone. Instead, our trace player (`useTracePlayer.ts`) derives the displayed visual state functionally from the slice of visible events `events.slice(0, currentStepIndex + 1)`. When the user steps backward from step $k$ to $k - 1$, the player re-derives the tentative distances and settled node sets from the first $k - 1$ events, ensuring mathematical state consistency without unsafe mutation rollback.

---

## 5. Live Demonstration Sequence (Step-by-Step for Presentation)

Follow this 5-minute workflow during a live project evaluation:

1. **Step 1: Show C++ Engine Online**:
   - Open [`http://localhost:3000`](http://localhost:3000). Show the green glowing pill: *"C++ Engine Online (ISO C++20)"*. Explain that Next.js is communicating with a real C++20 HTTP daemon on port 8080.
2. **Step 2: Operations Mode — Create an Emergency**:
   - Click **"Report Incident"**. Create a `CRITICAL` Fire incident at intersection `J5`.
   - Point out that it appears immediately at the top of the **Priority Incident Queue** due to the Binary Max-Heap triage invariant.
3. **Step 3: Operations Mode — Optimal Dispatch & Route Visualization**:
   - Click **"Dispatch Next"**.
   - Show the green vehicle polyline appear on the MetroGrid-16 map. Show the explanation card: *"ENG-02 selected: shortest travel time (6.40 min)"*.
   - Click on the road that the vehicle is using, open **"Road Controls"**, and toggle it to **CLOSED**.
   - Click **"Recalculate Route"**. Show that the path dynamically updates to a detour route, demonstrating Dijkstra rerouting.
4. **Step 4: Switch to DSA Lab (`/dsa-lab`)**:
   - Switch tabs to the **DSA Lab**.
   - Select **"Graph & Dijkstra"**: Choose J1 $\to$ J16, click **"Compute Route & Trace"**, and hit **Play**. Watch the algorithm explore edges, update the tentative distance table, relax nodes, and settle the optimal path.
   - Select **"Incident Priority Max-Heap"**: Click **"Extract Max"** and watch the sift-down comparisons and swaps between child nodes and root. Show both the binary tree and the 0-based array buffer.
   - Select **"Incident Records AVL Tree"**: Click **"LR (Double Left-Right Rotation)"** and watch the two-step rotation restore balance factor $BF = 0$.
   - Select **"Incident Lookup Hash Table"**: Click **"Trigger Dynamic Rehash"** and show the bucket array expand from 16 to 32 buckets when load factor $\ge 0.75$.
5. **Step 5: Run Automated Verification Tests**:
   - Open PowerShell and run:
     ```powershell
     cd frontend
     node test-dsa-lab.mjs
     ```
   - Show that all 8 automated checks pass, proving that educational demonstrations never mutate live Operations Mode state.

---

## 6. Honest Limitations & Future Enhancements

When asked *"What are the limitations of your project?"*, give this honest, mature engineering answer:

1. **Graph Scale**: MetroGrid-16 currently models 16 intersections and 26 road segments. This is ideal for academic viva visualization, but an enterprise city deployment would require thousands of nodes, where algorithms like Contraction Hierarchies (CH) or Transit Node Routing would be needed.
2. **Greedy Single-Unit Dispatch**: The dispatch engine currently matches incidents one at a time using a greedy shortest-time selection. Under simultaneous mass-casualty disasters, global batch optimization algorithms (such as the Hungarian Algorithm or Min-Cost Max-Flow) would achieve better fleet-wide utilization.
3. **In-Memory Volatility**: The simulation core deliberately avoids third-party database dependencies. If the server process is terminated, simulation state resets to the seed topology. Adding an optional write-ahead log (WAL) would provide persistence across crashes.
