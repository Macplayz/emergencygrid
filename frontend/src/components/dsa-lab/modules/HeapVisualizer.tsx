'use client';

import React, { useState, useEffect, useCallback, useMemo } from 'react';
import { HeapSnapshot, TraceEvent } from '@/lib/types';
import { api } from '@/lib/api';
import { useTracePlayer } from '@/lib/useTracePlayer';
import TracePlayerControls from '../common/TracePlayerControls';
import ComplexityCard from '../common/ComplexityCard';

export default function HeapVisualizer() {
  const [snapshot, setSnapshot] = useState<HeapSnapshot | null>(null);
  const [loading, setLoading] = useState(false);
  const [traceEvents, setTraceEvents] = useState<TraceEvent[]>([]);
  const [error, setError] = useState<string | null>(null);
  const [mode, setMode] = useState<'sandbox' | 'live'>('sandbox');
  const [insertSeverity, setInsertSeverity] = useState<number>(4);

  // Load initial sandbox sample sequence
  const runSandboxDemo = useCallback(
    async (action: 'sample_sequence' | 'extract_max' | 'insert') => {
      setLoading(true);
      setError(null);
      try {
        const res = await api.runHeapDemo(action, {
          severity: insertSeverity,
          id: `INC-LAB-${Math.floor(100 + Math.random() * 900)}`,
          locationId: 'J5',
        });
        if (res.success && res.data) {
          setSnapshot(res.data.snapshot);
          setTraceEvents((res.trace as TraceEvent[]) || []);
        } else {
          setError(res.error?.message || 'Failed to execute heap demo operation.');
        }
      } catch (err: unknown) {
        setError(err instanceof Error ? err.message : 'Demo execution error');
      } finally {
        setLoading(false);
      }
    },
    [insertSeverity]
  );

  // Fetch live heap snapshot from real operation mode
  const fetchLiveSnapshot = useCallback(async () => {
    setLoading(true);
    setError(null);
    try {
      const res = await api.getHeapSnapshot();
      if (res.success && res.data) {
        setSnapshot(res.data);
        setTraceEvents([]); // Read-only snapshot has no execution trace
      } else {
        setError(res.error?.message || 'Failed to inspect live heap state.');
      }
    } catch (err: unknown) {
      setError(err instanceof Error ? err.message : 'Fetch error');
    } finally {
      setLoading(false);
    }
  }, []);

  // Run initial sandbox sample on mount
  useEffect(() => {
    let cancelled = false;
    async function initSandbox() {
      try {
        const res = await api.runHeapDemo('sample_sequence');
        if (!cancelled && res.success && res.data) {
          setSnapshot(res.data.snapshot);
          setTraceEvents((res.trace as TraceEvent[]) || []);
        }
      } catch {
        // ignore
      }
    }
    initSandbox();
    return () => {
      cancelled = true;
    };
  }, []);

  // Trace player controller
  const player = useTracePlayer(traceEvents, { defaultSpeedMs: 700 });
  const { currentEvent } = player;

  // Highlight elements referenced by the current trace step
  const highlightedIndices = useMemo(() => {
    const s = new Set<number>();
    if (!currentEvent || !snapshot?.elements) return s;

    // Check entityId or metadata
    const entityId = currentEvent.entityId;
    snapshot.elements.forEach((el, idx) => {
      if (el.id === entityId) s.add(idx);
    });

    if (currentEvent.metadata) {
      if (typeof currentEvent.metadata.candidateIndex === 'number') {
        s.add(currentEvent.metadata.candidateIndex);
      }
      if (typeof currentEvent.metadata.currentLargestIndex === 'number') {
        s.add(currentEvent.metadata.currentLargestIndex);
      }
      if (typeof currentEvent.metadata.parentIndex === 'number') {
        s.add(currentEvent.metadata.parentIndex);
      }
      if (typeof currentEvent.metadata.currentIndex === 'number') {
        s.add(currentEvent.metadata.currentIndex);
      }
      if (typeof currentEvent.metadata.fromIndex === 'number') {
        s.add(currentEvent.metadata.fromIndex);
      }
      if (typeof currentEvent.metadata.toIndex === 'number') {
        s.add(currentEvent.metadata.toIndex);
      }
    }
    return s;
  }, [currentEvent, snapshot]);

  // Node position calculation for binary tree SVG (levels 0, 1, 2, 3...)
  const nodeLayout = useMemo(() => {
    if (!snapshot?.elements || snapshot.elements.length === 0) return [];

    return snapshot.elements.map((el, i) => {
      const level = Math.floor(Math.log2(i + 1));
      const levelCapacity = Math.pow(2, level);
      const posInLevel = i - (levelCapacity - 1);

      // SVG dimensions: width 540, height 320
      const levelY = 45 + level * 75;
      const slotWidth = 540 / (levelCapacity + 1);
      const x = slotWidth * (posInLevel + 1);

      return {
        ...el,
        x,
        y: levelY,
        level,
      };
    });
  }, [snapshot]);

  const severityColor = (sev: number) => {
    switch (sev) {
      case 4:
        return {
          bg: 'bg-red-50',
          border: 'border-red-200',
          fill: '#DC2626',
          text: 'text-red-700',
          label: 'CRITICAL',
        };
      case 3:
        return {
          bg: 'bg-amber-50',
          border: 'border-amber-200',
          fill: '#D97706',
          text: 'text-amber-800',
          label: 'HIGH',
        };
      case 2:
        return {
          bg: 'bg-indigo-50',
          border: 'border-indigo-200',
          fill: '#4F46E5',
          text: 'text-indigo-700',
          label: 'MEDIUM',
        };
      default:
        return {
          bg: 'bg-slate-50',
          border: 'border-slate-200',
          fill: '#64748B',
          text: 'text-slate-700',
          label: 'LOW',
        };
    }
  };

  return (
    <div className="space-y-6">
      {/* Sandbox / Live Toggle & Operations Bar */}
      <div className="bg-white rounded-xl border border-[#DCE3EC] p-4 shadow-sm space-y-4">
        <div className="flex flex-wrap items-center justify-between gap-3">
          <div>
            <h2 className="text-base font-bold text-[#172033] flex items-center space-x-2">
              <span className="w-2.5 h-2.5 rounded-full bg-[#DC2626]" />
              <span>Incident Priority Queue — Array-Backed Binary Max-Heap</span>
            </h2>
            <p className="text-xs text-[#526176] mt-0.5">
              Strict multi-attribute triage ordering: Severity (4➔1), arrival order sequence (FIFO), and deterministic ID tie-breaking.
            </p>
          </div>

          {/* Mode switch */}
          <div className="flex items-center space-x-1 bg-[#F5F7FA] p-1 rounded-lg border border-[#DCE3EC] text-xs">
            <button
              type="button"
              onClick={() => {
                setMode('sandbox');
                runSandboxDemo('sample_sequence');
              }}
              className={`px-3 py-1.5 rounded-md font-medium transition-colors cursor-pointer ${
                mode === 'sandbox'
                  ? 'bg-white text-[#172033] shadow-xs font-semibold'
                  : 'text-[#526176] hover:text-[#172033]'
              }`}
            >
              Lab Sandbox (Isolated)
            </button>
            <button
              type="button"
              onClick={() => {
                setMode('live');
                fetchLiveSnapshot();
              }}
              className={`px-3 py-1.5 rounded-md font-medium transition-colors cursor-pointer ${
                mode === 'live'
                  ? 'bg-white text-[#172033] shadow-xs font-semibold'
                  : 'text-[#526176] hover:text-[#172033]'
              }`}
            >
              Live Operations State
            </button>
          </div>
        </div>

        {/* Action Controls for Sandbox */}
        {mode === 'sandbox' ? (
          <div className="flex flex-wrap items-center justify-between gap-3 pt-3 border-t border-[#DCE3EC]">
            <div className="flex flex-wrap items-center gap-2">
              <button
                type="button"
                onClick={() => runSandboxDemo('sample_sequence')}
                disabled={loading}
                className="px-3.5 py-1.5 rounded-lg bg-[#F5F7FA] hover:bg-slate-200 text-[#172033] text-xs font-medium border border-[#DCE3EC] transition-colors cursor-pointer disabled:opacity-50"
              >
                Load Sample Triage
              </button>

              <button
                type="button"
                onClick={() => runSandboxDemo('extract_max')}
                disabled={loading || snapshot?.isEmpty}
                className="px-3.5 py-1.5 rounded-lg bg-[#DC2626] hover:bg-[#b91c1c] text-white text-xs font-semibold shadow-xs transition-colors cursor-pointer disabled:opacity-50"
              >
                Extract Max (Sift-Down)
              </button>
            </div>

            <div className="flex items-center space-x-2 text-xs">
              <span className="text-[#526176] font-medium">Insert Severity:</span>
              <select
                aria-label="Select incident severity to insert into heap"
                value={insertSeverity}
                onChange={(e) => setInsertSeverity(parseInt(e.target.value, 10))}
                className="bg-white border border-[#DCE3EC] rounded-lg px-2.5 py-1 text-[#172033] font-mono text-xs focus:outline-none focus:ring-2 focus:ring-[#3563D9]/20"
              >
                <option value={4}>4 — CRITICAL</option>
                <option value={3}>3 — HIGH</option>
                <option value={2}>2 — MEDIUM</option>
                <option value={1}>1 — LOW</option>
              </select>

              <button
                type="button"
                onClick={() => runSandboxDemo('insert')}
                disabled={loading}
                className="px-3.5 py-1 rounded-lg bg-[#3563D9] hover:bg-[#284eb3] text-white text-xs font-medium shadow-xs cursor-pointer disabled:opacity-50"
              >
                + Insert (Sift-Up)
              </button>
            </div>
          </div>
        ) : (
          <div className="flex items-center justify-between text-xs text-[#526176] pt-1">
            <span>Viewing real incidents currently queued in the active operational triage heap.</span>
            <button
              type="button"
              onClick={fetchLiveSnapshot}
              disabled={loading}
              className="px-3 py-1.5 rounded-lg bg-[#F5F7FA] hover:bg-slate-200 text-[#172033] font-medium border border-[#DCE3EC] cursor-pointer"
            >
              Refresh Snapshot
            </button>
          </div>
        )}

        {error && (
          <div className="p-3 rounded-lg bg-red-50 border border-red-200 text-xs text-red-700">
            {error}
          </div>
        )}
      </div>

      {/* Visualizations: Dual View (Binary Tree + Flat Array Buffer) */}
      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        {/* Tree SVG Visualization */}
        <div className="lg:col-span-2 bg-white rounded-xl border border-[#DCE3EC] p-4 flex flex-col shadow-sm relative min-h-[360px] overflow-hidden">
          <div className="flex flex-wrap items-center justify-between gap-2 border-b border-[#DCE3EC] pb-3 mb-2 text-xs">
            <div className="flex items-center space-x-3 font-mono text-[#526176]">
              <span>
                Heap Size: <strong className="text-[#172033]">{snapshot?.size || 0}</strong>
              </span>
              <span className="text-[#DCE3EC]">|</span>
              <span>Root (Max):</span>
              {snapshot?.elements && snapshot.elements.length > 0 ? (
                <span className="px-2 py-0.5 rounded bg-red-50 text-red-700 border border-red-200 font-bold">
                  {snapshot.elements[0].id} (Sev {snapshot.elements[0].severity})
                </span>
              ) : (
                <span className="text-[#718096] italic">Empty</span>
              )}
            </div>

            <div className="flex items-center space-x-3 text-[11px] text-[#526176]">
              <span className="flex items-center space-x-1">
                <span className="w-2.5 h-2.5 rounded-full bg-[#DC2626]" />
                <span>Critical (4)</span>
              </span>
              <span className="flex items-center space-x-1">
                <span className="w-2.5 h-2.5 rounded-full bg-[#D97706]" />
                <span>High (3)</span>
              </span>
              <span className="flex items-center space-x-1">
                <span className="w-2.5 h-2.5 rounded-full bg-[#4F46E5]" />
                <span>Medium (2)</span>
              </span>
            </div>
          </div>

          <div className="flex-1 w-full flex items-center justify-center bg-[#F8FAFC] rounded-lg border border-[#E2E8F0] p-2 relative">
            {snapshot?.isEmpty || !snapshot?.elements || snapshot.elements.length === 0 ? (
              <div className="text-xs text-[#718096] italic">
                Heap is currently empty. Insert items above to populate tree.
              </div>
            ) : (
              <svg viewBox="0 0 540 320" className="w-full h-auto max-h-[400px]">
                {/* Draw parent-child connecting lines */}
                {nodeLayout.map((node, i) => {
                  if (i === 0) return null;
                  const parentIdx = Math.floor((i - 1) / 2);
                  const parent = nodeLayout[parentIdx];
                  if (!parent) return null;

                  const isComparing =
                    currentEvent?.operation === 'compare' &&
                    (highlightedIndices.has(i) || highlightedIndices.has(parentIdx));

                  const isSwapping =
                    currentEvent?.operation === 'swap' &&
                    (highlightedIndices.has(i) || highlightedIndices.has(parentIdx));

                  return (
                    <line
                      key={`edge-${i}`}
                      x1={parent.x}
                      y1={parent.y}
                      x2={node.x}
                      y2={node.y}
                      stroke={isSwapping ? '#DC2626' : isComparing ? '#D97706' : '#CBD5E1'}
                      strokeWidth={isSwapping || isComparing ? 2.5 : 1.5}
                      strokeDasharray={isComparing ? '3,3' : undefined}
                      className="transition-colors duration-200"
                    />
                  );
                })}

                {/* Draw Nodes */}
                {nodeLayout.map((node, i) => {
                  const isRoot = i === 0;
                  const isHighlighted = highlightedIndices.has(i);
                  const sev = severityColor(node.severity);

                  return (
                    <g key={node.id} className="cursor-pointer transition-all duration-300">
                      {isHighlighted && (
                        <circle
                          cx={node.x}
                          cy={node.y}
                          r={24}
                          fill="#DC2626"
                          opacity={0.15}
                        />
                      )}
                      <circle
                        cx={node.x}
                        cy={node.y}
                        r={isRoot ? 20 : 17}
                        fill={isHighlighted ? '#DC2626' : sev.fill}
                        stroke="#FFFFFF"
                        strokeWidth={isRoot || isHighlighted ? 2.5 : 2}
                        className="transition-colors duration-300 shadow-sm"
                      />
                      <text
                        x={node.x}
                        y={node.y - 2}
                        fill="#FFFFFF"
                        fontSize="9"
                        fontWeight="bold"
                        fontFamily="monospace"
                        textAnchor="middle"
                        className="pointer-events-none select-none"
                      >
                        {node.id}
                      </text>
                      <text
                        x={node.x}
                        y={node.y + 9}
                        fill="#FFFFFF"
                        fontSize="8"
                        fontWeight="bold"
                        fontFamily="monospace"
                        textAnchor="middle"
                        className="pointer-events-none select-none opacity-90"
                      >
                        [i={i}]
                      </text>
                    </g>
                  );
                })}
              </svg>
            )}
          </div>
        </div>

        {/* Array Buffer Layout (0-based contiguous storage) */}
        <div className="bg-white rounded-xl border border-[#DCE3EC] p-4 flex flex-col space-y-3 shadow-sm">
          <div className="flex items-center justify-between border-b border-[#DCE3EC] pb-2">
            <div>
              <span className="text-xs font-bold uppercase tracking-wider text-[#172033]">
                Array Buffer [i]
              </span>
              <p className="text-[10px] text-[#718096] font-mono">
                Parent: (i-1)/2 | L: 2i+1 | R: 2i+2
              </p>
            </div>
            <span className="text-[11px] font-mono text-[#526176]">
              Capacity: {snapshot?.elements?.length || 0}
            </span>
          </div>

          <div className="flex-1 overflow-y-auto max-h-[360px] space-y-2 pr-1 font-mono text-xs">
            {snapshot?.elements?.map((el, idx) => {
              const isRoot = idx === 0;
              const isHighlighted = highlightedIndices.has(idx);
              const sev = severityColor(el.severity);

              return (
                <div
                  key={el.id}
                  className={`p-3 rounded-lg border transition-all ${
                    isHighlighted
                      ? 'bg-red-50/80 border-red-300 ring-1 ring-red-400'
                      : 'bg-[#F8FAFC] border-[#E2E8F0]'
                  }`}
                >
                  <div className="flex items-center justify-between">
                    <div className="flex items-center space-x-2">
                      <span className="px-1.5 py-0.5 rounded bg-white border border-[#DCE3EC] text-[#526176] font-bold text-[10px]">
                        [{idx}]
                      </span>
                      <span className="font-bold text-[#172033]">{el.id}</span>
                      {isRoot && (
                        <span className="px-1.5 py-0.2 rounded bg-red-100 text-red-800 border border-red-200 text-[9px] font-bold">
                          ROOT (MAX)
                        </span>
                      )}
                    </div>

                    <span
                      className={`px-2 py-0.5 rounded text-[10px] font-bold border ${sev.bg} ${sev.text} ${sev.border}`}
                    >
                      {sev.label} ({el.severity})
                    </span>
                  </div>

                  <div className="mt-2 grid grid-cols-3 gap-1 text-[10px] text-[#526176] pt-2 border-t border-[#E2E8F0]">
                    <div>
                      <span className="text-[#718096]">Parent:</span>{' '}
                      {el.parentIndex !== null ? `[${el.parentIndex}]` : 'None'}
                    </div>
                    <div>
                      <span className="text-[#718096]">Left:</span>{' '}
                      {el.leftChildIndex !== null ? `[${el.leftChildIndex}]` : '—'}
                    </div>
                    <div>
                      <span className="text-[#718096]">Right:</span>{' '}
                      {el.rightChildIndex !== null ? `[${el.rightChildIndex}]` : '—'}
                    </div>
                  </div>
                </div>
              );
            })}
          </div>
        </div>
      </div>

      {/* Trace Player Reusable Controller */}
      {traceEvents.length > 0 && (
        <TracePlayerControls
          player={player}
          title="Binary Max-Heap Sift Telemetry"
          sourceFile="backend/include/dsa/binary_heap.hpp"
        />
      )}

      {/* Educational Viva Reference */}
      <ComplexityCard
        algorithmName="Array-Backed Binary Max-Heap"
        problemSolved="Maintaining dynamic priority ordering of emergency incidents so the most critical and time-sensitive emergency is always serviced next."
        whyEmergencyGridUsesIt="In emergency triage, incidents arrive asynchronously with varying severity (Critical 4 to Low 1). An array-backed max-heap guarantees O(1) instantaneous peek of the highest-priority incident without full array sorting."
        howItWorks="Stored as a contiguous zero-indexed vector where node i has children at 2i+1 and 2i+2, and parent at (i-1)/2. Maintains max-heap invariant (parent has higher priority than children) using heapifyUp (swapping with parent on insert) and heapifyDown (swapping with largest child on extraction)."
        metrics={[
          {
            operation: "Peek Max Priority",
            averageTime: "O(1)",
            worstTime: "O(1)",
            space: "O(1)",
            notes: "Direct array index 0 lookup of root incident.",
          },
          {
            operation: "Insert (Heapify-Up / Sift-Up)",
            averageTime: "O(log N)",
            worstTime: "O(log N)",
            space: "O(1)",
            notes: "Appends to end of vector, compares and swaps up tree height h = floor(log2 N).",
          },
          {
            operation: "Extract-Max (Heapify-Down / Sift-Down)",
            averageTime: "O(log N)",
            worstTime: "O(log N)",
            space: "O(1)",
            notes: "Replaces root with last element, bubbles down by comparing children.",
          },
        ]}
        strengths={[
          "Contiguous cache locality with zero pointer overhead compared to pointer-based trees.",
          "O(1) peek time enables zero-delay dispatch triage under high incoming call volume.",
          "Strict deterministic tie-breaking prevents priority starvation among equal-severity incidents.",
        ]}
        limitations={[
          "Arbitrary key search or deletion takes O(N) linear time without an auxiliary inverted index.",
          "Not a sorted list; only partial ordering is maintained at each level.",
        ]}
        cppHeader="backend/include/dsa/binary_heap.hpp"
        cppSource="backend/src/dsa/binary_heap.cpp"
        testFile="backend/tests/test_binary_heap.cpp"
      />
    </div>
  );
}
