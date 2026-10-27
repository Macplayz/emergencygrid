import {
  ApiResponse,
  CityGraph,
  CompleteDispatchResponse,
  CreateIncidentInput,
  DispatchesListResponse,
  DispatchRecord,
  DispatchResponse,
  Incident,
  IncidentsListResponse,
  IncidentsSortedListResponse,
  RecalculateRouteResponse,
  ResetSimulationResponse,
  RouteResult,
  SimulationStateData,
  UnitsListResponse,
  UpdateRoadInput,
  HeapSnapshot,
  HeapDemoResponse,
  AVLTreeSnapshot,
  AvlDemoResponse,
  HashTableStats,
  HashTableDemoResponse,
} from './types';

const PROXY_PREFIX = '/api/proxy';

async function fetchFromApi<T>(
  endpoint: string,
  options?: RequestInit
): Promise<ApiResponse<T>> {
  const url = `${PROXY_PREFIX}${endpoint.startsWith('/') ? endpoint : `/${endpoint}`}`;

  try {
    const res = await fetch(url, {
      ...options,
      headers: {
        'Content-Type': 'application/json',
        Accept: 'application/json',
        ...(options?.headers || {}),
      },
      cache: 'no-store',
    });

    const json: ApiResponse<T> = await res.json();
    return json;
  } catch (err: unknown) {
    return {
      success: false,
      error: {
        code: 'CLIENT_NETWORK_ERROR',
        message: err instanceof Error ? err.message : 'Failed to connect to backend proxy',
      },
    };
  }
}

export const api = {
  // Simulation State
  async getSimulationState(): Promise<ApiResponse<SimulationStateData>> {
    return fetchFromApi<SimulationStateData>('/api/v1/simulation/state');
  },

  async resetSimulation(): Promise<ApiResponse<ResetSimulationResponse>> {
    return fetchFromApi<ResetSimulationResponse>('/api/v1/simulation/reset', {
      method: 'POST',
    });
  },

  // City Graph & Roads
  async getCityGraph(): Promise<ApiResponse<CityGraph>> {
    return fetchFromApi<CityGraph>('/api/v1/city/graph');
  },

  async updateRoad(
    roadId: string,
    data: UpdateRoadInput
  ): Promise<ApiResponse<unknown>> {
    return fetchFromApi<unknown>(`/api/v1/city/roads/${encodeURIComponent(roadId)}`, {
      method: 'PUT',
      body: JSON.stringify(data),
    });
  },

  async calculateRoute(
    sourceId: string,
    destinationId: string
  ): Promise<ApiResponse<{ route: RouteResult }>> {
    return fetchFromApi<{ route: RouteResult }>('/api/v1/routing/calculate', {
      method: 'POST',
      body: JSON.stringify({ sourceId, destinationId }),
    });
  },

  // Incidents
  async getIncidents(status?: string): Promise<ApiResponse<IncidentsListResponse>> {
    const query = status ? `?status=${encodeURIComponent(status)}` : '';
    return fetchFromApi<IncidentsListResponse>(`/api/v1/incidents${query}`);
  },

  async getIncidentsSorted(status?: string): Promise<ApiResponse<IncidentsSortedListResponse>> {
    const query = status ? `?status=${encodeURIComponent(status)}` : '';
    return fetchFromApi<IncidentsSortedListResponse>(`/api/v1/incidents/sorted${query}`);
  },

  async getNextIncident(): Promise<ApiResponse<Incident>> {
    return fetchFromApi<Incident>('/api/v1/incidents/next');
  },

  async getIncident(id: string): Promise<ApiResponse<Incident>> {
    return fetchFromApi<Incident>(`/api/v1/incidents/${encodeURIComponent(id)}`);
  },

  async createIncident(data: CreateIncidentInput): Promise<ApiResponse<Incident>> {
    return fetchFromApi<Incident>('/api/v1/incidents', {
      method: 'POST',
      body: JSON.stringify(data),
    });
  },

  async cancelIncident(id: string): Promise<ApiResponse<Incident>> {
    return fetchFromApi<Incident>(`/api/v1/incidents/${encodeURIComponent(id)}/cancel`, {
      method: 'POST',
    });
  },

  // Units
  async getUnits(params?: {
    serviceType?: string;
    status?: string;
    availableOnly?: boolean;
  }): Promise<ApiResponse<UnitsListResponse>> {
    const searchParams = new URLSearchParams();
    if (params?.serviceType) searchParams.append('serviceType', params.serviceType);
    if (params?.status) searchParams.append('status', params.status);
    if (params?.availableOnly) searchParams.append('availableOnly', 'true');

    const query = searchParams.toString() ? `?${searchParams.toString()}` : '';
    return fetchFromApi<UnitsListResponse>(`/api/v1/units${query}`);
  },

  // Dispatches
  async getDispatches(status?: string): Promise<ApiResponse<DispatchesListResponse>> {
    const query = status ? `?status=${encodeURIComponent(status)}` : '';
    return fetchFromApi<DispatchesListResponse>(`/api/v1/dispatches${query}`);
  },

  async getDispatch(id: string): Promise<ApiResponse<DispatchRecord>> {
    return fetchFromApi<DispatchRecord>(`/api/v1/dispatches/${encodeURIComponent(id)}`);
  },

  async dispatchIncident(incidentId: string): Promise<ApiResponse<DispatchResponse>> {
    return fetchFromApi<DispatchResponse>('/api/v1/dispatch', {
      method: 'POST',
      body: JSON.stringify({ incidentId }),
    });
  },

  async dispatchNext(): Promise<ApiResponse<DispatchResponse>> {
    return fetchFromApi<DispatchResponse>('/api/v1/dispatch/next', {
      method: 'POST',
      body: JSON.stringify({ highestPriority: true }),
    });
  },

  async completeDispatch(
    dispatchId: string
  ): Promise<ApiResponse<CompleteDispatchResponse>> {
    return fetchFromApi<CompleteDispatchResponse>(
      `/api/v1/dispatches/${encodeURIComponent(dispatchId)}/complete`,
      {
        method: 'POST',
      }
    );
  },

  async recalculateDispatch(
    dispatchId: string
  ): Promise<ApiResponse<RecalculateRouteResponse>> {
    return fetchFromApi<RecalculateRouteResponse>(
      `/api/v1/dispatches/${encodeURIComponent(dispatchId)}/recalculate`,
      {
        method: 'POST',
      }
    );
  },

  // ==========================================
  // DSA LAB ENDPOINTS & DEMO TRACES
  // ==========================================

  async getHeapSnapshot(): Promise<ApiResponse<HeapSnapshot>> {
    return fetchFromApi<HeapSnapshot>('/api/v1/dsa/heap/snapshot');
  },

  async runHeapDemo(
    action: 'sample_sequence' | 'extract_max' | 'insert',
    options?: { severity?: number; id?: string; locationId?: string }
  ): Promise<ApiResponse<HeapDemoResponse>> {
    return fetchFromApi<HeapDemoResponse>('/api/v1/dsa/heap/demo', {
      method: 'POST',
      body: JSON.stringify({ action, ...(options || {}) }),
    });
  },

  async getAvlSnapshot(): Promise<ApiResponse<AVLTreeSnapshot>> {
    return fetchFromApi<AVLTreeSnapshot>('/api/v1/dsa/avl');
  },

  async runAvlDemo(
    rotationType?: 'LL' | 'RR' | 'LR' | 'RL' | 'BALANCED',
    action?: 'rotation' | 'inorder'
  ): Promise<ApiResponse<AvlDemoResponse>> {
    return fetchFromApi<AvlDemoResponse>('/api/v1/dsa/avl/demo', {
      method: 'POST',
      body: JSON.stringify({
        action: action || (rotationType ? 'rotation' : 'inorder'),
        rotationType: rotationType || 'LL',
      }),
    });
  },

  async getHashTableStats(): Promise<ApiResponse<HashTableStats>> {
    return fetchFromApi<HashTableStats>('/api/v1/dsa/hash-table');
  },

  async runHashTableDemo(
    action: 'lookup' | 'collision' | 'rehash',
    key?: string
  ): Promise<ApiResponse<HashTableDemoResponse>> {
    return fetchFromApi<HashTableDemoResponse>('/api/v1/dsa/hash-table/demo', {
      method: 'POST',
      body: JSON.stringify({ action, key: key || 'INC-101' }),
    });
  },

  async calculateRouteWithTrace(
    sourceId: string,
    destinationId: string
  ): Promise<ApiResponse<{ route: RouteResult }>> {
    return fetchFromApi<{ route: RouteResult }>('/api/v1/routing/calculate', {
      method: 'POST',
      body: JSON.stringify({ sourceId, destinationId, enableTrace: true }),
    });
  },

  async runDispatchDemo(
    incidentId?: string
  ): Promise<ApiResponse<DispatchResponse>> {
    return fetchFromApi<DispatchResponse>('/api/v1/dsa/dispatch/demo', {
      method: 'POST',
      body: JSON.stringify({ incidentId }),
    });
  },

  async dispatchWithTrace(
    incidentId?: string
  ): Promise<ApiResponse<DispatchResponse>> {
    if (incidentId) {
      return fetchFromApi<DispatchResponse>('/api/v1/dispatch', {
        method: 'POST',
        body: JSON.stringify({ incidentId, enableTrace: true }),
      });
    }
    return fetchFromApi<DispatchResponse>('/api/v1/dispatch/next', {
      method: 'POST',
      body: JSON.stringify({ highestPriority: true, enableTrace: true }),
    });
  },
};
