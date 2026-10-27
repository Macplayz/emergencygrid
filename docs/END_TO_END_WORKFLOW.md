# EmergencyGrid — End-to-End Simulation Workflow & Data Flow

This document details the complete 10-step lifecycle of an emergency event in **EmergencyGrid**, illustrating how user interactions in the Next.js frontend propagate into the C++20 engine, trigger custom data structures, and stream algorithmic telemetry back to the user interface.

---

## 1. System Sequence Diagram (Mermaid)

```mermaid
sequenceDiagram
    autonumber
    actor Dispatcher as Dispatch Operator
    participant UI as Next.js Frontend (Operations Mode)
    participant Proxy as Next.js API Proxy (/api/proxy)
    participant API as C++ HTTP Server (port 8080)
    participant IncSvc as IncidentService
    participant Heap as BinaryMaxHeap
    participant AVL as AVLTree Index
    participant Hash as HashTable Index
    participant DispSvc as DispatchService
    participant Router as DijkstraRouter (Graph)
    participant Fleet as UnitService

    %% Step 1-4: Incident Creation & Triage
    Dispatcher->>UI: Submit "Create Incident" Modal (e.g. Critical Fire @ J5)
    UI->>Proxy: POST /api/proxy/api/v1/incidents
    Proxy->>API: Forward POST /api/v1/incidents
    API->>IncSvc: createIncident(IncidentInput)
    IncSvc->>Hash: insert(incident.id, incident) [Direct O(1) Index]
    IncSvc->>AVL: insert(incident.id, incident) [Chronological Sorted BST]
    IncSvc->>Heap: insert(incident) [Sift-Up O(log N) Priority Triage]
    IncSvc-->>API: Created Incident Record
    API-->>Proxy: Envelope { success: true, data: Incident }
    Proxy-->>UI: Update Redux/State & Draw Marker on CityMapCanvas

    %% Step 5-8: Dispatch Evaluation & Assignment
    Dispatcher->>UI: Click "Dispatch Next" (or Dispatch specific ID)
    UI->>Proxy: POST /api/proxy/api/v1/dispatch
    Proxy->>API: Forward POST /api/v1/dispatch
    API->>DispSvc: dispatchIncident(incidentId, enableTrace=true)
    DispSvc->>IncSvc: getIncident(incidentId) from HashTable
    DispSvc->>Fleet: getEligibleUnits(serviceType, capabilities)
    loop For each eligible candidate responder
        DispSvc->>Router: calculateShortestPath(candidate.locationId, incident.locationId)
        Router-->>DispSvc: Path & Travel Time (mins)
    end
    DispSvc->>DispSvc: Select minimum travel time (Tie-break by unitId)
    DispSvc->>Fleet: setUnitStatus(unitId, DISPATCHED)
    DispSvc->>IncSvc: updateStatus(incidentId, ASSIGNED)
    DispSvc->>Heap: purgeStaleTop() [Lazy Invalidation via skip_stale_entry]
    DispSvc-->>API: DispatchRecord + Telemetry Trace
    API-->>Proxy: Envelope { dispatch, explanation, trace: [...] }
    Proxy-->>UI: Highlight Route Polyline on Map & Populate Dispatch Details

    %% Step 9: Lifecycle Completion
    Dispatcher->>UI: Click "Complete Response & Release Unit"
    UI->>Proxy: POST /api/proxy/api/v1/dispatches/{id}/complete
    Proxy->>API: Forward Complete Request
    API->>DispSvc: completeDispatch(dispatchId)
    DispSvc->>Fleet: setUnitStatus(unitId, AVAILABLE)
    DispSvc->>IncSvc: updateStatus(incidentId, RESOLVED)
    DispSvc-->>API: Success { message, dispatch }
    API-->>Proxy: Response Envelope
    Proxy-->>UI: Unit available again; Incident marked RESOLVED

    %% Step 10: Educational Trace Replay (DSA Lab)
    Note over Dispatcher,UI: Student switches to "DSA Lab" (/dsa-lab)
    Dispatcher->>UI: Selects "Dijkstra" or "Heap" Module & hits Play
    UI->>Proxy: GET /api/proxy/api/v1/dsa/heap/snapshot (or POST /dsa/heap/demo)
    Proxy->>API: Request Snapshot / Demo
    API-->>Proxy: Return Isolated Data & Sift-Up/Down Trace Events
    Proxy-->>UI: useTracePlayer steps through comparisons, swaps & relaxations
```

---

## 2. Step-by-Step Execution Lifecycle

### Step 1: User Submits Incident in Operations Mode
The dispatcher opens the **"Report Incident"** modal (`CreateIncidentModal.tsx`) on the dashboard, selecting:
- **Location**: An intersection on MetroGrid-16 (e.g. `J5`).
- **Service Agency**: `MEDICAL`, `FIRE`, or `POLICE`.
- **Severity**: Critical (4), High (3), Medium (2), or Low (1).
- **Required Capabilities**: Optional specialized tags such as `ALS`, `HAZMAT`, or `HEAVY_RESCUE`.

### Step 2: Next.js API Proxy Interception
The client invokes `api.createIncident()` in `frontend/src/lib/api.ts`. The browser sends an HTTP `POST` request to `/api/proxy/api/v1/incidents`. The Next.js Route Handler (`frontend/src/app/api/proxy/[...path]/route.ts`):
- Strips the `/api/proxy` prefix.
- Forwards the request body to `http://127.0.0.1:8080/api/v1/incidents`.
- Ensures clean header forwarding without cross-origin browser blocking.

### Step 3: C++ Backend Validation & Registration
Inside [`backend/src/api/server.cpp`](file:///b:/ADS%20Project/backend/src/api/server.cpp), the request body is parsed using `nlohmann::json`. The `IncidentService` performs strict domain validation:
- Validates that `locationId` exists within the `Graph` topology.
- Ensures the generated or provided `id` does not already exist in the `HashTable` (rejects duplicates with `409 Conflict`).
- Sets initial status to `PENDING` and records arrival sequence number and reported timestamp.

### Step 4: Multi-Index Atomic Ingestion
The incident is indexed atomically across **three separate data structures** in [`IncidentService::createIncident()`](file:///b:/ADS%20Project/backend/src/simulation/incident_service.cpp):
1. **Direct Index (`HashTable`)**: Keyed by unique `incidentId`. Enables instantaneous $O(1)$ lookups when checking or updating status.
2. **Chronological / Lexical Index (`AVLTree`)**: Keyed by `incidentId`. Maintains a strictly balanced binary search tree ($|BF| \le 1$), supporting $O(N)$ inorder sorted dumps and $O(\log N)$ range queries.
3. **Priority Triage Queue (`BinaryHeap`)**: Stored in a contiguous array-backed max-heap. Appends the incident to the end and executes `heapifyUp()` ($O(\log N)$).

### Step 5: Candidate Discovery & Eligibility Filtering
When the dispatcher clicks **"Dispatch Next"** or dispatches a specific incident, [`DispatchService::dispatchIncident()`](file:///b:/ADS%20Project/backend/src/simulation/dispatch_service.cpp) begins evaluation:
- Validates that the incident is currently in `PENDING` status.
- Queries `UnitService` to filter candidate units across the entire fleet:
  - **Availability**: Must be in `AVAILABLE` status (units currently `DISPATCHED`, `RESPONDING`, `BUSY`, or `OUT_OF_SERVICE` are rejected).
  - **Agency Match**: Must match the incident service type (e.g., only `FIRE` agency units for a structure fire).
  - **Capability Set**: Must satisfy all required capabilities (e.g. `HAZMAT`).
- Every ineligible unit receives an explicit `candidate_rejected` trace event explaining why it was excluded.

### Step 6: Dijkstra Shortest Path Calculation
For all remaining eligible candidate units, the dispatch engine calculates actual road network travel times to the incident scene:
- Calls [`DijkstraRouter::calculateShortestPath()`](file:///b:/ADS%20Project/backend/src/dsa/dijkstra.cpp) from the unit's stationed intersection to the incident intersection.
- Dijkstra explores edges, takes live traffic congestion multipliers into account, skips closed roads, relaxes tentative distances, and extracts minimum nodes from its min-heap priority queue.
- If a candidate is physically cut off by road closures, it is rejected with reason `NO_REACHABLE_PATH`.

### Step 7: Optimal Candidate Selection
The dispatch engine selects the unit with the **minimum cumulative travel time** in minutes:
- If multiple candidates have identical optimal travel times, deterministic tie-breaking is enforced by comparing lexical `unitId`.
- An assignment rationale string is generated (e.g., *"ENG-02 selected: shortest travel time (6.40 min) among 3 available FIRE units"*).
- The complete route waypoints (`["J2", "J3", "J5"]`) and travel metrics are packed into an active `DispatchRecord`.

### Step 8: Transactional State Mutation
To guarantee system consistency, state transitions occur atomically:
- Incident status transitions from `PENDING` $\to$ `ASSIGNED`.
- Selected emergency unit status transitions from `AVAILABLE` $\to$ `DISPATCHED`.
- Active dispatch record is stored in `DispatchService`.
- **Lazy Heap Invalidation**: The top of the binary max-heap is inspected; if the root element is no longer `PENDING`, `purgeStaleTop()` extracts and discards it via a `skip_stale_entry` trace event.

### Step 9: Lifecycle Completion & Unit Release
When the emergency response resolves, the operator clicks **"Complete Response & Release Unit"**:
- Endpoint `POST /api/v1/dispatches/{id}/complete` is invoked.
- Dispatch status transitions to `COMPLETED`.
- Incident status transitions to `RESOLVED`.
- The emergency vehicle transitions back to `AVAILABLE` status at its current location, ready for immediate subsequent dispatches.

### Step 10: DSA Lab Trace Telemetry & Replay
When a student opens the **DSA Lab** (`/dsa-lab`):
- The frontend requests algorithm trace logs from the backend.
- The reusable trace player hook (`useTracePlayer.ts`) parses the sequence of recorded `TraceEvent` objects.
- The student can scrub, step forward, step backward, or auto-play through each comparison, swap, edge relaxation, and tree rotation step with clear explanations.

---

## 3. Operations Mode vs. DSA Lab: State Isolation

A critical requirement of EmergencyGrid is that **educational lab demonstrations must never corrupt live Operations Mode simulation data**:

| Dimension | Operations Mode (`/`) | DSA Lab (`/dsa-lab`) |
| :--- | :--- | :--- |
| **Primary Goal** | Real-time command-and-control emergency coordination simulation. | Interactive educational inspection and algorithm demonstration. |
| **Backend State** | Operates on the **Authoritative Singleton State** (`incidentService_`, `unitService_`, `dispatchService_`, `graph_`). | Operates on **Ephemeral Local Sandboxes** (`POST /api/v1/dsa/*/demo`) or **Read-Only Snapshots** (`GET /api/v1/dsa/*`). |
| **State Mutations** | Creates real incidents, assigns units, closes real roads, and changes fleet availability. | Creates isolated heap/tree/hash instances inside local lambda scopes that are destroyed when the HTTP response completes. |
| **Side Effects** | Updates the live city map, active dispatches list, and operational metrics. | **Zero side effects**: Running 100 heap sift demonstrations leaves operational incident count at exactly 0. |
| **Trace Playback** | Shows completed route waypoints and telemetry summaries. | Replays step-by-step internal algorithmic steps (swaps, balance factors, bucket chains). |
