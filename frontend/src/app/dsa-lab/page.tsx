'use client';

import React, { useState, useEffect, useCallback } from 'react';
import Header from '@/components/common/Header';
import BackendStatusCard from '@/components/common/BackendStatusCard';
import { BackendStatusResult, CityGraph } from '@/lib/types';
import { api } from '@/lib/api';

import DijkstraVisualizer from '@/components/dsa-lab/modules/DijkstraVisualizer';
import HeapVisualizer from '@/components/dsa-lab/modules/HeapVisualizer';
import AvlTreeVisualizer from '@/components/dsa-lab/modules/AvlTreeVisualizer';
import HashTableVisualizer from '@/components/dsa-lab/modules/HashTableVisualizer';
import DispatchEngineVisualizer from '@/components/dsa-lab/modules/DispatchEngineVisualizer';

type ActiveModuleTab = 'dijkstra' | 'heap' | 'avl' | 'hash-table' | 'dispatch';

const MODULES: {
  id: ActiveModuleTab;
  title: string;
  summary: string;
  complexity: string;
}[] = [
  {
    id: 'dijkstra',
    title: 'Graph & Shortest Path',
    summary:
      'Calculates the optimal travel-time route across the city road network using binary min-heap relaxation.',
    complexity: 'O((V+E) log V)',
  },
  {
    id: 'heap',
    title: 'Incident Priority',
    summary:
      'Prioritizes emergency calls dynamically with constant-time root peek and logarithmic sift-up/down rebalancing.',
    complexity: 'O(log N)',
  },
  {
    id: 'avl',
    title: 'AVL Tree',
    summary:
      'Maintains an ordered, height-balanced search tree of incident records (|BF| ≤ 1) via self-balancing rotations.',
    complexity: 'O(log N)',
  },
  {
    id: 'hash-table',
    title: 'Incident Lookup',
    summary:
      'Provides constant-time direct lookup of incident records by unique ID using 64-bit FNV-1a hashing and separate chaining.',
    complexity: 'Avg O(1)',
  },
  {
    id: 'dispatch',
    title: 'Dispatch',
    summary:
      'Evaluates multi-agency unit eligibility and shortest-path travel times to dispatch the optimal available responder.',
    complexity: 'Multi-DSA Pipeline',
  },
];

export default function DsaLabPage() {
  const [status, setStatus] = useState<BackendStatusResult | null>(null);
  const [loading, setLoading] = useState<boolean>(true);
  const [activeTab, setActiveTab] = useState<ActiveModuleTab>('dijkstra');
  const [cityGraph, setCityGraph] = useState<CityGraph | null>(null);
  const [showMatrix, setShowMatrix] = useState<boolean>(false);

  // Poll backend health status
  const fetchStatus = useCallback(async () => {
    try {
      const res = await fetch('/api/backend-status', { cache: 'no-store' });
      if (res.ok) {
        const data: BackendStatusResult = await res.json();
        setStatus(data);
      } else {
        setStatus({
          reachable: false,
          checkedAt: new Date().toISOString(),
          backendUrl: 'http://localhost:8080',
          error: `HTTP ${res.status}: Failed to reach backend proxy`,
        });
      }
    } catch (err: unknown) {
      setStatus({
        reachable: false,
        checkedAt: new Date().toISOString(),
        backendUrl: 'http://localhost:8080',
        error: err instanceof Error ? err.message : 'Unknown client fetch error',
      });
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    let isMounted = true;

    async function probeBackend() {
      try {
        const res = await fetch('/api/backend-status', { cache: 'no-store' });
        if (res.ok) {
          const data: BackendStatusResult = await res.json();
          if (isMounted) setStatus(data);
        } else {
          if (isMounted) {
            setStatus({
              reachable: false,
              checkedAt: new Date().toISOString(),
              backendUrl: 'http://localhost:8080',
              error: `HTTP ${res.status}: Failed to reach backend proxy`,
            });
          }
        }
      } catch (err: unknown) {
        if (isMounted) {
          setStatus({
            reachable: false,
            checkedAt: new Date().toISOString(),
            backendUrl: 'http://localhost:8080',
            error: err instanceof Error ? err.message : 'Unknown client fetch error',
          });
        }
      } finally {
        if (isMounted) setLoading(false);
      }
    }

    probeBackend();
    const interval = setInterval(probeBackend, 15000);

    // Fetch initial city graph
    api.getCityGraph().then((res) => {
      if (res.success && res.data && isMounted) {
        setCityGraph(res.data);
      }
    });

    return () => {
      isMounted = false;
      clearInterval(interval);
    };
  }, []);

  const activeModule = MODULES.find((m) => m.id === activeTab) || MODULES[0];

  return (
    <div className="min-h-screen bg-[#F5F7FA] text-[#172033] flex flex-col font-sans">
      <Header status={status} loading={loading} />

      <main className="flex-1 max-w-7xl w-full mx-auto px-4 sm:px-6 lg:px-8 py-6 space-y-6">
        {/* Simplified Header: Title + 1 short sentence */}
        <div>
          <div className="flex flex-wrap items-baseline justify-between gap-2">
            <h1 className="text-2xl font-bold text-[#172033] tracking-tight">
              DSA Lab
            </h1>
            <span className="text-xs font-mono text-[#526176] bg-white border border-[#DCE3EC] px-2.5 py-1 rounded-md shadow-xs">
              Simulation Core @ 127.0.0.1:8080
            </span>
          </div>
          <p className="text-sm text-[#526176] mt-1">
            {activeModule.summary}
          </p>
        </div>

        {/* Compact Segmented Module Selector */}
        <div className="bg-white p-1.5 rounded-xl border border-[#DCE3EC] shadow-xs flex flex-wrap gap-1.5">
          {MODULES.map((tab) => {
            const isActive = activeTab === tab.id;
            return (
              <button
                key={tab.id}
                type="button"
                onClick={() => setActiveTab(tab.id)}
                className={`flex-1 min-w-[140px] px-3.5 py-2 rounded-lg text-xs font-medium transition-all cursor-pointer flex items-center justify-between gap-2 ${
                  isActive
                    ? 'bg-[#3563D9] text-white shadow-xs font-semibold'
                    : 'text-[#526176] hover:bg-[#F5F7FA] hover:text-[#172033]'
                }`}
              >
                <span>{tab.title}</span>
                <span
                  className={`text-[10px] font-mono px-1.5 py-0.5 rounded ${
                    isActive
                      ? 'bg-white/20 text-white'
                      : 'bg-slate-100 text-[#718096]'
                  }`}
                >
                  {tab.complexity}
                </span>
              </button>
            );
          })}
        </div>

        {/* Active Module Visualizer */}
        <section aria-label="Selected algorithm visualizer">
          {activeTab === 'dijkstra' && <DijkstraVisualizer initialGraph={cityGraph} />}
          {activeTab === 'heap' && <HeapVisualizer />}
          {activeTab === 'avl' && <AvlTreeVisualizer />}
          {activeTab === 'hash-table' && <HashTableVisualizer />}
          {activeTab === 'dispatch' && <DispatchEngineVisualizer />}
        </section>

        {/* Collapsible DSA Architecture Matrix for Reference */}
        <div className="bg-white rounded-xl border border-[#DCE3EC] overflow-hidden shadow-xs">
          <button
            type="button"
            onClick={() => setShowMatrix(!showMatrix)}
            className="w-full px-4 py-3 bg-[#F8FAFC] flex items-center justify-between text-xs font-semibold text-[#172033] hover:bg-slate-100 transition-colors cursor-pointer"
          >
            <span className="flex items-center space-x-2">
              <span className="w-2 h-2 rounded-full bg-[#3563D9]" />
              <span>DSA Architecture & Complexity Reference Matrix</span>
            </span>
            <span className="text-[#526176] font-mono text-[11px]">
              {showMatrix ? '▲ Hide Matrix' : '▼ View Matrix'}
            </span>
          </button>

          {showMatrix && (
            <div className="p-4 border-t border-[#DCE3EC] overflow-x-auto">
              <table className="w-full text-left font-mono text-xs">
                <thead>
                  <tr className="bg-[#F8FAFC] text-[#526176] border-b border-[#DCE3EC]">
                    <th className="py-2 px-3 font-semibold">Component</th>
                    <th className="py-2 px-3 font-semibold">Data Structure</th>
                    <th className="py-2 px-3 font-semibold">Operation</th>
                    <th className="py-2 px-3 font-semibold">Time Complexity</th>
                    <th className="py-2 px-3 font-semibold">Space</th>
                    <th className="py-2 px-3 font-semibold font-sans">Key Invariant</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-[#E2E8F0]">
                  <tr className="hover:bg-slate-50">
                    <td className="py-2 px-3 font-bold text-[#3563D9]">Road Routing</td>
                    <td className="py-2 px-3 text-[#172033]">Graph + Min-Heap</td>
                    <td className="py-2 px-3 text-[#526176]">Dijkstra Shortest Path</td>
                    <td className="py-2 px-3 text-[#15803D] font-bold">O((V + E) log V)</td>
                    <td className="py-2 px-3 text-[#718096]">O(V + E)</td>
                    <td className="py-2 px-3 font-sans text-[#526176]">
                      Optimal path on non-negative weights
                    </td>
                  </tr>
                  <tr className="hover:bg-slate-50">
                    <td className="py-2 px-3 font-bold text-[#DC2626]">Incident Triage</td>
                    <td className="py-2 px-3 text-[#172033]">Array Max-Heap</td>
                    <td className="py-2 px-3 text-[#526176]">Peek Max / Sift-Down</td>
                    <td className="py-2 px-3 text-[#15803D] font-bold">O(1) peek / O(log N)</td>
                    <td className="py-2 px-3 text-[#718096]">O(N)</td>
                    <td className="py-2 px-3 font-sans text-[#526176]">
                      Parent ≥ child priority; FIFO tie-break
                    </td>
                  </tr>
                  <tr className="hover:bg-slate-50">
                    <td className="py-2 px-3 font-bold text-[#4F46E5]">Incident Archive</td>
                    <td className="py-2 px-3 text-[#172033]">AVL Tree</td>
                    <td className="py-2 px-3 text-[#526176]">Lookup / Inorder Sort</td>
                    <td className="py-2 px-3 text-[#15803D] font-bold">O(log N) guaranteed</td>
                    <td className="py-2 px-3 text-[#718096]">O(N)</td>
                    <td className="py-2 px-3 font-sans text-[#526176]">
                      Strict height balance |BF| ≤ 1
                    </td>
                  </tr>
                  <tr className="hover:bg-slate-50">
                    <td className="py-2 px-3 font-bold text-[#15803D]">Direct Index</td>
                    <td className="py-2 px-3 text-[#172033]">Hash Table (Chaining)</td>
                    <td className="py-2 px-3 text-[#526176]">ID Lookup</td>
                    <td className="py-2 px-3 text-[#15803D] font-bold">Avg O(1) / Worst O(N)</td>
                    <td className="py-2 px-3 text-[#718096]">O(B + N)</td>
                    <td className="py-2 px-3 font-sans text-[#526176]">
                      FNV-1a 64-bit; doubling rehash at α ≥ 0.75
                    </td>
                  </tr>
                  <tr className="hover:bg-slate-50">
                    <td className="py-2 px-3 font-bold text-[#087E8B]">Dispatch Engine</td>
                    <td className="py-2 px-3 text-[#172033]">Multi-DSA Pipeline</td>
                    <td className="py-2 px-3 text-[#526176]">Unit Selection</td>
                    <td className="py-2 px-3 text-[#15803D] font-bold">
                      O(log N + K(V+E)log V)
                    </td>
                    <td className="py-2 px-3 text-[#718096]">O(U + V + E)</td>
                    <td className="py-2 px-3 font-sans text-[#526176]">
                      Multi-attribute capability & travel-time optimality
                    </td>
                  </tr>
                </tbody>
              </table>
            </div>
          )}
        </div>

        {/* Backend Status Card (Unobtrusive) */}
        <BackendStatusCard status={status} loading={loading} onRefresh={fetchStatus} />
      </main>

      <footer className="border-t border-[#DCE3EC] py-6 text-center text-xs text-[#718096]">
        <div className="max-w-7xl mx-auto px-4 space-y-1">
          <p className="font-medium text-[#526176]">
            EmergencyGrid &bull; Advanced Data Structures &amp; Algorithms Lab
          </p>
          <p className="text-[11px] text-[#718096]">
            Strictly an educational simulation. Not connected to real emergency response networks.
          </p>
        </div>
      </footer>
    </div>
  );
}
