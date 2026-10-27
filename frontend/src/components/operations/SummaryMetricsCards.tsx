'use client';

import { SimulationStateData } from '@/lib/types';

interface SummaryMetricsCardsProps {
  state: SimulationStateData | null;
  loading?: boolean;
}

export default function SummaryMetricsCards({ state, loading }: SummaryMetricsCardsProps) {
  const metrics = state?.metrics;
  const roads = state?.roadsSummary;

  return (
    <div className="grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-6 gap-3 sm:gap-4">
      {/* 1. Pending Incidents (Triage Queue) */}
      <div className="bg-white border border-[#DCE3EC] rounded-xl p-3.5 sm:p-4 shadow-xs flex flex-col justify-between">
        <div className="flex items-center justify-between text-xs text-[#526176]">
          <span className="font-medium">Pending Triage</span>
          <span className="flex h-2 w-2 relative">
            {(metrics?.pendingIncidents ?? 0) > 0 && (
              <span className="animate-ping absolute inline-flex h-full w-full rounded-full bg-red-400 opacity-75" />
            )}
            <span className="relative inline-flex rounded-full h-2 w-2 bg-[#DC2626]" />
          </span>
        </div>
        <div className="mt-2 flex items-baseline space-x-1.5">
          <span className="text-2xl sm:text-3xl font-bold text-[#172033] font-mono tracking-tight">
            {loading && !state ? '--' : metrics?.pendingIncidents ?? 0}
          </span>
          <span className="text-xs text-[#718096]">in queue</span>
        </div>
        <div className="mt-2 text-[10px] text-[#526176] flex items-center gap-1 font-mono">
          <span>Max-Heap</span>
          <span className="text-[#DCE3EC]">•</span>
          <span className="text-[#DC2626] font-semibold">Priority Triage</span>
        </div>
      </div>

      {/* 2. Active Dispatches */}
      <div className="bg-white border border-[#DCE3EC] rounded-xl p-3.5 sm:p-4 shadow-xs flex flex-col justify-between">
        <div className="flex items-center justify-between text-xs text-[#526176]">
          <span className="font-medium">Active Dispatches</span>
          <span className="h-2 w-2 rounded-full bg-[#D97706]" />
        </div>
        <div className="mt-2 flex items-baseline space-x-1.5">
          <span className="text-2xl sm:text-3xl font-bold text-[#D97706] font-mono tracking-tight">
            {loading && !state ? '--' : metrics?.activeResponses ?? 0}
          </span>
          <span className="text-xs text-[#718096]">en route</span>
        </div>
        <div className="mt-2 text-[10px] text-[#526176] flex items-center gap-1 font-mono">
          <span>Dijkstra</span>
          <span className="text-[#DCE3EC]">•</span>
          <span className="text-[#D97706] font-semibold">Live Transit</span>
        </div>
      </div>

      {/* 3. Available Units */}
      <div className="bg-white border border-[#DCE3EC] rounded-xl p-3.5 sm:p-4 shadow-xs flex flex-col justify-between">
        <div className="flex items-center justify-between text-xs text-[#526176]">
          <span className="font-medium">Available Fleet</span>
          <span className="h-2 w-2 rounded-full bg-[#15803D]" />
        </div>
        <div className="mt-2 flex items-baseline space-x-1">
          <span className="text-2xl sm:text-3xl font-bold text-[#15803D] font-mono tracking-tight">
            {loading && !state ? '--' : metrics?.availableUnits ?? 0}
          </span>
          <span className="text-xs text-[#718096]">
            / {metrics?.totalUnits ?? 0}
          </span>
        </div>
        <div className="mt-2 text-[10px] text-[#526176] flex items-center gap-1 font-mono">
          <span className="text-[#15803D] font-semibold">
            {metrics?.totalUnits ? Math.round(((metrics.availableUnits) / metrics.totalUnits) * 100) : 0}%
          </span>
          <span className="text-[#DCE3EC]">•</span>
          <span>Ready</span>
        </div>
      </div>

      {/* 4. Total Incidents Reported */}
      <div className="bg-white border border-[#DCE3EC] rounded-xl p-3.5 sm:p-4 shadow-xs flex flex-col justify-between">
        <div className="flex items-center justify-between text-xs text-[#526176]">
          <span className="font-medium">Total Incidents</span>
          <span className="h-2 w-2 rounded-full bg-[#3563D9]" />
        </div>
        <div className="mt-2 flex items-baseline space-x-1.5">
          <span className="text-2xl sm:text-3xl font-bold text-[#172033] font-mono tracking-tight">
            {loading && !state ? '--' : metrics?.totalIncidents ?? 0}
          </span>
          <span className="text-xs text-[#718096]">registered</span>
        </div>
        <div className="mt-2 text-[10px] text-[#526176] flex items-center gap-1 font-mono">
          <span>Hash Table</span>
          <span className="text-[#DCE3EC]">•</span>
          <span className="text-[#3563D9] font-semibold">O(1) Indexed</span>
        </div>
      </div>

      {/* 5. Completed Incidents */}
      <div className="bg-white border border-[#DCE3EC] rounded-xl p-3.5 sm:p-4 shadow-xs flex flex-col justify-between">
        <div className="flex items-center justify-between text-xs text-[#526176]">
          <span className="font-medium">Resolved Calls</span>
          <span className="h-2 w-2 rounded-full bg-[#4F46E5]" />
        </div>
        <div className="mt-2 flex items-baseline space-x-1.5">
          <span className="text-2xl sm:text-3xl font-bold text-[#4F46E5] font-mono tracking-tight">
            {loading && !state ? '--' : metrics?.completedIncidents ?? 0}
          </span>
          <span className="text-xs text-[#718096]">closed</span>
        </div>
        <div className="mt-2 text-[10px] text-[#526176] flex items-center gap-1 font-mono">
          <span>AVL Tree</span>
          <span className="text-[#DCE3EC]">•</span>
          <span className="text-[#4F46E5] font-semibold">Audited</span>
        </div>
      </div>

      {/* 6. Road Network Status */}
      <div className="bg-white border border-[#DCE3EC] rounded-xl p-3.5 sm:p-4 shadow-xs flex flex-col justify-between">
        <div className="flex items-center justify-between text-xs text-[#526176]">
          <span className="font-medium">Road Network</span>
          <span
            className={`h-2 w-2 rounded-full ${
              (roads?.closedRoads ?? 0) > 0 ? 'bg-[#DC2626]' : 'bg-[#15803D]'
            }`}
          />
        </div>
        <div className="mt-2 flex items-baseline space-x-1.5">
          <span className="text-2xl sm:text-3xl font-bold text-[#172033] font-mono tracking-tight">
            {loading && !state ? '--' : roads?.totalRoads ?? 26}
          </span>
          <span className="text-xs text-[#718096]">edges</span>
        </div>
        <div className="mt-2 text-[10px] text-[#526176] flex items-center gap-1 font-mono">
          <span className={(roads?.closedRoads ?? 0) > 0 ? 'text-[#DC2626] font-bold' : 'text-[#526176]'}>
            {roads?.closedRoads ?? 0} closed
          </span>
          <span className="text-[#DCE3EC]">•</span>
          <span className={(roads?.congestedRoads ?? 0) > 0 ? 'text-[#D97706] font-medium' : 'text-[#526176]'}>
            {roads?.congestedRoads ?? 0} congested
          </span>
        </div>
      </div>
    </div>
  );
}
