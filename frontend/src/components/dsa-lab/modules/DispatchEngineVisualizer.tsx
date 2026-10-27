'use client';

import React, { useState, useEffect, useCallback, useMemo } from 'react';
import { Incident, EmergencyUnit, DispatchRecord, TraceEvent } from '@/lib/types';
import { api } from '@/lib/api';
import { useTracePlayer } from '@/lib/useTracePlayer';
import TracePlayerControls from '../common/TracePlayerControls';
import ComplexityCard from '../common/ComplexityCard';

export default function DispatchEngineVisualizer() {
  const [incidents, setIncidents] = useState<Incident[]>([]);
  const [units, setUnits] = useState<EmergencyUnit[]>([]);
  const [selectedIncidentId, setSelectedIncidentId] = useState<string>('');
  const [dispatching, setDispatching] = useState(false);
  const [dispatchResult, setDispatchResult] = useState<DispatchRecord | null>(null);
  const [dispatchExplanation, setDispatchExplanation] = useState<string>('');
  const [traceEvents, setTraceEvents] = useState<TraceEvent[]>([]);
  const [error, setError] = useState<string | null>(null);

  // Load current pending incidents and emergency units from backend
  const loadState = useCallback(async () => {
    setError(null);
    try {
      const [incRes, unitRes] = await Promise.all([
        api.getIncidents('PENDING'),
        api.getUnits(),
      ]);

      if (incRes.success && incRes.data) {
        setIncidents(incRes.data.incidents);
        if (incRes.data.incidents.length > 0 && !selectedIncidentId) {
          setSelectedIncidentId(incRes.data.incidents[0].id);
        }
      }

      if (unitRes.success && unitRes.data) {
        setUnits(unitRes.data.units);
      }
    } catch (err: unknown) {
      setError(err instanceof Error ? err.message : 'Failed to load dispatch telemetry data.');
    }
  }, [selectedIncidentId]);

  useEffect(() => {
    let cancelled = false;
    async function init() {
      try {
        const [incRes, unitRes] = await Promise.all([
          api.getIncidents('PENDING'),
          api.getUnits(),
        ]);

        if (!cancelled && incRes.success && incRes.data) {
          setIncidents(incRes.data.incidents);
          if (incRes.data.incidents.length > 0) {
            setSelectedIncidentId((prev) => prev || incRes.data!.incidents[0].id);
          }
        }

        if (!cancelled && unitRes.success && unitRes.data) {
          setUnits(unitRes.data.units);
        }
      } catch {
        // ignore
      }
    }
    init();
    return () => {
      cancelled = true;
    };
  }, []);

  // Execute Dispatch with trace collection
  const handleDispatch = useCallback(async () => {
    if (!selectedIncidentId) {
      setError('Please select a pending incident to dispatch.');
      return;
    }

    setDispatching(true);
    setError(null);
    try {
      const res = await api.runDispatchDemo(selectedIncidentId);
      if (res.success && res.data) {
        setDispatchResult(res.data.dispatch);
        setDispatchExplanation(res.data.explanation);
        setTraceEvents((res.trace as TraceEvent[]) || []);
        // Refresh unit availability
        loadState();
      } else {
        setError(res.error?.message || 'Failed to dispatch unit for selected incident.');
      }
    } catch (err: unknown) {
      setError(err instanceof Error ? err.message : 'Dispatch error');
    } finally {
      setDispatching(false);
    }
  }, [selectedIncidentId, loadState]);

  // Trace player controller
  const player = useTracePlayer(traceEvents, { defaultSpeedMs: 800 });
  const { currentEvent } = player;

  // Derive candidate evaluations strictly up to current playback step
  const candidateStatusMap = useMemo(() => {
    const map = new Map<
      string,
      { status: 'eligible' | 'rejected' | 'selected'; reason?: string; travelTime?: number }
    >();

    for (const ev of player.visibleEvents) {
      if (ev.operation === 'candidate_rejected' && ev.entityId) {
        const reason = (ev.metadata?.reason as string) || ev.explanation;
        map.set(ev.entityId, { status: 'rejected', reason });
      } else if (ev.operation === 'candidate_route_evaluated' && ev.entityId) {
        map.set(ev.entityId, { status: 'eligible', travelTime: ev.cost });
      } else if (ev.operation === 'candidate_selected' && ev.entityId) {
        map.set(ev.entityId, { status: 'selected', travelTime: ev.cost });
      }
    }

    return map;
  }, [player.visibleEvents]);

  // Check if winner has been selected in current trace steps
  const winnerSelectedEvent = useMemo(() => {
    return player.visibleEvents.find((e) => e.operation === 'candidate_selected') || null;
  }, [player.visibleEvents]);

  const activeHighlightedUnitId = currentEvent?.entityId || null;

  return (
    <div className="space-y-6">
      {/* Parameter Controls & Trigger Bar */}
      <div className="bg-white rounded-xl border border-[#DCE3EC] p-4 shadow-sm space-y-4">
        <div className="flex flex-wrap items-center justify-between gap-3">
          <div>
            <h2 className="text-base font-bold text-[#172033] flex items-center space-x-2">
              <span className="w-2.5 h-2.5 rounded-full bg-[#087E8B]" />
              <span>Multi-Agency Emergency Dispatch Pipeline</span>
            </h2>
            <p className="text-xs text-[#526176] mt-0.5">
              Integrates Max-Heap incident triage, multi-agency capability filtering, and Dijkstra shortest-path calculations.
            </p>
          </div>

          <div className="flex flex-wrap items-center gap-3">
            <div className="flex items-center space-x-2 text-xs">
              <label htmlFor="incident-select" className="text-[#526176] font-semibold">
                Pending Incident:
              </label>
              <select
                id="incident-select"
                aria-label="Select pending incident to dispatch"
                value={selectedIncidentId}
                onChange={(e) => setSelectedIncidentId(e.target.value)}
                className="bg-white border border-[#DCE3EC] rounded-lg px-2.5 py-1.5 text-[#172033] font-mono text-xs focus:outline-none focus:ring-2 focus:ring-[#3563D9]/20"
              >
                {incidents.length === 0 ? (
                  <option value="">No pending incidents</option>
                ) : (
                  incidents.map((inc) => (
                    <option key={inc.id} value={inc.id}>
                      {inc.id} — Sev {inc.severity} ({inc.type} @ {inc.locationId})
                    </option>
                  ))
                )}
              </select>
            </div>

            <button
              type="button"
              onClick={handleDispatch}
              disabled={dispatching || !selectedIncidentId}
              className="px-4 py-2 rounded-lg bg-[#3563D9] hover:bg-[#284eb3] disabled:opacity-50 text-white text-xs font-semibold shadow-xs transition-colors flex items-center space-x-1.5 cursor-pointer"
            >
              <span>{dispatching ? 'Evaluating Candidates...' : 'Run Dispatch Decision'}</span>
            </button>
          </div>
        </div>

        {error && (
          <div className="p-3 rounded-lg bg-red-50 border border-red-200 text-xs text-red-700">
            {error}
          </div>
        )}
      </div>

      {/* Visualizations: Multi-Stage Dispatch Pipeline */}
      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        {/* Stage 1: Incident Under Evaluation */}
        <div className="bg-white rounded-xl border border-[#DCE3EC] p-4 space-y-3 shadow-sm">
          <div className="flex items-center justify-between border-b border-[#DCE3EC] pb-2">
            <span className="text-xs font-bold uppercase tracking-wider text-[#172033]">
              Stage 1: Incident Triage
            </span>
            <span className="text-[11px] text-[#087E8B] font-mono font-semibold">Max-Heap Top</span>
          </div>

          {selectedIncidentId && incidents.find((i) => i.id === selectedIncidentId) ? (
            (() => {
              const inc = incidents.find((i) => i.id === selectedIncidentId)!;
              return (
                <div className="space-y-3 text-xs">
                  <div className="p-3 rounded-lg bg-[#F8FAFC] border border-[#E2E8F0] space-y-2">
                    <div className="flex items-center justify-between">
                      <span className="font-mono font-bold text-[#172033]">{inc.id}</span>
                      <span className="px-2 py-0.5 rounded text-[10px] font-bold bg-red-100 text-red-800 border border-red-200">
                        {inc.severityLabel} (Sev {inc.severity})
                      </span>
                    </div>

                    <div className="grid grid-cols-2 gap-2 text-[11px] text-[#526176] font-mono">
                      <div>
                        <span className="text-[#718096]">Service:</span> {inc.type}
                      </div>
                      <div>
                        <span className="text-[#718096]">Scene:</span> {inc.locationId}
                      </div>
                      <div>
                        <span className="text-[#718096]">Arrival:</span> #{inc.arrivalSequence}
                      </div>
                      <div>
                        <span className="text-[#718096]">Priority:</span> {inc.computedPriority}
                      </div>
                    </div>

                    <p className="text-[11px] text-[#526176] font-sans italic pt-2 border-t border-[#E2E8F0]">
                      &quot;{inc.description}&quot;
                    </p>
                  </div>

                  <div className="p-3 rounded-lg bg-blue-50 border border-blue-200 text-[11px] text-blue-900 leading-relaxed">
                    <strong>Triage Invariant:</strong> The incident with maximum severity is extracted from root index 0 of the binary max-heap in O(1) peek time.
                  </div>
                </div>
              );
            })()
          ) : (
            <div className="p-4 rounded-lg bg-[#F8FAFC] text-[#718096] text-xs italic text-center">
              No pending incident selected.
            </div>
          )}
        </div>

        {/* Stage 2 & 3: Fleet Candidate Evaluation Cards */}
        <div className="lg:col-span-2 bg-white rounded-xl border border-[#DCE3EC] p-4 space-y-3 shadow-sm">
          <div className="flex items-center justify-between border-b border-[#DCE3EC] pb-2">
            <span className="text-xs font-bold uppercase tracking-wider text-[#172033]">
              Stages 2 & 3: Capability Filter & Dijkstra Travel Times
            </span>
            <span className="text-[11px] text-[#526176] font-mono">{units.length} total units</span>
          </div>

          <div className="grid grid-cols-1 sm:grid-cols-2 gap-3 max-h-[380px] overflow-y-auto pr-1">
            {units.map((unit) => {
              const evalInfo = candidateStatusMap.get(unit.id);
              const isSelected = evalInfo?.status === 'selected';
              const isRejected = evalInfo?.status === 'rejected';
              const isEligible = evalInfo?.status === 'eligible';
              const isHighlighted = activeHighlightedUnitId === unit.id;

              return (
                <div
                  key={unit.id}
                  className={`p-3 rounded-lg border transition-all text-xs ${
                    isSelected
                      ? 'bg-emerald-50 border-emerald-500 shadow-sm ring-1 ring-emerald-400'
                      : isHighlighted
                      ? 'bg-sky-50 border-sky-400 ring-1 ring-sky-300'
                      : isRejected
                      ? 'bg-[#F8FAFC] border-slate-200 opacity-65'
                      : isEligible
                      ? 'bg-sky-50/40 border-sky-200'
                      : 'bg-white border-[#DCE3EC] shadow-xs'
                  }`}
                >
                  <div className="flex items-center justify-between">
                    <div className="flex items-center space-x-1.5 font-mono">
                      <span className="font-bold text-[#172033]">{unit.callSign}</span>
                      <span className="text-[10px] text-[#718096]">({unit.agency})</span>
                    </div>

                    <span
                      className={`px-2 py-0.5 rounded text-[10px] font-bold border ${
                        isSelected
                          ? 'bg-emerald-100 text-emerald-800 border-emerald-300'
                          : isRejected
                          ? 'bg-red-50 text-red-700 border-red-200'
                          : isEligible
                          ? 'bg-sky-100 text-sky-800 border-sky-200'
                          : unit.status === 'AVAILABLE'
                          ? 'bg-slate-100 text-slate-700 border-slate-200'
                          : 'bg-slate-100 text-slate-500 border-slate-200'
                      }`}
                    >
                      {isSelected ? 'SELECTED 🏆' : isRejected ? 'INELIGIBLE' : isEligible ? 'ELIGIBLE' : unit.status}
                    </span>
                  </div>

                  <div className="mt-2 grid grid-cols-2 gap-1 text-[11px] font-mono text-[#526176]">
                    <div>
                      <span className="text-[#718096]">Location:</span> {unit.locationId}
                    </div>
                    <div>
                      <span className="text-[#718096]">Service:</span> {unit.serviceType}
                    </div>
                  </div>

                  {/* Telemetry Evaluation Results */}
                  {evalInfo?.travelTime !== undefined && (
                    <div className="mt-2 pt-1.5 border-t border-[#E2E8F0] flex items-center justify-between text-[11px] font-mono">
                      <span className="text-[#526176]">Dijkstra Travel Time:</span>
                      <span
                        className={`font-bold ${
                          isSelected ? 'text-[#15803D]' : 'text-[#3563D9]'
                        }`}
                      >
                        {evalInfo.travelTime.toFixed(2)} min
                      </span>
                    </div>
                  )}

                  {evalInfo?.reason && (
                    <div className="mt-2 pt-1.5 border-t border-[#E2E8F0] text-[10px] text-red-700 font-mono">
                      <span className="font-semibold">Rejected:</span> {evalInfo.reason}
                    </div>
                  )}
                </div>
              );
            })}
          </div>
        </div>
      </div>

      {/* Decision Summary Card (visible once selection step is reached) */}
      {(winnerSelectedEvent || player.isAtEnd) && dispatchResult && (
        <div className="p-4 rounded-xl border border-emerald-300 bg-emerald-50/70 space-y-2 shadow-xs transition-all duration-300">
          <div className="flex items-center justify-between">
            <span className="text-xs font-bold uppercase tracking-wider text-emerald-900 flex items-center space-x-1.5">
              <span>Optimal Dispatch Assignment Created</span>
            </span>
            <span className="text-xs font-mono text-[#526176]">ID: {dispatchResult.id}</span>
          </div>
          <p className="text-xs text-[#172033] leading-relaxed">
            {dispatchExplanation || dispatchResult.decisionRationale}
          </p>
          <div className="flex flex-wrap gap-4 text-xs font-mono text-[#526176] pt-1.5 border-t border-emerald-200">
            <span>
              Unit: <strong className="text-emerald-900">{dispatchResult.unitId}</strong>
            </span>
            <span>
              Target: <strong className="text-emerald-900">{dispatchResult.targetLocationId}</strong>
            </span>
            <span>
              Optimal ETA:{' '}
              <strong className="text-[#3563D9]">
                {dispatchResult.travelTimeMinutes.toFixed(2)} min
              </strong>
            </span>
            <span>
              Path:{' '}
              <strong className="text-[#172033]">
                {dispatchResult.route?.path?.join(' ➔ ') || 'Calculated'}
              </strong>
            </span>
          </div>
        </div>
      )}

      {/* Trace Player Reusable Controller */}
      {traceEvents.length > 0 && (
        <TracePlayerControls
          player={player}
          title="Dispatch Pipeline Multi-Stage Telemetry"
          sourceFile="backend/src/simulation/dispatch_service.cpp"
        />
      )}

      {/* Educational Viva Reference */}
      <ComplexityCard
        algorithmName="Multi-Agency Dispatch Decision Engine"
        problemSolved="Selecting the optimal emergency vehicle to respond to a prioritized incident based on service agency capability, availability, and road network travel time."
        whyEmergencyGridUsesIt="Dispatches cannot simply pick the geographically closest unit (Euclidean distance ignores road closures and congestion) or the first available unit (service mismatch causes failed responses). The engine combines multi-attribute filtering with Dijkstra shortest path computation."
        howItWorks="1. Triage: Extracts highest-priority incident from Binary Max-Heap (O(log N)). 2. Candidate Filtering: Filters fleet for units matching required service type and available status (O(U)). 3. Shortest Path Evaluation: Runs Dijkstra routing from each eligible candidate's current intersection to scene location (O(K * (V + E) log V)). 4. Selection: Chooses unit with minimum travel time, resolving ties via unit ID."
        metrics={[
          {
            operation: "Incident Priority Extraction",
            averageTime: "O(log N)",
            worstTime: "O(log N)",
            space: "O(1)",
            notes: "Extract-max from binary heap; restores max-heap invariant via sift-down.",
          },
          {
            operation: "Candidate Eligibility Filter",
            averageTime: "O(U)",
            worstTime: "O(U)",
            space: "O(K)",
            notes: "Iterates through total fleet U to filter K eligible candidate units.",
          },
          {
            operation: "Travel Time Computations",
            averageTime: "O(K * (V + E) log V)",
            worstTime: "O(K * (V + E) log V)",
            space: "O(V + E)",
            notes: "Executes K separate single-source Dijkstra shortest path computations.",
          },
          {
            operation: "Total Dispatch Decision",
            averageTime: "O(log N + K(V+E)log V)",
            worstTime: "O(log N + K(V+E)log V)",
            space: "O(U + V + E)",
            notes: "Exact complexity: Priority queue extraction + K candidate Dijkstra route queries.",
          },
        ]}
        strengths={[
          "True travel-time optimality: Considers actual live traffic congestion and street closures.",
          "Agency and capability safety: Eliminates invalid dispatches (e.g. sending police to fire incident).",
          "Deterministic decision rationale recorded in telemetry log for post-incident auditing.",
        ]}
        limitations={[
          "Executes K separate Dijkstra computations; for very large fleets, multi-source Dijkstra or precomputed contraction hierarchies would scale better.",
          "Single-unit greedy assignment does not perform global fleet batch optimization (e.g., Hungarian algorithm).",
        ]}
        cppHeader="backend/include/simulation/dispatch_service.hpp"
        cppSource="backend/src/simulation/dispatch_service.cpp"
        testFile="backend/tests/test_dispatch_service.cpp"
      />
    </div>
  );
}
