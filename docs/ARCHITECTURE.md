# EmergencyGrid — Architecture Blueprint

## 1. High-Level System Architecture

EmergencyGrid follows a **decoupled client-engine architecture** with a strict separation of concerns:
- **Presentation & Interaction Layer (Frontend)**: A Next.js (TypeScript + Tailwind CSS) single-page application handling command-and-control operations, interactive graph canvas rendering, state synchronization, and step-by-step DSA visualization playback.
- **Computation & Simulation Core (Backend)**: A high-performance, single-process C++20 engine that holds simulation state, executes manual data structure operations, calculates optimal dispatches, and streams detailed execution traces via a lightweight HTTP JSON API.

```mermaid
flowchart TB
    subgraph Frontend["Next.js Web Client (Localhost:3000)"]
        direction TB
        subgraph OperationsMode["Mode A: Operations Console"]
            Dashboard[Incident & Fleet KPI Dashboard]
            CityMap[Interactive City Graph Canvas]
            DispatchPanel[Incident Creator & Unit Dispatcher]
            RoadControls[Road Closure & Traffic Controls]
        end

        subgraph DSALab["Mode B: DSA Lab"]
            TracePlayer[Playback Controller (Step / Play / Pause)]
            GraphVisualizer[Dijkstra Pathfinding Visualizer]
            HeapVisualizer[Binary Heap Priority Queue Visualizer]
            AVLVisualizer[AVL Tree Index Visualizer]
            HashVisualizer[Hash Table Bucket Visualizer]
            ComplexityMetrics[Operation Counters & Big-O Notes]
        end

        StateClient[Frontend State Cache & API Client]
    end

    subgraph Backend["C++20 Simulation Core (Localhost:8080)"]
        direction TB
        HttpServer["Lightweight HTTP API Server (Crow / httplib)"]
        SimulationManager[Simulation State Manager]

        subgraph CoreEngine["Custom Data Structures & Algorithms Core"]
            GraphEngine["AdjacencyListGraph & DijkstraRouter"]
            PriorityEngine["BinaryHeap (Incident Priority Queue)"]
            AVLIndex["AVLTree (Incident Log by Timestamp/Severity)"]
            EntityHash["HashTable (Entity Lookups by ID)"]
            TraceCollector[Trace Event Collector / Logger]
        end
    end

    OperationsMode --> StateClient
    DSALab --> StateClient
    StateClient <===>|HTTP / JSON (REST)| HttpServer
    HttpServer --> SimulationManager
    SimulationManager --> CoreEngine
    CoreEngine --> TraceCollector
    TraceCollector -.->|Structured Traces in Response| HttpServer
```

---

## 2. Layer Responsibilities & System Boundaries

| System Layer | Primary Responsibilities | Strict Non-Responsibilities |
| :--- | :--- | :--- |
| **Next.js Frontend** | • Render UI, City Map, and DSA Visualizers<br>• Manage user interaction and animation timers<br>• Store recent trace batches in memory for playback<br>• Send user commands (add incident, close road, dispatch) | • **Never calculate routes or shortest paths**<br>• **Never rebalance trees or maintain client-side heaps**<br>• **Never author dispatch logic or decide unit assignment** |
| **C++ Engine** | • Maintain single authoritative simulation state<br>• Execute manual algorithms (Dijkstra, Heap, AVL, Hash)<br>• Enforce business rules (unit availability, road closures)<br>• Collect step-by-step trace events during execution | • **No UI rendering or HTML generation**<br>• **No persistent disk database (in-memory state with seed reload)**<br>• **No multi-client session management** |
| **HTTP API Interface** | • Validate request payload structure and IDs<br>• Serialize/deserialize C++ objects to JSON<br>• Provide predictable HTTP error status codes | • No business logic execution outside routing to engine |

---

## 3. Core Domain Entities & Relationships

```mermaid
erDiagram
    LOCATION ||--o{ ROAD_SEGMENT : "originates / terminates"
    LOCATION ||--o{ INCIDENT : "occurs at"
    LOCATION ||--o{ EMERGENCY_UNIT : "stationed / located at"
    
    INCIDENT ||--o| DISPATCH_ASSIGNMENT : "triggers"
    EMERGENCY_UNIT ||--o| DISPATCH_ASSIGNMENT : "allocated to"

    LOCATION {
        string id PK "e.g., J1, J2"
        string name "e.g., Central Square"
        float x_coord "Canvas coordinate X"
        float y_coord "Canvas coordinate Y"
        string zone "Commercial / Residential / Industrial"
    }

    ROAD_SEGMENT {
        string id PK "e.g., R1_2"
        string source_id FK "References LOCATION"
        string target_id FK "References LOCATION"
        float base_length_km "Physical distance"
        float speed_limit_kmh "Design speed"
        float congestion_factor "Multiplier (1.0 to 5.0)"
        boolean is_closed "True if impassable"
        boolean is_bidirectional "True if two-way road"
    }

    INCIDENT {
        string id PK "e.g., INC-101"
        string location_id FK "References LOCATION"
        enum service_type "MEDICAL | FIRE | POLICE"
        int severity "1 (Low) to 4 (Critical)"
        int report_timestamp "Simulation tick / arrival sequence"
        enum status "PENDING | ASSIGNED | DISPATCHED | ON_SCENE | RESOLVED | CANCELLED"
        float computed_priority "Calculated heap key"
    }

    EMERGENCY_UNIT {
        string id PK "e.g., AMB-01, ENG-02, POL-03"
        string call_sign "Medic One"
        enum agency "EMS | FIRE | POLICE"
        string home_station_id FK "Base location"
        string current_location_id FK "Current location"
        enum status "AVAILABLE | DISPATCHED | BUSY | OUT_OF_SERVICE"
        string assigned_incident_id FK "Nullable reference"
    }

    DISPATCH_ASSIGNMENT {
        string incident_id PK, FK
        string unit_id PK, FK
        float estimated_travel_time_min "Dijkstra total weight"
        string route_path "Ordered node list: J1->J4->J7"
        int dispatch_timestamp "Tick when assigned"
        string decision_rationale "Plain-English explanation"
    }
```

---

## 4. Entity Lifecycle State Machines

### 4.1 Incident Lifecycle
```mermaid
stateDiagram-v2
    [*] --> PENDING: Created (Inserted into Priority Heap & AVL)
    PENDING --> DISPATCHED: Dispatch Unit Command (Popped from Heap, Unit Assigned)
    DISPATCHED --> ON_SCENE: Unit Arrives at Node
    ON_SCENE --> RESOLVED: Incident Completed (Unit Released)
    PENDING --> CANCELLED: Operator Cancels Call
    RESOLVED --> [*]
    CANCELLED --> [*]
```

### 4.2 Emergency Unit Lifecycle
```mermaid
stateDiagram-v2
    [*] --> AVAILABLE: Initialized at Station
    AVAILABLE --> DISPATCHED: Assigned to Incident
    DISPATCHED --> BUSY: Unit Working On-Scene
    BUSY --> AVAILABLE: Incident Marked Resolved (Returns to Base or Stays at Node)
    AVAILABLE --> OUT_OF_SERVICE: Maintenance / Offline
    OUT_OF_SERVICE --> AVAILABLE: Back in Service
```

---

## 5. Authoritative Simulation State Design

All mutable state lives inside a single thread-safe C++ class: `SimulationState`.

```cpp
class SimulationState {
public:
    // Core custom data structures
    AdjacencyListGraph cityGraph;               // Graph topology & live edge weights
    BinaryHeap<IncidentPriorityNode> pendingHeap;// Max-heap prioritizing pending emergencies
    AVLTree<string, IncidentRecord> incidentHistoryTree; // Balanced tree ordered by ID/timestamp
    HashTable<string, EmergencyUnit> unitTable;  // Fast O(1) lookup for responder units
    HashTable<string, IncidentRecord> activeIncidentTable; // Fast O(1) lookup for active calls

    // Global simulation metrics
    uint64_t currentSimulationTick{0};
    uint32_t totalIncidentsProcessed{0};
    uint32_t totalDispatchesCompleted{0};

    // State mutations (mutators emit trace events)
    Incident createIncident(const IncidentCreateRequest& req, TraceCollector& trace);
    DispatchResult dispatchUnitForIncident(const std::string& incidentId, TraceCollector& trace);
    bool updateRoadStatus(const std::string& roadId, bool isClosed, float congestion, TraceCollector& trace);
    bool completeIncident(const std::string& incidentId, TraceCollector& trace);
    void resetToSeedData();
};
```

### Invalidation & Stale Route Handling Policy:
When a road is closed or traffic conditions worsen:
1. **Pending Dispatches**: Evaluated at the exact moment of dispatch using current edge weights.
2. **In-Transit Dispatches**: If an active route is severed by a mid-response road closure, the simulation updates the assignment route by immediately re-running Dijkstra from the unit's last known intersection to the incident location. If no alternative path exists, the incident transitions to `BLOCKED_UNREACHABLE` and alerts the operator.

---

## 6. Trace-Event Architecture & Flow

Every algorithm in the core engine is instrumented with an optional `TraceCollector` reference. The trace collector records atomic, strongly-typed operations as they occur, without changing algorithm behavior or asymptotic complexity.

### 6.1 Trace Flow Sequence

```mermaid
sequenceDiagram
    autonumber
    actor User as Operator / Student
    participant UI as Next.js Frontend
    participant API as C++ HTTP Handler
    participant Sim as SimulationState
    participant Algo as Dijkstra / Heap / AVL
    participant Trace as TraceCollector

    User->>UI: Clicks "Dispatch Nearest Ambulance"
    UI->>API: POST /api/v1/dispatch { incidentId: "INC-101", enableTrace: true }
    API->>Sim: dispatchUnitForIncident("INC-101", trace)
    Sim->>Algo: router.findShortestPath(unitNode, incidentNode, &trace)
    
    loop Exploration & Relaxation
        Algo->>Trace: addEvent("DIJKSTRA_VISIT_NODE", node, distance)
        Algo->>Trace: addEvent("DIJKSTRA_RELAX_EDGE", u, v, oldDist, newDist)
    end
    
    Algo-->>Sim: returns shortest route & travel time
    Sim->>Algo: heap.remove("INC-101", &trace)
    Algo->>Trace: addEvent("HEAP_SINK", index, incidentId)
    Sim-->>API: DispatchResult + trace.getEvents()
    API-->>UI: 200 OK (JSON with result + array of trace steps)
    UI->>UI: Initialize Playback Controller in DSA Lab
    User->>UI: Presses "Step Forward" or "Play 1x"
    UI->>UI: Highlights Graph edges & Node relaxations incrementally
```

### 6.2 Trace Performance Strategy
- In **Operations Mode**, callers pass `enableTrace = false`, causing `TraceCollector` to execute no-op stubs (zero allocation overhead).
- In **DSA Lab Mode**, `enableTrace = true`, pre-allocating an internal vector of events.
- Trace vectors are serialized directly into the HTTP response JSON payload, eliminating disk I/O and socket streaming complexity.

---

## 7. Directory Structure Blueprint

```text
EmergencyGrid/
├── docs/                               # Architecture, API & Algorithm documentation
│   ├── PROJECT_OVERVIEW.md
│   ├── ARCHITECTURE.md
│   ├── DATA_STRUCTURES_AND_ALGORITHMS.md
│   ├── API_CONTRACT.md
│   ├── IMPLEMENTATION_PLAN.md
│   └── DECISIONS_AND_ASSUMPTIONS.md
│
├── backend/                            # C++20 Simulation Core
│   ├── CMakeLists.txt                  # Build configuration
│   ├── include/
│   │   ├── core/
│   │   │   ├── types.hpp               # Common enums, structs, entity definitions
│   │   │   ├── config.hpp              # City presets, default weights
│   │   │   └── trace.hpp               # TraceEvent and TraceCollector definitions
│   │   ├── dsa/
│   │   │   ├── graph.hpp               # Custom AdjacencyListGraph
│   │   │   ├── dijkstra.hpp            # Custom DijkstraRouter with trace hooks
│   │   │   ├── binary_heap.hpp         # Custom templated BinaryHeap (min/max)
│   │   │   ├── avl_tree.hpp            # Custom self-balancing AVLTree
│   │   │   └── hash_table.hpp          # Custom HashTable with separate chaining
│   │   ├── simulation/
│   │   │   ├── state.hpp               # Authoritative SimulationState
│   │   │   ├── dispatch_engine.hpp     # Multi-criteria unit selector & router
│   │   │   └── seed_data.hpp           # Predefined MetroGrid-16 topology & fleet
│   │   └── api/
│   │       ├── router.hpp              # HTTP route declarations
│   │       └── serialization.hpp       # JSON request/response mappers
│   ├── src/
│   │   ├── core/trace.cpp
│   │   ├── simulation/state.cpp
│   │   ├── simulation/dispatch_engine.cpp
│   │   ├── api/router.cpp
│   │   └── main.cpp                    # HTTP Server entry point
│   └── tests/                          # C++ Unit & Integration Tests
│       ├── test_graph_dijkstra.cpp
│       ├── test_binary_heap.cpp
│       ├── test_avl_tree.cpp
│       ├── test_hash_table.cpp
│       └── test_dispatch_flow.cpp
│
└── frontend/                           # Next.js 14+ App Router Client
    ├── package.json
    ├── tsconfig.json
    ├── tailwind.config.ts
    ├── src/
    │   ├── app/
    │   │   ├── layout.tsx              # Shell & global navigation
    │   │   ├── page.tsx                # Operations Mode dashboard
    │   │   └── dsa-lab/
    │   │       └── page.tsx            # DSA Lab inspection view
    │   ├── components/
    │   │   ├── common/                 # Buttons, badges, modals, metric cards
    │   │   ├── operations/
    │   │   │   ├── CityMapCanvas.tsx   # Interactive SVG/Canvas graph renderer
    │   │   │   ├── IncidentQueue.tsx   # Priority triage table
    │   │   │   ├── FleetStatus.tsx     # Responder availability list
    │   │   │   └── RoadControlModal.tsx# Closure and congestion sliders
    │   │   └── dsa-lab/
    │   │       ├── TraceTimeline.tsx   # Scrub bar, speed control, step buttons
    │   │       ├── DijkstraGraphView.tsx# Visual node exploration canvas
    │   │       ├── BinaryHeapView.tsx  # Interactive tree & array visualizer
    │   │       ├── AVLTreeView.tsx     # Tree layout with balance factor indicators
    │   │       └── HashTableBucketView.tsx# Bucket chain inspect modal
    │   ├── lib/
    │   │   ├── api.ts                  # Typed client for C++ HTTP backend
    │   │   ├── types.ts                # TypeScript mirrors of backend entity schemas
    │   │   └── tracePlayer.ts          # State machine for playback stepping
    │   └── data/
    │       └── cityCoordinates.ts      # Visual canvas node coordinates
```
