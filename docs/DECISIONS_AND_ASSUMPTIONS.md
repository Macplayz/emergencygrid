# EmergencyGrid — Architectural Decisions, Trade-Offs & Assumptions

## 1. Architectural Decisions & Trade-Offs

### ADR-01: C++ HTTP Server Choice — `cpp-httplib` vs `Crow`
- **Context**: The C++ engine needs to expose a JSON REST API for the Next.js frontend.
- **Decision**: Select **`cpp-httplib`** (with nlohmann/json) as the primary lightweight HTTP library, with **Crow** as an alternative option.
- **Rationale**:
  - `cpp-httplib` is a proven, cross-platform, single-header C++11/20 library with zero external dependencies (no Boost or OpenSSL required for local HTTP).
  - Crow is feature-packed but frequently requires external CMake submodules, Boost ASIO dependencies, or complex Windows build configurations.
  - On Windows, a single-header header-only HTTP server and `nlohmann/json` compiles seamlessly with MSVC 2022 or MinGW without third-party package management friction.
- **Trade-off**: Lower throughput than multi-threaded asynchronous epoll servers, but more than sufficient for local simulation workloads (<100 requests/sec).

---

### ADR-02: Map Rendering — Cartesian Canvas/SVG vs Leaflet/Mapbox Tiles
- **Context**: The simulation requires visualizing intersections, road connections, incident pins, and travel routes.
- **Decision**: Implement a **custom Canvas/SVG topological renderer** using normalized 2D Cartesian coordinates $(x, y)$ for the predefined city ("MetroGrid-16").
- **Rationale**:
  - Real GIS tile engines (Mapbox, Leaflet, Google Maps) require coordinate projection conversions (WGS84 lat/long), internet connectivity, and API keys.
  - An SVG/Canvas graph renderer provides pixel-perfect alignment with the underlying C++ graph vertices, allowing direct visual mapping between graph node IDs (e.g., `J1`, `J4`) and canvas coordinates.
  - Enhances DSA visual clarity: edges can be rendered as pure graph vectors with mathematical weights and congestion colors.
- **Trade-off**: Lacks photorealistic satellite or road map imagery, but significantly increases educational clarity and guarantees 100% offline reliability.

---

### ADR-03: Collision Resolution Strategy for Custom Hash Table
- **Context**: The project requires a custom C++ Hash Table for entity lookups.
- **Decision**: Use **Separate Chaining with Singly-Linked Lists** rather than Open Addressing (Linear Probing / Double Hashing).
- **Rationale**:
  - Educational transparency: Separate chaining allows students to visually inspect collision chains as distinct linked lists in the DSA Lab.
  - Simpler, cleaner deletion semantics: unlinking a node from a bucket chain does not require tombstone markers (`DELETED` flags) or complex cluster cleanup.
  - Predictable performance under moderate load factors ($\lambda \le 0.75$).
- **Trade-off**: Minor memory overhead from next-pointers compared to contiguous open addressing tables.

---

### ADR-04: Trace Delivery Mechanism — REST Payloads vs WebSockets
- **Context**: Algorithm trace events must be transferred from the C++ engine to the Next.js DSA Lab.
- **Decision**: Deliver trace events as an array directly inside the **standard HTTP response payload** of the initiating API request.
- **Rationale**:
  - Deterministic pairing: The response to `POST /api/v1/dispatch` immediately includes the exact sequence of graph relaxations and heap updates that produced that specific dispatch.
  - Eliminates WebSocket connection lifecycle issues, reconnection edge cases, message reordering, and asynchronous race conditions.
  - The frontend can store the trace batch in memory and hand it directly to the `TracePlayer` for step-by-step scrubbing.
- **Trade-off**: Not suitable for continuous streaming of millions of high-frequency events, but ideal for discrete algorithmic operations (typically 10 to 100 steps per dispatch).

---

### ADR-05: Time Progression Model — Discrete Simulation Ticks
- **Context**: Incidents age, travel times elapse, and response units move across the city.
- **Decision**: Use a **discrete tick-based simulation model** rather than continuous real-time threads.
- **Rationale**:
  - Reproducibility: A tick-based model ensures that running the same scenario produces the exact same state, priority scores, and unit locations regardless of CPU speed or network lag.
  - Controllability: Allows the user to pause, step forward, or advance time by fixed increments during an academic demonstration.
  - Eliminates complex multithreading race conditions across data structures.
- **Trade-off**: Vehicle motion is discrete rather than continuously animated with physical vehicle dynamics.

---

## 2. Documented Assumptions

1. **Host Environment**:
   - Operating System: Windows 11.
   - Available Runtimes: Node.js (v20+) and npm are pre-installed.
   - C++ Toolchain: Visual Studio 2022 (MSVC) and MinGW are available on `C:\`. CMake will be used to generate native build files.
2. **City Scale & Topology**:
   - The simulated city ("MetroGrid-16") contains 16 intersections and 26 road segments.
   - The fleet size is fixed at 9 to 12 total units distributed across 3 service bases (EMS, Fire, Police).
3. **Single-User Local Execution**:
   - All components run on `localhost`. The frontend runs on port 3000, and the C++ engine runs on port 8080.
   - No authentication, user sessions, or concurrent multi-operator access is required.
4. **Data Persistence**:
   - In-memory storage is authoritative. A seed configuration resets the system to a clean, known state on command or server restart. No relational database (PostgreSQL/MySQL) or disk database is necessary.

---

## 3. Unresolved Questions & Future Extensions

| Topic | Current Assumption | Alternative for Future Consideration |
| :--- | :--- | :--- |
| **A* Heuristic Search** | We use Dijkstra since all edge weights are dynamic congestion times. | A* search with Euclidean distance heuristic could be added as an algorithmic comparison in the DSA Lab. |
| **Dynamic Graph Resizing** | The city network has a fixed set of intersections during simulation. | Allow user to draw new intersections and roads dynamically via the UI in a future iteration. |
| **Multi-Unit Coordinated Dispatch** | Dispatches 1 primary unit per incident. | Multi-agency dispatch (e.g., both Fire and EMS dispatched to a major structural fire). |
| **Trace Export** | Traces are held in memory during the session. | Allow exporting trace logs to a downloadable JSON file for homework or project grading. |
