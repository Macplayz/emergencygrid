# EmergencyGrid — Step-by-Step Run & Setup Guide

This guide walks you through building, testing, running, and demonstrating **EmergencyGrid** on a Windows computer.

---

## 1. Prerequisites Checklist

Ensure the following tools are installed and accessible in your system `PATH`:

| Requirement | Minimum Version | Check Command | Purpose |
|---|---|---|---|
| **C++ Compiler** | GCC 13+ (MinGW-w64) or MSVC 2022 | `g++ --version` | Compiling backend C++20 core & tests |
| **CMake** | 3.20+ | `cmake --version` | Build configuration system |
| **Node.js** | 18.17+ or 20+ | `node -v` | Running Next.js frontend application |
| **npm** | 9+ or 10+ | `npm -v` | Node package manager |
| **Python** | 3.8+ (Optional) | `python --version` | Running automated regression scripts |

---

## 2. Quick-Start (Run in 2 Terminals)

If the project is already built, you only need to run two commands in two separate terminal windows:

### Terminal 1 — Start C++ Backend Server (Port 8080):
```powershell
# In project root (b:\ADS Project)
.\backend\build\bin\emergency_grid_backend.exe
```
*You should see:*
```
[EmergencyGrid Backend] Binding to http://127.0.0.1:8080 ...
[EmergencyGrid Backend] Server starting (blocking call) ...
```

### Terminal 2 — Start Next.js Frontend (Port 3000):
```powershell
# Navigate to the frontend directory
cd frontend
npm run dev
# OR for production mode:
# npm run build
# npm run start
```
*You should see:*
```
▲ Next.js 16.4.2
- Local: http://localhost:3000
✓ Ready in 450ms
```

### Terminal 3 — Open Your Browser:
- **Operations Dashboard:** [http://localhost:3000](http://localhost:3000)
- **DSA Algorithm Lab:** [http://localhost:3000/dsa-lab](http://localhost:3000/dsa-lab)

---

## 3. Complete Build From Scratch Walkthrough

If you are setting up the project for the first time, follow these steps in order:

### Step 1: Open the Project
Open the folder in **Visual Studio Code** or open PowerShell inside `b:\ADS Project`.

---

### Step 2: Build the C++ Backend
The project includes a convenient automated Windows build script:

```powershell
# From the project root:
backend\build_backend.bat
```

Alternatively, if you prefer running manual CMake commands:
```powershell
mkdir backend\build
cd backend\build
cmake -G "MinGW Makefiles" ..
cmake --build . --config Release
cd ..\..
```
This produces:
- `backend\build\bin\emergency_grid_backend.exe` (main HTTP API server)
- `backend\build\bin\test_graph_dijkstra.exe` (Dijkstra & Graph test suite)
- `backend\build\bin\test_incident_heap.exe` (Binary Max-Heap test suite)
- `backend\build\bin\test_avl_hash.exe` (AVL Tree & Hash Table test suite)
- `backend\build\bin\test_dispatch_allocation.exe` (Dispatch & Allocation test suite)

---

### Step 3: Run C++ Test Suites (Verification)
You can run all 4 test suites to prove that all data structures and algorithms work with 100% test pass rates:

```powershell
# 1. Dijkstra Routing & Graph Tests (15 tests)
.\backend\build\bin\test_graph_dijkstra.exe

# 2. Binary Max-Heap Priority Queue Tests (28 tests)
.\backend\build\bin\test_incident_heap.exe

# 3. AVL Tree & Hash Table Index Tests (36 tests)
.\backend\build\bin\test_avl_hash.exe

# 4. Multi-Agency Dispatch & Unit Allocation Tests (35 tests)
.\backend\build\bin\test_dispatch_allocation.exe
```
*Total: 114 tests passed, 0 failed.*

---

### Step 4: Install Frontend Dependencies
```powershell
cd frontend
npm install
cd ..
```

---

### Step 5: Start the Backend Server
In your first terminal window, start the C++ backend:
```powershell
.\backend\build\bin\emergency_grid_backend.exe
```
*Keep this terminal running.*

---

### Step 6: Start the Frontend Application
In a second terminal window, launch the Next.js development server:
```powershell
cd frontend
npm run dev
```
Open [http://localhost:3000](http://localhost:3000) in your web browser (Chrome, Edge, or Firefox).

---

## 4. How to Demonstrate During a Viva

### Demonstration 1: Operations Mode (`http://localhost:3000/`)
1. **Interactive City Map:** Show the MetroGrid-16 map with 16 intersections ($J_1$ to $J_{16}$) and 26 road segments.
2. **Report New Incident:**
   - Click the blue **"+ Report Incident"** button.
   - Select an intersection (e.g., $J_{16}$), choose severity (e.g., Critical/High), select agency type (Medical/Fire/Police).
   - Click Submit. Watch the incident immediately insert into the Binary Max-Heap, AVL Tree, and Hash Table.
3. **Dispatch Allocation:**
   - Click **"Recommend Unit"** on the pending incident.
   - The engine filters available fleet units, runs Dijkstra's algorithm from each unit to the scene, and recommends the closest unit.
   - Click **"Confirm Dispatch"** to see the unit transition to `DISPATCHED` and the route draw on the map.
4. **Road Closures & Real-Time Rerouting:**
   - Click **"Road Controls"** in the top navigation.
   - Toggle road $R_{1\_4}$ to **CLOSED**.
   - Notice the road turns dashed red on the map.
   - Run a route calculation or dispatch to East Avenue ($J_4$); observe that Dijkstra automatically reroutes around the closed road.

---

### Demonstration 2: DSA Lab (`http://localhost:3000/dsa-lab`)
Switch to the **DSA Lab** tab via the top navigation bar to showcase the algorithm visualizers:

1. **Dijkstra Shortest-Path Tree (SPT) Module:**
   - Select Origin: `J1` (Central Plaza), Destination: `J16` (Industrial Hub).
   - Click **"Compute Route & Trace"**.
   - Use the **Play / Pause** and **Next / Previous** buttons to step through every relaxation.
   - Notice:
     - Edge currently examined is **Amber** (`#D97706`).
     - Settled nodes turn **Green** with their tentative distances displayed.
     - Confirmed Shortest-Path Tree (SPT) edges are highlighted in **Teal** (`#0D9488`).
     - At 100% progress / final step, the optimal route ($J_1 \rightarrow J_4 \rightarrow J_{16}$) is highlighted in **Royal Blue** (`#2563EB`) with source ($J_1$), intermediate node ($J_4$), and destination ($J_{16}$) badges.
2. **Binary Max-Heap Module:**
   - Demonstrates complete binary tree layout and underlying array indexing ($2i+1, 2i+2$).
   - Demonstrates $O(\log N)$ bubble-up (sift-up) on insert and trickle-down (sift-down) on extract-max.
3. **AVL Tree Module:**
   - Demonstrates balance factor maintenance: $\text{BalanceFactor} \in \{-1, 0, +1\}$.
   - Interactive demos for Left-Left (LL), Right-Right (RR), Left-Right (LR), and Right-Left (RL) rotations.
4. **Custom Hash Table Module:**
   - Demonstrates polynomial rolling hash distribution across bucket array.
   - Demonstrates separate chaining linked lists on collision.
   - Shows live load factor $\alpha = N / M$ and triggers automatic resizing/rehashing when $\alpha > 0.75$.
5. **Dispatch Engine Pipeline:**
   - Demonstrates candidate unit filtering, capability matching, travel time scoring, and deterministic tie-breaking.

---

## 5. Automated Verification Script

To verify all backend endpoints, Dijkstra SPT routing, road closures, and sandbox modules end-to-end, run:

```powershell
python scratch\verify_dijkstra_spt.py
```
*Expected output:*
```
==================================================
DIJKSTRA SHORTEST-PATH TREE (SPT) VERIFICATION
==================================================
[PASS] 1. Regression Test J1 -> J16: Path=['J1', 'J4', 'J16'], Roads=['R1_4', 'R4_16'], Time=5.4m
       Final trace event: route_reconstructed (J1 -> J16)
       Simultaneous Blue Edge Highlighting for R1_4 & R4_16 verified!
       SPT Roads Count: 14, SPT Edges Count: 14
[PASS] 2. Predecessor Metadata in Trace Events verified
[PASS] 3. Edge Case Source = Destination (J1 -> J1): Travel Time=0.0m, Path=['J1'], Settled=1
[PASS] 4. Closed Road R1_4 Rerouting: Path=J1 -> J13 -> J3 -> J4, Travel Time=8.0m
[PASS] 5. Reopened Road R1_4 Direct Path Restored: Path=J1 -> J4, Travel Time=3.0m
[PASS] 6. Regression: AVL Tree LL Demonstration working properly
[PASS] 7. Regression: Binary Heap Sandbox working properly
[PASS] 8. Regression: Hash Table Sandbox working properly
[PASS] 9. Regression: Dispatch Engine Sandbox working properly

==================================================
ALL DIJKSTRA SPT AND REGRESSION CHECKS PASSED!
==================================================
```

---

## 6. Troubleshooting & FAQ

### Q1: "Port 8080 already in use" error when starting backend
> **Fix:** Check if another instance of `emergency_grid_backend.exe` is already running. In PowerShell:
> ```powershell
> Get-Process emergency_grid_backend | Stop-Process -Force
> ```

### Q2: "Port 3000 already in use" error when starting frontend
> **Fix:** Stop existing Node process using port 3000:
> ```powershell
> npx kill-port 3000
> ```

### Q3: Frontend says "Backend Offline"
> **Fix:** Ensure `emergency_grid_backend.exe` is running in Terminal 1. Verify by opening `http://127.0.0.1:8080/api/v1/health` in your browser. It should return:
> ```json
> {
>   "success": true,
>   "data": { "status": "UP", "version": "1.0.0" }
> }
> ```

### Q4: How do I do a clean rebuild of the C++ backend?
> **Fix:** In PowerShell:
> ```powershell
> Remove-Item -Recurse -Force backend\build
> backend\build_backend.bat
> ```
