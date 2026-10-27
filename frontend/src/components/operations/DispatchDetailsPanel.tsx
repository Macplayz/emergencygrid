'use client';

import { useState } from 'react';
import { DispatchRecord } from '@/lib/types';

interface DispatchDetailsPanelProps {
  dispatches: DispatchRecord[];
  activeDispatch: DispatchRecord | null;
  onSelectDispatch: (dispatch: DispatchRecord) => void;
  onCompleteDispatch: (dispatchId: string) => Promise<void>;
  onRecalculateDispatch: (dispatchId: string) => Promise<void>;
}

export default function DispatchDetailsPanel({
  dispatches,
  activeDispatch,
  onSelectDispatch,
  onCompleteDispatch,
  onRecalculateDispatch,
}: DispatchDetailsPanelProps) {
  const [actionLoading, setActionLoading] = useState<boolean>(false);
  const [actionError, setActionError] = useState<string | null>(null);

  const handleComplete = async (dispatchId: string) => {
    setActionLoading(true);
    setActionError(null);
    try {
      await onCompleteDispatch(dispatchId);
    } catch (err: unknown) {
      setActionError(err instanceof Error ? err.message : 'Failed to complete dispatch');
    } finally {
      setActionLoading(false);
    }
  };

  const handleRecalculate = async (dispatchId: string) => {
    setActionLoading(true);
    setActionError(null);
    try {
      await onRecalculateDispatch(dispatchId);
    } catch (err: unknown) {
      setActionError(err instanceof Error ? err.message : 'Failed to recalculate route');
    } finally {
      setActionLoading(false);
    }
  };

  return (
    <div className="bg-white border border-[#DCE3EC] rounded-xl flex flex-col shadow-xs overflow-hidden">
      {/* Header */}
      <div className="p-4 border-b border-[#DCE3EC] bg-white flex items-center justify-between">
        <div>
          <div className="flex items-center space-x-2">
            <span className="w-2.5 h-2.5 rounded-full bg-[#087E8B]" />
            <h2 className="text-sm font-bold text-[#172033] uppercase tracking-wide">
              Dispatch Telemetry &amp; Routing
            </h2>
            <span className="px-2 py-0.5 rounded-full text-[11px] font-mono bg-slate-100 text-[#526176] border border-slate-200">
              {dispatches.length} Total
            </span>
          </div>
          <p className="text-[11px] text-[#526176] mt-0.5">
            Dijkstra optimal shortest-path assignments, routing telemetry, and unit release
          </p>
        </div>
      </div>

      {actionError && (
        <div className="p-3 bg-red-50 border-b border-red-200 text-red-700 text-xs flex items-center justify-between">
          <span>{actionError}</span>
          <button
            type="button"
            onClick={() => setActionError(null)}
            className="text-red-700 hover:text-red-900 font-bold"
          >
            ✕
          </button>
        </div>
      )}

      {/* Main Content: Focused Dispatch Inspection (if any) + Dispatches History Table */}
      <div className="p-4 space-y-4">
        {activeDispatch ? (
          <div className="p-4 rounded-xl bg-[#F8FAFC] border border-[#DCE3EC] shadow-xs space-y-4">
            {/* Top row of active dispatch */}
            <div className="flex flex-wrap items-center justify-between gap-2 border-b border-[#E2E8F0] pb-3">
              <div className="flex items-center space-x-2">
                <span className="text-xs text-[#526176]">Dispatch ID:</span>
                <span className="font-mono font-bold text-[#3563D9] text-sm">
                  {activeDispatch.id}
                </span>
                <span
                  className={`px-2 py-0.5 rounded text-[10px] font-bold border ${
                    activeDispatch.status === 'ACTIVE'
                      ? 'bg-amber-50 text-amber-800 border-amber-200'
                      : 'bg-emerald-50 text-emerald-800 border-emerald-200'
                  }`}
                >
                  {activeDispatch.status}
                </span>
              </div>

              {/* Action Buttons */}
              <div className="flex items-center space-x-2">
                {activeDispatch.status === 'ACTIVE' && (
                  <>
                    <button
                      type="button"
                      disabled={actionLoading}
                      onClick={() => handleRecalculate(activeDispatch.id)}
                      className="px-2.5 py-1.5 rounded-lg text-xs font-semibold bg-white hover:bg-slate-50 text-[#3563D9] border border-[#DCE3EC] transition-colors cursor-pointer shadow-xs"
                      title="Recalculate shortest path considering current road closures and traffic"
                    >
                      {actionLoading ? 'Calculating...' : 'Recalculate Route'}
                    </button>

                    <button
                      type="button"
                      disabled={actionLoading}
                      onClick={() => handleComplete(activeDispatch.id)}
                      className="px-3 py-1.5 rounded-lg text-xs font-semibold bg-[#15803D] hover:bg-[#126832] text-white shadow-xs transition-colors cursor-pointer"
                      title="Mark response completed and release unit back to available pool"
                    >
                      {actionLoading ? 'Releasing...' : '✓ Complete Response'}
                    </button>
                  </>
                )}
              </div>
            </div>

            {/* Metrics Row */}
            <div className="grid grid-cols-2 sm:grid-cols-4 gap-3 text-xs">
              <div className="bg-white border border-[#DCE3EC] rounded-lg p-2.5 shadow-xs">
                <div className="text-[10px] text-[#718096]">Assigned Unit</div>
                <div className="font-mono font-bold text-[#172033] text-sm mt-0.5">
                  {activeDispatch.unitId}
                </div>
                <div className="text-[10px] text-[#526176]">
                  from {activeDispatch.startLocationId}
                </div>
              </div>

              <div className="bg-white border border-[#DCE3EC] rounded-lg p-2.5 shadow-xs">
                <div className="text-[10px] text-[#718096]">Target Incident</div>
                <div className="font-mono font-bold text-[#DC2626] text-sm mt-0.5">
                  {activeDispatch.incidentId}
                </div>
                <div className="text-[10px] text-[#526176]">
                  at {activeDispatch.targetLocationId}
                </div>
              </div>

              <div className="bg-white border border-[#DCE3EC] rounded-lg p-2.5 shadow-xs">
                <div className="text-[10px] text-[#718096]">Estimated Travel Time</div>
                <div className="font-mono font-bold text-[#15803D] text-sm mt-0.5">
                  {activeDispatch.travelTimeMinutes?.toFixed(2)} min
                </div>
                <div className="text-[10px] text-[#526176]">
                  Distance: {activeDispatch.route?.totalDistanceKm?.toFixed(1) ?? '--'} km
                </div>
              </div>

              <div className="bg-white border border-[#DCE3EC] rounded-lg p-2.5 shadow-xs">
                <div className="text-[10px] text-[#718096]">Dijkstra Workload</div>
                <div className="font-mono font-bold text-[#3563D9] text-sm mt-0.5">
                  {activeDispatch.route?.nodesSettled ?? 0} nodes settled
                </div>
                <div className="text-[10px] text-[#526176]">
                  {activeDispatch.route?.relaxations ?? 0} edge relaxations
                </div>
              </div>
            </div>

            {/* Calculated Ordered Route Path */}
            <div className="bg-white border border-[#DCE3EC] rounded-lg p-3 space-y-1.5 shadow-xs">
              <div className="flex items-center justify-between text-xs text-[#526176]">
                <span className="font-medium">Calculated Route Path:</span>
                <span className="text-[10px] font-mono text-[#3563D9] font-semibold">
                  {activeDispatch.route?.path?.length ?? 0} Intersections
                </span>
              </div>
              <div className="flex items-center gap-1.5 flex-wrap font-mono text-xs">
                {activeDispatch.route?.path && activeDispatch.route.path.length > 0 ? (
                  activeDispatch.route.path.map((nodeId, idx, arr) => (
                    <span key={`${nodeId}-${idx}`} className="flex items-center">
                      <span
                        className={`px-2 py-0.5 rounded text-[11px] font-bold border ${
                          idx === 0
                            ? 'bg-emerald-50 text-emerald-800 border-emerald-200'
                            : idx === arr.length - 1
                            ? 'bg-red-50 text-red-800 border-red-200'
                            : 'bg-[#F8FAFC] text-[#172033] border-[#DCE3EC]'
                        }`}
                      >
                        {nodeId}
                      </span>
                      {idx < arr.length - 1 && (
                        <span className="text-[#94A3B8] mx-1">→</span>
                      )}
                    </span>
                  ))
                ) : (
                  <span className="text-[#718096] italic">No route nodes recorded</span>
                )}
              </div>
            </div>

            {/* Decision Rationale */}
            {activeDispatch.decisionRationale && (
              <div className="p-3 rounded-lg bg-blue-50/70 border border-blue-200 text-xs text-blue-950">
                <div className="font-semibold text-blue-900 text-[11px] uppercase tracking-wider mb-1">
                  Engine Assignment Rationale
                </div>
                <p className="leading-relaxed font-sans text-[#172033]">
                  {activeDispatch.decisionRationale}
                </p>
              </div>
            )}
          </div>
        ) : (
          <div className="p-6 text-center rounded-xl bg-[#F8FAFC] border border-[#DCE3EC] text-xs text-[#718096]">
            <p className="font-medium text-[#526176]">No dispatch currently inspected.</p>
            <p className="text-[11px] text-[#718096] mt-1">
              Click &quot;Dispatch Next&quot; or select any incident to calculate optimal routes and view dispatch decisions here.
            </p>
          </div>
        )}

        {/* Recent Dispatches Table */}
        <div className="space-y-2">
          <div className="text-xs font-bold text-[#172033] uppercase tracking-wider flex items-center justify-between">
            <span>Recent Dispatches Log ({dispatches.length})</span>
          </div>

          {dispatches.length === 0 ? (
            <div className="text-center py-6 text-[#718096] text-xs bg-[#F8FAFC] rounded-xl border border-[#DCE3EC]">
              No recent dispatches executed yet.
            </div>
          ) : (
            <div className="overflow-x-auto max-h-[220px] overflow-y-auto rounded-xl border border-[#DCE3EC]">
              <table className="w-full text-left text-xs border-collapse">
                <thead className="bg-[#F8FAFC] text-[11px] font-semibold text-[#526176] uppercase tracking-wider border-b border-[#DCE3EC] sticky top-0 z-10">
                  <tr>
                    <th className="py-2 px-3">Dispatch ID</th>
                    <th className="py-2 px-3">Incident</th>
                    <th className="py-2 px-3">Unit</th>
                    <th className="py-2 px-3">Travel Time</th>
                    <th className="py-2 px-3">Status</th>
                    <th className="py-2 px-3 text-right">Action</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-[#E2E8F0] font-sans">
                  {dispatches.map((disp) => {
                    const isSelected = activeDispatch?.id === disp.id;
                    return (
                      <tr
                        key={disp.id}
                        onClick={() => onSelectDispatch(disp)}
                        className={`cursor-pointer transition-colors ${
                          isSelected ? 'bg-blue-50/70' : 'hover:bg-slate-50'
                        }`}
                      >
                        <td className="py-2 px-3 font-mono font-bold text-[#172033]">
                          {disp.id}
                        </td>
                        <td className="py-2 px-3 font-mono text-[#DC2626]">
                          {disp.incidentId}
                        </td>
                        <td className="py-2 px-3 font-mono text-[#15803D]">
                          {disp.unitId}
                        </td>
                        <td className="py-2 px-3 font-mono text-[#526176]">
                          {disp.travelTimeMinutes?.toFixed(1)}m
                        </td>
                        <td className="py-2 px-3">
                          <span
                            className={`px-1.5 py-0.2 rounded text-[10px] font-bold border ${
                              disp.status === 'ACTIVE'
                                ? 'bg-amber-50 text-amber-800 border-amber-200'
                                : 'bg-emerald-50 text-emerald-800 border-emerald-200'
                            }`}
                          >
                            {disp.status}
                          </span>
                        </td>
                        <td className="py-2 px-3 text-right" onClick={(e) => e.stopPropagation()}>
                          {disp.status === 'ACTIVE' && (
                            <button
                              type="button"
                              onClick={() => handleComplete(disp.id)}
                              className="px-2 py-0.5 rounded bg-[#15803D] hover:bg-[#126832] text-white text-[10px] font-medium transition-colors cursor-pointer"
                            >
                              Complete
                            </button>
                          )}
                        </td>
                      </tr>
                    );
                  })}
                </tbody>
              </table>
            </div>
          )}
        </div>
      </div>
    </div>
  );
}
