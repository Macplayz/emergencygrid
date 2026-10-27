# EmergencyGrid — Project Overview

## 1. Project Purpose

**EmergencyGrid** is an educational, interactive, real-time multi-agency emergency response simulation platform designed as an Advanced Data Structures and Algorithms (ADS) college-level capstone project. 

Its primary mission is to demonstrate how fundamental and advanced data structures and algorithms (graphs, shortest-path solvers, balanced binary search trees, binary heaps, and hash tables) solve real-world logistical coordination challenges. Rather than displaying detached textbook visualizations, EmergencyGrid drives an operational dispatch simulation where every vehicle routing decision, incident priority escalation, and resource index lookup is powered by an engine with verifiable, educational algorithm traces.

> **Academic & Safety Disclaimer**:  
> EmergencyGrid is strictly a simulation built for academic and demonstration purposes. It does not interface with actual Emergency Medical Services (EMS), 911/112 dispatch systems, real GPS feeds, or municipal traffic control networks. It must never be used for real-life operational dispatch.

---

## 2. Problem Statement

Emergency response coordination requires dispatching specialized units (Ambulances, Fire & Rescue squads, Police Patrols) across a dynamic road network subject to traffic fluctuations and road closures. Every incident has varying urgency and waiting time, while every emergency unit has distinct capabilities and locations.

Coordinating this effectively requires solving several core computer science problems simultaneously:
1. **Dynamic Spatial Routing**: Finding the quickest route across a weighted directed graph while accounting for live congestion and blockages.
2. **Prioritization Under Scarcity**: Deciding which incident gets serviced next when demand exceeds available emergency resources.
3. **Multi-Constraint Resource Matching**: Filtering units by agency capability and ranking eligible responders by travel time.
4. **High-Efficiency Record Management & Search**: Maintaining ordered temporal/severity logs and providing constant-time entity lookups as units and incidents evolve.

EmergencyGrid solves these problems by pairing a high-performance **C++20 algorithm simulation engine** with an intuitive **Next.js interactive dashboard**.

---

## 3. Target Users and Primary Use Cases

### 3.1 Primary Personas
- **The Student / Presenter**: Needs a transparent, well-structured, reproducible system that clearly demonstrates algorithmic correctness, time-complexity trade-offs, and clean architectural separation during an oral project defense.
- **The Academic Evaluator / Professor**: Wants to inspect real custom data structure implementations (not thin wrappers over STL containers), evaluate edge-case robustness (e.g., disconnected graphs, heap tie-breakers, AVL tree rebalancing), and see live algorithmic traces.
- **The Simulated Dispatch Operator**: Interacts with an emergency coordination UI to triage emergencies, inspect unit status, handle road blockages, and review dispatch rationales.

### 3.2 Key Use Cases
- **Triage and Prioritize Emergencies**: Ingest simulated incidents (e.g., multi-vehicle collision, structure fire, cardiac arrest) and rank them deterministically.
- **Automated Unit Dispatch**: Calculate the optimal responder unit based on agency type, capability, and Dijkstra-computed travel time.
- **Road Network Alterations**: Simulate traffic bottlenecks or emergency road closures (e.g., flooding, fallen trees) and immediately observe route recalculations.
- **Step-by-Step Algorithm Inspection (DSA Lab)**: Step forward and backward through the exact graph relaxations, heap swim/sink operations, AVL rotations, and hash bucket lookups that produced a dispatch decision.

---

## 4. The Two Main Application Modes

EmergencyGrid integrates two synchronized views powered by the identical backend simulation state and algorithm engine:

```
                  ┌─────────────────────────────────────────┐
                  │          EmergencyGrid Engine           │
                  │        (C++20 Simulation Core)          │
                  └────────────────────┬────────────────────┘
                                       │ HTTP / JSON API
                     ┌─────────────────┴─────────────────┐
                     ▼                                   ▼
        ┌─────────────────────────┐         ┌─────────────────────────┐
        │      Mode A: Operations │         │    Mode B: DSA Lab      │
        │      (Dispatch Console) │         │ (Algorithmic Inspector) │
        └─────────────────────────┘         └─────────────────────────┘
```

### 4.1 Mode A: Operations Mode (Dispatch Console)
The Operations Mode provides a clean, modern command-and-control interface designed for an emergency dispatcher:
- **City Command Dashboard**: High-level telemetry showing active incidents, pending queues, deployed responders, available fleet percentages, and resolved calls.
- **Interactive City Map**: Visual layout of intersections, road segments, active incident beacons, unit positions, and calculated dispatch paths.
- **Incident Dispatcher**: Create simulated emergency calls (specifying location, agency requirement, severity score) and trigger optimal unit assignment.
- **Dynamic Fleet & Road Controls**: Toggle road segment closures, adjust road congestion multipliers, and transition responders through dispatch lifecycles (Dispatched → On-Scene → Resolved → Available).
- **Dispatch Explanation Cards**: Transparent explanations summarizing why Unit $X$ was chosen over Unit $Y$ (e.g., travel time difference, agency capability match).

### 4.2 Mode B: DSA Lab (Algorithmic Inspector)
The DSA Lab is the academic differentiator of EmergencyGrid. It transforms every simulation action into an interactive, step-by-step educational exploration:
- **Algorithm Execution Traces**: Direct consumption of structured JSON trace events emitted by the C++ engine (node visits, edge relaxations, heap swaps, tree balance adjustments).
- **Dijkstra Pathfinding Visualizer**: Step through the shortest-path search across the city graph, viewing tentative distances, priority queue pop operations, and edge relaxations in real time.
- **Incident Priority Heap Visualizer**: Inspect binary heap tree and array representations; observe swim/sink operations when incidents are added, prioritized, or dequeued.
- **AVL Tree Record Inspector**: Inspect balanced search tree storage for incident history; visualize node balance factors and single/double rotations (LL, RR, LR, RL) upon insertion and deletion.
- **Hash Table Collision Visualizer**: Inspect hash distribution, bucket chain lengths, and collision handling for constant-time entity lookups.
- **Complexity and Metrics Panel**: Live operation counts (comparisons, pointer swaps, relaxations) paired with theoretical Big-O complexity analyses.

---

## 5. Core Features

1. **Predefined Simulated City Topology ("MetroGrid-16")**:
   - 16 distinct intersections/districts connected by 26 weighted directed/bidirectional road segments.
   - Distinct road classifications (Highways, Arterials, Local streets) with base travel times.
2. **Deterministic Multi-Agency Response Fleet**:
   - Dedicated stations hosting Ambulances (EMS), Fire Engines / Rescue Units (Fire), and Patrol Cars (Police).
3. **Custom C++ Educational Data Structure Library**:
   - Handcrafted `AdjacencyListGraph` with dynamic weight and closure support.
   - Handcrafted `BinaryHeap` for deterministic priority queue management.
   - Handcrafted `AVLTree` with explicit self-balancing rotation traces.
   - Handcrafted `HashTable` with separate chaining and collision tracking.
   - Custom `DijkstraRouter` emitting granular exploration trace events.
4. **Deterministic Dispatch Engine**:
   - Calculates priority using a clear formula combining severity and waiting time.
   - Dispatches the lowest estimated travel-time unit that satisfies all capability constraints.
5. **Interactive Playback & Timeline Scrubber**:
   - Play, pause, step forward, and reset algorithm traces in the DSA Lab.
6. **Zero External Cloud Dependencies**:
   - Runs completely offline on localhost with straightforward build steps.

---

## 6. Explicit Non-Goals and Boundaries

To guarantee student maintainability, predictable demonstration execution, and high architectural quality, the following are strictly out of scope:

- **No Real World GIS / Mapbox / OpenStreetMap Tiles**: The city is a simulated topological graph with clean Cartesian $(x, y)$ coordinates. This avoids external tile server lag, API keys, and geographical projection complications.
- **No Production Distributed Architecture**: No Docker containers, Kubernetes, Redis, Kafka, or microservice meshes. The application consists of two local processes: the Next.js dev server and the C++ HTTP daemon.
- **No Multi-User Authentication**: No JWTs, OAuth2, database user accounts, or role-based access control. The dashboard operates as a single-operator local console.
- **No Microscopic Physics / Continuous Traffic Simulation**: Vehicles do not simulate continuous acceleration, braking, lane-switching, or traffic light cycles. Routing operates on discrete road network edge weights.
- **No Mock or Client-Side Faked Algorithms**: The Next.js frontend will never calculate Dijkstra or simulate AVL rotations in TypeScript. Every visualization strictly renders backend trace data.
- **No Black-Box Container Cheating**: The core data structures will not wrap `std::priority_queue`, `std::map`, or `std::unordered_map`. Standard containers are restricted to non-educational plumbing (e.g., JSON serialization helpers).
