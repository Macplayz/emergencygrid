'use client';

import { BackendStatusResult } from '@/lib/types';

interface BackendStatusCardProps {
  status: BackendStatusResult | null;
  loading: boolean;
  onRefresh: () => void;
}

export default function BackendStatusCard({
  status,
  loading,
  onRefresh,
}: BackendStatusCardProps) {
  return (
    <div className="bg-white border border-[#DCE3EC] rounded-xl p-5 shadow-xs">
      <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-4 pb-4 border-b border-[#DCE3EC]">
        <div>
          <div className="flex items-center space-x-2.5">
            <h2 className="text-base font-bold text-[#172033]">
              Simulation Core Status
            </h2>
            <span
              className={`px-2.5 py-0.5 text-xs font-semibold rounded-full border ${
                loading
                  ? 'bg-amber-50 text-amber-800 border-amber-200'
                  : status?.reachable
                  ? 'bg-emerald-50 text-emerald-800 border-emerald-200'
                  : 'bg-red-50 text-red-800 border-red-200'
              }`}
            >
              {loading
                ? 'Probing Engine...'
                : status?.reachable
                ? 'Connected'
                : 'Offline / Unreachable'}
            </span>
          </div>
          <p className="text-xs text-[#526176] mt-0.5">
            Authoritative simulation state and custom DSA solvers (Dijkstra, BinaryHeap, AVLTree, HashTable)
          </p>
        </div>

        <button
          onClick={onRefresh}
          disabled={loading}
          className="inline-flex items-center justify-center space-x-2 px-3.5 py-1.5 text-xs font-medium rounded-lg bg-white hover:bg-slate-50 text-[#172033] border border-[#DCE3EC] transition-colors disabled:opacity-50 cursor-pointer shadow-xs"
        >
          <svg
            className={`w-3.5 h-3.5 text-[#526176] ${loading ? 'animate-spin' : ''}`}
            fill="none"
            stroke="currentColor"
            viewBox="0 0 24 24"
          >
            <path
              strokeLinecap="round"
              strokeLinejoin="round"
              strokeWidth={2}
              d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15"
            />
          </svg>
          <span>{loading ? 'Checking...' : 'Refresh Status'}</span>
        </button>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-3 gap-3 pt-4">
        {/* Backend Target URL */}
        <div className="bg-[#F8FAFC] border border-[#E2E8F0] rounded-lg p-3">
          <span className="text-[11px] font-semibold uppercase tracking-wider text-[#718096]">
            Engine Endpoint
          </span>
          <p className="text-sm font-mono text-[#172033] font-medium mt-0.5 truncate">
            {status?.backendUrl || 'http://localhost:8080'}
          </p>
          <span className="text-[10px] text-[#718096] mt-0.5 block">
            via Next.js Proxy
          </span>
        </div>

        {/* Latency / Response Time */}
        <div className="bg-[#F8FAFC] border border-[#E2E8F0] rounded-lg p-3">
          <span className="text-[11px] font-semibold uppercase tracking-wider text-[#718096]">
            Latency
          </span>
          <p className="text-sm font-mono mt-0.5">
            {loading ? (
              <span className="text-[#718096]">Measuring...</span>
            ) : status?.reachable && status.latencyMs !== undefined ? (
              <span className="text-[#15803D] font-bold">{status.latencyMs} ms</span>
            ) : (
              <span className="text-[#DC2626] font-semibold">Unavailable</span>
            )}
          </p>
          <span className="text-[10px] text-[#718096] mt-0.5 block">
            {status?.checkedAt
              ? `Checked at ${new Date(status.checkedAt).toLocaleTimeString()}`
              : 'Awaiting check'}
          </span>
        </div>

        {/* Engine Specification */}
        <div className="bg-[#F8FAFC] border border-[#E2E8F0] rounded-lg p-3">
          <span className="text-[11px] font-semibold uppercase tracking-wider text-[#718096]">
            Engine Architecture
          </span>
          <p className="text-sm font-semibold text-[#172033] mt-0.5">
            {status?.health?.standard || 'C++20'}{' '}
            <span className="text-xs text-[#526176] font-normal">
              ({status?.health?.service || 'Simulation Core'})
            </span>
          </p>
          <span className="text-[10px] text-[#718096] mt-0.5 block">
            Version: {status?.health?.version || '1.0.0'}
          </span>
        </div>
      </div>

      {/* Online Status Telemetry */}
      {status?.reachable && status.state && (
        <div className="mt-4 p-3.5 rounded-lg bg-emerald-50/70 border border-emerald-200 text-xs text-emerald-950">
          <div className="flex items-center space-x-2 font-bold text-emerald-900">
            <span className="w-2 h-2 rounded-full bg-[#15803D]" />
            <span>Authoritative State Synchronized:</span>
          </div>
          <div className="grid grid-cols-2 sm:grid-cols-4 gap-3 mt-2 text-[#172033]">
            <div>
              <span className="text-[10px] text-[#526176] block">Simulation Tick</span>
              <span className="font-mono font-bold text-sm">{status.state.tick}</span>
            </div>
            <div>
              <span className="text-[10px] text-[#526176] block">Total Units</span>
              <span className="font-mono font-bold text-sm">
                {status.state.metrics.totalUnits} Units
              </span>
            </div>
            <div>
              <span className="text-[10px] text-[#526176] block">Available Fleet</span>
              <span className="font-mono font-bold text-sm text-[#15803D]">
                {status.state.metrics.availableUnits}
              </span>
            </div>
            <div>
              <span className="text-[10px] text-[#526176] block">Road Segments</span>
              <span className="font-mono font-bold text-sm">
                {status.state.roadsSummary.totalRoads} Segments
              </span>
            </div>
          </div>
        </div>
      )}

      {/* Offline Status Guidance */}
      {!loading && !status?.reachable && (
        <div className="mt-4 p-3.5 rounded-lg bg-red-50 border border-red-200 text-xs text-red-950">
          <div className="flex items-start space-x-2.5">
            <span className="text-base text-red-600 shrink-0">⚠</span>
            <div className="space-y-1.5">
              <p className="font-bold text-red-900">
                Simulation engine core is not currently running.
              </p>
              <p className="text-red-800 leading-relaxed text-[11px]">
                {status?.error || 'Unable to establish HTTP connection to http://localhost:8080.'}
              </p>
              <div className="mt-2 bg-white p-2.5 rounded border border-red-200 font-mono text-[11px] text-[#172033]">
                <span className="text-[#718096] block mb-1"># Start the backend engine binary:</span>
                ./backend/build/bin/emergency_grid_backend.exe
              </div>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
