# EmergencyGrid — Setup & Running Guide (Windows PowerShell)

This guide provides exact, verified commands for setting up, building, running, and testing **EmergencyGrid** on a Windows computer using PowerShell.

---

## 1. Prerequisites & Required Software

Verify that the following tools are installed on your workstation:

| Tool | Recommended Version | Verification Command | Description |
| :--- | :--- | :--- | :--- |
| **Node.js** | `v20.0.0+` (LTS) | `node -v` | JavaScript runtime for Next.js 16 frontend |
| **npm** | `v9.0.0+` | `npm -v` | Node package manager |
| **CMake** | `v3.20.0+` | `cmake --version` | Build system generator for C++ backend |
| **C++20 Compiler** | GCC 13+ (MinGW-w64) or Visual Studio 2022 (MSVC) | `g++ -v` or `cl` | ISO C++20 compliant compiler |

> **Compiler Path Note**:
> If `cmake` or `g++` is not recognized globally in your PowerShell terminal, add its containing directory to your session PATH. For example:
> ```powershell
> $env:PATH = "C:\path\to\your\compiler\bin;C:\path\to\cmake\bin;$env:PATH"
> ```

---

## 2. Frontend Installation & Environment Setup

1. Open PowerShell and navigate to the `frontend/` directory:
   ```powershell
   cd "b:\ADS Project\frontend"
   ```

2. Install all npm dependencies:
   ```powershell
   npm install
   ```

3. Configure local environment variables:
   Create or verify `frontend/.env.local`:
   ```powershell
   # frontend/.env.local
   NEXT_PUBLIC_API_URL=http://127.0.0.1:8080
   BACKEND_URL=http://127.0.0.1:8080
   ```
   *Note: If no `.env.local` exists, the frontend defaults to `http://127.0.0.1:8080` automatically.*

---

## 3. C++ Backend Configuration & Build

1. Open PowerShell and navigate to the `backend/` directory:
   ```powershell
   cd "b:\ADS Project\backend"
   ```

2. Generate the build configuration with CMake:
   ```powershell
   # Using MinGW Makefiles (w64devkit / GCC):
   cmake -B build -S . -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

   # OR using default Visual Studio MSVC:
   # cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
   ```

3. Compile the backend executable and test binaries:
   ```powershell
   cmake --build build --config Release
   ```

Upon successful compilation, the backend binary will be located at:
`backend/build/bin/emergency_grid_backend.exe` (or `backend/build/bin/Release/emergency_grid_backend.exe` if using MSVC).

---

## 4. Running the Application (Dual-Process Setup)

EmergencyGrid runs as a decoupled architecture requiring two processes:

### Terminal 1: Start the C++ Backend Server
```powershell
cd "b:\ADS Project\backend"
& ".\build\bin\emergency_grid_backend.exe"
```
**Expected Terminal Output:**
```text
[EmergencyGrid Backend] Binding to http://127.0.0.1:8080 ...
[EmergencyGrid Backend] Endpoints registered:
  - GET  http://127.0.0.1:8080/api/v1/health
  - GET  http://127.0.0.1:8080/api/v1/simulation/state
  - GET  http://127.0.0.1:8080/api/v1/city/graph
  - POST http://127.0.0.1:8080/api/v1/routing/calculate
  - GET  http://127.0.0.1:8080/api/v1/dsa/heap/snapshot
[EmergencyGrid Backend] Server starting (blocking call) ...
```

### Terminal 2: Start the Next.js Frontend Client
```powershell
cd "b:\ADS Project\frontend"
npm run dev
```
**Expected Terminal Output:**
```text
▲ Next.js 16.4.2 (Turbopack)
- Local:        http://localhost:3000
- Network:      http://192.168.x.x:3000
✓ Ready in 1200ms
```

---

## 5. Accessing the Application

Open your web browser and navigate to:

- **Operations Mode**: [`http://localhost:3000`](http://localhost:3000)
  - Real-time city map, pending incidents queue, responder dispatching, road closure toggles, and traffic congestion controls.
- **DSA Lab (Interactive Suite)**: [`http://localhost:3000/dsa-lab`](http://localhost:3000/dsa-lab)
  - Interactive trace player for Dijkstra, Binary Max-Heap, AVL Tree, Hash Table, and Dispatch Engine.

---

## 6. Running Tests & Verifications

### A. Run C++ Unit Tests (All 113 Tests)
In the project root or backend directory:
```powershell
cd "b:\ADS Project\backend"
ctest --test-dir build --output-on-failure
```
**Expected Result**:
```text
Test project B:/ADS Project/backend/build
    Start 1: TestGraphDijkstra
1/4 Test #1: TestGraphDijkstra ................   Passed    0.03 sec
    Start 2: TestIncidentHeap
2/4 Test #2: TestIncidentHeap .................   Passed    0.04 sec
    Start 3: TestAvlHash
3/4 Test #3: TestAvlHash ......................   Passed    0.03 sec
    Start 4: TestDispatchAllocation
4/4 Test #4: TestDispatchAllocation ...........   Passed    0.11 sec

100% tests passed, 0 tests failed out of 4 (113 total unit tests passed, 0 failed)
```

Individual test executables can also be run directly:
```powershell
& ".\build\bin\test_graph_dijkstra.exe"
& ".\build\bin\test_incident_heap.exe"
& ".\build\bin\test_avl_hash.exe"
& ".\build\bin\test_dispatch_allocation.exe"
```

### B. Run Live Backend API Integration Script (38 Steps)
With the C++ backend server running on port 8080:
```powershell
powershell -ExecutionPolicy Bypass -File "b:\ADS Project\backend\tests\test_api.ps1"
```
**Expected Result**:
```text
======================================================
 ALL 38 API INTEGRATION TESTS COMPLETED SUCCESSFULLY! 
======================================================
```

### C. Run DSA Lab Frontend & State-Isolation Verification
With the C++ backend server running:
```powershell
cd "b:\ADS Project\frontend"
node test-dsa-lab.mjs
```
**Expected Result**:
```text
===========================================
🎉 ALL DSA LAB VERIFICATION CHECKS PASSED!
===========================================
```

### D. Run Frontend Type-Checking, Linting, & Production Build
```powershell
cd "b:\ADS Project\frontend"

# 1. Type-check TypeScript files
npx tsc --noEmit

# 2. Run ESLint checks
node lint-check.js

# 3. Create production bundle
npm run build
```

---

## 7. Common Troubleshooting & FAQ

### Issue 1: "Address already in use" on Port 8080
- **Cause**: An earlier backend instance is already running in the background.
- **Solution**: Find and terminate the process holding port 8080:
  ```powershell
  # Find PID:
  Get-NetTCPConnection -LocalPort 8080 | Select-Object OwningProcess
  # Stop process:
  Stop-Process -Id <PID> -Force
  ```

### Issue 2: Frontend Shows "C++ Engine Offline"
- **Cause**: Next.js proxy cannot connect to `http://127.0.0.1:8080`.
- **Solution**:
  1. Verify the C++ backend is running in Terminal 1.
  2. Test health directly in PowerShell:
     ```powershell
     Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/health"
     ```
  3. Ensure no firewall software is blocking local loopback connections on port 8080.

### Issue 3: CMake Error "No CMAKE_CXX_COMPILER could be found"
- **Cause**: C++ compiler tools (MinGW or MSVC) are not present in your current environment PATH.
- **Solution**:
  - For MinGW: Add `C:\path\to\mingw64\bin` to `$env:PATH`.
  - For MSVC: Run PowerShell from the **"Developer PowerShell for VS 2022"** shortcut in your Start menu.

### Issue 4: Resetting the Simulation Network
If you closed multiple roads or registered too many test incidents, reset the simulation to the initial MetroGrid-16 seed state without restarting the server:
```powershell
Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/city/reset" -Method Post
```
