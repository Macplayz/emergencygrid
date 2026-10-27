# EmergencyGrid — Developer Handbook & Codebase Walkthrough

Welcome to the **EmergencyGrid** developer handbook. This document provides a file-by-file walkthrough of the entire codebase, explaining how the C++20 simulation core and the Next.js App Router frontend collaborate to deliver real-time emergency dispatch and interactive data structures education.

---

## 1. Project Directory Tree

Below is the clean directory structure of the repository, highlighting files essential for understanding, debugging, and explaining the system during a college viva:

```text
EmergencyGrid/
├── backend/                                # ISO C++20 Simulation Core
│   ├── CMakeLists.txt                      # Build configuration (GCC, Clang, MSVC)
│   ├── include/
│   │   ├── api/
│   │   │   └── server.hpp                  # HTTP server declaration & endpoint signatures
│   │   ├── core/
│   │   │   ├── trace.hpp                   # Algorithmic telemetry TraceCollector & TraceEvent
│   │   │   └── types.hpp                   # Domain models (Incident, Unit, RoadSegment, Statuses)
│   │   ├── dsa/
│   │   │   ├── avl_tree.hpp                # Custom self-balancing AVL Tree template
│   │   │   ├── binary_heap.hpp             # Custom array-backed Binary Max-Heap declaration
│   │   │   ├── dijkstra.hpp                # Single-source shortest path routing interface
│   │   │   ├── graph.hpp                   # Road network topology & Adjacency List interface
│   │   │   └── hash_table.hpp              # Custom 64-bit FNV-1a Hash Table with chaining
│   │   ├── external/
│   │   │   ├── httplib.h                   # Single-header HTTP server (cpp-httplib)
│   │   │   └── json.hpp                    # Single-header JSON parsing (nlohmann/json)
│   │   └── simulation/
│   │       ├── dispatch_service.hpp        # Candidate discovery & multi-agency selection
│   │       ├── incident_service.hpp        # Multi-index triage service (Heap + AVL + Hash)
│   │       └── unit_service.hpp            # Fleet state management & capability validation
│   ├── src/
│   │   ├── api/
│   │   │   └── server.cpp                  # REST API implementation & isolated DSA sandboxes
│   │   ├── dsa/
│   │   │   ├── binary_heap.cpp             # Heapify-up, heapify-down, multi-key comparator
│   │   │   ├── dijkstra.cpp                # Dijkstra routing, edge relaxation, trace recording
│   │   │   └── graph.cpp                   # Adjacency list storage & MetroGrid-16 seed generator
│   │   ├── simulation/
│   │   │   ├── dispatch_service.cpp        # Dispatch pipeline & active route recalculation
│   │   │   ├── incident_service.cpp        # Atomic synchronization across Heap, AVL, and Hash
│   │   │   └── unit_service.cpp            # Default fleet seed & unit state transitions
│   │   └── main.cpp                        # CLI entry point, signal handler, and server startup
│   └── tests/
│       ├── test_api.ps1                    # 38-step live PowerShell HTTP integration suite
│       ├── test_avl_hash.cpp               # 36 tests: rotations, FNV-1a, collision, rehashing
│       ├── test_dispatch_allocation.cpp    # 35 tests: candidate eligibility, routing, lifecycles
│       ├── test_graph_dijkstra.cpp         # 14 tests: graph topology, edge weights, rerouting
│       └── test_incident_heap.cpp          # 28 tests: triage invariant, sift traces, stale purges
│
├── frontend/                               # Next.js 16 (App Router) + TypeScript + Tailwind CSS
│   ├── package.json                        # Dependencies, build scripts, ESLint configuration
│   ├── tsconfig.json                       # Strict TypeScript type-checking configuration
│   ├── lint-check.js                       # Programmatic ESLint scanner script
│   ├── test-dsa-lab.mjs                    # Automated DSA Lab integration & isolation test suite
│   └── src/
│       ├── app/
│       │   ├── layout.tsx                  # Global shell, dark theme, and typography
│       │   ├── page.tsx                    # Operations Mode dashboard route (/)
│       │   ├── dsa-lab/
│       │   │   └── page.tsx                # DSA Lab interactive educational route (/dsa-lab)
│       │   └── api/
│       │       ├── backend-status/route.ts # Health check proxy endpoint
│       │       └── proxy/[...path]/route.ts # Universal proxy forwarding requests to port 8080
│       ├── components/
│       │   ├── common/
│       │   │   ├── BackendStatusCard.tsx   # Visual indicator of C++ backend health
│       │   │   ├── Header.tsx              # Navigation bar with mode switcher and live status
│       │   │   └── SimulationNoticeBanner.tsx # Academic disclaimer banner
│       │   ├── operations/
│       │   │   ├── CityMapCanvas.tsx       # Interactive Cartesian SVG city road network
│       │   │   ├── CreateIncidentModal.tsx # Dialog to report emergency incidents
│       │   │   ├── DispatchDetailsPanel.tsx# Active dispatch telemetry & release unit actions
│       │   │   ├── FleetUnitsPanel.tsx     # Responder readiness & capability overview
│       │   │   ├── IncidentListPanel.tsx   # Triaged emergency queue & filter console
│       │   │   ├── OperationsDashboard.tsx # Unified container managing real-time polling
│       │   │   ├── RoadControlsModal.tsx   # Road closure & congestion multiplier controls
│       │   │   └── SummaryMetricsCards.tsx # Key performance indicator metrics strip
│       │   └── dsa-lab/
│       │       ├── common/
│       │       │   ├── ComplexityCard.tsx  # Viva reference guide & Big-O complexity tables
│       │       │   └── TracePlayerControls.tsx # Scrubbing, speed, step forward/backward player
│       │       └── modules/
│       │           ├── AvlTreeVisualizer.tsx       # Recursive SVG AVL tree with LL/RR/LR/RL demos
│       │           ├── DijkstraVisualizer.tsx      # Step-by-step road relaxation animator
│       │           ├── DispatchEngineVisualizer.tsx# Candidate eligibility & routing visualizer
│       │           ├── HashTableVisualizer.tsx     # FNV-1a hash bucket array & separate chains
│       │           └── HeapVisualizer.tsx          # Dual tree/array buffer binary max-heap view
│       └── lib/
│           ├── api.ts                      # Strongly typed frontend API client
│           ├── types.ts                    # Shared domain interfaces matching C++ models
│           └── useTracePlayer.ts           # Reusable state machine hook for trace navigation
│
└── docs/                                   # Architectural, API & Academic Documentation
    ├── API_CONTRACT.md                     # Complete REST API schemas & trace format
    ├── ARCHITECTURE.md                     # System architecture & component relationships
    ├── DATA_STRUCTURES_AND_ALGORITHMS.md   # Algorithmic deep dive & theoretical proofs
    ├── DECISIONS_AND_ASSUMPTIONS.md        # Architectural Decision Records (ADRs)
    ├── DEVELOPER_GUIDE.md                  # This file: codebase orientation & walkthrough
    ├── END_TO_END_WORKFLOW.md              # 10-step full simulation lifecycle & data flow
    ├── IMPLEMENTATION_PLAN.md              # 6-phase vertical-slice roadmap
    ├── PROJECT_OVERVIEW.md                 # Problem statement & academic objectives
    ├── SETUP_AND_RUNNING.md                # Build, run, and test commands for Windows PowerShell
    └── VIVA_PREPARATION.md                 # 35+ likely viva questions, answers, and demo checklist
```

---

## 2. Directory Responsibilities

### `backend/` — The C++20 Simulation Core
The backend is an autonomous, high-performance simulation engine written in ISO C++20. It has **no external database** (PostgreSQL, SQLite, Redis) and **no third-party heavy frameworks**. All storage, triage, balancing, collision resolution, and routing logic are written using foundational computer science data structures.

- **`backend/include/dsa/` and `backend/src/dsa/`**: Houses the custom data structures: Graph, Dijkstra, Binary Max-Heap, AVL Tree, and Hash Table.
- **`backend/include/simulation/` and `backend/src/simulation/`**: Houses domain services that coordinate the data structures: `IncidentService`, `UnitService`, and `DispatchService`.
- **`backend/include/api/` and `backend/src/api/`**: Houses the HTTP REST server built on `cpp-httplib`, handling JSON deserialization, input validation, and telemetry envelopes.
- **`backend/tests/`**: Contains 4 C++ unit test executables (113 total test cases) and a 38-step PowerShell end-to-end integration test suite.

### `frontend/` — The Next.js Web Client
The frontend is built using Next.js 16 (App Router), TypeScript, and Tailwind CSS. It communicates with the backend via a reverse-proxy server route and serves two distinct operational modes:
1. **Operations Mode (`/`)**: A dark-themed command-and-control center for simulating city emergency response in real time.
2. **DSA Lab (`/dsa-lab`)**: An educational laboratory providing animated, step-by-step playback of the actual C++ algorithmic execution traces.

### `docs/` — Academic & Architectural Documentation
Detailed markdown documents covering system design, API contracts, algorithmic complexity analysis, setup guides, end-to-end workflows, and viva preparation.

---

## 3. C++ Source Files & Data Structure Mapping

This section maps each theoretical concept to its concrete C++ implementation:

| Concept / Algorithm | C++ Header File | C++ Source File | Primary Responsibility |
| :--- | :--- | :--- | :--- |
| **City Graph (Adjacency List)** | [`backend/include/dsa/graph.hpp`](file:///b:/ADS%20Project/backend/include/dsa/graph.hpp) | [`backend/src/dsa/graph.cpp`](file:///b:/ADS%20Project/backend/src/dsa/graph.cpp) | Stores city topology using `std::unordered_map<std::string, std::vector<RoadSegment>>`. Handles road closures and traffic congestion multipliers. |
| **Dijkstra's Shortest Path** | [`backend/include/dsa/dijkstra.hpp`](file:///b:/ADS%20Project/backend/include/dsa/dijkstra.hpp) | [`backend/src/dsa/dijkstra.cpp`](file:///b:/ADS%20Project/backend/src/dsa/dijkstra.cpp) | Computes single-source shortest path travel times using a binary min-heap priority queue. Emits detailed edge relaxation trace events. |
| **Incident Priority Max-Heap** | [`backend/include/dsa/binary_heap.hpp`](file:///b:/ADS%20Project/backend/include/dsa/binary_heap.hpp) | [`backend/src/dsa/binary_heap.cpp`](file:///b:/ADS%20Project/backend/src/dsa/binary_heap.cpp) | Custom array-backed (`std::vector<Incident>`) binary max-heap implementing `heapifyUp` and `heapifyDown` with multi-tier deterministic triage. |
| **Incident Records AVL Tree** | [`backend/include/dsa/avl_tree.hpp`](file:///b:/ADS%20Project/backend/include/dsa/avl_tree.hpp) | Header-only generic template | Pointer-based self-balancing BST using `std::unique_ptr`. Maintains $|BF| \le 1$ via Single (LL, RR) and Double (LR, RL) rotations. |
| **Direct Lookup Hash Table** | [`backend/include/dsa/hash_table.hpp`](file:///b:/ADS%20Project/backend/include/dsa/hash_table.hpp) | Header-only generic template | 64-bit FNV-1a hashing with separate-chaining linked collision lists. Implements dynamic doubling rehash when load factor $\alpha \ge 0.75$. |
| **Multi-Index Incident Service** | [`backend/include/simulation/incident_service.hpp`](file:///b:/ADS%20Project/backend/include/simulation/incident_service.hpp) | [`backend/src/simulation/incident_service.cpp`](file:///b:/ADS%20Project/backend/src/simulation/incident_service.cpp) | Atomically coordinates the three indexes: Heap for priority, Hash Table for $O(1)$ ID lookup, and AVL Tree for sorted chronological listings. |
| **Fleet Readiness Service** | [`backend/include/simulation/unit_service.hpp`](file:///b:/ADS%20Project/backend/include/simulation/unit_service.hpp) | [`backend/src/simulation/unit_service.cpp`](file:///b:/ADS%20Project/backend/src/simulation/unit_service.cpp) | Validates responder state transitions, tracks station locations, and filters units by capability (ALS, BLS, HAZMAT, etc.). |
| **Dispatch Decision Engine** | [`backend/include/simulation/dispatch_service.hpp`](file:///b:/ADS%20Project/backend/include/simulation/dispatch_service.hpp) | [`backend/src/simulation/dispatch_service.cpp`](file:///b:/ADS%20Project/backend/src/simulation/dispatch_service.cpp) | Filters eligible fleet units, calculates Dijkstra travel times, and assigns the quickest available responder. |
| **REST HTTP Server** | [`backend/include/api/server.hpp`](file:///b:/ADS%20Project/backend/include/api/server.hpp) | [`backend/src/api/server.cpp`](file:///b:/ADS%20Project/backend/src/api/server.cpp) | Implements REST endpoints, handles JSON requests, and provisions isolated DSA demo sandboxes. |

---

## 4. Frontend Architecture & API Proxy

### Where the Next.js Frontend Connects to C++
To prevent Cross-Origin Resource Sharing (CORS) complications and avoid hardcoding internal backend URLs into client JavaScript bundles, the frontend routes all requests through a server-side API proxy:

- **Proxy Route Handler**: [`frontend/src/app/api/proxy/[...path]/route.ts`](file:///b:/ADS%20Project/frontend/src/app/api/proxy/[...path]/route.ts)
  - Intercepts requests matching `/api/proxy/*`.
  - Forwards the request to `http://127.0.0.1:8080/*` (or `process.env.BACKEND_URL`).
  - Supports `GET`, `POST`, `PUT`, `DELETE` methods, copies headers, and streams JSON response envelopes back to the client.
- **Frontend API Client**: [`frontend/src/lib/api.ts`](file:///b:/ADS%20Project/frontend/src/lib/api.ts)
  - Exports a typed `api` object exposing methods like `api.getCityGraph()`, `api.createIncident()`, `api.dispatchWithTrace()`, and `api.runHeapDemo()`.
  - Automatically prepends `/api/proxy` to all endpoint URLs.

### Frontend Components Breakdown

#### Operations Mode Components (`frontend/src/components/operations/`)
- [`OperationsDashboard.tsx`](file:///b:/ADS%20Project/frontend/src/components/operations/OperationsDashboard.tsx): Top-level controller managing initial data fetching, auto-refresh intervals, and modal state.
- [`CityMapCanvas.tsx`](file:///b:/ADS%20Project/frontend/src/components/operations/CityMapCanvas.tsx): Custom SVG renderer that plots MetroGrid-16 intersections, weighted road segments (with red dashed lines for closures and amber for congestion), vehicle markers, and glowing green/cyan route polylines.
- [`IncidentListPanel.tsx`](file:///b:/ADS%20Project/frontend/src/components/operations/IncidentListPanel.tsx): Renders the prioritized emergency queue, filter pills (Medical, Fire, Police), and the **"Dispatch Next"** action button.
- [`FleetUnitsPanel.tsx`](file:///b:/ADS%20Project/frontend/src/components/operations/FleetUnitsPanel.tsx): Shows emergency unit telemetry (call sign, agency, status, capabilities, and stationed intersection).
- [`DispatchDetailsPanel.tsx`](file:///b:/ADS%20Project/frontend/src/components/operations/DispatchDetailsPanel.tsx): Displays assigned vehicle route details, Dijkstra statistics (nodes settled, edge relaxations), and the **"Complete Dispatch & Release Unit"** button.
- [`CreateIncidentModal.tsx`](file:///b:/ADS%20Project/frontend/src/components/operations/CreateIncidentModal.tsx): Dialog to submit simulated emergency calls with customizable severity (1 to 4), type, location, and required capabilities.
- [`RoadControlsModal.tsx`](file:///b:/ADS%20Project/frontend/src/components/operations/RoadControlsModal.tsx): Allows operators to dynamically close roads or adjust congestion multipliers ($1.0\times\text{--}5.0\times$) to test immediate rerouting.

#### DSA Lab Educational Modules (`frontend/src/components/dsa-lab/`)
- [`DijkstraVisualizer.tsx`](file:///b:/ADS%20Project/frontend/src/components/dsa-lab/modules/DijkstraVisualizer.tsx): Interactive source/destination selector, live tentative distance table ($d[v]$), and step-by-step edge relaxation animation.
- [`HeapVisualizer.tsx`](file:///b:/ADS%20Project/frontend/src/components/dsa-lab/modules/HeapVisualizer.tsx): Dual visualization: SVG binary tree and flat contiguous array buffer displaying parent/child indices and sift-up/down comparisons and swaps.
- [`AvlTreeVisualizer.tsx`](file:///b:/ADS%20Project/frontend/src/components/dsa-lab/modules/AvlTreeVisualizer.tsx): Renders hierarchical tree with node heights and balance factors, featuring interactive triggers for LL, RR, LR, and RL rotations.
- [`HashTableVisualizer.tsx`](file:///b:/ADS%20Project/frontend/src/components/dsa-lab/modules/HashTableVisualizer.tsx): Renders bucket array grid, separate-chaining linked nodes, FNV-1a hash calculation breakdown, and dynamic doubling rehash ($16 \to 32$).
- [`DispatchEngineVisualizer.tsx`](file:///b:/ADS%20Project/frontend/src/components/dsa-lab/modules/DispatchEngineVisualizer.tsx): Visualizes the multi-step dispatch decision pipeline: Max-Heap triage $\to$ capability filtering $\to$ Dijkstra route evaluations $\to$ optimal unit selection.
- [`TracePlayerControls.tsx`](file:///b:/ADS%20Project/frontend/src/components/dsa-lab/common/TracePlayerControls.tsx): Common playback bar with Play/Pause, Step Next/Prev, Reset, Replay, scrubbing slider, and speed toggles ($0.5\times\text{--}4\times$).
- [`ComplexityCard.tsx`](file:///b:/ADS%20Project/frontend/src/components/dsa-lab/common/ComplexityCard.tsx): Standardized technical card embedded in each module displaying time/space Big-O tables, invariants, and source links.

---

## 5. Seed Data & Initial Configuration

Where is the initial simulation state defined?

1. **MetroGrid-16 Road Network**:
   - Defined in [`backend/src/dsa/graph.cpp`](file:///b:/ADS%20Project/backend/src/dsa/graph.cpp) in `createMetroGrid16()`.
   - Provisions 16 intersections (`J1` to `J16`) configured as a $4 \times 4$ urban grid with 26 bidirectional road segments (`R1_2`, `R1_4`, `R2_3`, etc.).
   - Each road segment has a base length in km, speed limit in km/h, and base travel time in minutes.

2. **Default Emergency Fleet**:
   - Defined in [`backend/src/simulation/unit_service.cpp`](file:///b:/ADS%20Project/backend/src/simulation/unit_service.cpp) in `createDefaultUnits()`.
   - Provisions 9 emergency vehicles across 3 agencies:
     - **Medical (3)**: `AMB-01` (Medic 1 @ J13), `AMB-02` (Medic 2 @ J8), `AMB-03` (Medic 3 @ J4).
     - **Fire & Rescue (3)**: `ENG-01` (Engine 1 @ J1), `ENG-02` (Engine 2 @ J2), `TRK-01` (Ladder 1 @ J9).
     - **Police (3)**: `POL-01` (Patrol 1 @ J1), `POL-02` (Patrol 2 @ J5), `POL-03` (Patrol 3 @ J12).

3. **Simulation Reset Function**:
   - Defined in [`backend/src/api/server.cpp`](file:///b:/ADS%20Project/backend/src/api/server.cpp) in `HttpServer::resetGraph()`.
   - Invoked via `POST /api/v1/city/reset` to reset the entire city to its clean initial state without restarting the server binary.

---

## 6. Where Tests Live & What They Verify

| Test File | Type | Target | What It Verifies |
| :--- | :--- | :--- | :--- |
| [`test_graph_dijkstra.cpp`](file:///b:/ADS%20Project/backend/tests/test_graph_dijkstra.cpp) | C++ Unit Test (14 tests) | `Graph`, `DijkstraRouter` | Shortest path correctness, congestion rerouting, closed road avoidance, negative weight rejection, deterministic tie-breaking. |
| [`test_incident_heap.cpp`](file:///b:/ADS%20Project/backend/tests/test_incident_heap.cpp) | C++ Unit Test (28 tests) | `BinaryHeap`, `IncidentService` | Heap invariant, severity priority (4 > 3 > 2 > 1), FIFO arrival ordering, deterministic ID tie-breaking, lazy invalidation of cancelled incidents. |
| [`test_avl_hash.cpp`](file:///b:/ADS%20Project/backend/tests/test_avl_hash.cpp) | C++ Unit Test (36 tests) | `AVLTree`, `HashTable` | LL/RR/LR/RL tree rotations, height/balance factor checks, inorder sorting, FNV-1a distribution, collision chaining, automatic doubling rehash ($\ge 0.75$). |
| [`test_dispatch_allocation.cpp`](file:///b:/ADS%20Project/backend/tests/test_dispatch_allocation.cpp) | C++ Unit Test (35 tests) | `DispatchService`, `UnitService` | Candidate filtering (agency & capability match), travel-time based unit selection, unit state transitions, completion release, route recalculation. |
| [`test_api.ps1`](file:///b:/ADS%20Project/backend/tests/test_api.ps1) | PowerShell Integration (38 tests) | Live HTTP Server | Verifies all 38 REST endpoints against a running backend instance on port 8080. |
| [`test-dsa-lab.mjs`](file:///b:/ADS%20Project/frontend/test-dsa-lab.mjs) | Node.js Integration (8 tests) | DSA Lab Endpoints | Verifies Dijkstra traces, Heap sandbox, AVL sandbox, Hash Table sandbox, and proves that lab demos never corrupt live Operations Mode state. |

---

## 7. Recommended Reading Order for Students

When preparing to understand or present this project, read the files in this sequence:

1. **Architecture & Scope**:
   - Read [`docs/PROJECT_OVERVIEW.md`](file:///b:/ADS%20Project/docs/PROJECT_OVERVIEW.md) to understand the emergency response simulation context.
   - Read [`docs/ARCHITECTURE.md`](file:///b:/ADS%20Project/docs/ARCHITECTURE.md) to understand the decoupling between the C++ engine and Next.js client.

2. **Core Data Structures & Algorithms**:
   - Read [`backend/include/dsa/binary_heap.hpp`](file:///b:/ADS%20Project/backend/include/dsa/binary_heap.hpp) and [`backend/src/dsa/binary_heap.cpp`](file:///b:/ADS%20Project/backend/src/dsa/binary_heap.cpp) to understand the multi-tier priority queue.
   - Read [`backend/include/dsa/avl_tree.hpp`](file:///b:/ADS%20Project/backend/include/dsa/avl_tree.hpp) to see how pointer-based self-balancing binary search trees work.
   - Read [`backend/include/dsa/hash_table.hpp`](file:///b:/ADS%20Project/backend/include/dsa/hash_table.hpp) to understand FNV-1a hashing, separate chaining, and doubling rehash.
   - Read [`backend/src/dsa/dijkstra.cpp`](file:///b:/ADS%20Project/backend/src/dsa/dijkstra.cpp) to inspect edge relaxation and min-heap extraction.

3. **Domain Coordination & Dispatch**:
   - Read [`backend/src/simulation/incident_service.cpp`](file:///b:/ADS%20Project/backend/src/simulation/incident_service.cpp) to see how the three data structures are synchronized atomically.
   - Read [`backend/src/simulation/dispatch_service.cpp`](file:///b:/ADS%20Project/backend/src/simulation/dispatch_service.cpp) to see the 4-stage dispatch decision pipeline.

4. **Frontend Integration & Trace Replay**:
   - Read [`frontend/src/lib/useTracePlayer.ts`](file:///b:/ADS%20Project/frontend/src/lib/useTracePlayer.ts) to understand trace event playback.
   - Read [`frontend/src/app/dsa-lab/page.tsx`](file:///b:/ADS%20Project/frontend/src/app/dsa-lab/page.tsx) to inspect how the educational lab is structured.
   - Read [`docs/VIVA_PREPARATION.md`](file:///b:/ADS%20Project/docs/VIVA_PREPARATION.md) to practice answering common viva questions.
