# EmergencyGrid — Operations Mode Frontend

Educational simulation console for multi-agency emergency-response coordination, built with **Next.js (App Router)**, **TypeScript**, and **Tailwind CSS**.

> **Academic Simulation Notice**  
> This application is strictly an educational tool developed for an Advanced Data Structures and Algorithms (ADS) capstone project. It does **not** interface with real-world municipal 911 dispatch networks, real emergency units, or live GPS telemetry.

---

## 1. Environment Configuration

Create or update `.env.local` inside `frontend/`:

```bash
# URL of the running C++ HTTP Simulation Server
BACKEND_URL=http://localhost:8080

# Application Branding
NEXT_PUBLIC_APP_NAME=EmergencyGrid
```

---

## 2. Quick Start

### A. Start the C++ Engine Backend
Make sure the C++ backend is running on `http://localhost:8080`:

```powershell
# From project root
.\backend\build\bin\emergency_grid_backend.exe
```

### B. Start the Next.js Frontend
```bash
# In frontend/ directory
npm install
npm run dev
```

Open [http://localhost:3000](http://localhost:3000) in your browser.

---

## 3. Architecture & Operations Mode Features

1. **City Map Canvas (`CityMapCanvas.tsx`)**:
   - Lightweight, responsive interactive SVG visualization of **MetroGrid-16**.
   - Renders 16 intersections at authoritative deterministic display coordinates ($x: 20\text{--}320$, $y: 20\text{--}160$).
   - Renders 26 road segments with travel-time weights, closed-road warning styling, and congestion modifiers.
   - Highlights active Dijkstra routes following ordered location IDs returned by the C++ engine.
   - Visualizes live incident alerts and stationed fleet units.

2. **Incident Triage Panel (`IncidentListPanel.tsx`)**:
   - Displays real pending incidents ordered by the C++ custom `BinaryHeap`.
   - Filters by agency (EMS, Fire, Police), severity (1–4), and status.
   - Quick one-click action: "Dispatch Next Eligible Incident" (dequeues root of heap).
   - "New Incident" modal populating locations from the live graph topology.

3. **Fleet Readiness Panel (`FleetUnitsPanel.tsx`)**:
   - Lists all 9 emergency response units (EMS, Fire, Police).
   - Displays capabilities (`BLS`, `ALS`, `HAZMAT`, etc.), station coordinates, and current call assignments.

4. **Dispatch Telemetry Panel (`DispatchDetailsPanel.tsx`)**:
   - Displays assigned responder, target incident, step-by-step path, travel time, and engine assignment rationale.
   - Actions to **Complete Response & Release Unit** (`POST /api/v1/dispatches/:id/complete`) and **Recalculate Route** (`POST /api/v1/dispatches/:id/recalculate`).
   - Audit log of recent dispatches.

5. **Road Network Controls (`RoadControlsModal.tsx`)**:
   - Allows closing/reopening roads and adjusting traffic congestion multipliers ($1.0\times\text{--}5.0\times$).
   - Immediately updates the C++ Adjacency List Graph for all subsequent Dijkstra routing.

6. **Server-Side API Proxy (`app/api/proxy/[...path]/route.ts`)**:
   - Shields C++ backend endpoints, protects server configuration, handles timeouts, and normalizes responses.

---

## 4. Verification Commands

```bash
# Run TypeScript type check
npx tsc --noEmit

# Run ESLint scan
npm run lint

# Build production bundle
npm run build
```
