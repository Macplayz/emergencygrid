# EmergencyGrid — Complete Project Context & Architecture Guide

> **Purpose of this document:**  
> This file contains the complete, self-contained context of the **EmergencyGrid** codebase. You can copy and paste the entire contents of this file into any AI agent (such as ChatGPT, Claude, or Gemini) or open it in Notepad / VS Code during Advanced Data Structures & Algorithms (ADS) viva preparation to gain immediate, comprehensive understanding of every folder, file, data structure, algorithm, API endpoint, and architectural decision.

---

## 1. Executive Summary

**Project Name:** EmergencyGrid — Real-Time Multi-Agency Emergency Response Simulation System  
**Domains:** Advanced Data Structures & Algorithms (ADS), Distributed Systems, Real-Time City Logistics, Interactive Educational Visualizations.  
**Tech Stack:**
- **Backend Core:** C++20 standard library, `cpp-httplib` (single-header HTTP server), `nlohmann/json` (modern C++ JSON). Built with CMake and MinGW-w64 GCC 13+ (or MSVC).
- **Frontend Application:** Next.js 16 (App Router with Turbopack), TypeScript, Tailwind CSS, Lucide React icons.
- **Architecture:** Client-Server decoupled architecture. The Next.js frontend forwards requests via an internal proxy (`/api/proxy/*`) to the C++ HTTP engine running on `http://127.0.0.1:8080`.

EmergencyGrid simulates a fictional metropolitan city (**MetroGrid-16**) comprising 16 major intersections and 26 bidirectional road segments. The system manages emergency incidents across three municipal agencies:
1. **EMS (Ambulance / Medical)**
2. **Fire Rescue**
3. **Police Department**

The core purpose of EmergencyGrid is twofold:
1. **Operations Mode (`/`):** A real-time dispatch dashboard where operators report emergencies, view dynamic city maps, inspect fleet unit availability, update road congestion, close/reopen roads, and trigger optimal multi-agency unit dispatches.
2. **DSA Lab (`/dsa-lab`):** An educational laboratory featuring step-by-step algorithm trace visualizers for each custom data structure implemented from scratch in C++.

---

## 2. Complete Repository Directory & File Map

```
b:\ADS Project\
├── .env.example                               # Sample environment variables for frontend/backend
├── package.json                               # Root workspace meta package
├── README.md                                  # Repository overview and quick start guide
├── PROJECT_CONTEXT.md                         # (This file) Complete project context for AI agents & viva
├── HOW_TO_RUN.md                              # Plain step-by-step instructions to build & run everything
│
├── backend/                                   # C++20 HTTP Engine & Custom DSA Libraries
│   ├── CMakeLists.txt                         # CMake build script defining backend binary and 4 test targets
│   ├── build_backend.bat                      # Automated Windows batch script to build backend with MinGW/CMake
│   │
│   ├── include/                               # C++ Header Files
│   │   ├── api/
│   │   │   └── server.hpp                     # HttpServer class declaration (HTTP endpoints & mutex synchronization)
│   │   ├── core/
│   │   │   ├── trace.hpp                      # TraceEvent & TraceCollector for step-by-step algorithm telemetry
│   │   │   └── types.hpp                      # Core data structures (Incident, Location, RoadSegment, EmergencyUnit, etc.)
│   │   ├── dsa/
│   │   │   ├── avl_tree.hpp                   # Custom self-balancing AVL Tree template with full rotation tracing
│   │   │   ├── binary_heap.hpp                # Custom Binary Max-Heap class for priority queue scheduling
│   │   │   ├── dijkstra.hpp                   # DijkstraRouter class declaration
│   │   │   ├── graph.hpp                      # RoadGraph class (adjacency list, locations, roads, closures)
│   │   │   └── hash_table.hpp                 # Custom separate-chaining Hash Table with polynomial rolling hash
│   │   ├── external/
│   │   │   ├── httplib.h                      # Yhirose cpp-httplib v0.14+ (header-only HTTP/HTTPS server)
│   │   │   └── json.hpp                       # Niels Lohmann JSON v3.11+ library
│   │   └── simulation/
│   │       ├── dispatch_service.hpp           # DispatchService class (candidate evaluation & unit allocation)
│   │       ├── incident_service.hpp           # IncidentService class (combines AVL Tree, Hash Table, and Max-Heap)
│   │       ├── seed_data.hpp                  # Default MetroGrid-16 topology, locations, roads, and emergency units
│   │       └── unit_service.hpp               # UnitService class (fleet tracking, status, capability filtering)
│   │
│   ├── src/                                   # C++ Implementation Files
│   │   ├── main.cpp                           # Backend executable entry point (initializes graph, seeds, starts server)
│   │   ├── api/
│   │   │   └── server.cpp                     # Full HTTP REST API implementation, routing endpoints, DSA sandbox endpoints
│   │   ├── dsa/
│   │   │   ├── binary_heap.cpp                # Binary Max-Heap implementation (sift-up, sift-down, tracing)
│   │   │   ├── dijkstra.cpp                   # Dijkstra shortest-path router, SPT extraction, and trace generator
│   │   │   └── graph.cpp                      # RoadGraph implementation, congestion updates, serialization
│   │   └── simulation/
│   │       ├── dispatch_service.cpp           # Multi-agency dispatch candidate evaluation & unit assignment
│   │       ├── incident_service.cpp           # Dual-indexed incident lifecycle management (AVL + Hash Table + Heap)
│   │       └── unit_service.cpp               # Fleet state transitions, movement, and capability checks
│   │
│   └── tests/                                 # C++ Unit & Integration Test Suites
│       ├── test_graph_dijkstra.cpp            # 15 tests: Dijkstra routing, road closures, SPT, tie-breaking
│       ├── test_incident_heap.cpp             # 28 tests: Max-Heap priority ordering, tie-breaking, lazy invalidation
│       ├── test_avl_hash.cpp                  # 36 tests: AVL rotations, BST search/delete, Hash collisions, dual-indexing
│       ├── test_dispatch_allocation.cpp       # 35 tests: Unit allocation, capability matching, travel time, state changes
│       └── test_api.ps1                       # PowerShell automated integration test against running HTTP server
│
├── frontend/                                  # Next.js 16 App Router Frontend (Port 3000)
│   ├── package.json                           # Frontend scripts & npm dependencies (Next.js, Tailwind, Lucide)
│   ├── tsconfig.json                          # TypeScript configuration with `@/*` path alias
│   ├── next.config.ts                         # Next.js build and proxy configuration
│   ├── tailwind.config.js                     # Tailwind CSS styling design tokens
│   ├── lint-check.js                          # Custom ESLint scanner script
│   │
│   └── src/
│       ├── app/
│       │   ├── layout.tsx                     # Root HTML layout with navigation Header and status banner
│       │   ├── page.tsx                       # Operations Mode view (`/`) - real-time simulation dashboard
│       │   ├── globals.css                    # Global CSS variables and utility classes
│       │   ├── dsa-lab/
│       │   │   └── page.tsx                   # DSA Lab view (`/dsa-lab`) - interactive algorithm visualizers
│       │   └── api/
│       │       ├── backend-status/route.ts    # Health check route probing C++ backend connectivity
│       │       └── proxy/[...path]/route.ts   # Reverse proxy streaming requests to backend `http://127.0.0.1:8080`
│       │
│       ├── components/
│       │   ├── common/
│       │   │   ├── Header.tsx                 # Top navigation bar with live backend status indicator & tab switching
│       │   │   ├── BackendStatusCard.tsx      # Telemetry card showing backend latency, tick, and engine health
│       │   │   └── SimulationNoticeBanner.tsx # Non-intrusive notification banner for status updates
│       │   │
│       │   ├── operations/                    # Operations Mode Dashboard Components
│       │   │   ├── OperationsDashboard.tsx    # Master controller coordinating map, fleet, incidents, dispatches
│       │   │   ├── CityMapCanvas.tsx          # Interactive SVG map of MetroGrid-16 (roads, units, routes, incidents)
│       │   │   ├── IncidentListPanel.tsx      # Priority-sorted queue and pending emergency list
│       │   │   ├── FleetUnitsPanel.tsx        # Emergency fleet unit cards with status badges and location indicators
│       │   │   ├── DispatchDetailsPanel.tsx   # Recommended unit card, shortest route preview, dispatch action
│       │   │   ├── SummaryMetricsCards.tsx    # High-level statistics: active dispatches, pending incidents, available units
│       │   │   ├── CreateIncidentModal.tsx    # Modal form to trigger new emergency incident
│       │   │   └── RoadControlsModal.tsx      # Modal to toggle road closures and adjust congestion factors
│       │   │
│       │   └── dsa-lab/                       # DSA Lab Algorithm Visualizer Components
│       │       ├── common/
│       │       │   ├── TracePlayerControls.tsx# Play/Pause, Step Next/Prev, Replay, Reset, and playback speed slider
│       │       │   └── ComplexityCard.tsx     # Time/space complexity card with viva reference notes
│       │       └── modules/
│       │           ├── DijkstraVisualizer.tsx # Shortest-Path Tree (SPT) & route visualizer with step trace playback
│       │           ├── HeapVisualizer.tsx     # Complete binary tree SVG layout with array index mapping & sift tracing
│       │           ├── AvlTreeVisualizer.tsx  # Dynamic AVL tree renderer demonstrating LL, RR, LR, RL balancing rotations
│       │           ├── HashTableVisualizer.tsx# Bucket array visualization showing chaining, collisions, and load factor
│       │           └── DispatchEngineVisualizer.tsx# Candidate unit evaluation pipeline (filter, score, shortest route)
│       │
│       └── lib/
│           ├── api.ts                         # Strongly typed TypeScript API client communicating with backend
│           ├── types.ts                       # TypeScript interfaces mirroring C++ structs and JSON envelopes
│           └── useTracePlayer.ts              # Custom React hook for synchronized, frame-accurate trace event playback
│
├── docs/                                      # Comprehensive Engineering & Viva Documentation
│   ├── ARCHITECTURE.md                        # High-level component interactions, data flows, and design rationale
│   ├── API_CONTRACT.md                        # Complete REST API specifications with request/response schemas
│   ├── DATA_STRUCTURES_AND_ALGORITHMS.md      # Deep dive into custom DSA implementations and mathematical analysis
│   ├── DEVELOPER_GUIDE.md                     # Step-by-step developer onboarding, codebase tour, and viva answers
│   ├── DECISIONS_AND_ASSUMPTIONS.md           # Engineering trade-offs, design choices, and assumptions
│   ├── DSA_CONCEPTS_AND_COMPLEXITY.md         # Big-O complexity tables, recurrence relations, and invariant proofs
│   ├── END_TO_END_WORKFLOW.md                 # Detailed incident-to-dispatch lifecycle narrative
│   ├── IMPLEMENTATION_PLAN.md                 # Phase breakdown and milestone verification checklist
│   ├── PROJECT_OVERVIEW.md                    # Problem statement, system boundaries, and scope
│   ├── SETUP_AND_RUNNING.md                   # Environment setup guide and compilation walkthrough
│   └── VIVA_PREPARATION.md                    # 30+ viva examination questions with model academic answers
│
└── scratch/                                   # Diagnostic & Automated Integration Scripts
    └── verify_dijkstra_spt.py                 # End-to-end Python test verifying C++ Dijkstra, SPT, and UI highlighting
```

---

## 3. The 5 Core Data Structures & Algorithms

Every core data structure in EmergencyGrid is implemented from scratch in modern C++20 without relying on external libraries (like `std::priority_queue` or `std::map` for core DSA demonstrations).

### 1. Road Graph & Dijkstra Shortest-Path Tree (SPT) Router
- **Files:** `backend/include/dsa/graph.hpp`, `backend/src/dsa/graph.cpp`, `backend/include/dsa/dijkstra.hpp`, `backend/src/dsa/dijkstra.cpp`
- **Topology:** Adjacency list representation using `std::unordered_map<std::string, std::vector<std::string>>` mapping intersection IDs to road IDs.
- **Cost Function:** Travel time in minutes calculated dynamically:
  $$\text{travelTimeMinutes} = \left(\frac{\text{baseLengthKm}}{\text{speedLimitKmh}}\right) \times \text{congestionFactor} \times 60.0$$
- **Algorithm:** Dijkstra with min-priority queue candidate extraction.
- **Shortest-Path Tree (SPT):**
  - Dijkstra computes the **Shortest-Path Tree (SPT)** rooted at the source node for all reachable nodes.
  - Predecessor pointers $\text{pred}[v] = (u, \text{roadId})$ are tracked during successful relaxations ($\text{tentativeDist} < \text{currentDist}$).
  - Guaranteed tree structure: $|\text{Reachable}(s)| - 1$ confirmed predecessor edges.
  - Deterministic tie-breaking for equal-cost paths compares parent node IDs, then road IDs.
- **Time Complexity:** $O((V + E) \log V)$ with binary heap min-priority queue.
- **Space Complexity:** $O(V + E)$ for adjacency list, distances, predecessors, and settled sets.

### 2. Custom Binary Max-Heap (Incident Priority Queue)
- **Files:** `backend/include/dsa/binary_heap.hpp`, `backend/src/dsa/binary_heap.cpp`
- **Purpose:** Prioritizes incoming emergency incidents based on severity and waiting time.
- **Formula:**
  $$\text{computedPriority} = (\text{severityLevel} \times 1000.0) + \max(0.0, (\text{currentTick} - \text{reportedTick}) \times 10.0)$$
- **Tie-Breaking:** If two incidents have identical priority scores, tie-breaking selects the smaller `arrivalSequence` (FIFO fairness).
- **Operations:**
  - `insert(incident)`: Appends to array and performs `siftUp()` in $O(\log N)$.
  - `extractMax()`: Swaps root with last element, pops back, and performs `siftDown()` in $O(\log N)$.
  - `peek()`: Returns root element in $O(1)$.
- **Lazy Invalidation:** Cancelled incidents buried deep in the heap are marked `CANCELLED` in the Hash Table/AVL Tree. When extracted at the root, the engine discards them lazily in $O(\log N)$ rather than doing an expensive $O(N)$ arbitrary heap deletion.

### 3. Custom AVL Tree (Self-Balancing Binary Search Tree)
- **Files:** `backend/include/dsa/avl_tree.hpp`
- **Purpose:** Maintains a dynamically sorted index of incidents ordered lexicographically by `id` (e.g., `INC-001`, `INC-002`, `INC-010`).
- **Invariants:**
  - Standard Binary Search Tree (BST) ordering property: $\text{Left} < \text{Node} < \text{Right}$.
  - AVL Balance Factor invariant: for every node $X$, $\text{BalanceFactor}(X) = \text{height}(\text{left}) - \text{height}(\text{right}) \in \{-1, 0, +1\}$.
- **Rotations Implemented:**
  1. **Left-Left (LL) Single Rotation:** Right rotation about the root.
  2. **Right-Right (RR) Single Rotation:** Left rotation about the root.
  3. **Left-Right (LR) Double Rotation:** Left rotation on left child, followed by right rotation on root.
  4. **Right-Left (RL) Double Rotation:** Right rotation on right child, followed by left rotation on root.
- **Operations:**
  - `insert(key, value)`: $O(\log N)$
  - `search(key)`: $O(\log N)$
  - `remove(key)`: $O(\log N)$
  - `inorderTraversal()`: Produces globally sorted incident records in $O(N)$.

### 4. Custom Hash Table (Separate Chaining)
- **Files:** `backend/include/dsa/hash_table.hpp`
- **Purpose:** Provides $O(1)$ average-time retrieval, status updates, and verification of incident records by `id`.
- **Hash Function:** Polynomial rolling hash with prime multiplier $p = 31$:
  $$\text{hash}(S) = \left(\sum_{i=0}^{L-1} S[i] \cdot 31^{L-1-i}\right) \pmod M$$
- **Collision Resolution:** Separate chaining using a singly linked bucket list (`HashNode<K, V>`).
- **Dynamic Resizing & Rehashing:**
  - Monitored load factor: $\alpha = \frac{N}{M}$.
  - When $\alpha > 0.75$, bucket capacity $M$ doubles ($M_{\text{new}} = 2M_{\text{old}}$), and all entries are re-hashed into the new table in $O(N)$ amortized time.

### 5. Multi-Agency Dispatch & Unit Allocation Engine
- **Files:** `backend/include/simulation/dispatch_service.hpp`, `backend/src/simulation/dispatch_service.cpp`
- **Lifecycle:**
  1. Dequeues highest-priority incident from Binary Max-Heap.
  2. Filters fleet units by agency type (`MEDICAL` $\rightarrow$ EMS, `FIRE` $\rightarrow$ Fire Rescue, `POLICE` $\rightarrow$ Police).
  3. Filters by capability requirements (e.g., `BLS`, `ALS`, `HEAVY_RESCUE`, `K9`).
  4. Computes Dijkstra shortest paths from every candidate unit's current intersection to the incident scene.
  5. Selects unit with minimal travel time (deterministic tie-breaking by `unitId`).
  6. Atomically marks unit `DISPATCHED` and incident `ASSIGNED`.

---

## 4. HTTP REST API Specifications

The C++ backend listens on port `8080`. All responses follow a standardized JSON envelope:
```json
{
  "success": true,
  "data": { ... },
  "trace": [ ... ],
  "meta": { "tick": 42, "executionTimeMs": 0.45 }
}
```

### Core API Endpoints:
| Method | Endpoint | Description |
|---|---|---|
| `GET` | `/api/v1/health` | Backend status, version, uptime, and tick |
| `GET` | `/api/v1/simulation/state` | Global metrics, active dispatches, fleet counts |
| `POST`| `/api/v1/simulation/reset` | Resets graph, incidents, and units to seed data |
| `GET` | `/api/v1/city/graph` | Returns all 16 intersections and 26 road segments |
| `POST`| `/api/v1/routing/calculate` | Computes Dijkstra shortest path, SPT edges, and trace |
| `PUT` | `/api/v1/city/roads/{id}` | Updates road status (`isClosed`) or congestion factor |
| `POST`| `/api/v1/incidents` | Creates incident, inserts into Heap, AVL, and Hash Table |
| `GET` | `/api/v1/incidents` | Lists all incidents from Hash Table index |
| `GET` | `/api/v1/incidents/sorted` | Returns alphabetically sorted incidents via AVL inorder traversal |
| `GET` | `/api/v1/incidents/next` | Peeks the highest priority pending incident from Max-Heap |
| `POST`| `/api/v1/incidents/next/select` | Pops highest priority incident from Max-Heap |
| `POST`| `/api/v1/incidents/{id}/cancel` | Cancels incident in AVL & Hash Table (lazy Heap purge) |
| `GET` | `/api/v1/units` | Lists emergency units with optional agency filtering |
| `POST`| `/api/v1/dispatch/recommend` | Evaluates candidates and recommends optimal unit + route |
| `POST`| `/api/v1/dispatch/assign` | Executes formal unit dispatch and updates statuses |
| `POST`| `/api/v1/dispatch/{id}/complete`| Resolves incident and releases unit to available status |
| `POST`| `/api/v1/dsa/heap/demo` | Interactive sandbox for Binary Max-Heap tracing |
| `POST`| `/api/v1/dsa/avl/demo` | Interactive sandbox for AVL Tree rotation tracing |
| `POST`| `/api/v1/dsa/hash-table/demo` | Interactive sandbox for Hash Table collision tracing |
| `POST`| `/api/v1/dsa/dispatch/demo` | Interactive sandbox for Dispatch allocation tracing |

---

## 5. Educational Trace Telemetry System

Both the C++ backend and the Next.js frontend share a unified trace contract defined in `trace.hpp` and `types.ts`:

```cpp
struct TraceEvent {
    uint32_t step{0};
    std::string algorithm;      // "Dijkstra", "BinaryHeap", "AVLTree", "HashTable", "Dispatch"
    std::string operation;      // e.g., "relax_edge", "sift_up", "rotate_left", "rehash"
    std::string entityId;       // Node ID, Key, or Road ID
    std::string from;           // Origin node / predecessor
    std::string to;             // Target node / relaxed neighbor
    std::optional<double> cost; // Tentative distance or priority score
    nlohmann::json metadata;    // Snapshot details, road IDs, balance factors
    std::string explanation;    // Plain-English explanation of why this step happened
};
```

In the frontend, `useTracePlayer.ts` consumes this event stream to drive visual step-by-step playback, scrubber navigation, variable playback speeds (0.5x, 1x, 2x), and automatic state reconstruction.

---

## 6. Key Viva Examination Q&A Cheatsheet

### Q1: Why does Dijkstra compute a Shortest-Path Tree (SPT) and not a Minimum Spanning Tree (MST)?
> **Answer:**  
> Dijkstra minimizes the cumulative path distance from a single source $s$ to every reachable vertex: $D(v) = \min \text{dist}(s, v)$. The resulting set of predecessor edges forms an SPT.  
> An MST (computed by Prim's or Kruskal's algorithm) minimizes the total sum of all edge weights across the entire connected graph without regard to distance from a single root. An SPT edge weight sum can be strictly larger than an MST edge weight sum.

### Q2: Why use a dual index (AVL Tree + Hash Table) for incidents?
> **Answer:**  
> - The **Hash Table** provides $O(1)$ average-time lookup by incident ID, which is critical when dispatching units or checking status during high-frequency simulation ticks.  
> - The **AVL Tree** maintains entries in strict sorted order, allowing $O(N)$ inorder sequential traversal for pagination and range queries in $O(\log N + K)$, which hash tables cannot do without a separate sort step costing $O(N \log N)$.

### Q3: Why AVL Tree instead of Red-Black Tree?
> **Answer:**  
> AVL trees have stricter balancing criteria ($\Delta h \le 1$ vs Red-Black's path length ratio $\le 2$). This guarantees a strictly smaller maximum height ($\approx 1.44 \log_2 N$ vs $2 \log_2 N$), making AVL lookups faster. For an incident registry with frequent lookups and moderate insertions, AVL delivers superior search performance.

### Q4: How does the Binary Max-Heap handle cancelled incidents?
> **Answer:**  
> We use **lazy invalidation**. Finding and deleting an arbitrary element inside a binary heap takes $O(N)$ time because heaps only guarantee order between parent and child, not between siblings. Instead, when an incident is cancelled, we update its status in the Hash Table in $O(1)$. When the heap root is later extracted via `extractMax()`, we verify its status. If marked cancelled or already assigned, it is discarded in $O(\log N)$ without corrupting the queue.

### Q5: How are equal-cost shortest paths handled in Dijkstra?
> **Answer:**  
> Deterministic tie-breaking is enforced during edge relaxation. If $\text{tentativeDist} == \text{currentDist}$, we break ties by comparing parent node IDs lexicographically, and if equal, comparing road IDs. This guarantees identical route reconstruction across runs regardless of hash map traversal order.
