# EmergencyGrid — HTTP API Contract & Trace Specification

## 1. Overview & Conventions

All communication between the Next.js frontend and the C++ simulation backend occurs over HTTP using JSON payloads.

- **Base URL**: `http://localhost:8080/api/v1`
- **Content-Type**: `application/json`
- **Idempotency & Determinism**: Given the same seed and sequence of requests, backend responses and algorithm trace event sequences are fully reproducible.
- **Trace Toggle**: Endpoints that execute algorithms accept an optional query parameter or request body field: `enableTrace=true|false`. When `true`, the response payload includes a populated `trace` array.

---

## 2. Standard Response Wrapper & Error Format

### 2.1 Standard Success Envelope
```json
{
  "success": true,
  "data": { ... },
  "trace": [ ... ],
  "meta": {
    "simulationTick": 142,
    "executionTimeMs": 0.42
  }
}
```

### 2.2 Standard Error Response
```json
{
  "success": false,
  "error": {
    "code": "RESOURCE_UNAVAILABLE",
    "message": "No available EMS units with capability 'ADVANCED_LIFE_SUPPORT' could be reached.",
    "details": {
      "incidentId": "INC-104",
      "requiredAgency": "EMS",
      "attemptedUnits": ["AMB-01", "AMB-02"]
    }
  }
}
```

### 2.3 Common HTTP Status Codes
- `200 OK`: Request succeeded.
- `201 Created`: Entity successfully registered.
- `400 Bad Request`: Schema validation failure or invalid parameter.
- `404 Not Found`: Incident, unit, or intersection ID does not exist.
- `409 Conflict`: Illegal state transition (e.g., dispatching an already busy unit).
- `422 Unprocessable Entity`: Routing impossible (e.g., destination is partitioned by road closures).
- `500 Internal Error`: Unexpected engine fault.

---

## 3. Algorithm Trace Event Schema

Every algorithmic step emitted by the C++ engine conforms to this standardized schema:

```json
{
  "step": 4,
  "algorithm": "Dijkstra",
  "operation": "relax_edge",
  "entityId": "J4",
  "from": "J1",
  "to": "J4",
  "previousValue": 14.5,
  "newValue": 9.2,
  "dataStructure": "AdjacencyListGraph",
  "metadata": {
    "edgeWeight": 4.2,
    "congestionMultiplier": 1.0,
    "isClosed": false
  },
  "explanation": "Found shorter travel time to J4 via J1 (9.2 mins vs previous 14.5 mins). Relaxed edge and updated priority queue."
}
```

### 3.1 Trace Event Catalog

| Algorithm / Structure | `operation` | Key Fields Populated | Description |
| :--- | :--- | :--- | :--- |
| **Dijkstra** | `init_distances` | `source`, `initialValues` | Initializing all tentative distances to $\infty$, source to 0 |
| **Dijkstra** | `extract_min` | `entityId`, `cost` | Min-heap pops the intersection with shortest tentative distance |
| **Dijkstra** | `inspect_edge` | `from`, `to`, `edgeWeight` | Checking neighbor edge passability and congestion |
| **Dijkstra** | `relax_edge` | `from`, `to`, `previousValue`, `newValue` | Tentative distance shortened, predecessor updated |
| **Dijkstra** | `path_reconstructed` | `path` (array of node IDs) | Backtracking completed from destination to origin |
| **BinaryHeap** | `heap_push` | `entityId`, `key`, `index` | Appending element to heap array |
| **BinaryHeap** | `heap_swim` | `fromIndex`, `toIndex`, `comparison` | Swapping child with parent during upward sift |
| **BinaryHeap** | `heap_sink` | `fromIndex`, `toIndex`, `comparison` | Sifting root down to restore heap property after pop |
| **BinaryHeap** | `heap_extract` | `entityId`, `key` | Dequeuing the highest-priority incident |
| **AVLTree** | `compare` | `searchKey`, `currentNodeKey`, `currentHeight`, `balanceFactor` | Comparing key during tree navigation |
| **AVLTree** | `insert` | `key`, `height`, `balanceFactor` | Inserting new node |
| **AVLTree** | `delete` | `deletedKey`, `hadLeftChild`, `hadRightChild` | Deleting node from subtree |
| **AVLTree** | `height_update` | `nodeKey`, `oldHeight`, `newHeight`, `balanceFactor` | Recalculating node height after mutation |
| **AVLTree** | `balance_factor` | `nodeKey`, `balanceFactor`, `unbalanced` | Detecting balance factor violation ($|BF| > 1$) |
| **AVLTree** | `rotate_ll` | `pivotKey`, `promotedKey`, `newPivotHeight`, `newBalanceFactor` | Executing single right rotation (LL) |
| **AVLTree** | `rotate_rr` | `pivotKey`, `promotedKey`, `newPivotHeight`, `newBalanceFactor` | Executing single left rotation (RR) |
| **AVLTree** | `rotate_lr_start` | `pivotKey`, `childKey` | Starting double left-right rotation (LR) |
| **AVLTree** | `rotate_rl_start` | `pivotKey`, `childKey` | Starting double right-left rotation (RL) |
| **AVLTree** | `inorder_visit` | `visitedKey`, `height`, `balanceFactor` | Node visited in sorted ascending order |
| **AVLTree** | `search_found` | `foundKey` | Key found in tree |
| **AVLTree** | `search_not_found`| `missingKey` | Key not found in tree |
| **HashTable** | `hash_key` | `key`, `rawHash`, `bucketIndex`, `bucketCount` | 64-bit FNV-1a hash computed and bucket selected |
| **HashTable** | `bucket_traversal` | `bucketIndex`, `chainIndex`, `examinedKey`, `targetKey` | Traversing bucket chain checking keys |
| **HashTable** | `collision_detected` | `bucketIndex`, `existingChainLength`, `newKey` | Detecting collision in non-empty bucket |
| **HashTable** | `duplicate_rejected` | `bucketIndex`, `rejectedKey` | Duplicate key rejected during insert |
| **HashTable** | `entry_inserted` | `key`, `bucketIndex`, `chainLength`, `totalEntries`, `loadFactor` | Entry inserted into bucket chain |
| **HashTable** | `entry_found` | `key`, `bucketIndex`, `chainPosition` | Entry found in bucket chain |
| **HashTable** | `entry_not_found` | `key`, `bucketIndex`, `probedEntries` | Key not found in bucket chain |
| **HashTable** | `entry_updated` | `updatedKey` | Existing key value updated |
| **HashTable** | `entry_deleted` | `key`, `bucketIndex`, `remainingBucketLength`, `totalEntries` | Entry removed from bucket chain |
| **HashTable** | `rehash_started` | `oldBucketCount`, `newBucketCount`, `currentSize`, `currentLoadFactor` | Load factor exceeded, resizing started |
| **HashTable** | `rehash_completed` | `newBucketCount`, `totalEntries`, `newLoadFactor`, `collisionCount` | Rehashing completed |
| **HashTable** | `bucket_insert` | `bucketIndex`, `key` | Node inserted at bucket head |
| **HashTable** | `table_rehash` | `oldCapacity`, `newCapacity`, `loadFactor` | Table capacity doubled and elements migrated |

---

## 4. Endpoints Specification

### 4.1 System & Simulation Lifecycle

#### `GET /api/v1/simulation/state`
Returns the complete snapshot of the simulation: active incidents, unit positions, road network state, and global counters.

**Response `200 OK`**:
```json
{
  "success": true,
  "data": {
    "tick": 45,
    "metrics": {
      "totalIncidents": 12,
      "pendingIncidents": 3,
      "activeResponses": 2,
      "completedIncidents": 7,
      "availableUnits": 8,
      "totalUnits": 10
    },
    "roadsSummary": {
      "totalRoads": 26,
      "closedRoads": 1,
      "congestedRoads": 3
    }
  }
}
```

#### `POST /api/v1/simulation/reset`
Resets the simulation state back to the standard deterministic "MetroGrid-16" seed data.

---

### 4.2 Road Network & Graph Topology

#### `GET /api/v1/city/graph`
Returns the static and dynamic graph topology for rendering the city canvas.

**Response `200 OK`**:
```json
{
  "success": true,
  "data": {
    "intersections": [
      { "id": "J1", "name": "Central Plaza", "x": 120.0, "y": 80.0, "zone": "Commercial" },
      { "id": "J2", "name": "North Gate", "x": 120.0, "y": 20.0, "zone": "Residential" }
    ],
    "roads": [
      {
        "id": "R1_2",
        "source": "J1",
        "target": "J2",
        "baseLengthKm": 3.2,
        "speedLimitKmh": 50,
        "congestionFactor": 1.0,
        "isClosed": false,
        "isBidirectional": true,
        "travelTimeMinutes": 3.84
      }
    ]
  }
}
```

#### `POST /api/v1/routing/calculate` (or `POST /api/v1/route`)
Calculates the minimum-travel-time route between two intersections using Dijkstra's shortest-path algorithm. Optionally streams structured algorithm trace events when `enableTrace` is true.

**Request Body**:
```json
{
  "sourceId": "J1",
  "destinationId": "J5",
  "enableTrace": true
}
```

**Response `200 OK` (Route Found)**:
```json
{
  "success": true,
  "data": {
    "route": {
      "algorithm": "Dijkstra",
      "destinationId": "J5",
      "edgesExamined": 18,
      "nodesExplored": 7,
      "nodesSettled": 7,
      "path": ["J1", "J4", "J5"],
      "relaxations": 8,
      "roadIds": ["R1_4", "R4_5"],
      "routeFound": true,
      "sourceId": "J1",
      "totalDistanceKm": 4.5,
      "totalTravelTimeMinutes": 6.0
    }
  },
  "trace": [
    {
      "step": 1,
      "algorithm": "Dijkstra",
      "operation": "initialize_distances",
      "entityId": "J1",
      "from": "J1",
      "cost": 0.0,
      "explanation": "Initialized all tentative distances to infinity. Set source J1 to 0.0 min."
    },
    {
      "step": 2,
      "algorithm": "Dijkstra",
      "operation": "extract_min",
      "entityId": "J1",
      "cost": 0.0,
      "explanation": "Extracted closest unvisited candidate J1 with tentative travel time 0.00 min."
    },
    {
      "step": 3,
      "algorithm": "Dijkstra",
      "operation": "finalize_node",
      "entityId": "J1",
      "cost": 0.0,
      "explanation": "Finalized shortest travel time to J1 as 0.00 min."
    },
    {
      "step": 4,
      "algorithm": "Dijkstra",
      "operation": "examine_edge",
      "entityId": "R1_4",
      "from": "J1",
      "to": "J4",
      "metadata": {
        "roadId": "R1_4",
        "travelTimeMinutes": 3.0,
        "baseLengthKm": 2.0,
        "isClosed": false
      },
      "explanation": "Examining road R1_4 from J1 to J4 (weight: 3.00 min)."
    },
    {
      "step": 5,
      "algorithm": "Dijkstra",
      "operation": "relax_edge",
      "entityId": "J4",
      "from": "J1",
      "to": "J4",
      "newValue": 3.0,
      "explanation": "Relaxed edge via J1 to J4: travel time improved from INF to 3.00 min."
    },
    {
      "step": 94,
      "algorithm": "Dijkstra",
      "operation": "destination_reached",
      "entityId": "J5",
      "cost": 6.0,
      "explanation": "Target destination J5 settled with optimal travel time 6.00 min."
    },
    {
      "step": 95,
      "algorithm": "Dijkstra",
      "operation": "route_reconstructed",
      "from": "J1",
      "to": "J5",
      "cost": 6.0,
      "metadata": {
        "path": ["J1", "J4", "J5"],
        "roadIds": ["R1_4", "R4_5"],
        "totalDistanceKm": 4.5,
        "totalTravelTimeMinutes": 6.0
      },
      "explanation": "Successfully reconstructed shortest route from J1 to J5 (3 nodes, 6.00 min)."
    }
  ],
  "meta": {
    "simulationTick": 0,
    "executionTimeMs": 0.38
  }
}
```

#### `PUT /api/v1/city/roads/{id}` (or `POST /api/v1/city/roads/{id}`)
Updates road conditions (closures, congestion multiplier, or direct travel time). Validates ID and positive values.

**Request Body**:
```json
{
  "isClosed": true,
  "congestionFactor": 2.5,
  "enableTrace": true
}
```

**Response `200 OK`**:
```json
{
  "success": true,
  "data": {
    "id": "R1_4",
    "source": "J1",
    "target": "J4",
    "baseLengthKm": 2.0,
    "speedLimitKmh": 40.0,
    "congestionFactor": 2.5,
    "isClosed": true,
    "isBidirectional": true,
    "travelTimeMinutes": 7.5
  },
  "trace": [
    {
      "step": 1,
      "algorithm": "Graph",
      "operation": "set_edge_status",
      "entityId": "R1_4",
      "from": "J1",
      "to": "J4",
      "previousValue": 0.0,
      "newValue": 1.0,
      "metadata": {
        "roadId": "R1_4",
        "isClosed": true,
        "baseTravelTimeMinutes": 3.0
      },
      "explanation": "Road R1_4 (J1 <-> J4) status updated to CLOSED."
    }
  ],
  "meta": {
    "simulationTick": 0,
    "executionTimeMs": 0.15
  }
}
```

#### `POST /api/v1/city/reset` (or `POST /api/v1/simulation/reset`)
Restores the city network back to the canonical MetroGrid-16 seed topology.

**Response `200 OK`**:
```json
{
  "success": true,
  "data": {
    "message": "Simulation network successfully reset to MetroGrid-16 seed topology.",
    "intersections": 16,
    "roads": 26,
    "closedRoads": 0
  },
  "trace": [],
  "meta": {
    "simulationTick": 0,
    "executionTimeMs": 0.12
  }
}
```

---

### 4.3 Incidents Management

*Note: Endpoints are available under both `/api/v1/incidents...` and `/api/incidents...` prefixes.*

#### `GET /api/v1/incidents`
Returns incidents filtered by optional status query parameter (`PENDING`, `ASSIGNED`, `RESOLVED`, `CANCELLED`).

**Response `200 OK`**:
```json
{
  "success": true,
  "data": {
    "incidents": [
      {
        "id": "INC-101",
        "type": "FIRE",
        "serviceType": "FIRE",
        "severity": 4,
        "severityLabel": "CRITICAL",
        "locationId": "J5",
        "arrivalSequence": 1,
        "reportedTick": 1,
        "status": "PENDING",
        "computedPriority": 4000.0,
        "description": "Commercial kitchen electrical fire"
      }
    ],
    "count": 1
  },
  "trace": [],
  "meta": { "simulationTick": 1, "executionTimeMs": 0.10 }
}
```

#### `POST /api/v1/incidents`
Creates a new emergency incident. Inserts it into the custom `BinaryHeap` (priority queue) and authoritative registry, emitting real insertion and heapify traces.

**Request Body**:
```json
{
  "id": "INC-101",
  "locationId": "J5",
  "type": "FIRE",
  "severity": 4,
  "description": "Commercial kitchen electrical fire with smoke hazard",
  "enableTrace": true
}
```

**Response `201 Created`**:
```json
{
  "success": true,
  "data": {
    "id": "INC-101",
    "type": "FIRE",
    "serviceType": "FIRE",
    "severity": 4,
    "severityLabel": "CRITICAL",
    "locationId": "J5",
    "arrivalSequence": 1,
    "reportedTick": 1,
    "status": "PENDING",
    "computedPriority": 4000.0,
    "description": "Commercial kitchen electrical fire with smoke hazard"
  },
  "trace": [
    {
      "step": 1,
      "algorithm": "BinaryHeap",
      "operation": "insert",
      "entityId": "INC-101",
      "metadata": {
        "arrivalSequence": 1,
        "computedPriority": 4000.0,
        "index": 0,
        "severity": 4,
        "severityLabel": "CRITICAL"
      },
      "explanation": "Inserted incident INC-101 (severity: CRITICAL) at array index 0."
    }
  ],
  "meta": { "simulationTick": 0, "executionTimeMs": 0.20 }
}
```

#### `GET /api/v1/incidents/next`
Inspects the highest-priority eligible pending incident from the binary heap without removing it (`peek`). Automatically drains stale entries from the root.

**Query Parameter**: `?enableTrace=true|false`

**Response `200 OK`**:
```json
{
  "success": true,
  "data": {
    "id": "INC-101",
    "type": "FIRE",
    "serviceType": "FIRE",
    "severity": 4,
    "severityLabel": "CRITICAL",
    "locationId": "J5",
    "arrivalSequence": 1,
    "status": "PENDING",
    "computedPriority": 4000.0
  },
  "trace": [
    {
      "step": 1,
      "algorithm": "BinaryHeap",
      "operation": "peek",
      "entityId": "INC-101",
      "metadata": {
        "arrivalSequence": 1,
        "heapSize": 1,
        "rootId": "INC-101",
        "severity": 4,
        "severityLabel": "CRITICAL"
      },
      "explanation": "Inspected root incident INC-101 (severity: CRITICAL, arrival: 1) without removing from heap."
    }
  ]
}
```

#### `POST /api/v1/incidents/next/select`
Extracts the highest-priority eligible pending incident, transitions its status to `ASSIGNED`, and returns it for the dispatch workflow.

**Response `200 OK`**:
```json
{
  "success": true,
  "data": {
    "id": "INC-101",
    "status": "ASSIGNED"
  },
  "trace": [
    {
      "step": 1,
      "algorithm": "BinaryHeap",
      "operation": "extract_max",
      "entityId": "INC-101",
      "explanation": "Extracted root incident INC-101 from heap (heap is now empty)."
    }
  ]
}
```

#### `GET /api/v1/incidents/{id}`
Retrieves authoritative incident details by unique identifier.

**Response `200 OK`**:
```json
{
  "success": true,
  "data": {
    "id": "INC-101",
    "type": "FIRE",
    "severity": 4,
    "status": "PENDING"
  }
}
```

#### `POST /api/v1/incidents/{id}/cancel`
Cancels an incident currently in `PENDING` status. Updates the authoritative record to `CANCELLED`. If the incident is queued in the binary heap, it is lazily discarded via `skip_stale_entry` when reaching the root.

**Response `200 OK`**:
```json
{
  "success": true,
  "data": {
    "id": "INC-101",
    "status": "CANCELLED"
  },
  "trace": [
    {
      "step": 1,
      "algorithm": "BinaryHeap",
      "operation": "cancel_incident",
      "entityId": "INC-101",
      "explanation": "Marked incident INC-101 as CANCELLED in authoritative registry. Heap entry will be lazily purged."
    }
  ]
}
```

---

### 4.4 Emergency Fleet & Dispatch Operations

#### `GET /api/v1/units`
Returns the status, agency, and station locations of all emergency response units.

#### `POST /api/v1/dispatch`
Calculates optimal unit assignment for an incident. Runs Dijkstra for all eligible responders, selects the quickest responder, updates unit/incident status, and returns full path and algorithm trace.

**Request Body**:
```json
{
  "incidentId": "INC-105",
  "preferredUnitId": null,
  "enableTrace": true
}
```

**Response `200 OK`**:
```json
{
  "success": true,
  "data": {
    "incidentId": "INC-105",
    "assignedUnit": {
      "id": "ENG-02",
      "callSign": "Engine 2",
      "agency": "FIRE",
      "startLocationId": "J2"
    },
    "route": {
      "nodes": ["J2", "J3", "J5"],
      "totalTravelTimeMinutes": 6.4,
      "totalDistanceKm": 4.8
    },
    "decisionRationale": "ENG-02 selected: shortest travel time (6.4 min) among 3 available FIRE units. ENG-01 was blocked by road closure on R1_5."
  },
  "trace": [
    {
      "step": 1,
      "algorithm": "Dijkstra",
      "operation": "init_distances",
      "source": "J2",
      "explanation": "Initialized Dijkstra search from Station 2 (J2)."
    },
    {
      "step": 2,
      "algorithm": "Dijkstra",
      "operation": "relax_edge",
      "from": "J2",
      "to": "J3",
      "previousValue": 999999.0,
      "newValue": 2.8,
      "explanation": "Discovered path to J3 via J2 with travel time 2.8 min."
    },
    {
      "step": 3,
      "algorithm": "Dijkstra",
      "operation": "relax_edge",
      "from": "J3",
      "to": "J5",
      "previousValue": 999999.0,
      "newValue": 6.4,
      "explanation": "Discovered path to destination J5 via J3 with travel time 6.4 min."
    },
    {
      "step": 4,
      "algorithm": "BinaryHeap",
      "operation": "heap_extract",
      "entityId": "INC-105",
      "explanation": "Incident INC-105 successfully dequeued from pending triage heap."
    }
  ]
}
```

#### `POST /api/v1/incidents/{id}/complete`
Marks an incident as resolved, releases the assigned unit back to `AVAILABLE` status, and indexes the resolution record in the AVL tree.

---

#### `GET /api/v1/incidents/sorted`
Performs an $O(N)$ inorder traversal of the custom `AVLTree`, returning incident records ordered in strict ascending lexical order by `incidentId`. Supports optional filtering by `status` and full algorithmic trace emission.

**Query Parameters**:
- `status`: Optional filter (`PENDING`, `ASSIGNED`, `RESOLVED`, `CANCELLED`).
- `enableTrace`: `true` or `false`.

**Response `200 OK`**:
```json
{
  "success": true,
  "data": {
    "sortedIncidents": [
      {
        "id": "INC-010",
        "type": "MEDICAL",
        "severity": 2,
        "status": "PENDING"
      },
      {
        "id": "INC-050",
        "type": "FIRE",
        "severity": 4,
        "status": "ASSIGNED"
      }
    ],
    "count": 2,
    "orderingKey": "id"
  },
  "trace": [
    {
      "step": 1,
      "algorithm": "AVLTree",
      "operation": "inorder_visit",
      "entityId": "INC-010",
      "metadata": {
        "visitedKey": "INC-010",
        "height": 1,
        "balanceFactor": 0
      },
      "explanation": "Inorder visit at node 'INC-010'."
    }
  ]
}
```

---

### 4.4 Emergency Fleet & Dispatch Operations

#### `GET /api/v1/units`
Returns the status, agency, and station locations of all emergency response units.

#### `POST /api/v1/dispatch`
Calculates optimal unit assignment for an incident. Runs Dijkstra for all eligible responders, selects the quickest responder, updates unit/incident status, and returns full path and algorithm trace.

#### `POST /api/v1/incidents/{id}/complete`
Marks an incident as resolved, releases the assigned unit back to `AVAILABLE` status, and updates the incident status in both the Hash Table and the AVL tree.

---

### 4.5 DSA Lab Inspector Endpoints

#### `GET /api/v1/dsa/heap/snapshot`
Returns the raw internal array and tree structure of the `BinaryHeap` for live visualization.

#### `GET /api/v1/dsa/avl` & `GET /api/v1/dsa/avl/snapshot`
Returns the recursive node hierarchy of the `AVLTree`, including heights, balance factors, child links, and incident metadata for DSA Lab tree visualization.

**Response `200 OK`**:
```json
{
  "success": true,
  "data": {
    "size": 3,
    "height": 2,
    "isEmpty": false,
    "root": {
      "key": "INC-102",
      "height": 2,
      "balanceFactor": 0,
      "incidentId": "INC-102",
      "type": "FIRE",
      "severity": 3,
      "status": "PENDING",
      "locationId": "J2",
      "leftChild": "INC-101",
      "rightChild": "INC-103",
      "left": {
        "key": "INC-101",
        "height": 1,
        "balanceFactor": 0,
        "leftChild": null,
        "rightChild": null,
        "left": null,
        "right": null
      },
      "right": {
        "key": "INC-103",
        "height": 1,
        "balanceFactor": 0,
        "leftChild": null,
        "rightChild": null,
        "left": null,
        "right": null
      }
    }
  }
}
```

#### `GET /api/v1/dsa/hash-table` & `GET /api/v1/dsa/hashtable/snapshot`
Returns bucket occupancy, load factor, collision statistics, and non-pointer key distributions across all buckets.

**Response `200 OK`**:
```json
{
  "success": true,
  "data": {
    "size": 3,
    "bucketCount": 16,
    "occupiedBuckets": 3,
    "loadFactor": 0.1875,
    "maxLoadFactor": 0.75,
    "maxBucketDepth": 1,
    "collisionCount": 0,
    "buckets": [
      {
        "bucketIndex": 0,
        "entryCount": 0,
        "keys": []
      },
      {
        "bucketIndex": 2,
        "entryCount": 1,
        "keys": ["INC-101"]
      }
    ]
  }
}
```

#### `POST /api/v1/dsa/heap/demo`
Executes an isolated educational demonstration on an ephemeral `BinaryHeap` instance without altering live Operations Mode state. Returns the resulting heap snapshot and the complete sift-up/sift-down trace events.

**Request Body**:
```json
{
  "action": "sample_sequence" | "extract_max" | "insert",
  "severity": 4,
  "id": "INC-LAB-101",
  "locationId": "J5"
}
```

#### `POST /api/v1/dsa/avl/demo`
Executes an isolated educational demonstration on an ephemeral `IncidentAVLTree` instance without mutating live operational records. Generates an unbalanced insertion sequence and emits single or double rotation trace events.

**Request Body**:
```json
{
  "action": "rotation" | "inorder",
  "rotationType": "LL" | "RR" | "LR" | "RL"
}
```

#### `POST /api/v1/dsa/hash-table/demo`
Executes an isolated educational demonstration on an ephemeral `IncidentHashTable` instance without mutating live incident records. Supports collision generation and dynamic doubling rehash triggers when load factor $\ge 0.75$.

**Request Body**:
```json
{
  "action": "lookup" | "collision" | "rehash",
  "key": "INC-101"
}
```

---

## 7. Emergency Unit Management Endpoints

### 7.1 Register Emergency Unit
- **Endpoint**: `POST /api/v1/units`
- **Description**: Registers a new emergency response unit in the simulation fleet.
- **Request Body**:
```json
{
  "id": "AMB-04",
  "name": "Medic 4",
  "callSign": "Medic 4",
  "serviceType": "AMBULANCE",
  "locationId": "J1",
  "capabilities": ["BLS", "ALS"]
}
```
- **Response `201 Created`**:
```json
{
  "success": true,
  "data": {
    "id": "AMB-04",
    "name": "Medic 4",
    "callSign": "Medic 4",
    "serviceType": "MEDICAL",
    "agency": "MEDICAL",
    "locationId": "J1",
    "status": "AVAILABLE",
    "capabilities": ["BLS", "ALS"],
    "activeIncidentId": null
  },
  "trace": [],
  "meta": {
    "simulationTick": 0,
    "executionTimeMs": 0.05
  }
}
```

### 7.2 List Emergency Units
- **Endpoint**: `GET /api/v1/units`
- **Query Parameters**:
  - `serviceType`: `AMBULANCE`, `FIRE_RESCUE`, `POLICE` (optional)
  - `status`: `AVAILABLE`, `DISPATCHED`, `RESPONDING`, `BUSY`, `OUT_OF_SERVICE` (optional)
  - `availableOnly`: `true` | `false` (optional)
- **Response `200 OK`**:
```json
{
  "success": true,
  "data": {
    "total": 9,
    "units": [
      {
        "id": "AMB-01",
        "name": "Medic 1",
        "callSign": "Medic 1",
        "serviceType": "MEDICAL",
        "agency": "MEDICAL",
        "locationId": "J13",
        "status": "AVAILABLE",
        "capabilities": ["BLS", "ALS"],
        "activeIncidentId": null
      }
    ]
  }
}
```

### 7.3 Retrieve Emergency Unit by ID
- **Endpoint**: `GET /api/v1/units/:id`
- **Response `200 OK`**:
```json
{
  "success": true,
  "data": {
    "id": "AMB-01",
    "name": "Medic 1",
    "callSign": "Medic 1",
    "serviceType": "MEDICAL",
    "agency": "MEDICAL",
    "locationId": "J13",
    "status": "AVAILABLE",
    "capabilities": ["BLS", "ALS"],
    "activeIncidentId": null
  }
}
```

### 7.4 Update Emergency Unit Location / Status
- **Endpoint**: `PUT /api/v1/units/:id` or `PATCH /api/v1/units/:id`
- **Description**: Updates unit location (only when available/not busy) or availability status.
- **Request Body**:
```json
{
  "locationId": "J4",
  "status": "AVAILABLE"
}
```
- **Response `200 OK`**:
```json
{
  "success": true,
  "data": {
    "id": "AMB-01",
    "name": "Medic 1",
    "locationId": "J4",
    "status": "AVAILABLE"
  }
}
```

---

## 8. Dispatch & Resource Allocation Endpoints

### 8.1 Dispatch Incident
- **Endpoint**: `POST /api/v1/dispatch` or `POST /api/v1/dispatch/next`
- **Description**: Evaluates all candidate units matching incident service type and required capabilities, computes shortest routes using the C++ Dijkstra engine, selects the unit with minimum travel time (breaking ties deterministically by unit ID), atomically transitions the unit to `DISPATCHED` and the incident to `ASSIGNED`, and returns a detailed explanation and route.
- **Request Body**:
```json
{
  "incidentId": "INC-ALS-1",
  "enableTrace": true
}
```
*Alternatively, for dispatching the highest-priority heap incident:*
```json
{
  "highestPriority": true,
  "enableTrace": true
}
```
- **Response `201 Created`**:
```json
{
  "success": true,
  "data": {
    "dispatch": {
      "id": "DISP-001",
      "incidentId": "INC-ALS-1",
      "unitId": "AMB-01",
      "status": "ACTIVE",
      "dispatchedAtTick": 0,
      "completedAtTick": null,
      "travelTimeMinutes": 4.0,
      "route": {
        "sourceId": "J13",
        "destinationId": "J14",
        "routeFound": true,
        "path": ["J13", "J1", "J14"],
        "roadIds": ["R1_13", "R1_14"],
        "totalTravelTimeMinutes": 4.0,
        "totalDistanceKm": 3.0,
        "nodesSettled": 12,
        "algorithm": "Dijkstra"
      }
    },
    "explanation": "Unit AMB-01 (Medic 1) was available, satisfied required capabilities, and had the lowest valid travel time (4.00 min) among 3 eligible evaluated candidates."
  },
  "trace": [
    {
      "step": 1,
      "algorithm": "DispatchEngine",
      "operation": "validate_incident",
      "entityId": "INC-ALS-1"
    },
    {
      "step": 2,
      "algorithm": "DispatchEngine",
      "operation": "candidate_rejected",
      "entityId": "AMB-02",
      "metadata": {
        "reason": "missing_capabilities",
        "missingCapabilities": ["ALS"]
      }
    },
    {
      "step": 3,
      "algorithm": "DispatchEngine",
      "operation": "candidate_selected",
      "entityId": "AMB-01"
    }
  ]
}
```

### 8.2 List Dispatch Records
- **Endpoint**: `GET /api/v1/dispatches`
- **Query Parameters**: `status=ACTIVE|COMPLETED|CANCELLED` (optional)
- **Response `200 OK`**:
```json
{
  "success": true,
  "data": {
    "total": 1,
    "dispatches": [
      {
        "id": "DISP-001",
        "incidentId": "INC-ALS-1",
        "unitId": "AMB-01",
        "status": "ACTIVE",
        "travelTimeMinutes": 4.0
      }
    ]
  }
}
```

### 8.3 Retrieve Dispatch Record by ID
- **Endpoint**: `GET /api/v1/dispatches/:id`
- **Response `200 OK`**:
```json
{
  "success": true,
  "data": {
    "id": "DISP-001",
    "incidentId": "INC-ALS-1",
    "unitId": "AMB-01",
    "status": "ACTIVE",
    "dispatchedAtTick": 0,
    "travelTimeMinutes": 4.0,
    "route": { ... }
  }
}
```

### 8.4 Recalculate Active Dispatch Route
- **Endpoint**: `POST /api/v1/dispatches/:id/recalculate`
- **Description**: Reroutes an active dispatch on-demand using current road network closures and congestion conditions.
- **Response `200 OK`**:
```json
{
  "success": true,
  "data": {
    "dispatch": {
      "id": "DISP-001",
      "travelTimeMinutes": 6.5,
      "route": { ... }
    },
    "message": "Route successfully recalculated under current road conditions."
  }
}
```

### 8.5 Complete Dispatch
- **Endpoint**: `POST /api/v1/dispatches/:id/complete`
- **Description**: Atomically marks the dispatch `COMPLETED`, resolves the incident (`RESOLVED`), and releases the unit back to `AVAILABLE`.
- **Response `200 OK`**:
```json
{
  "success": true,
  "data": {
    "dispatch": {
      "id": "DISP-001",
      "status": "COMPLETED",
      "completedAtTick": 14
    },
    "message": "Dispatch DISP-001 completed. Unit released and incident resolved."
  }
}
```
