// TypeScript models mirroring EmergencyGrid backend contracts

export interface ApiResponse<T> {
  success: boolean;
  data?: T;
  trace?: unknown[];
  meta?: {
    simulationTick?: number;
    executionTimeMs?: number;
  };
  error?: {
    code: string;
    message: string;
    details?: Record<string, unknown>;
  };
}

export interface BackendHealthData {
  status: 'UP' | 'DOWN';
  service: string;
  version: string;
  standard: string;
  timestampMs: number;
}

export interface SimulationMetrics {
  totalIncidents: number;
  pendingIncidents: number;
  activeResponses: number;
  completedIncidents: number;
  availableUnits: number;
  totalUnits: number;
}

export interface RoadsSummary {
  totalRoads: number;
  closedRoads: number;
  congestedRoads: number;
}

export interface SimulationStateData {
  tick: number;
  status: string;
  metrics: SimulationMetrics;
  roadsSummary: RoadsSummary;
}

export interface BackendStatusResult {
  reachable: boolean;
  checkedAt: string;
  backendUrl: string;
  health?: BackendHealthData;
  state?: SimulationStateData;
  error?: string;
  latencyMs?: number;
}

// Graph & Topology Models
export interface Intersection {
  id: string;
  name: string;
  x: number;
  y: number;
  zone: string;
}

export interface RoadSegment {
  id: string;
  sourceId: string;
  targetId: string;
  source?: string; // Fallback compatibility
  target?: string;
  baseLengthKm: number;
  speedLimitKmh: number;
  congestionFactor: number;
  isClosed: boolean;
  isBidirectional: boolean;
  travelTimeMinutes: number;
}

export interface CityGraph {
  intersections: Intersection[];
  roads: RoadSegment[];
}

// Incident Models
export type IncidentSeverity = 1 | 2 | 3 | 4;
export type IncidentSeverityLabel = 'LOW' | 'MEDIUM' | 'HIGH' | 'CRITICAL';
export type IncidentType = 'MEDICAL' | 'FIRE' | 'POLICE';
export type IncidentStatus = 'PENDING' | 'ASSIGNED' | 'DISPATCHED' | 'ON_SCENE' | 'RESOLVED' | 'CANCELLED';

export interface Incident {
  id: string;
  type: string;
  serviceType: string;
  severity: number;
  severityLabel: string;
  locationId: string;
  arrivalSequence: number;
  reportedTick: number;
  status: IncidentStatus;
  computedPriority: number;
  description: string;
  requiredCapabilities?: string[];
}

export interface IncidentsListResponse {
  incidents: Incident[];
  count: number;
}

export interface IncidentsSortedListResponse {
  sortedIncidents: Incident[];
  count: number;
  orderingKey: string;
}

// Emergency Fleet & Unit Models
export type UnitStatus = 'AVAILABLE' | 'DISPATCHED' | 'RESPONDING' | 'BUSY' | 'OUT_OF_SERVICE';

export interface EmergencyUnit {
  id: string;
  name: string;
  callSign: string;
  serviceType: string;
  agency: string;
  locationId: string;
  status: UnitStatus;
  capabilities: string[];
  activeIncidentId?: string | null;
}

export interface UnitsListResponse {
  total: number;
  units: EmergencyUnit[];
}

export interface SPTEdge {
  from: string;
  to: string;
  roadId: string;
  travelTimeMinutes?: number;
  lengthKm?: number;
}

export interface RouteResult {
  sourceId: string;
  destinationId: string;
  routeFound: boolean;
  path: string[];
  roadIds: string[];
  shortestPathTreeRoadIds?: string[];
  shortestPathTree?: SPTEdge[];
  totalTravelTimeMinutes: number;
  totalDistanceKm: number;
  nodesSettled: number;
  nodesExplored: number;
  edgesExamined: number;
  relaxations: number;
  algorithm: string;
}

export type DispatchStatus = 'ACTIVE' | 'COMPLETED' | 'CANCELLED';

export interface DispatchRecord {
  id: string;
  incidentId: string;
  unitId: string;
  startLocationId: string;
  targetLocationId: string;
  route: RouteResult;
  travelTimeMinutes: number;
  dispatchedTick: number;
  completedTick?: number | null;
  status: DispatchStatus;
  decisionRationale: string;
}

export interface DispatchResponse {
  dispatch: DispatchRecord;
  explanation: string;
}

export interface DispatchesListResponse {
  total: number;
  dispatches: DispatchRecord[];
}

export interface CompleteDispatchResponse {
  dispatch: DispatchRecord;
  message: string;
}

export interface RecalculateRouteResponse {
  dispatch: DispatchRecord;
  message: string;
}

// Form & Mutation Inputs
export interface CreateIncidentInput {
  id?: string;
  locationId: string;
  type: string;
  severity: number | string;
  description?: string;
  requiredCapabilities?: string[];
  enableTrace?: boolean;
}

export interface UpdateRoadInput {
  isClosed?: boolean;
  congestionFactor?: number;
  travelTimeMinutes?: number;
  enableTrace?: boolean;
}

export interface ResetSimulationResponse {
  message: string;
  intersections: number;
  roads: number;
  closedRoads: number;
}

// ==========================================
// DSA LAB & ALGORITHM TRACE MODELS
// ==========================================

export interface TraceEvent {
  step?: number;
  algorithm: string;
  operation: string;
  entityId?: string;
  from?: string;
  to?: string;
  previousValue?: number;
  newValue?: number;
  cost?: number;
  metadata?: Record<string, unknown>;
  explanation: string;
}

// Binary Max-Heap Models
export interface HeapElementSnapshot extends Incident {
  heapIndex: number;
  parentIndex: number | null;
  leftChildIndex: number | null;
  rightChildIndex: number | null;
}

export interface HeapSnapshot {
  size: number;
  isEmpty: boolean;
  elements: HeapElementSnapshot[];
}

export interface HeapDemoResponse {
  snapshot: HeapSnapshot;
  action: string;
}

// AVL Tree Models
export interface AVLNodeSnapshot {
  key: string;
  height: number;
  balanceFactor: number;
  incidentId?: string;
  type?: string;
  severity?: number;
  status?: string;
  locationId?: string;
  leftChild: string | null;
  rightChild: string | null;
  left: AVLNodeSnapshot | null;
  right: AVLNodeSnapshot | null;
}

export interface AVLTreeSnapshot {
  size: number;
  height: number;
  root: AVLNodeSnapshot | null;
}

export interface AvlDemoResponse {
  snapshot: AVLTreeSnapshot;
  action: string;
  rotationType: string;
}

// Hash Table Models
export interface HashTableBucketInfo {
  bucketIndex: number;
  entryCount: number;
  keys: string[];
}

export interface HashTableStats {
  size: number;
  bucketCount: number;
  occupiedBuckets: number;
  loadFactor: number;
  maxLoadFactor: number;
  maxBucketDepth: number;
  collisionCount: number;
  buckets: HashTableBucketInfo[];
}

export interface HashTableDemoResponse {
  snapshot: HashTableStats;
  action: string;
}
