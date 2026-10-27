'use client';

import { BackendStatusResult } from '@/lib/types';

interface OperationsPlaceholderProps {
  status: BackendStatusResult | null;
}

export default function OperationsPlaceholder({ status }: OperationsPlaceholderProps) {
  const isOnline = Boolean(status?.reachable);
  const state = status?.state;

  return (
    <div className="space-y-6">
      {/* KPI Overview Telemetry Grid */}
      <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4">
        {/* KPI 1: Active Incidents */}
        <div className="bg-slate-900/60 border border-slate-800 rounded-xl p-4 shadow-sm">
          <div className="flex items-center justify-between text-xs text-slate-400">
            <span>Pending Incidents</span>
            <span className="w-2 h-2 rounded-full bg-rose-500" />
          </div>
          <div className="mt-2 flex items-baseline space-x-2">
            <span className="text-2xl font-extrabold text-slate-100 font-mono">
              {isOnline ? state?.metrics.pendingIncidents ?? 0 : '--'}
            </span>
            <span className="text-xs text-slate-400">triage queue</span>
          </div>
          <div className="mt-2 text-[11px] text-slate-400">
            Prioritized by custom <code className="text-amber-400 font-mono">BinaryHeap</code>
          </div>
        </div>

        {/* KPI 2: Available Units */}
        <div className="bg-slate-900/60 border border-slate-800 rounded-xl p-4 shadow-sm">
          <div className="flex items-center justify-between text-xs text-slate-400">
            <span>Available Fleet</span>
            <span className="w-2 h-2 rounded-full bg-emerald-400" />
          </div>
          <div className="mt-2 flex items-baseline space-x-2">
            <span className="text-2xl font-extrabold text-slate-100 font-mono">
              {isOnline ? state?.metrics.availableUnits ?? 9 : '--'}
            </span>
            <span className="text-xs text-slate-400">
              / {isOnline ? state?.metrics.totalUnits ?? 9 : 9} total
            </span>
          </div>
          <div className="mt-2 text-[11px] text-slate-400">
            EMS (3) • Fire (3) • Police (3)
          </div>
        </div>

        {/* KPI 3: Road Network Segments */}
        <div className="bg-slate-900/60 border border-slate-800 rounded-xl p-4 shadow-sm">
          <div className="flex items-center justify-between text-xs text-slate-400">
            <span>Road Segments</span>
            <span className="w-2 h-2 rounded-full bg-sky-400" />
          </div>
          <div className="mt-2 flex items-baseline space-x-2">
            <span className="text-2xl font-extrabold text-slate-100 font-mono">
              {isOnline ? state?.roadsSummary.totalRoads ?? 26 : 26}
            </span>
            <span className="text-xs text-slate-400">edges</span>
          </div>
          <div className="mt-2 text-[11px] text-slate-400">
            {isOnline ? state?.roadsSummary.closedRoads ?? 0 : 0} closed • MetroGrid-16
          </div>
        </div>

        {/* KPI 4: Dispatches Completed */}
        <div className="bg-slate-900/60 border border-slate-800 rounded-xl p-4 shadow-sm">
          <div className="flex items-center justify-between text-xs text-slate-400">
            <span>Completed Calls</span>
            <span className="w-2 h-2 rounded-full bg-indigo-400" />
          </div>
          <div className="mt-2 flex items-baseline space-x-2">
            <span className="text-2xl font-extrabold text-slate-100 font-mono">
              {isOnline ? state?.metrics.completedIncidents ?? 0 : '--'}
            </span>
            <span className="text-xs text-slate-400">audited</span>
          </div>
          <div className="mt-2 text-[11px] text-slate-400">
            Indexed into custom <code className="text-indigo-400 font-mono">AVLTree</code>
          </div>
        </div>
      </div>

      {/* Main Operations Canvas Shell Preview */}
      <div className="bg-slate-900/50 border border-slate-800 rounded-2xl p-8 text-center relative overflow-hidden">
        <div className="absolute inset-0 bg-gradient-to-b from-rose-500/5 via-transparent to-transparent pointer-events-none" />

        <div className="max-w-xl mx-auto space-y-4">
          <div className="inline-flex items-center justify-center w-12 h-12 rounded-xl bg-rose-500/10 text-rose-400 border border-rose-500/20 shadow-inner">
            <svg
              className="w-6 h-6"
              fill="none"
              stroke="currentColor"
              viewBox="0 0 24 24"
            >
              <path
                strokeLinecap="round"
                strokeLinejoin="round"
                strokeWidth={1.75}
                d="M9 20l-5.447-2.724A1 1 0 013 16.382V5.618a1 1 0 011.447-.894L9 7m0 13l6-3m-6 3V7m6 10l4.553 2.276A1 1 0 0021 18.382V7.618a1 1 0 00-.553-.894L15 4m0 13V4m0 0L9 7"
              />
            </svg>
          </div>

          <h3 className="text-xl font-bold text-slate-100">
            Operations Console — Phase 1 Foundation Active
          </h3>
          <p className="text-sm text-slate-400 leading-relaxed">
            The Next.js application shell and C++ HTTP server foundation are integrated. In Phase 2, this section will host the interactive <strong className="text-slate-300">MetroGrid-16 Cartesian Canvas</strong>, live dispatch panel, and real-time shortest-path route overlay.
          </p>

          <div className="pt-2 flex flex-wrap justify-center gap-2">
            <span className="px-3 py-1 rounded-md bg-slate-800/80 text-xs font-mono text-slate-300 border border-slate-700/60">
              MetroGrid-16 (16 Nodes / 26 Edges)
            </span>
            <span className="px-3 py-1 rounded-md bg-slate-800/80 text-xs font-mono text-slate-300 border border-slate-700/60">
              Multi-Agency Dispatch (EMS / Fire / Police)
            </span>
            <span className="px-3 py-1 rounded-md bg-slate-800/80 text-xs font-mono text-slate-300 border border-slate-700/60">
              Live Edge Congestion Modifiers
            </span>
          </div>
        </div>
      </div>
    </div>
  );
}
