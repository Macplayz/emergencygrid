'use client';

import React, { useState, useEffect, useCallback } from 'react';
import { AVLTreeSnapshot, AVLNodeSnapshot, TraceEvent } from '@/lib/types';
import { api } from '@/lib/api';
import { useTracePlayer } from '@/lib/useTracePlayer';
import TracePlayerControls from '../common/TracePlayerControls';
import ComplexityCard from '../common/ComplexityCard';

type RotationDemoType = 'LL' | 'RR' | 'LR' | 'RL' | 'BALANCED';

interface LayoutNode {
  key: string;
  incidentId: string;
  type?: string;
  height: number;
  balanceFactor: number;
  x: number;
  y: number;
  unbalanced: boolean;
  leftChild?: string;
  rightChild?: string;
}

interface LayoutEdge {
  id: string;
  x1: number;
  y1: number;
  x2: number;
  y2: number;
}

function countNodes(node: AVLNodeSnapshot | null | undefined): number {
  if (!node) return 0;
  return 1 + countNodes(node.left) + countNodes(node.right);
}

export default function AvlTreeVisualizer() {
  const [snapshot, setSnapshot] = useState<AVLTreeSnapshot | null>(null);
  const [loading, setLoading] = useState(false);
  const [traceEvents, setTraceEvents] = useState<TraceEvent[]>([]);
  const [error, setError] = useState<string | null>(null);
  const [mode, setMode] = useState<'sandbox' | 'live'>('sandbox');
  const [rotationDemo, setRotationDemo] = useState<RotationDemoType>('LL');

  const reqIdRef = React.useRef(0);

  // Trace player controller
  const player = useTracePlayer(traceEvents, { defaultSpeedMs: 900 });
  const { currentEvent } = player;

  // Run isolated AVL sandbox demonstration
  const runSandboxDemo = useCallback(
    async (rotType: RotationDemoType, action: 'rotation' | 'inorder' = 'rotation') => {
      const currentReqId = ++reqIdRef.current;
      player.pause();
      player.reset();
      setLoading(true);
      setError(null);
      setSnapshot(null);
      setTraceEvents([]);
      try {
        const res = await api.runAvlDemo(rotType, action);
        if (currentReqId !== reqIdRef.current) return;
        if (res.success && res.data) {
          setSnapshot(res.data.snapshot);
          setTraceEvents((res.trace as TraceEvent[]) || []);
        } else {
          setError(res.error?.message || 'Failed to execute AVL demonstration.');
        }
      } catch (err: unknown) {
        if (currentReqId !== reqIdRef.current) return;
        setError(err instanceof Error ? err.message : 'Demo execution error');
      } finally {
        if (currentReqId === reqIdRef.current) {
          setLoading(false);
        }
      }
    },
    [player]
  );

  // Fetch live AVL tree from backend
  const fetchLiveSnapshot = useCallback(async () => {
    const currentReqId = ++reqIdRef.current;
    player.pause();
    player.reset();
    setLoading(true);
    setError(null);
    setSnapshot(null);
    setTraceEvents([]);
    try {
      const res = await api.getAvlSnapshot();
      if (currentReqId !== reqIdRef.current) return;
      if (res.success && res.data) {
        setSnapshot(res.data);
        setTraceEvents([]);
      } else {
        setError(res.error?.message || 'Failed to inspect live AVL tree.');
      }
    } catch (err: unknown) {
      if (currentReqId !== reqIdRef.current) return;
      setError(err instanceof Error ? err.message : 'Fetch error');
    } finally {
      if (currentReqId === reqIdRef.current) {
        setLoading(false);
      }
    }
  }, [player]);

  // Run initial demo on mount
  useEffect(() => {
    let cancelled = false;
    async function loadInitial() {
      try {
        const res = await api.runAvlDemo('LL', 'rotation');
        if (!cancelled && res.success && res.data) {
          setSnapshot(res.data.snapshot);
          setTraceEvents((res.trace as TraceEvent[]) || []);
        }
      } catch {
        // ignore
      }
    }
    loadInitial();
    return () => {
      cancelled = true;
    };
  }, []);

  // Compute active snapshot dynamically from trace playback step
  let activeSnapshot: AVLTreeSnapshot | null = snapshot;
  if (mode === 'sandbox' && traceEvents.length > 0) {
    let foundRoot: AVLNodeSnapshot | null = null;
    let isExplicitEmpty = false;
    for (let i = player.currentStepIndex; i >= 0; i--) {
      const ev = traceEvents[i];
      if (ev?.operation === 'initial_state' || ev?.metadata?.empty === true) {
        isExplicitEmpty = true;
        break;
      }
      const snap = ev?.metadata?.treeSnapshot as AVLNodeSnapshot | undefined;
      if (snap) {
        foundRoot = snap;
        break;
      }
    }
    if (isExplicitEmpty && !foundRoot) {
      activeSnapshot = { root: null, size: 0, height: 0 };
    } else if (foundRoot) {
      activeSnapshot = {
        root: foundRoot,
        size: countNodes(foundRoot),
        height: foundRoot.height,
      };
    } else {
      activeSnapshot = { root: null, size: 0, height: 0 };
    }
  }

  // Active highlighted key and rotation from trace
  const highlightedKey = currentEvent?.entityId || null;
  const activeRotation =
    currentEvent?.operation?.includes('rotate') || currentEvent?.operation?.includes('rebalance')
      ? currentEvent.operation
      : null;

  // Flatten tree for list/table inspection
  const flattenedNodes: AVLNodeSnapshot[] = [];
  if (activeSnapshot?.root) {
    const traverseInorder = (node: AVLNodeSnapshot | null | undefined) => {
      if (!node) return;
      traverseInorder(node.left);
      flattenedNodes.push(node);
      traverseInorder(node.right);
    };
    traverseInorder(activeSnapshot.root);
  }

  // Compute 2D node coordinates & connecting edges for stable keyed SVG layout
  const layoutNodes: LayoutNode[] = [];
  const layoutEdges: LayoutEdge[] = [];
  if (activeSnapshot?.root) {
    const traverseLayout = (node: AVLNodeSnapshot, x: number, y: number, dx: number) => {
      const isUnbalanced = Math.abs(node.balanceFactor) > 1;
      layoutNodes.push({
        key: node.key,
        incidentId: node.incidentId || node.key,
        type: node.type,
        height: node.height,
        balanceFactor: node.balanceFactor,
        x,
        y,
        unbalanced: isUnbalanced,
        leftChild: node.left?.key,
        rightChild: node.right?.key,
      });

      const childY = y + 70;
      const nextDx = Math.max(36, dx * 0.52);

      if (node.left) {
        const childX = x - dx;
        layoutEdges.push({
          id: [node.key, node.left.key].sort().join('--'),
          x1: x,
          y1: y,
          x2: childX,
          y2: childY,
        });
        traverseLayout(node.left, childX, childY, nextDx);
      }

      if (node.right) {
        const childX = x + dx;
        layoutEdges.push({
          id: [node.key, node.right.key].sort().join('--'),
          x1: x,
          y1: y,
          x2: childX,
          y2: childY,
        });
        traverseLayout(node.right, childX, childY, nextDx);
      }
    };

    traverseLayout(activeSnapshot.root, 270, 50, 110);
  }

  return (
    <div className="space-y-6">
      {/* Controls & Rotation Trigger Bar */}
      <div className="bg-white rounded-xl border border-[#DCE3EC] p-4 shadow-sm space-y-4">
        <div className="flex flex-wrap items-center justify-between gap-3">
          <div>
            <h2 className="text-base font-bold text-[#172033] flex items-center space-x-2">
              <span className="w-2.5 h-2.5 rounded-full bg-[#4F46E5]" />
              <span>Self-Balancing Binary Search Tree — AVL Tree</span>
            </h2>
            <p className="text-xs text-[#526176] mt-0.5">
              Guarantees logarithmic height bound |BF| ≤ 1 at all times via Single (LL, RR) and Double (LR, RL) rotations.
            </p>
          </div>

          {/* Mode switch */}
          <div className="flex items-center space-x-1 bg-[#F5F7FA] p-1 rounded-lg border border-[#DCE3EC] text-xs">
            <button
              type="button"
              onClick={() => {
                setMode('sandbox');
                runSandboxDemo('LL');
              }}
              className={`px-3 py-1.5 rounded-md font-medium transition-colors cursor-pointer ${
                mode === 'sandbox'
                  ? 'bg-white text-[#172033] shadow-xs font-semibold'
                  : 'text-[#526176] hover:text-[#172033]'
              }`}
            >
              Rotation Demonstrations
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
              Live Simulation State
            </button>
          </div>
        </div>

        {/* Rotation Sandbox Presets */}
        {mode === 'sandbox' ? (
          <div className="flex flex-wrap items-center justify-between gap-2 pt-3 border-t border-[#DCE3EC] text-xs">
            <div className="flex flex-wrap items-center gap-2">
              <span className="text-[#526176] font-semibold text-xs">
                Run Sequence:
              </span>
              {[
                { type: 'LL', label: 'LL (Right Rotation)', desc: 'Insert 30, 20, 10' },
                { type: 'RR', label: 'RR (Left Rotation)', desc: 'Insert 10, 20, 30' },
                { type: 'LR', label: 'LR (Double Rotation)', desc: 'Insert 30, 10, 20' },
                { type: 'RL', label: 'RL (Double Rotation)', desc: 'Insert 10, 30, 20' },
                { type: 'BALANCED', label: 'Balanced (No Rotation)', desc: 'Insert 20, 10, 30' },
              ].map((r) => (
                <button
                  key={r.type}
                  type="button"
                  onClick={() => {
                    const t = r.type as RotationDemoType;
                    setRotationDemo(t);
                    runSandboxDemo(t);
                  }}
                  className={`px-3 py-1.5 rounded-lg font-medium transition-colors border cursor-pointer ${
                    rotationDemo === r.type
                      ? 'bg-[#3563D9] text-white border-[#3563D9] shadow-xs'
                      : 'bg-[#F5F7FA] text-[#172033] border-[#DCE3EC] hover:bg-slate-200'
                  }`}
                  title={r.desc}
                >
                  {r.label}
                </button>
              ))}

              <button
                type="button"
                onClick={() => runSandboxDemo(rotationDemo, 'inorder')}
                className="px-3 py-1.5 rounded-lg bg-emerald-50 border border-emerald-200 text-emerald-800 hover:bg-emerald-100 font-medium cursor-pointer"
              >
                Inorder Traversal
              </button>
            </div>
          </div>
        ) : (
          <div className="flex items-center justify-between text-xs text-[#526176] pt-1">
            <span>Viewing active incident record index stored in the balanced AVL tree index.</span>
            <button
              type="button"
              onClick={fetchLiveSnapshot}
              disabled={loading}
              className="px-3 py-1.5 rounded-lg bg-[#F5F7FA] hover:bg-slate-200 text-[#172033] font-medium border border-[#DCE3EC] cursor-pointer"
            >
              Refresh Tree
            </button>
          </div>
        )}

        {error && (
          <div className="p-3 rounded-lg bg-red-50 border border-red-200 text-xs text-red-700">
            {error}
          </div>
        )}
      </div>

      {/* Visualizations: SVG Tree View + Inorder Node Index Table */}
      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        {/* SVG Tree View with Keyed Animated Nodes & Edges */}
        <div className="lg:col-span-2 bg-white rounded-xl border border-[#DCE3EC] p-4 flex flex-col shadow-sm relative min-h-[380px] overflow-hidden">
          <div className="flex flex-wrap items-center justify-between gap-2 border-b border-[#DCE3EC] pb-3 mb-2 text-xs">
            <div className="flex items-center space-x-3 font-mono text-[#526176]">
              <span>
                Total Nodes: <strong className="text-[#172033]">{activeSnapshot?.size || 0}</strong>
              </span>
              <span className="text-[#DCE3EC]">|</span>
              <span>
                Tree Height: <strong className="text-[#3563D9]">{activeSnapshot?.height || 0}</strong>
              </span>
              <span className="text-[#DCE3EC]">|</span>
              <span>
                Balance Invariant:{' '}
                <strong
                  className={
                    layoutNodes.some((n) => n.unbalanced) ? 'text-[#DC2626]' : 'text-[#15803D]'
                  }
                >
                  {layoutNodes.some((n) => n.unbalanced) ? 'Unbalanced (|BF| > 1)' : '|BF| ≤ 1 Preserved'}
                </strong>
              </span>
            </div>

            <div className="flex items-center space-x-3 text-[11px] text-[#526176]">
              <span className="flex items-center space-x-1">
                <span className="w-2.5 h-2.5 rounded-full bg-[#15803D]" />
                <span>|BF| ≤ 1 Balanced</span>
              </span>
              <span className="flex items-center space-x-1">
                <span className="w-2.5 h-2.5 rounded-full bg-[#DC2626]" />
                <span>|BF| &gt; 1 Unbalanced</span>
              </span>
            </div>
          </div>

          <div className="flex-1 w-full flex items-center justify-center bg-[#F8FAFC] rounded-lg border border-[#E2E8F0] p-2 relative min-h-[290px]">
            {loading ? (
              <div className="flex items-center space-x-2 text-xs text-[#526176]">
                <div className="w-4 h-4 border-2 border-[#3563D9] border-t-transparent rounded-full animate-spin" />
                <span>Loading AVL demonstration...</span>
              </div>
            ) : error ? (
              <div className="text-xs text-red-600 bg-red-50 p-3 rounded-lg border border-red-200">
                {error}
              </div>
            ) : !activeSnapshot?.root ? (
              <div className="text-xs text-[#718096] italic">AVL Tree is empty at this step.</div>
            ) : (
              <svg viewBox="0 0 540 320" className="w-full h-auto max-h-[360px]">
                {/* Connecting Edges with Stable Keyed Transition */}
                {layoutEdges.map((edge) => (
                  <line
                    key={edge.id}
                    x1={edge.x1}
                    y1={edge.y1}
                    x2={edge.x2}
                    y2={edge.y2}
                    stroke="#CBD5E1"
                    strokeWidth={2}
                    className="transition-all duration-600 ease-in-out motion-reduce:transition-none"
                  />
                ))}

                {/* Nodes with Stable Keyed Coordinates & Spring Transition */}
                {layoutNodes.map((node) => {
                  const isHighlighted = highlightedKey === node.key;
                  const isUnbalanced = node.unbalanced;

                  return (
                    <g
                      key={node.key}
                      style={{
                        transform: `translate(${node.x}px, ${node.y}px)`,
                        transition: 'transform 600ms cubic-bezier(0.34, 1.25, 0.64, 1)',
                      }}
                      className="cursor-pointer motion-reduce:transition-none"
                    >
                      {/* Active highlight / unbalance halo */}
                      {isHighlighted && (
                        <circle cx={0} cy={0} r={26} fill="#087E8B" opacity={0.18} />
                      )}
                      {isUnbalanced && !isHighlighted && (
                        <circle cx={0} cy={0} r={26} fill="#DC2626" opacity={0.16} />
                      )}

                      {/* Main Node Circle */}
                      <circle
                        cx={0}
                        cy={0}
                        r={19}
                        fill={
                          isHighlighted
                            ? '#E0F2FE'
                            : isUnbalanced
                            ? '#FEF2F2'
                            : '#FFFFFF'
                        }
                        stroke={
                          isHighlighted
                            ? '#087E8B'
                            : isUnbalanced
                            ? '#DC2626'
                            : '#3563D9'
                        }
                        strokeWidth={isHighlighted || isUnbalanced ? 2.5 : 2}
                        className="transition-colors duration-300 shadow-sm"
                      />

                      {/* Node Key */}
                      <text
                        x={0}
                        y={-2}
                        fill="#172033"
                        fontSize="9"
                        fontWeight="bold"
                        fontFamily="monospace"
                        textAnchor="middle"
                        className="pointer-events-none select-none"
                      >
                        {node.key}
                      </text>

                      {/* Balance Factor Tag */}
                      <text
                        x={0}
                        y={8}
                        fill={Math.abs(node.balanceFactor) <= 1 ? '#15803D' : '#DC2626'}
                        fontSize="8"
                        fontWeight="bold"
                        fontFamily="monospace"
                        textAnchor="middle"
                        className="pointer-events-none select-none"
                      >
                        BF:{node.balanceFactor > 0 ? `+${node.balanceFactor}` : node.balanceFactor}
                      </text>

                      {/* Height Pill */}
                      <text
                        x={0}
                        y={28}
                        fill="#526176"
                        fontSize="8"
                        fontFamily="monospace"
                        textAnchor="middle"
                        className="pointer-events-none select-none"
                      >
                        h={node.height}
                      </text>
                    </g>
                  );
                })}
              </svg>
            )}

            {activeRotation && (
              <div className="absolute bottom-3 right-3 bg-white/95 border border-[#DCE3EC] rounded-lg px-3 py-1.5 flex items-center space-x-2 text-xs font-mono text-[#172033] shadow-sm">
                <span className="text-[#D97706]">🔄</span>
                <span>
                  Active Rotation: <strong>{activeRotation}</strong>
                </span>
              </div>
            )}
          </div>
        </div>

        {/* Inorder Nodes Table (Key, Height, Balance Factor, Subtrees) */}
        <div className="bg-white rounded-xl border border-[#DCE3EC] p-4 flex flex-col space-y-3 shadow-sm">
          <div className="flex items-center justify-between border-b border-[#DCE3EC] pb-2">
            <span className="text-xs font-bold uppercase tracking-wider text-[#172033]">
              Sorted Inorder Records (BST)
            </span>
            <span className="text-[11px] text-[#526176] font-mono">
              {flattenedNodes.length} nodes
            </span>
          </div>

          <div className="flex-1 overflow-y-auto max-h-[360px] space-y-2 pr-1 font-mono text-xs">
            {flattenedNodes.length === 0 ? (
              <div className="p-4 rounded-lg bg-[#F8FAFC] text-[#718096] text-xs italic text-center">
                No nodes in tree at current trace step.
              </div>
            ) : (
              flattenedNodes.map((n) => {
                const isHl = highlightedKey === n.key;
                const bfValid = Math.abs(n.balanceFactor) <= 1;

                return (
                  <div
                    key={n.key}
                    className={`p-3 rounded-lg border transition-all ${
                      isHl
                        ? 'bg-sky-50 border-sky-300 ring-1 ring-sky-400'
                        : 'bg-[#F8FAFC] border-[#E2E8F0]'
                    }`}
                  >
                    <div className="flex items-center justify-between">
                      <div className="flex items-center space-x-2">
                        <span className="font-bold text-[#172033]">{n.key}</span>
                        {n.type && (
                          <span className="text-[11px] text-[#526176] font-sans">
                            {n.type}
                          </span>
                        )}
                      </div>

                      <div className="flex items-center space-x-1.5">
                        <span className="px-1.5 py-0.5 rounded bg-white border border-[#DCE3EC] text-[#526176] text-[10px]">
                          h={n.height}
                        </span>
                        <span
                          className={`px-1.5 py-0.5 rounded text-[10px] font-bold border ${
                            bfValid
                              ? 'bg-emerald-50 text-emerald-800 border-emerald-200'
                              : 'bg-red-50 text-red-800 border-red-200'
                          }`}
                        >
                          BF: {n.balanceFactor > 0 ? `+${n.balanceFactor}` : n.balanceFactor}
                        </span>
                      </div>
                    </div>

                    <div className="mt-2 grid grid-cols-2 gap-1 text-[10px] text-[#526176] pt-2 border-t border-[#E2E8F0]">
                      <div>
                        <span className="text-[#718096]">L Child:</span>{' '}
                        {n.leftChild || 'null'}
                      </div>
                      <div>
                        <span className="text-[#718096]">R Child:</span>{' '}
                        {n.rightChild || 'null'}
                      </div>
                    </div>
                  </div>
                );
              })
            )}
          </div>
        </div>
      </div>

      {/* Trace Player Reusable Controller */}
      {traceEvents.length > 0 && (
        <TracePlayerControls
          player={player}
          title="AVL Tree Balance & Rotation Step Telemetry"
          sourceFile="backend/include/dsa/avl_tree.hpp"
        />
      )}

      {/* Algorithm Details & Complexity Reference */}
      <ComplexityCard
        algorithmName="Custom Self-Balancing AVL Tree"
        problemSolved="Maintaining an ordered binary search tree of incident records with guaranteed logarithmic time bounds, preventing BST degeneration into an O(N) linked list."
        whyEmergencyGridUsesIt="Incident records must support range queries, sorted chronological traversal (inorder), and deterministic worst-case lookups. Unlike a standard binary search tree, AVL trees guarantee height h ≤ 1.44 log2(N + 2) by rebalancing after insertions and deletions."
        howItWorks="Every node tracks height h and balance factor BF = height(left) - height(right). When an insertion or deletion results in |BF| > 1, the unbalanced node is restored via constant-time O(1) pointer rotations: Single Left (RR), Single Right (LL), Double Left-Right (LR), or Double Right-Left (RL)."
        metrics={[
          {
            operation: "Exact Key Lookup",
            averageTime: "O(log N)",
            worstTime: "O(log N)",
            space: "O(1)",
            notes: "Strict height guarantee ensures worst-case search never exceeds 1.44 log2 N comparisons.",
          },
          {
            operation: "Insert with Rotation Rebalance",
            averageTime: "O(log N)",
            worstTime: "O(log N)",
            space: "O(log N)",
            notes: "Standard BST insertion followed by at most 1 single or double rotation to restore balance.",
          },
          {
            operation: "Delete with Rebalance",
            averageTime: "O(log N)",
            worstTime: "O(log N)",
            space: "O(log N)",
            notes: "Node replacement with inorder successor; rebalancing may propagate up to root (at most O(log N) rotations).",
          },
          {
            operation: "Inorder Traversal (Sorted Output)",
            averageTime: "O(N)",
            worstTime: "O(N)",
            space: "O(log N)",
            notes: "L-Root-R traversal produces strictly sorted incident IDs in linear time.",
          },
        ]}
        strengths={[
          "Guaranteed worst-case O(log N) time bound eliminates worst-case skew degenerations of basic BSTs.",
          "More rigidly balanced than Red-Black trees (shorter tree height), yielding faster average search queries.",
          "Produces sorted records in O(N) time without requiring an external sorting algorithm.",
        ]}
        limitations={[
          "Costly rebalancing on deletion compared to Red-Black trees (multiple rotations can propagate to root).",
          "Higher memory footprint per node (requires height field alongside key, value, left, and right pointers).",
        ]}
        cppHeader="backend/include/dsa/avl_tree.hpp"
        testFile="backend/tests/test_avl_tree.cpp"
      />
    </div>
  );
}
