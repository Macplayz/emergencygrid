'use client';

import React, { useState, useEffect, useCallback, useMemo } from 'react';
import { HashTableStats, TraceEvent } from '@/lib/types';
import { api } from '@/lib/api';
import { useTracePlayer } from '@/lib/useTracePlayer';
import TracePlayerControls from '../common/TracePlayerControls';
import ComplexityCard from '../common/ComplexityCard';

export default function HashTableVisualizer() {
  const [stats, setStats] = useState<HashTableStats | null>(null);
  const [loading, setLoading] = useState(false);
  const [traceEvents, setTraceEvents] = useState<TraceEvent[]>([]);
  const [error, setError] = useState<string | null>(null);
  const [mode, setMode] = useState<'sandbox' | 'live'>('sandbox');
  const [lookupKey, setLookupKey] = useState<string>('INC-101');

  // Run isolated Hash Table sandbox demonstration
  const runSandboxDemo = useCallback(
    async (action: 'lookup' | 'collision' | 'rehash', customKey?: string) => {
      setLoading(true);
      setError(null);
      try {
        const res = await api.runHashTableDemo(action, customKey || lookupKey);
        if (res.success && res.data) {
          setStats(res.data.snapshot);
          setTraceEvents((res.trace as TraceEvent[]) || []);
        } else {
          setError(res.error?.message || 'Failed to execute hash table demo.');
        }
      } catch (err: unknown) {
        setError(err instanceof Error ? err.message : 'Demo execution error');
      } finally {
        setLoading(false);
      }
    },
    [lookupKey]
  );

  // Fetch live Hash Table snapshot from backend
  const fetchLiveStats = useCallback(async () => {
    setLoading(true);
    setError(null);
    try {
      const res = await api.getHashTableStats();
      if (res.success && res.data) {
        setStats(res.data);
        setTraceEvents([]);
      } else {
        setError(res.error?.message || 'Failed to inspect live hash table.');
      }
    } catch (err: unknown) {
      setError(err instanceof Error ? err.message : 'Fetch error');
    } finally {
      setLoading(false);
    }
  }, []);

  // Initial demo on mount
  useEffect(() => {
    let cancelled = false;
    async function initSandbox() {
      try {
        const res = await api.runHashTableDemo('lookup', 'INC-101');
        if (!cancelled && res.success && res.data) {
          setStats(res.data.snapshot);
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

  // Active highlighted bucket and key from current trace step
  const highlightedBucketIndex = useMemo(() => {
    if (!currentEvent) return null;
    if (typeof currentEvent.metadata?.bucketIndex === 'number') {
      return currentEvent.metadata.bucketIndex;
    }
    return null;
  }, [currentEvent]);

  const activeKey = currentEvent?.entityId || null;

  return (
    <div className="space-y-6">
      {/* Controls & Operations Bar */}
      <div className="bg-white rounded-xl border border-[#DCE3EC] p-4 shadow-sm space-y-4">
        <div className="flex flex-wrap items-center justify-between gap-3">
          <div>
            <h2 className="text-base font-bold text-[#172033] flex items-center space-x-2">
              <span className="w-2.5 h-2.5 rounded-full bg-[#15803D]" />
              <span>Incident Direct Lookup — Custom Separate-Chaining Hash Table</span>
            </h2>
            <p className="text-xs text-[#526176] mt-0.5">
              64-bit FNV-1a hash calculation, separate-chaining collision resolution, and dynamic doubling rehash at load factor ≥ 0.75.
            </p>
          </div>

          {/* Mode switch */}
          <div className="flex items-center space-x-1 bg-[#F5F7FA] p-1 rounded-lg border border-[#DCE3EC] text-xs">
            <button
              type="button"
              onClick={() => {
                setMode('sandbox');
                runSandboxDemo('lookup');
              }}
              className={`px-3 py-1.5 rounded-md font-medium transition-colors cursor-pointer ${
                mode === 'sandbox'
                  ? 'bg-white text-[#172033] shadow-xs font-semibold'
                  : 'text-[#526176] hover:text-[#172033]'
              }`}
            >
              Lab Sandbox
            </button>
            <button
              type="button"
              onClick={() => {
                setMode('live');
                fetchLiveStats();
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

        {/* Sandbox Presets & Lookup Bar */}
        {mode === 'sandbox' ? (
          <div className="flex flex-wrap items-center justify-between gap-3 pt-3 border-t border-[#DCE3EC] text-xs">
            <div className="flex flex-wrap items-center gap-2">
              <button
                type="button"
                onClick={() => runSandboxDemo('collision')}
                disabled={loading}
                className="px-3 py-1.5 rounded-lg bg-amber-50 border border-amber-200 text-amber-900 hover:bg-amber-100 font-medium transition-colors cursor-pointer disabled:opacity-50"
              >
                Demonstrate Chained Collisions
              </button>

              <button
                type="button"
                onClick={() => runSandboxDemo('rehash')}
                disabled={loading}
                className="px-3 py-1.5 rounded-lg bg-indigo-50 border border-indigo-200 text-indigo-900 hover:bg-indigo-100 font-medium transition-colors cursor-pointer disabled:opacity-50"
              >
                Trigger Dynamic Rehash (16 ➔ 32)
              </button>
            </div>

            <div className="flex items-center space-x-2">
              <span className="text-[#526176] font-medium">Search Key:</span>
              <input
                type="text"
                value={lookupKey}
                onChange={(e) => setLookupKey(e.target.value)}
                placeholder="INC-101"
                className="bg-white border border-[#DCE3EC] rounded-lg px-2.5 py-1 text-[#172033] font-mono text-xs w-24 focus:outline-none focus:ring-2 focus:ring-[#3563D9]/20"
              />
              <button
                type="button"
                onClick={() => runSandboxDemo('lookup', lookupKey)}
                disabled={loading || !lookupKey}
                className="px-3.5 py-1 rounded-lg bg-[#3563D9] hover:bg-[#284eb3] text-white font-medium shadow-xs transition-colors cursor-pointer disabled:opacity-50"
              >
                Search
              </button>
            </div>
          </div>
        ) : (
          <div className="flex items-center justify-between text-xs text-[#526176] pt-1">
            <span>Viewing active incident records from live simulation hash index.</span>
            <button
              type="button"
              onClick={fetchLiveStats}
              disabled={loading}
              className="px-3 py-1.5 rounded-lg bg-[#F5F7FA] hover:bg-slate-200 text-[#172033] font-medium border border-[#DCE3EC] cursor-pointer"
            >
              Refresh Stats
            </button>
          </div>
        )}

        {error && (
          <div className="p-3 rounded-lg bg-red-50 border border-red-200 text-xs text-red-700">
            {error}
          </div>
        )}
      </div>

      {/* Internal Hash Table Metrics Strip */}
      {stats && (
        <div className="grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-6 gap-3 text-xs font-mono">
          <div className="p-3 rounded-xl bg-white border border-[#DCE3EC] shadow-xs">
            <span className="text-[10px] text-[#718096] uppercase font-bold">Stored Elements</span>
            <p className="text-base font-bold text-[#172033] mt-0.5">{stats.size}</p>
          </div>

          <div className="p-3 rounded-xl bg-white border border-[#DCE3EC] shadow-xs">
            <span className="text-[10px] text-[#718096] uppercase font-bold">Bucket Capacity</span>
            <p className="text-base font-bold text-[#3563D9] mt-0.5">{stats.bucketCount}</p>
          </div>

          <div className="p-3 rounded-xl bg-white border border-[#DCE3EC] shadow-xs">
            <span className="text-[10px] text-[#718096] uppercase font-bold">Occupied Buckets</span>
            <p className="text-base font-bold text-[#15803D] mt-0.5">
              {stats.occupiedBuckets} / {stats.bucketCount}
            </p>
          </div>

          <div className="p-3 rounded-xl bg-white border border-[#DCE3EC] shadow-xs">
            <span className="text-[10px] text-[#718096] uppercase font-bold">Load Factor (α)</span>
            <p
              className={`text-base font-bold mt-0.5 ${
                stats.loadFactor >= 0.75 ? 'text-[#D97706]' : 'text-[#087E8B]'
              }`}
            >
              {stats.loadFactor.toFixed(3)}
            </p>
          </div>

          <div className="p-3 rounded-xl bg-white border border-[#DCE3EC] shadow-xs">
            <span className="text-[10px] text-[#718096] uppercase font-bold">Rehash Threshold</span>
            <p className="text-base font-bold text-[#526176] mt-0.5">α ≥ {stats.maxLoadFactor.toFixed(2)}</p>
          </div>

          <div className="p-3 rounded-xl bg-white border border-[#DCE3EC] shadow-xs">
            <span className="text-[10px] text-[#718096] uppercase font-bold">Total Collisions</span>
            <p
              className={`text-base font-bold mt-0.5 ${
                stats.collisionCount > 0 ? 'text-[#D97706]' : 'text-[#718096]'
              }`}
            >
              {stats.collisionCount} (Max depth: {stats.maxBucketDepth})
            </p>
          </div>
        </div>
      )}

      {/* Visualizations: Bucket Grid & Separate-Chaining Linked Nodes */}
      <div className="bg-white rounded-xl border border-[#DCE3EC] p-4 space-y-4 shadow-sm">
        <div className="flex flex-wrap items-center justify-between gap-2 border-b border-[#DCE3EC] pb-3 text-xs">
          <div className="flex items-center space-x-2">
            <span className="text-xs font-bold uppercase tracking-wider text-[#172033]">
              Bucket Array & Separate Chains (hash % {stats?.bucketCount || 16})
            </span>
            <span className="text-[11px] text-[#718096] font-mono hidden sm:inline">
              Hash: 64-bit FNV-1a
            </span>
          </div>

          <div className="flex items-center space-x-3 text-[11px] font-mono text-[#526176]">
            <span className="flex items-center space-x-1">
              <span className="w-2 h-2 rounded-full bg-slate-200 border border-slate-300" />
              <span>Empty</span>
            </span>
            <span className="flex items-center space-x-1">
              <span className="w-2 h-2 rounded-full bg-[#15803D]" />
              <span>Occupied</span>
            </span>
            <span className="flex items-center space-x-1">
              <span className="w-2 h-2 rounded-full bg-[#D97706]" />
              <span>Collision (depth &gt; 1)</span>
            </span>
          </div>
        </div>

        {/* Bucket Cards Grid */}
        <div className="grid grid-cols-1 sm:grid-cols-2 md:grid-cols-3 lg:grid-cols-4 gap-3">
          {stats?.buckets.map((b) => {
            const isTargetBucket = highlightedBucketIndex === b.bucketIndex;
            const hasCollision = b.entryCount > 1;
            const isEmpty = b.entryCount === 0;

            return (
              <div
                key={b.bucketIndex}
                className={`p-3 rounded-lg border transition-all ${
                  isTargetBucket
                    ? 'bg-sky-50 border-sky-400 ring-1 ring-sky-300'
                    : isEmpty
                    ? 'bg-[#F8FAFC] border-slate-200 opacity-60'
                    : hasCollision
                    ? 'bg-amber-50/50 border-amber-300'
                    : 'bg-white border-[#DCE3EC] shadow-xs'
                }`}
              >
                {/* Bucket Header */}
                <div className="flex items-center justify-between border-b border-slate-100 pb-1.5">
                  <div className="flex items-center space-x-1.5 font-mono text-xs">
                    <span className="font-bold text-[#172033]">Bucket #{b.bucketIndex}</span>
                    {isTargetBucket && (
                      <span className="px-1.5 py-0.2 rounded bg-sky-100 text-sky-800 text-[9px] font-bold">
                        ACTIVE
                      </span>
                    )}
                  </div>
                  <span
                    className={`text-[10px] font-mono px-1.5 py-0.2 rounded font-semibold ${
                      isEmpty
                        ? 'bg-slate-100 text-slate-500'
                        : hasCollision
                        ? 'bg-amber-100 text-amber-800 border border-amber-200'
                        : 'bg-emerald-100 text-emerald-800 border border-emerald-200'
                    }`}
                  >
                    {b.entryCount} {b.entryCount === 1 ? 'item' : 'items'}
                  </span>
                </div>

                {/* Separate Chaining Entries */}
                <div className="mt-2 space-y-1.5 font-mono text-xs">
                  {isEmpty ? (
                    <div className="text-[11px] text-[#718096] italic py-1">Empty chain [nullptr]</div>
                  ) : (
                    b.keys.map((k, chainIdx) => {
                      const isKeyTarget = activeKey === k;
                      return (
                        <div
                          key={k}
                          className={`flex items-center space-x-1.5 px-2 py-1 rounded transition-colors ${
                            isKeyTarget
                              ? 'bg-sky-100 text-sky-900 border border-sky-300 font-bold'
                              : 'bg-[#F8FAFC] text-[#172033] border border-slate-200'
                          }`}
                        >
                          <span className="text-[10px] text-[#718096]">
                            {chainIdx === 0 ? 'head ➔' : 'node ➔'}
                          </span>
                          <span className="font-semibold">{k}</span>
                          {chainIdx === b.keys.length - 1 && (
                            <span className="text-[9px] text-[#718096] ml-auto font-mono">➔ ∅</span>
                          )}
                        </div>
                      );
                    })
                  )}
                </div>
              </div>
            );
          })}
        </div>
      </div>

      {/* Trace Player Reusable Controller */}
      {traceEvents.length > 0 && (
        <TracePlayerControls
          player={player}
          title="Hash Table Inspection & Resolution Telemetry"
          sourceFile="backend/include/dsa/hash_table.hpp"
        />
      )}

      {/* Educational Viva Reference */}
      <ComplexityCard
        algorithmName="Custom Separate-Chaining Hash Table"
        problemSolved="Instantaneous O(1) direct lookup of incident details by unique incident ID (e.g. INC-101), bypassing graph and tree traversals."
        whyEmergencyGridUsesIt="Dispatches, unit completions, and road updates continually reference incident IDs. An O(1) hash table ensures operations do not degrade to O(N) linear scans as incident history expands."
        howItWorks="Computes a 64-bit FNV-1a non-cryptographic hash from the incident ID string. Applies modular hashing (hash % bucketCount) to select bucket index. Resolves collisions using separate chaining (linked list per bucket). Monitors load factor α = N / B; when α exceeds maxLoadFactor (0.75), automatically doubles bucketCount (e.g., 16 ➔ 32) and rehashes all elements."
        metrics={[
          {
            operation: "Key Lookup (Search)",
            averageTime: "O(1)",
            worstTime: "O(N)",
            space: "O(1)",
            notes: "Average O(1) assuming uniform hash distribution; worst-case O(N) if all keys collide into one bucket chain.",
          },
          {
            operation: "Insert Record",
            averageTime: "O(1)",
            worstTime: "O(N)",
            space: "O(1)",
            notes: "Inserts at head/tail of bucket chain; amortized O(1) including periodic dynamic rehash.",
          },
          {
            operation: "Delete Record",
            averageTime: "O(1)",
            worstTime: "O(N)",
            space: "O(1)",
            notes: "Unlinks node from bucket collision chain in constant time once found.",
          },
          {
            operation: "Dynamic Doubling Rehash",
            averageTime: "O(N)",
            worstTime: "O(N)",
            space: "O(B + N)",
            notes: "Allocates 2B buckets and re-inserts all existing N records when α ≥ 0.75.",
          },
        ]}
        strengths={[
          "Average O(1) instantaneous access independent of total incident count.",
          "Separate chaining degrades gracefully under collisions compared to open addressing (no clustering).",
          "Dynamic doubling rehashing preserves average chain length ≤ 0.75 under sustained insert loads.",
        ]}
        limitations={[
          "Worst-case O(N) lookup occurs if the hash function produces pathological collisions.",
          "Does not maintain chronological or priority order; cannot answer range queries (delegated to AVL tree).",
        ]}
        cppHeader="backend/include/dsa/hash_table.hpp"
        testFile="backend/tests/test_hash_table.cpp"
      />
    </div>
  );
}
