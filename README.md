# EmergencyGrid

<div align="center">

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg?style=flat-square&logo=cplusplus)
![Next.js](https://img.shields.io/badge/Next.js-16-black.svg?style=flat-square&logo=next.js)
![TypeScript](https://img.shields.io/badge/TypeScript-5.0-blue.svg?style=flat-square&logo=typescript)
![CMake](https://img.shields.io/badge/CMake-3.20%2B-064F8C.svg?style=flat-square&logo=cmake)
![License](https://img.shields.io/badge/License-MIT-green.svg?style=flat-square)

**Multi-Agency Emergency Response Simulation & Interactive DSA Visualizer**

A dual-mode full-stack simulation platform showcasing how custom C++ data structures and graph algorithms solve real-world emergency dispatch and logistical coordination challenges.

[Features](#-key-features) • [Data Structures](#-data-structures--complexity) • [Architecture](#-architecture) • [Quick Start](#-quick-start) • [Project Structure](#-project-structure) • [Documentation](#-documentation)

</div>

---

> **Simulation & Academic Notice**  
> EmergencyGrid is an educational simulation platform developed for Advanced Data Structures & Algorithms. It operates on an isolated in-memory simulation model (**MetroGrid-16**) and does not connect to live emergency service CAD networks or municipal 911/112 dispatch systems.

---

## ⚡ Key Features

### 1. Operations Command Center (`/`)
- **Interactive City Canvas**: Visualizes the **MetroGrid-16** road network (16 intersections, 26 segments), fleet positions, dynamic road closures, and traffic congestion multipliers.
- **Triage Queue**: Prioritizes emergency incidents based on severity (Levels 1–4), wait times, and agency requirements (Police, Fire, EMS).
- **Automated Dispatch Engine**: Instantly calculates the optimal responder unit matching capability requirements with minimum travel time.
- **Dynamic Traffic & Road Controls**: Toggle road closures and congestion multipliers ($1.0\times\text{--}5.0\times$) in real time with immediate route recalculations.

### 2. Interactive DSA Lab (`/dsa-lab`)
- **Step-by-Step Algorithm Inspector**: Visual trace playback with Play, Pause, Step Next/Prev, Replay, and variable speed controls ($0.5\times\text{--}4.0\times$).
- **5 Algorithmic Modules**:
  - **Dijkstra Router**: Live shortest path computation, candidate edge relaxations, and tentative distance updates.
  - **Incident Priority Heap**: Dual binary tree and contiguous array memory view animating sift-up and sift-down swaps.
  - **Incident AVL Tree**: Self-balancing chronological index tracking balance factors and rotation triggers (LL, RR, LR, RL).
  - **FNV-1a Hash Table**: 64-bit hashing breakdown, separate chaining bucket grid, collision resolution, and dynamic rehash.
  - **Dispatch Decision Pipeline**: End-to-end visualizer displaying triage extraction, capability filtering, Dijkstra route candidate scoring, and unit assignment.

---

## 📊 Data Structures & Complexity

All core data structures are built entirely from scratch in modern **C++20** without reliance on external container libraries:

| Data Structure | C++ Implementation | Simulation Role | Average | Worst Case |
|---|---|---|---|---|
| **Adjacency List Graph** | `dsa::Graph` | City road network & congestion weights | $\mathcal{O}(V + E)$ space | $\mathcal{O}(V + E)$ space |
| **Dijkstra Router** | `dsa::DijkstraRouter` | Shortest travel time routing | $\mathcal{O}(E \log V)$ | $\mathcal{O}(E \log V)$ |
| **Binary Max-Heap** | `dsa::BinaryHeap` | Triage queue prioritized by urgency | $\mathcal{O}(\log N)$ push/pop | $\mathcal{O}(\log N)$ push/pop |
| **AVL Tree** | `dsa::AvlTree` | Chronological incident audit index | $\mathcal{O}(\log N)$ search/insert | $\mathcal{O}(\log N)$ search/insert |
| **Hash Table** | `dsa::HashTable` | Rapid key-value lookups (64-bit FNV-1a) | $\mathcal{O}(1)$ lookup/insert | $\mathcal{O}(N)$ collision |

---

## 🏗 Architecture

```
┌────────────────────────────────────────────────────────┐
│               Next.js 16 Client (App Router)           │
│  - Operations Dashboard (/)  - DSA Lab (/dsa-lab)      │
│  - Interactive SVG Canvas    - Unified Trace Player    │
└──────────────────────────┬─────────────────────────────┘
                           │ HTTP REST via /api/proxy (Port 3000)
┌──────────────────────────▼─────────────────────────────┐
│             C++20 Simulation Core (Port 8080)          │
│  - Graph & Dijkstra Router   - Binary Max-Heap Triage  │
│  - Chronological AVL Tree    - FNV-1a Hash Table Index │
│  - Dispatch Decision Engine  - Trace Event Streamer    │
└────────────────────────────────────────────────────────┘
```

The system comprises two decoupled layers:
1. **C++20 Backend Engine**: High-performance HTTP service (`cpp-httplib`, `nlohmann/json`) running on port `8080`.
2. **Next.js 16 Frontend**: Modern UI (`React 19`, `Tailwind CSS`, `Lucide Icons`) serving operations and educational visualization tools on port `3000`.

---

## 🚀 Quick Start

### Prerequisites
- **C++ Compiler**: GCC 13+ (MinGW-w64) or MSVC 2022 (C++20 required)
- **CMake**: Version 3.20 or newer
- **Node.js**: Version 18.17+ or 20+
- **npm**: Version 9+

---

### Step 1: Build & Launch C++ Backend

```bash
# Navigate to backend directory
cd backend

# Configure build files
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Compile backend and test targets
cmake --build build --config Release

# Run backend executable
# Windows:
.\build\bin\emergency_grid_backend.exe
# Linux / macOS:
./build/bin/emergency_grid_backend
```
*The simulation core will start listening on `http://127.0.0.1:8080`.*

---

### Step 2: Launch Next.js Frontend

Open a new terminal window:

```bash
# Navigate to frontend directory
cd frontend

# Install dependencies
npm install

# Start development server
npm run dev
```

Visit **[http://localhost:3000](http://localhost:3000)** in your browser:
- **Operations Dashboard**: `http://localhost:3000/`
- **Interactive DSA Lab**: `http://localhost:3000/dsa-lab`

---

## 📁 Project Structure

```text
emergencygrid/
├── backend/                      # C++20 Simulation Core
│   ├── include/
│   │   ├── api/                  # HTTP route controllers & server headers
│   │   ├── core/                 # Shared domain types & execution tracing
│   │   ├── dsa/                  # Custom Graph, Dijkstra, Heap, AVL, HashTable
│   │   └── simulation/           # Dispatch, incident, and fleet unit services
│   ├── src/                      # Implementation files (.cpp)
│   ├── tests/                    # CTest unit & integration suites
│   └── CMakeLists.txt            # CMake build configuration
├── frontend/                     # Next.js 16 Web Application
│   ├── src/
│   │   ├── app/                  # App Router pages (/ & /dsa-lab)
│   │   ├── components/
│   │   │   ├── operations/       # Map canvas, incident queue, fleet panels
│   │   │   └── dsa-lab/          # Algorithm trace viewers & controls
│   │   └── lib/                  # API client, type definitions, trace player
│   └── package.json
├── docs/                         # In-depth architectural & viva documentation
├── scratch/                      # Automated system verification test scripts
├── .gitignore
├── HOW_TO_RUN.md
└── README.md
```

---

## 🧪 Testing & Verification

Run the test suites across backend and frontend layers:

```bash
# 1. Run all C++ CTest unit suites:
cd backend
ctest --test-dir build --output-on-failure

# 2. Run PowerShell API scenario tests:
powershell -ExecutionPolicy Bypass -File tests/test_api.ps1

# 3. Run Frontend DSA verification & state-isolation:
cd ../frontend
node test-dsa-lab.mjs

# 4. Run Frontend typecheck and build:
npm run lint
npm run build
```

---

## 📚 Documentation

Detailed architectural notes, endpoint specs, and academic viva resources are available in [`docs/`](docs/):

- [Architecture Guide](docs/ARCHITECTURE.md) — System boundaries, thread models, and state design.
- [API Contract](docs/API_CONTRACT.md) — REST endpoints and trace event JSON payloads.
- [DSA Concepts & Big-O Complexity](docs/DSA_CONCEPTS_AND_COMPLEXITY.md) — Algorithmic breakdown and proofs.
- [End-to-End Workflow](docs/END_TO_END_WORKFLOW.md) — Incident lifecycle and dispatch pipeline.
- [Viva Voce Preparation](docs/VIVA_PREPARATION.md) — 35+ technical viva Q&A and demonstration checklist.
- [Developer Handbook](docs/DEVELOPER_GUIDE.md) — File-by-file walkthrough and extension guide.

---

## 📄 License

This project is licensed under the [MIT License](LICENSE).
