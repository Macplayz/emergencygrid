# EmergencyGrid — Data Structures & Algorithms Specification

## 1. Academic Philosophy & Implementation Integrity

In keeping with the standards of an Advanced Data Structures (ADS) project:
- **Zero Black-Box Wrapping for Core Components**: We will **not** wrap standard library containers (`std::priority_queue`, `std::map`, `std::unordered_map`) and misrepresent them as custom data structures. 
- **Genuine C++ Implementations**: All core educational structures (`AdjacencyListGraph`, `BinaryHeap`, `AVLTree`, `HashTable`, and `DijkstraRouter`) are implemented manually from pointer and memory primitives in C++20.
- **Trace-First Architecture**: Each educational structure is designed with optional trace interceptors that record the internal atomic mutations (rotations, swim/sink swaps, relaxations, bucket traversals) without mutating or distorting algorithm results.

---

## 2. Comprehensive Data Structure & Algorithm Catalog

### 2.1 Graph with Adjacency List (`RoadGraph` / `AdjacencyListGraph`)

#### A. Purpose & Role
Models the physical road network of the simulated city ("MetroGrid-16"). Intersections, stations, and hospital centers are represented as vertices (`Location`); connecting road segments are represented as bidirectional or directed edges (`RoadSegment`).

#### B. Implementation Strategy
- **Structure**: Adjacency list backed by indexed associative storage and deterministic node/edge ordering vectors.
- **Edge Weight Model**: Travel time $T_{e}$ is computed dynamically using base physical length $L_{e}$ (km), speed limit $S_{e}$ (km/h), and dynamic congestion factor $C_{e} \ge 1.0$:
  $$T_{e} = \frac{L_{e}}{S_{e}} \times C_{e} \times 60 \quad (\text{minutes})$$
- **Road Closures**: If `isClosed == true`, the edge is bypassed during graph traversal and treated as impassable ($+\infty$).
- **Dynamic Updates**:
  - `setRoadClosed(roadId, bool)`: Toggles segment accessibility.
  - `setRoadCongestion(roadId, factor)`: Multiplies base travel time.
  - `updateRoadTravelTime(roadId, travelTimeMinutes)`: Recalculates effective congestion factor.
  - Invalid inputs (negative weights, zero speed limit, unknown IDs) are rejected safely.

#### C. Core Operations & Complexities

| Operation | Description | Average Complexity | Worst-Case Complexity |
| :--- | :--- | :---: | :---: |
| `addLocation(loc)` | Inserts a new intersection into the graph | $O(1)$ | $O(1)$ |
| `addRoad(road)` | Adds bidirectional or directed road segment | $O(1)$ | $O(1)$ |
| `setRoadClosed(id, closed)` | Toggles segment passability | $O(1)$ | $O(1)$ |
| `setRoadCongestion(id, factor)` | Modifies dynamic travel time multiplier | $O(1)$ | $O(1)$ |
| `getOutgoingEdges(locId)` | Returns sorted outgoing directed edges | $O(\deg(u) \log \deg(u))$ | $O(\deg(u) \log \deg(u))$ |

*(Note: In planar city road networks, maximum vertex degree $\deg(u) \le 6$, effectively making neighbor retrieval $O(1)$).*

---

### 2.2 Dijkstra's Shortest-Path Algorithm (`DijkstraRouter`)

#### A. Purpose & Role
Calculates the minimum-travel-time route between two locations across the city network, accounting for road closures, speed limits, and traffic congestion.

#### B. Implementation Strategy
- **Priority Queue**: Min-priority frontier comparing candidate tentative travel times.
- **Deterministic Tie-Breaking**: When two candidates have equal tentative distances (within $\epsilon = 10^{-6}$), the tie is broken lexicographically by `nodeId`. For equal-cost relaxations, predecessor ties are broken deterministically by `fromNodeId` followed by `roadId`.
- **Stale Entry Elimination**: Compares popped candidate travel time against settled state and tentative distance map, emitting explicit `skip_stale_entry` trace events without re-evaluating settled nodes.
- **Path Reconstruction**: Backtracks predecessor pointers from destination to origin, reconstructing both node sequences and road IDs.

#### C. Trace Event Lifecycle

During execution, `DijkstraRouter` streams fine-grained operational events:
1. `initialize_distances`: Sets tentative distances to $\infty$, source to $0.0$.
2. `extract_min`: Min-priority queue extracts the closest candidate node.
3. `skip_stale_entry`: Discards redundant queue items already surpassed by a better path.
4. `examine_edge`: Inspects neighbor passability and edge weight.
5. `skip_closed_road`: Bypasses edges where `isClosed == true`.
6. `relax_edge`: Shorter tentative travel time discovered; updates distance and predecessor.
7. `finalize_node`: Marks intersection as settled with definitive shortest path.
8. `destination_reached`: Halts search early upon settling destination.
9. `route_reconstructed`: Reverses predecessor chain into final ordered node/road list.
10. `no_route_found`: Emitted if reachable frontier is exhausted without reaching target.

#### D. Core Complexities

| Phase | Description | Complexity |
| :--- | :--- | :---: |
| Initialization | Set initial distances and predecessor pointers | $O(V)$ |
| Frontier Queue | At most $V$ extractions and $E$ candidate pushes | $O((V + E) \log V)$ |
| Path Reconstruction | Backtrack predecessor pointers from target to source | $O(V)$ |
| **Total Runtime** | Overall shortest-path computation | **$O((V + E) \log V)$** |
| **Space Complexity** | Distance, predecessor, and settled collections | **$O(V)$** |

### 2.3 Priority Queue for Incident Triage (`BinaryHeap`)

#### A. Purpose & Role
Maintains pending emergency calls in strict order of response priority so that high-urgency incidents (e.g., massive structure fires, cardiac arrests) receive immediate dispatch attention even under heavy simulation load.

#### B. Implementation Strategy
- **Manual Implementation**: Implemented manually in C++20 as a contiguous zero-based array-backed binary max-heap (`std::vector<Incident>`). No `std::priority_queue` or standard container wrappers are used.
- **Zero-Based Array Index Calculations**:
  - $\text{Parent}(i) = \lfloor (i - 1) / 2 \rfloor$
  - $\text{LeftChild}(i) = 2i + 1$
  - $\text{RightChild}(i) = 2i + 2$
- **Deterministic Priority Ordering Rule**:
  1. **Higher Severity First**: $\text{Critical} (4) > \text{High} (3) > \text{Medium} (2) > \text{Low} (1)$.
  2. **FIFO for Equal Severity**: If $\text{Severity}(A) == \text{Severity}(B)$, earlier arrival comes first ($\text{arrivalSequence}_A < \text{arrivalSequence}_B$).
  3. **Deterministic Tie-Breaker**: If both severity and arrival sequence are identical, lexicographical comparison of unique $\text{IncidentID}$ breaks ties deterministically ($A.\text{id} < B.\text{id}$).
- **Heap Invariant Maintenance**:
  - `heapifyUp(index)`: Sifts an inserted element upwards by iteratively swapping with its parent whenever the child has higher priority.
  - `heapifyDown(index)`: Sifts an element downward by identifying the highest-priority candidate among the node, left child, and right child, swapping and continuing until the heap invariant is restored.

#### C. Authoritative Incident Records & Lazy Invalidation Strategy
To prevent duplicate or stale entries:
- **Authoritative Registry**: Authoritative incident records and their lifecycle statuses are stored separately in `IncidentService::records_`. The binary heap acts solely as a prioritization queue.
- **Incident Lifecycle**:
  $$\text{PENDING} \longrightarrow \text{ASSIGNED} \longrightarrow \text{RESOLVED}$$
  $$\text{PENDING} \longrightarrow \text{CANCELLED}$$
- **Duplicate ID Rejection**: `createIncident` validates uniqueness against the authoritative registry; duplicates are rejected with `DUPLICATE_INCIDENT_ID` (`409 Conflict`).
- **Graph Location Validation**: `locationId` is verified against the active `RoadGraph`; unknown nodes trigger `LOCATION_NOT_FOUND` (`404 Not Found`).
- **Lazy Invalidation**: When an incident is cancelled or assigned, its authoritative status is updated immediately. The heap entry is not arbitrarily removed; rather, when it reaches the root during `peekNext` or `selectNext`, `purgeStaleTop` inspects the authoritative record. If the status is not `PENDING`, the entry is discarded from the heap, an explicit `skip_stale_entry` trace event is recorded, and the queue continues draining until a valid pending incident is surfaced or the heap becomes empty.

#### D. Core Operations & Complexities

| Operation | Description | Average Complexity | Worst-Case Complexity |
| :--- | :--- | :---: | :---: |
| `insert(incident)` | Appends to array and executes `heapifyUp(index)` | $O(\log N)$ | $O(\log N)$ |
| `extractMax()` | Swaps root with last element, pops, executes `heapifyDown(0)` | $O(\log N)$ | $O(\log N)$ |
| `peek()` | Inspects top-priority incident without removing | $O(1)$ | $O(1)$ |
| `heapifyUp(index)` | Sifts element up using parent calculation $(i-1)/2$ | $O(\log N)$ | $O(\log N)$ |
| `heapifyDown(index)` | Sifts element down using child calculations $2i+1, 2i+2$ | $O(\log N)$ | $O(\log N)$ |
| `isEmpty()` / `size()` | Queries heap element count | $O(1)$ | $O(1)$ |
| `purgeStaleTop()` | Discards non-pending entries from top of heap | Amortized $O(1)$ | $O(N \log N)$ (draining) |

#### E. Trace Event Catalog (`BinaryHeap`)

| `operation` | Key Fields Populated | Description |
| :--- | :--- | :--- |
| `insert` | `entityId`, `index`, `severity`, `arrivalSequence` | Appending element to heap array |
| `heapify_up` | `startIndex`, `incidentId` | Starting upward sift to restore max-heap |
| `compare` | `childIndex`, `parentIndex`, `childId`, `parentId` | Comparing priority between nodes |
| `swap` | `fromIndex`, `toIndex`, `childId`, `parentId` | Swapping elements in array |
| `heapify_down`| `startIndex`, `incidentId` | Starting downward sift from root |
| `peek` | `rootId`, `severity`, `heapSize` | Non-mutating inspection of root |
| `extract_max` | `extractedId`, `severity`, `remainingSize` | Dequeuing the highest-priority incident |
| `skip_stale_entry`| `entityId`, `staleStatus`, `reason` | Discarding cancelled/assigned entry at root |

#### F. DSA Lab Visualization
- **Dual Representation**: Synchronized side-by-side rendering of:
  1. The hierarchical **Tree View** showing parent-child links.
  2. The flat **Array Memory View** showing exact 0-indexed contiguous storage ($2i+1, 2i+2$).
- **Live Snapshot Endpoint**: `GET /api/v1/dsa/heap/snapshot` exposes the full internal array, element metadata, and parent/child index pointers for live visualization.

---

### 2.4 Multi-Constraint Resource Selection Engine

#### A. Purpose & Role
When an incident is pulled from the priority queue, the dispatch engine must select the best available emergency unit.

#### B. Implementation Strategy
- **Two-Stage Decision Pipeline**:
  1. **Eligibility Filtering**: Query the unit fleet for responders matching required agency criteria (`agency == incident.required_service`) and status (`status == AVAILABLE`).
  2. **Travel-Time Ranking**: For all eligible units, run `DijkstraRouter` from the unit's current station/location to the incident location.
  3. **Selection**: Select the unit with $\min(\text{TravelTime})$. If tied, pick the unit with lowest cumulative responses to balance workload.
- **Educational Algorithmic Utility**: Uses a temporary Min-Heap of eligible candidate units ordered by travel time, demonstrating an $O(K \log K)$ ranking of $K$ candidate units.

#### C. Core Operations & Complexities
- Filtering $M$ total fleet units: $O(M)$.
- Running Dijkstra for $K$ eligible units to target: $K \times O((V + E) \log V)$.
- For our small city topology ($V=16, M=12, K \le 4$), this executes in under 2 milliseconds, maintaining instantaneous UI response.

#### D. DSA Lab Visualization
- **Candidate Evaluation Table**: Shows eligible units, disqualified units (with reasons: "Unit Busy", "Wrong Agency"), Dijkstra travel times, and final ranking.

---

### 2.5 Self-Balancing Binary Search Tree (`AVLTree`)

#### A. Purpose & Role
Maintains incident records in strict ascending sorted order by key (e.g. `Incident::id`), guaranteeing logarithmic search, insertion, and deletion while enabling deterministic sorted inorder traversals for audit logs and tabular displays. The AVL Tree serves as a secondary ordered index over authoritative incident records and does **not** replace the binary max-heap.

#### B. Implementation Strategy
- **Manual vs STL**: Fully manual implementation in C++20 (`emergencygrid::AVLTree<Key, Value>`). No `std::map`, `std::set`, or balanced container wrappers are used.
- **Safe Memory Management**: Utilizes `std::unique_ptr<AVLNode<Key, Value>>` for strict RAII ownership and subtree lifetime management, preventing memory leaks, dangling pointers, and double-free vulnerabilities.
- **Balancing Invariant**: After every insertion and deletion, preserves the strict AVL balance invariant:
  $$\text{BF}(\text{node}) = \text{Height}(\text{left}) - \text{Height}(\text{right}) \in \{-1, 0, 1\}$$
- **Deterministic 4-Case Rotations**:
  1. **Left-Left (LL) Case** (Right Rotation):
     $$\text{BF}(\text{node}) > 1 \text{ and } \text{BF}(\text{left}) \ge 0 \implies \text{rotateLL}(\text{node})$$
  2. **Right-Right (RR) Case** (Left Rotation):
     $$\text{BF}(\text{node}) < -1 \text{ and } \text{BF}(\text{right}) \le 0 \implies \text{rotateRR}(\text{node})$$
  3. **Left-Right (LR) Case** (Double Rotation):
     $$\text{BF}(\text{node}) > 1 \text{ and } \text{BF}(\text{left}) < 0 \implies \text{rotateRR}(\text{left}) \text{ then } \text{rotateLL}(\text{node})$$
  4. **Right-Left (RL) Case** (Double Rotation):
     $$\text{BF}(\text{node}) < -1 \text{ and } \text{BF}(\text{right}) > 0 \implies \text{rotateLL}(\text{right}) \text{ then } \text{rotateRR}(\text{node})$$
- **Deletion Rebalancing**: Supports deleting leaf nodes, nodes with a single left or right child, and nodes with two children (via inorder successor replacement), followed by upward height recalculation and rotation checks.
- **Duplicate Key Policy**: Rejects duplicate keys deterministically, preserving the original record and leaving the tree invariant intact.

#### C. Core Operations & Complexities

| Operation | Description | Average Complexity | Worst-Case Complexity |
| :--- | :--- | :---: | :---: |
| `insert(key, value)` | BST insertion followed by balance check & rotations | $O(\log N)$ | $O(\log N)$ |
| `search(key)` | Exact-key lookup | $O(\log N)$ | $O(\log N)$ |
| `contains(key)` | Boolean membership query | $O(\log N)$ | $O(\log N)$ |
| `remove(key)` | Deletion with successor replacement, rebalancing upward | $O(\log N)$ | $O(\log N)$ |
| `update(key, value)` | In-place value modification of existing key | $O(\log N)$ | $O(\log N)$ |
| `inorderTraversal()` | Returns sorted list of records in ascending key order | $O(N)$ | $O(N)$ |
| `inorderKeys()` | Returns keys in ascending order | $O(N)$ | $O(N)$ |
| `toJsonSnapshot()` | Exports complete hierarchy for DSA Lab visualization | $O(N)$ | $O(N)$ |

#### D. Trace Event Catalog (`AVLTree`)

| `operation` | Key Fields Populated | Description |
| :--- | :--- | :--- |
| `compare` | `searchKey`, `currentNodeKey`, `currentHeight`, `balanceFactor` | Key comparison during traversal |
| `insert` | `key`, `height`, `balanceFactor` | Insertion of a new node |
| `delete` | `deletedKey`, `hadLeftChild`, `hadRightChild` | Removal of an existing node |
| `height_update` | `nodeKey`, `oldHeight`, `newHeight`, `balanceFactor` | Height recalculation after mutation |
| `balance_factor`| `nodeKey`, `balanceFactor`, `unbalanced` | Detection of invariant violation ($|BF| > 1$) |
| `rotate_ll` | `pivotKey`, `promotedKey`, `newPivotHeight`, `newBalanceFactor` | Single right rotation (LL) |
| `rotate_rr` | `pivotKey`, `promotedKey`, `newPivotHeight`, `newBalanceFactor` | Single left rotation (RR) |
| `rotate_lr_start`| `pivotKey`, `childKey` | Beginning double left-right rotation (LR) |
| `rotate_rl_start`| `pivotKey`, `childKey` | Beginning double right-left rotation (RL) |
| `inorder_visit` | `visitedKey`, `height`, `balanceFactor` | Visiting node during inorder traversal |
| `search_found` | `foundKey` | Successful key search |
| `search_not_found`| `missingKey` | Unsuccessful key search |

---

### 2.6 Custom Hash Table with Separate Chaining (`HashTable`)

#### A. Purpose & Role
Provides guaranteed average $O(1)$ constant-time lookup, insertion, update, and deletion of authoritative incident records by unique string identifier (e.g., `"INC-101"`).

#### B. Implementation Strategy
- **Manual vs STL**: Implemented manually in C++20 (`emergencygrid::HashTable<Key, Value>`). No `std::unordered_map` or external hash tables are used.
- **Deterministic Hash Function**: Computes 64-bit FNV-1a hash values for keys:
  $$\text{hash} = \text{FNV\_OFFSET\_BASIS} \ (14695981039346656037)$$
  $$\text{hash} = (\text{hash} \oplus c) \times \text{FNV\_PRIME} \ (1099511628211)$$
  $$\text{bucketIndex} = \text{hash} \pmod{\text{bucketCount}}$$
- **Collision Handling**: Separate chaining per bucket (`std::vector<std::vector<HashEntry>>`). When collisions occur, colliding entries are appended to the bucket chain.
- **Dynamic Capacity Scaling & Automatic Rehashing**:
  - Initial default capacity: $16$ buckets.
  - Load factor threshold: $\lambda_{\max} = 0.75$.
  - When $\text{size} + 1 > \text{bucketCount} \times \lambda_{\max}$, the table automatically doubles capacity ($2 \times \text{bucketCount}$) and rehashes all existing entries into their new bucket positions.
- **Safe Statistics Export**: Exposes bucket occupancy, max chain depth, collision count, and load factor without revealing raw memory addresses or pointers.

#### C. Core Operations & Complexities

| Operation | Description | Average Case | Worst Case (All Collide) |
| :--- | :--- | :---: | :---: |
| `insert(key, val)` | Hashing, collision check, bucket chain insertion | $O(1)$ | $O(N)$ |
| `search(key)` | Hashing, bucket chain traversal | $O(1)$ | $O(N)$ |
| `contains(key)` | Boolean membership query | $O(1)$ | $O(N)$ |
| `update(key, val)` | Hashing, chain traversal, value replacement | $O(1)$ | $O(N)$ |
| `remove(key)` | Hashing, unlinking entry from bucket chain | $O(1)$ | $O(N)$ |
| `rehash(newCap)` | Allocates new bucket vector, redistributes all entries | $O(N)$ | $O(N)$ |

#### D. Trace Event Catalog (`HashTable`)

| `operation` | Key Fields Populated | Description |
| :--- | :--- | :--- |
| `hash_key` | `key`, `rawHash`, `bucketIndex`, `bucketCount` | Hash calculation and bucket index selection |
| `bucket_traversal`| `bucketIndex`, `chainIndex`, `examinedKey`, `targetKey` | Inspecting chained entry during lookup |
| `collision_detected`| `bucketIndex`, `existingChainLength`, `newKey` | Insertion into an already occupied bucket |
| `duplicate_rejected`| `bucketIndex`, `rejectedKey` | Duplicate key rejected during insertion |
| `entry_inserted`| `key`, `bucketIndex`, `chainLength`, `totalEntries`, `loadFactor` | Entry appended to bucket chain |
| `entry_found` | `key`, `bucketIndex`, `chainPosition` | Key located in bucket chain |
| `entry_not_found`| `key`, `bucketIndex`, `probedEntries` | Key not found in bucket |
| `entry_updated` | `updatedKey` | Existing key value updated |
| `entry_deleted` | `key`, `bucketIndex`, `remainingBucketLength`, `totalEntries` | Entry removed from bucket chain |
| `rehash_started`| `oldBucketCount`, `newBucketCount`, `currentSize`, `currentLoadFactor` | Resizing threshold triggered |
| `rehash_completed`| `newBucketCount`, `totalEntries`, `newLoadFactor`, `collisionCount` | Rehashing finished |

---

### 2.7 Index Consistency Strategy Across Incident Life Cycle

To avoid storing independently mutable copies of incidents and guarantee total consistency:
1. **Authoritative Lookup**: `IncidentService::table_` (`IncidentHashTable`) owns the authoritative primary records indexed by incident ID.
2. **Chronological / Lexical Ordering**: `IncidentService::avlTree_` (`IncidentAVLTree`) maintains the sorted secondary index by incident ID.
3. **Dispatch Triage Priority**: `IncidentService::pendingHeap_` (`BinaryHeap`) maintains the priority queue for pending incidents.
4. **Coordinated Mutations**:
   - `createIncident`: Inserts the incident into `table_`, `avlTree_`, and `pendingHeap_` in a single atomic transaction under `mutex_`. If duplicate ID is detected in `table_`, the operation fails with `DUPLICATE_INCIDENT_ID` (`409 Conflict`) without touching `avlTree_` or `pendingHeap_`.
   - `selectNext`: Extracts top item from `pendingHeap_`, checks authoritative status in `table_`, sets status to `ASSIGNED`, and updates `avlTree_` in lockstep.
   - `cancelIncident`: Updates status to `CANCELLED` in `table_` and `avlTree_`. The heap entry is lazily discarded when it surfaces to the root via `purgeStaleTop()`.
   - `resolveIncident`: Updates status to `RESOLVED` in `table_` and `avlTree_`.

---

## 3. Summary Matrix of Educational Data Structures

| Data Structure | Primary Role in EmergencyGrid | Custom Manual C++ Implementation Details | Trace Events Emitted |
| :--- | :--- | :--- | :--- |
| **`AdjacencyListGraph`** | Topology, roads, congestion | Dynamic edge arrays, directed/bidirectional toggle | Edge weight update, road closure |
| **`DijkstraRouter`** | Travel time & route generation | Min-Heap priority queue integration, predecessor map | Node visit, edge check, edge relaxation, path backtrace |
| **`BinaryHeap`** | Incident triage prioritization | Contiguous array storage, parent/child math, swim/sink | Push, pop, swap, priority escalation, skip stale |
| **`AVLTree`** | Ordered incident index | Pointer-based tree (`std::unique_ptr`), heights, balance factors | Compare, insert, delete, height update, LL/RR/LR/RL rotation, inorder visit |
| **`HashTable`** | Instant $O(1)$ ID lookups | 64-bit FNV-1a hash, separate chaining, automatic rehashing | Hash key, bucket traversal, collision, insert, found, delete, rehash |

---

## 4. Emergency Unit Allocation & Dispatch Engine (Prompt 6)

### 4.1 Architecture Overview
The emergency unit allocation and dispatch engine coordinates the response fleet across the simulated city network by coupling the graph shortest-path router, the priority heap triage queue, and custom hash table indices.

```
                  +-------------------------+
                  |    Pending Incidents    |
                  |   (Binary Heap Triage)  |
                  +------------+------------+
                               |
                        select next / ID
                               v
                  +-------------------------+
                  |     Dispatch Service    |
                  +------------+------------+
                               |
         +---------------------+---------------------+
         |                                           |
         v                                           v
+------------------+                       +-------------------+
|   Unit Service   |                       |   DijkstraRouter  |
|  (Fleet Table)   |                       |   (Graph Engine)  |
+--------+---------+                       +---------+---------+
         |                                           |
  1. Filter Agency                                   |
  2. Filter Capabilities                             |
  3. Filter AVAILABLE Status                         |
         |                                           |
         +---> Eligible Candidates ------------------+
                            |
                     Compute Routes
                            v
              Select Min Travel Time (Dijkstra)
              (Tie-break: lexicographical unitId)
                            |
                            v
             +------------------------------+
             |    Atomic State Transition   |
             |  Unit -> DISPATCHED          |
             |  Incident -> ASSIGNED        |
             |  DispatchRecord -> ACTIVE    |
             +------------------------------+
```

### 4.2 Emergency Unit Data Model & Lifecycle
Each emergency unit (`EmergencyUnit`) models a physical first-responder resource:
- **`id`**: Unique string identifier (e.g. `AMB-01`, `ENG-01`, `POL-01`).
- **`name` / `callSign`**: Human-readable call sign or agency label.
- **`serviceType`**: Primary agency (`MEDICAL` / Ambulance, `FIRE` / Rescue, `POLICE`).
- **`locationId`**: Current intersection node in `RoadGraph`.
- **`capabilities`**: Specific tags for equipment and medical/rescue qualifications (e.g., `BLS`, `ALS`, `FIRE_SUPPRESSION`, `HEAVY_RESCUE`, `HAZMAT`, `PATROL`, `TRAFFIC_CONTROL`).
- **`status`**: Current operational state.
- **`activeIncidentId`**: ID of assigned incident when busy/dispatched.

#### Allowed Unit Lifecycle State Transitions:
- `AVAILABLE` $\rightarrow$ `DISPATCHED` (assigned to an incident)
- `AVAILABLE` $\rightarrow$ `OUT_OF_SERVICE` (maintenance / manual status change)
- `DISPATCHED` $\rightarrow$ `RESPONDING` (en route / updated response)
- `DISPATCHED` $\rightarrow$ `AVAILABLE` (response completed or cancelled)
- `RESPONDING` $\rightarrow$ `BUSY` / `ON_SCENE` (on scene at incident)
- `RESPONDING` $\rightarrow$ `AVAILABLE` (response completed)
- `OUT_OF_SERVICE` $\rightarrow$ `AVAILABLE` (only transition permitted for out-of-service units)
- *Illegal Transitions*: Direct transition from `OUT_OF_SERVICE` to `DISPATCHED` or `RESPONDING` is strictly rejected. Relocating a unit while `DISPATCHED`, `RESPONDING`, or `BUSY` is prohibited.

### 4.3 Dispatch-Selection Algorithm
When dispatch is triggered for an incident:
1. **Incident Validation**: Look up incident in custom `HashTable`; verify it exists and is in `PENDING` status.
2. **Capability & Service Matching**:
   $$\text{Eligible}(u) \iff \text{status}(u) = \text{AVAILABLE} \;\land\; \text{type}(u) = \text{type}(inc) \;\land\; \text{reqCaps}(inc) \subseteq \text{caps}(u)$$
3. **Shortest Path Computation via Reusable Dijkstra Engine**:
   For each candidate $u \in \text{Eligible}$:
   $$\text{route}_u = \text{DijkstraRouter::findShortestPath}(\text{graph}, \text{location}(u), \text{location}(inc))$$
   Units with $\text{route}_u.\text{routeFound} = \text{false}$ (e.g. road partitions) are discarded.
4. **Deterministic Selection Rule**:
   $$\text{SelectedUnit} = \arg\min_{u \in \text{Reachable}} \Big(\text{route}_u.\text{travelTimeMinutes}, \; \text{lexicographical}(u.\text{id})\Big)$$
5. **Atomic State Mutation**:
   - `unit.status` $\leftarrow$ `DISPATCHED`
   - `unit.activeIncidentId` $\leftarrow$ `incident.id`
   - `incident.status` $\leftarrow$ `ASSIGNED`
   - Create `DispatchRecord` with status `ACTIVE`.
6. **Completion**:
   - `dispatch.status` $\leftarrow$ `COMPLETED`
   - `incident.status` $\leftarrow$ `RESOLVED`
   - `unit.status` $\leftarrow$ `AVAILABLE`, `unit.activeIncidentId` cleared.

### 4.4 Trace Event Catalog (`DispatchEngine` & `ResourceAllocation`)

| `operation` | Key Fields Populated | Description |
| :--- | :--- | :--- |
| `validate_incident` | `incidentId`, `serviceType`, `locationId`, `requiredCapabilities` | Initial incident eligibility check |
| `candidate_rejected` | `unitId`, `reason` (`missing_capabilities`, `wrong_service_type`, `status_unavailable`, `no_route_found`) | Candidate filtered out |
| `route_calculation_requested` | `unitId`, `origin`, `destination` | Invoking Dijkstra routing for candidate |
| `candidate_route_evaluated` | `unitId`, `travelTimeMinutes`, `totalDistanceKm`, `path` | Successful candidate route result |
| `candidate_selected` | `unitId`, `travelTimeMinutes`, `candidatesEvaluated` | Lowest travel-time candidate selected |
| `dispatch_created` | `dispatchId`, `incidentId`, `unitId`, `travelTimeMinutes` | Active dispatch recorded |
| `dispatch_completed` | `dispatchId`, `incidentId`, `unitId` | Response finished and unit freed |
| `route_recalculated` | `dispatchId`, `oldTravelTimeMinutes`, `newTravelTimeMinutes` | Route recalculated under modified road conditions |

