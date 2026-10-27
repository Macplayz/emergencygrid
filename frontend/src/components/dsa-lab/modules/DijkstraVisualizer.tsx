'use client';

import React, { useState, useEffect, useMemo, useCallback, useRef } from 'react';
import { CityGraph, RouteResult, TraceEvent } from '@/lib/types';
import { api } from '@/lib/api';
import { useTracePlayer } from '@/lib/useTracePlayer';
import TracePlayerControls from '../common/TracePlayerControls';
import ComplexityCard from '../common/ComplexityCard';

interface DijkstraVisualizerProps {
  initialGraph?: CityGraph | null;
}

export default function DijkstraVisualizer({ initialGraph }: DijkstraVisualizerProps) {
  const [graph, setGraph] = useState<CityGraph | null>(initialGraph || null);
  const [loadingGraph, setLoadingGraph] = useState(!initialGraph);
  const [sourceId, setSourceId] = useState('J1');
  const [destId, setDestId] = useState('J16');
  const [calculating, setCalculating] = useState(false);
  const [routeResult, setRouteResult] = useState<RouteResult | null>(null);
  const [traceEvents, setTraceEvents] = useState<TraceEvent[]>([]);
  const [error, setError] = useState<string | null>(null);

  const reqIdRef = useRef(0);

  // Load city graph if not provided
  useEffect(() => {
    let cancelled = false;
    async function loadGraph() {
      if (!initialGraph) {
        setLoadingGraph(true);
        try {
          const res = await api.getCityGraph();
          if (!cancelled && res.success && res.data) {
            setGraph(res.data);
          }
        } finally {
          if (!cancelled) setLoadingGraph(false);
        }
      }
    }
    loadGraph();
    return () => {
      cancelled = true;
    };
  }, [initialGraph]);

  // Trace player controller
  const player = useTracePlayer(traceEvents, { defaultSpeedMs: 650 });
  const { currentEvent, currentStepIndex } = player;

  // Run Dijkstra route calculation
  const runCalculation = useCallback(async () => {
    if (!sourceId || !destId) {
      setError('Please select both a source and destination intersection.');
      return;
    }
    const currentReqId = ++reqIdRef.current;
    setError(null);
    setCalculating(true);
    player.pause();
    player.reset();

    try {
      const res = await api.calculateRouteWithTrace(sourceId, destId);
      if (currentReqId !== reqIdRef.current) return;
      if (res.success && res.data) {
        setRouteResult(res.data.route);
        setTraceEvents((res.trace as TraceEvent[]) || []);
      } else {
        setError(res.error?.message || 'Failed to calculate shortest path.');
      }
    } catch (err: unknown) {
      if (currentReqId !== reqIdRef.current) return;
      setError(err instanceof Error ? err.message : 'Calculation error');
    } finally {
      if (currentReqId === reqIdRef.current) {
        setCalculating(false);
      }
    }
  }, [sourceId, destId, player]);

  // Auto-run initial demonstration on mount once graph is loaded
  const initialRunRef = useRef(false);
  useEffect(() => {
    if (graph && graph.intersections.length > 0 && !initialRunRef.current) {
      initialRunRef.current = true;
      runCalculation();
    }
  }, [graph, runCalculation]);

  // Derive dynamic algorithm state from visible events up to current playback step
  const derivedState = useMemo(() => {
    const settledNodes = new Set<string>();
    const tentativeDistances: Record<string, number> = {};
    const sptPredecessors = new Map<string, { from: string; roadId: string }>();
    const sptRoadIds = new Set<string>();
    let activeNodeId: string | null = null;
    let activeEdge: { from: string; to: string; roadId?: string } | null = null;

    for (let i = 0; i <= currentStepIndex && i < traceEvents.length; i++) {
      const ev = traceEvents[i];
      if (ev.operation === 'initialize_distances' && ev.entityId) {
        tentativeDistances[ev.entityId] = 0;
      } else if (ev.operation === 'extract_min' && ev.entityId) {
        activeNodeId = ev.entityId;
        if (ev.cost !== undefined) tentativeDistances[ev.entityId] = ev.cost;
      } else if (ev.operation === 'finalize_node' && ev.entityId) {
        settledNodes.add(ev.entityId);
      } else if (ev.operation === 'examine_edge' && ev.from && ev.to) {
        activeEdge = { from: ev.from, to: ev.to, roadId: (ev.metadata?.roadId as string) || ev.entityId };
      } else if (ev.operation === 'relax_edge' && ev.to && ev.newValue !== undefined) {
        tentativeDistances[ev.to] = ev.newValue;
        const fromNode = ev.from || (ev.metadata?.fromNodeId as string);
        const roadId = (ev.metadata?.roadId as string) || ev.entityId;
        if (fromNode) {
          activeEdge = { from: fromNode, to: ev.to, roadId };
          if (roadId) {
            sptPredecessors.set(ev.to, { from: fromNode, roadId });
          }
        }
      } else if (ev.operation === 'destination_reached' || ev.operation === 'route_reconstructed') {
        activeEdge = null;
      }
    }

    // Accumulate the current Shortest-Path Tree (SPT) road IDs
    sptPredecessors.forEach((pred) => {
      sptRoadIds.add(pred.roadId);
    });

    // If at final step or route reconstructed, merge verified SPT road IDs from result
    if (player.isAtEnd || currentEvent?.operation === 'route_reconstructed') {
      routeResult?.shortestPathTreeRoadIds?.forEach((rId) => sptRoadIds.add(rId));
    }

    return { settledNodes, tentativeDistances, activeNodeId, activeEdge, sptRoadIds, sptPredecessors };
  }, [currentStepIndex, traceEvents, player.isAtEnd, currentEvent?.operation, routeResult]);

  // Dynamic bounding-box coordinate normalizer for full SVG utilization
  const { coords, viewBox } = useMemo(() => {
    const map = new Map<string, { x: number; y: number }>();
    const svgW = 600;
    const svgH = 340;
    const padX = 50;
    const padY = 40;

    if (!graph || graph.intersections.length === 0) {
      return { coords: map, viewBox: `0 0 ${svgW} ${svgH}` };
    }

    let minX = Infinity;
    let maxX = -Infinity;
    let minY = Infinity;
    let maxY = -Infinity;

    for (const node of graph.intersections) {
      if (node.x < minX) minX = node.x;
      if (node.x > maxX) maxX = node.x;
      if (node.y < minY) minY = node.y;
      if (node.y > maxY) maxY = node.y;
    }

    const spanX = maxX - minX || 1;
    const spanY = maxY - minY || 1;
    const availW = svgW - 2 * padX;
    const availH = svgH - 2 * padY;

    for (const node of graph.intersections) {
      const svgX = padX + ((node.x - minX) / spanX) * availW;
      const svgY = padY + ((node.y - minY) / spanY) * availH;
      map.set(node.id, { x: svgX, y: svgY });
    }

    return { coords: map, viewBox: `0 0 ${svgW} ${svgH}` };
  }, [graph]);

  // Set of edges on the final shortest path to destination
  const finalPathSet = useMemo(() => {
    if (!routeResult?.path || routeResult.path.length < 2) return new Set<string>();
    const s = new Set<string>();
    for (let i = 0; i < routeResult.path.length - 1; i++) {
      s.add(`${routeResult.path[i]}->${routeResult.path[i + 1]}`);
      s.add(`${routeResult.path[i + 1]}->${routeResult.path[i]}`);
    }
    return s;
  }, [routeResult]);

  // Set of confirmed road IDs forming the final destination route
  const finalRouteRoadIds = useMemo(() => {
    return new Set<string>(routeResult?.roadIds || []);
  }, [routeResult]);

  // Intermediate nodes on the final destination route (excluding source and destination)
  const intermediateNodeIds = useMemo(() => {
    if (!routeResult?.path || routeResult.path.length <= 2) return new Set<string>();
    return new Set<string>(routeResult.path.slice(1, -1));
  }, [routeResult]);

  // Destination route is active at the final trace step, 100% progress, or when playback stops at the end
  const showDestinationRoute = Boolean(
    routeResult?.routeFound &&
    (player.isAtEnd ||
     player.progressPercent >= 100 ||
     currentEvent?.operation === 'route_reconstructed')
  );

  // Sort roads so that confirmed destination route roads and candidate edges are drawn on top
  const sortedRoads = useMemo(() => {
    if (!graph?.roads) return [];
    return [...graph.roads].sort((a, b) => {
      const aIsRoute = showDestinationRoute && finalRouteRoadIds.has(a.id) ? 2 : 0;
      const bIsRoute = showDestinationRoute && finalRouteRoadIds.has(b.id) ? 2 : 0;
      return aIsRoute - bIsRoute;
    });
  }, [graph?.roads, finalRouteRoadIds, showDestinationRoute]);

  const settledCount = derivedState.settledNodes.size;
  const totalNodesCount = graph?.intersections.length || 0;
  const isSourceDestEqual = sourceId === destId;

  return (
    <div className="space-y-6">
      {/* Parameter Controls Bar */}
      <div className="bg-white rounded-xl border border-[#DCE3EC] p-4 shadow-sm">
        <div className="flex flex-wrap items-center justify-between gap-4">
          <div className="flex flex-wrap items-center gap-4">
            <div className="flex items-center space-x-2 text-xs">
              <label htmlFor="source-select" className="text-[#526176] font-semibold">
                Origin (Source):
              </label>
              <select
                id="source-select"
                aria-label="Select source intersection"
                value={sourceId}
                onChange={(e) => setSourceId(e.target.value)}
                className="bg-white border border-[#DCE3EC] rounded-lg px-3 py-1.5 text-[#172033] font-mono text-xs focus:outline-none focus:ring-2 focus:ring-[#3563D9]/20 focus:border-[#3563D9]"
              >
                {graph?.intersections.map((n) => (
                  <option key={n.id} value={n.id}>
                    {n.id} — {n.name}
                  </option>
                ))}
              </select>
            </div>

            <div className="flex items-center space-x-2 text-xs">
              <label htmlFor="dest-select" className="text-[#526176] font-semibold">
                Destination:
              </label>
              <select
                id="dest-select"
                aria-label="Select destination intersection"
                value={destId}
                onChange={(e) => setDestId(e.target.value)}
                className="bg-white border border-[#DCE3EC] rounded-lg px-3 py-1.5 text-[#172033] font-mono text-xs focus:outline-none focus:ring-2 focus:ring-[#3563D9]/20 focus:border-[#3563D9]"
              >
                {graph?.intersections.map((n) => (
                  <option key={n.id} value={n.id}>
                    {n.id} — {n.name}
                  </option>
                ))}
              </select>
            </div>

            {isSourceDestEqual && (
              <span className="text-[11px] font-mono text-amber-700 bg-amber-50 border border-amber-200 px-2 py-1 rounded">
                Edge Case: Source = Destination (0.00 min travel cost)
              </span>
            )}
          </div>

          <div className="flex items-center gap-3">
            <button
              type="button"
              onClick={runCalculation}
              disabled={calculating || loadingGraph}
              className="px-4 py-2 rounded-lg bg-[#3563D9] hover:bg-[#284eb3] disabled:opacity-50 text-white font-medium text-xs shadow-sm transition-colors flex items-center space-x-1.5 cursor-pointer"
            >
              <span>{calculating ? 'Running Dijkstra...' : 'Compute Route & Trace'}</span>
            </button>
          </div>
        </div>

        {error && (
          <div className="mt-3 p-3 rounded-lg bg-red-50 border border-red-200 text-xs text-red-700">
            {error}
          </div>
        )}
      </div>

      {/* Interactive Visualization Area */}
      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        {/* SVG Graph View */}
        <div className="lg:col-span-2 bg-white rounded-xl border border-[#DCE3EC] p-4 flex flex-col shadow-sm relative overflow-hidden min-h-[380px]">
          {/* Status & Shortest-Path Tree (SPT) Legend Bar */}
          <div className="flex flex-wrap items-center justify-between gap-2 border-b border-[#DCE3EC] pb-3 mb-2 text-xs">
            <div className="flex items-center space-x-2 font-medium text-[#172033]">
              <span>MetroGrid-16 Routing Topology</span>
              <span className="text-[#718096] font-mono text-[11px]">
                ({totalNodesCount} nodes, {graph?.roads.length || 0} roads)
              </span>
            </div>

            {/* Compact Legend */}
            <div className="flex flex-wrap items-center gap-3 text-[11px] text-[#526176]">
              <span className="flex items-center space-x-1.5" title="Confirmed edge in the Shortest-Path Tree (SPT)">
                <span className="w-3.5 h-1 rounded bg-[#0D9488]" />
                <span className="font-semibold text-[#0D9488]">SPT Edge</span>
              </span>
              <span className="flex items-center space-x-1.5" title="Optimal shortest path to destination">
                <span className="w-3.5 h-1 rounded bg-[#2563EB]" />
                <span className="font-semibold text-[#2563EB]">Destination Route</span>
              </span>
              <span className="flex items-center space-x-1.5" title="Road currently being examined or relaxed">
                <span className="w-3.5 h-1 rounded bg-[#D97706]" />
                <span className="font-semibold text-[#D97706]">Examining</span>
              </span>
              <span className="flex items-center space-x-1.5">
                <span className="w-2.5 h-2.5 rounded-full bg-[#15803D]" />
                <span>Settled Node</span>
              </span>
              <span className="flex items-center space-x-1.5">
                <span className="w-2.5 h-2.5 rounded-full bg-[#94A3B8]" />
                <span>Unvisited</span>
              </span>
              <span className="flex items-center space-x-1.5">
                <span className="w-3.5 h-0.5 border-b border-dashed border-[#DC2626]" />
                <span className="text-[#DC2626]">Closed Road</span>
              </span>
            </div>
          </div>

          {/* SVG Canvas */}
          <div className="flex-1 w-full flex items-center justify-center bg-[#F8FAFC] rounded-lg border border-[#E2E8F0] p-2 relative">
            {loadingGraph ? (
              <div className="text-center py-16 text-[#718096] text-xs">
                Loading city road network graph...
              </div>
            ) : (
              <svg viewBox={viewBox} className="w-full h-auto max-h-[460px] overflow-visible">
                {/* 1. LAYER: ROADS / EDGES */}
                {sortedRoads.map((r) => {
                  const source = r.sourceId || r.source || '';
                  const target = r.targetId || r.target || '';
                  const u = coords.get(source);
                  const v = coords.get(target);
                  if (!u || !v) return null;

                  const isCandidateEdge =
                    derivedState.activeEdge &&
                    ((derivedState.activeEdge.from === source && derivedState.activeEdge.to === target) ||
                      (derivedState.activeEdge.from === target && derivedState.activeEdge.to === source) ||
                      derivedState.activeEdge.roadId === r.id);

                  const isRouteRoad =
                    finalRouteRoadIds.has(r.id) ||
                    finalPathSet.has(`${source}->${target}`) ||
                    finalPathSet.has(`${target}->${source}`);

                  const isFinalDestinationRoute = isRouteRoad && showDestinationRoute;

                  const isSptEdge =
                    derivedState.sptRoadIds.has(r.id) ||
                    ((player.isAtEnd || currentEvent?.operation === 'route_reconstructed') &&
                      routeResult?.shortestPathTreeRoadIds?.includes(r.id));

                  let stroke = '#CBD5E1'; // Slate-300: neutral road
                  let strokeWidth = 2;
                  const strokeDasharray = r.isClosed ? '4,4' : undefined;

                  if (r.isClosed) {
                    stroke = '#DC2626'; // Red-600: closed road
                    strokeWidth = 2.5;
                  } else if (isFinalDestinationRoute) {
                    stroke = '#2563EB'; // Royal Blue: optimal destination route
                    strokeWidth = 4.5;
                  } else if (isCandidateEdge) {
                    stroke = '#D97706'; // Amber-600: active edge examined / relaxed
                    strokeWidth = 3.5;
                  } else if (isSptEdge) {
                    stroke = '#0D9488'; // Teal-600: confirmed Shortest-Path Tree edge
                    strokeWidth = 3;
                  }

                  const midX = (u.x + v.x) / 2;
                  const midY = (u.y + v.y) / 2;

                  return (
                    <g key={r.id}>
                      <line
                        x1={u.x}
                        y1={u.y}
                        x2={v.x}
                        y2={v.y}
                        stroke={stroke}
                        strokeWidth={strokeWidth}
                        strokeDasharray={strokeDasharray}
                        strokeLinecap="round"
                        className="transition-colors duration-200"
                      />
                      {/* Road Travel Cost Badge */}
                      <rect
                        x={midX - 16}
                        y={midY - 8}
                        width="32"
                        height="14"
                        rx="3"
                        fill="#FFFFFF"
                        stroke={isFinalDestinationRoute ? '#2563EB' : isSptEdge ? '#0D9488' : '#E2E8F0'}
                        strokeWidth={isFinalDestinationRoute ? '2' : isSptEdge ? '1.5' : '1'}
                        className="pointer-events-none select-none shadow-2xs"
                      />
                      <text
                        x={midX}
                        y={midY + 2.5}
                        fill={isFinalDestinationRoute ? '#2563EB' : isSptEdge ? '#0D9488' : '#526176'}
                        fontSize="8.5"
                        fontFamily="monospace"
                        fontWeight={isFinalDestinationRoute || isSptEdge ? 'bold' : 'normal'}
                        textAnchor="middle"
                        className="pointer-events-none select-none"
                      >
                        {r.travelTimeMinutes.toFixed(1)}m
                      </text>
                    </g>
                  );
                })}

                {/* 2. LAYER: INTERSECTIONS / NODES */}
                {graph?.intersections.map((node) => {
                  const pt = coords.get(node.id);
                  if (!pt) return null;

                  const isSource = node.id === sourceId;
                  const isDest = node.id === destId;
                  const isIntermediate = showDestinationRoute && intermediateNodeIds.has(node.id);
                  const isSettled = derivedState.settledNodes.has(node.id);
                  const isActive = derivedState.activeNodeId === node.id;

                  let fillColor = '#FFFFFF';
                  let strokeColor = '#94A3B8'; // Slate-400
                  let strokeWidth = 2;
                  let r = 12;

                  if (isActive) {
                    fillColor = '#E0F2FE'; // Sky-100
                    strokeColor = '#087E8B'; // Teal-700
                    strokeWidth = 3;
                    r = 14;
                  } else if (isIntermediate) {
                    fillColor = isSettled ? '#DCFCE7' : '#EFF6FF'; // Green-100 if settled, Sky-50 otherwise
                    strokeColor = '#2563EB'; // Royal Blue for intermediate path node
                    strokeWidth = 3;
                  } else if (isSource) {
                    fillColor = isSettled ? '#DCFCE7' : '#DBEAFE'; // Green-100 if settled, Blue-100 otherwise
                    strokeColor = '#2563EB'; // Primary Royal Blue
                    strokeWidth = 3;
                  } else if (isDest) {
                    fillColor = isSettled ? '#DCFCE7' : '#F3E8FF'; // Green-100 if settled, Purple-100 otherwise
                    strokeColor = '#9333EA'; // Purple
                    strokeWidth = 3;
                  } else if (isSettled) {
                    fillColor = '#DCFCE7'; // Green-100
                    strokeColor = '#15803D'; // Green-700
                    strokeWidth = 2.5;
                  }

                  const dist = derivedState.tentativeDistances[node.id];

                  return (
                    <g key={node.id} className="cursor-pointer">
                      {/* Active highlight halo */}
                      {isActive && (
                        <circle cx={pt.x} cy={pt.y} r={r + 6} fill="#087E8B" opacity={0.18} />
                      )}

                      {/* Source/Destination distinct ring & tag */}
                      {isSource && isDest ? (
                        <>
                          <circle cx={pt.x} cy={pt.y} r={r + 5} fill="none" stroke="#4F46E5" strokeWidth={2} />
                          <rect x={pt.x - 24} y={pt.y - 23} width="48" height="11" rx="3" fill="#4F46E5" />
                          <text x={pt.x} y={pt.y - 15} fill="#FFFFFF" fontSize="7" fontWeight="bold" textAnchor="middle">SRC & DST</text>
                        </>
                      ) : isSource ? (
                        <>
                          <circle cx={pt.x} cy={pt.y} r={r + 5} fill="none" stroke="#2563EB" strokeWidth={2} strokeDasharray="3,2" />
                          <rect x={pt.x - 14} y={pt.y - 23} width="28" height="11" rx="3" fill="#2563EB" />
                          <text x={pt.x} y={pt.y - 15} fill="#FFFFFF" fontSize="7" fontWeight="bold" textAnchor="middle">SRC</text>
                        </>
                      ) : isDest ? (
                        <>
                          <circle cx={pt.x} cy={pt.y} r={r + 5} fill="none" stroke="#9333EA" strokeWidth={2} strokeDasharray="3,2" />
                          <rect x={pt.x - 14} y={pt.y - 23} width="28" height="11" rx="3" fill="#9333EA" />
                          <text x={pt.x} y={pt.y - 15} fill="#FFFFFF" fontSize="7" fontWeight="bold" textAnchor="middle">DST</text>
                        </>
                      ) : isIntermediate ? (
                        <>
                          <circle cx={pt.x} cy={pt.y} r={r + 5} fill="none" stroke="#2563EB" strokeWidth={2} strokeDasharray="3,2" />
                          <rect x={pt.x - 14} y={pt.y - 23} width="28" height="11" rx="3" fill="#2563EB" />
                          <text x={pt.x} y={pt.y - 15} fill="#FFFFFF" fontSize="7" fontWeight="bold" textAnchor="middle">VIA</text>
                        </>
                      ) : null}

                      {/* Main Node Circle */}
                      <circle
                        cx={pt.x}
                        cy={pt.y}
                        r={r}
                        fill={fillColor}
                        stroke={strokeColor}
                        strokeWidth={strokeWidth}
                        className="transition-all duration-200 shadow-sm"
                      />
                      <text
                        x={pt.x}
                        y={pt.y + 3.5}
                        fill="#172033"
                        fontSize="9.5"
                        fontWeight="bold"
                        fontFamily="monospace"
                        textAnchor="middle"
                        className="pointer-events-none select-none"
                      >
                        {node.id}
                      </text>

                      {/* Tentative Distance Label Badge */}
                      {dist !== undefined ? (
                        <g>
                          <rect
                            x={pt.x - 18}
                            y={pt.y + 15}
                            width="36"
                            height="14"
                            rx="3"
                            fill={isSettled ? '#F0FDF4' : '#F0F9FF'}
                            stroke={isSettled ? '#86EFAC' : '#BAE6FD'}
                            strokeWidth="1"
                          />
                          <text
                            x={pt.x}
                            y={pt.y + 25}
                            fill={isSettled ? '#15803D' : '#0369A1'}
                            fontSize="8.5"
                            fontFamily="monospace"
                            fontWeight="bold"
                            textAnchor="middle"
                            className="pointer-events-none select-none"
                          >
                            {dist.toFixed(1)}m
                          </text>
                        </g>
                      ) : (
                        <g>
                          <rect
                            x={pt.x - 12}
                            y={pt.y + 15}
                            width="24"
                            height="14"
                            rx="3"
                            fill="#F8FAFC"
                            stroke="#E2E8F0"
                            strokeWidth="1"
                          />
                          <text
                            x={pt.x}
                            y={pt.y + 25}
                            fill="#94A3B8"
                            fontSize="8.5"
                            fontFamily="monospace"
                            textAnchor="middle"
                            className="pointer-events-none select-none"
                          >
                            ∞
                          </text>
                        </g>
                      )}
                    </g>
                  );
                })}
              </svg>
            )}
          </div>

          {/* Accurate Telemetry & Shortest-Path Tree Summary Bar */}
          {routeResult && (
            <div className="mt-3 flex flex-wrap items-center justify-between gap-3 p-3 bg-white rounded-lg border border-[#DCE3EC] text-xs font-mono">
              <div className="flex flex-wrap items-center gap-3">
                <span className="flex items-center space-x-1">
                  <span className="text-[#526176]">Origin:</span>
                  <strong className="text-[#2563EB]">{routeResult.sourceId}</strong>
                </span>
                <span className="text-[#DCE3EC]">|</span>
                <span className="flex items-center space-x-1">
                  <span className="text-[#526176]">Destination:</span>
                  <strong className="text-[#9333EA]">{routeResult.destinationId}</strong>
                </span>
                <span className="text-[#DCE3EC]">|</span>
                <span className="flex items-center space-x-1">
                  <span className="text-[#526176]">Travel Time:</span>
                  <strong className={routeResult.routeFound ? 'text-[#15803D]' : 'text-[#DC2626]'}>
                    {routeResult.routeFound ? `${routeResult.totalTravelTimeMinutes.toFixed(2)} min` : 'Unreachable'}
                  </strong>
                </span>
                {routeResult.routeFound && (
                  <>
                    <span className="text-[#DCE3EC]">|</span>
                    <span className="flex items-center space-x-1">
                      <span className="text-[#526176]">Distance:</span>
                      <strong className="text-[#172033]">{routeResult.totalDistanceKm.toFixed(1)} km</strong>
                    </span>
                  </>
                )}
              </div>

              <div className="flex flex-wrap items-center gap-3 text-[11px]">
                <span className="flex items-center space-x-1">
                  <span className="text-[#526176]">Settled Nodes:</span>
                  <strong className="text-[#15803D]">{derivedState.settledNodes.size} / {totalNodesCount}</strong>
                </span>
                <span className="text-[#DCE3EC]">|</span>
                <span className="flex items-center space-x-1" title="Confirmed edges forming the Shortest-Path Tree from origin">
                  <span className="text-[#526176]">SPT Edges:</span>
                  <strong className="text-[#0D9488]">
                    {player.isAtEnd || currentEvent?.operation === 'route_reconstructed'
                      ? (routeResult.shortestPathTreeRoadIds?.length ?? derivedState.sptRoadIds.size)
                      : derivedState.sptRoadIds.size}
                  </strong>
                </span>
                <span className="text-[#DCE3EC]">|</span>
                <span className="flex items-center space-x-1">
                  <span className="text-[#526176]">Relaxations:</span>
                  <strong className="text-[#D97706]">{routeResult.relaxations}</strong>
                </span>
              </div>
            </div>
          )}

          {/* Unreachable Destination Notice */}
          {routeResult && !routeResult.routeFound && routeResult.sourceId !== routeResult.destinationId && (
            <div className="mt-2 p-2.5 rounded-lg bg-amber-50 border border-amber-200 text-xs text-amber-800 flex items-center space-x-2">
              <span className="font-bold">Notice:</span>
              <span>
                Destination {routeResult.destinationId} is unreachable from {routeResult.sourceId} due to network disconnection or road closures. The Shortest-Path Tree (SPT) reflects all reachable nodes explored by Dijkstra before termination.
              </span>
            </div>
          )}
        </div>

        {/* Tentative Distances, Predecessor & Priority Queue State Table */}
        <div className="bg-white rounded-xl border border-[#DCE3EC] p-4 flex flex-col space-y-3 shadow-sm">
          <div className="flex items-center justify-between border-b border-[#DCE3EC] pb-2">
            <span className="text-xs font-bold uppercase tracking-wider text-[#172033]">
              Distance Table d[v] & SPT Parent
            </span>
            <span className="text-[11px] text-[#526176] font-mono">
              {settledCount} of {totalNodesCount} settled
            </span>
          </div>

          <div className="flex-1 overflow-y-auto max-h-[420px] space-y-1.5 pr-1 font-mono text-xs">
            {graph?.intersections.map((node) => {
              const dist = derivedState.tentativeDistances[node.id];
              const isSettled = derivedState.settledNodes.has(node.id);
              const isActive = derivedState.activeNodeId === node.id;
              const pred = derivedState.sptPredecessors.get(node.id);

              return (
                <div
                  key={node.id}
                  className={`flex items-center justify-between px-3 py-2 rounded-lg border transition-colors ${
                    isActive
                      ? 'bg-sky-50 border-sky-300 text-sky-950 font-semibold'
                      : isSettled
                      ? 'bg-emerald-50/70 border-emerald-200 text-emerald-950'
                      : dist !== undefined
                      ? 'bg-amber-50/60 border-amber-200 text-amber-950'
                      : 'bg-[#F8FAFC] border-slate-100 text-[#718096]'
                  }`}
                >
                  <div className="flex flex-col">
                    <div className="flex items-center space-x-2">
                      <span className="font-bold text-[#172033]">{node.id}</span>
                      <span className="text-[11px] text-[#526176] font-sans truncate max-w-[90px]">
                        {node.name}
                      </span>
                    </div>
                    {/* SPT Predecessor Indicator */}
                    <span className="text-[10px] text-[#526176] font-sans">
                      {node.id === sourceId ? (
                        <span className="text-[#2563EB] font-medium">Origin Root</span>
                      ) : pred ? (
                        <span>
                          Parent: <strong className="text-[#0D9488] font-mono">{pred.from}</strong> ({pred.roadId})
                        </span>
                      ) : (
                        <span className="text-[#94A3B8]">No parent</span>
                      )}
                    </span>
                  </div>

                  <div className="flex items-center space-x-2">
                    <span className="font-semibold text-xs">
                      {dist !== undefined ? `${dist.toFixed(1)}m` : '∞'}
                    </span>
                    <span
                      className={`text-[9px] px-1.5 py-0.5 rounded font-sans uppercase font-bold tracking-wider ${
                        isSettled
                          ? 'bg-emerald-100 text-emerald-800 border border-emerald-200'
                          : isActive
                          ? 'bg-sky-100 text-sky-800 border border-sky-200'
                          : dist !== undefined
                          ? 'bg-amber-100 text-amber-800 border border-amber-200'
                          : 'bg-slate-100 text-slate-500 border border-slate-200'
                      }`}
                    >
                      {isSettled ? 'Settled' : isActive ? 'Active' : dist !== undefined ? 'Queued' : 'Unvisited'}
                    </span>
                  </div>
                </div>
              );
            })}
          </div>
        </div>
      </div>

      {/* Trace Player Reusable Controller */}
      <TracePlayerControls
        player={player}
        title="Dijkstra Shortest-Path Tree (SPT) Execution Telemetry"
        sourceFile="backend/src/dsa/dijkstra.cpp"
      />

      {/* Educational Viva Reference */}
      <ComplexityCard
        algorithmName="Dijkstra's Single-Source Shortest-Path Tree (SPT)"
        problemSolved="Finding the minimum cumulative travel time from an emergency responder origin to an incident scene over a weighted road network, and building the Shortest-Path Tree spanning all reachable intersections."
        whyEmergencyGridUsesIt="Emergency vehicles require optimal routing under dynamically changing road conditions (congestion multipliers, emergency closures). Dijkstra guarantees optimal paths without exhaustive all-pairs recomputation."
        howItWorks="Maintains tentative distance array d[] initialized to ∞ (0 at source). Uses a binary min-heap priority queue to repeatedly extract the unvisited node u with minimum d[u], relaxes all outgoing candidate edges (u, v) where d[u] + w(u, v) < d[v], updates predecessor pointers to form the Shortest-Path Tree, and terminates when destination is settled or all reachable nodes have been finalized."
        metrics={[
          {
            operation: "Min-Heap Extract-Min",
            averageTime: "O(log V)",
            worstTime: "O(log V)",
            space: "O(V)",
            notes: "Binary min-heap stores up to V active candidate intersection records.",
          },
          {
            operation: "Edge Relaxation",
            averageTime: "O(log V)",
            worstTime: "O(log V)",
            space: "O(1)",
            notes: "Called at most E times across graph edges; updates predecessor pointers and inserts updated tentative distance into heap.",
          },
          {
            operation: "Total Algorithm Time",
            averageTime: "O((V + E) log V)",
            worstTime: "O((V + E) log V)",
            space: "O(V + E)",
            notes: "Matches implementation: Adjacency list representation + std::priority_queue min-heap.",
          },
        ]}
        strengths={[
          "Guarantees mathematically optimal shortest paths and a complete Shortest-Path Tree (SPT) for all non-negative edge weights.",
          "Can terminate early as soon as the target destination node is extracted from the min-heap.",
          "Naturally adapts to dynamic weight updates (congestion multiplier changes) on next query.",
        ]}
        limitations={[
          "Cannot handle negative edge weights (requires Bellman-Ford algorithm, O(V * E)).",
          "Explores equally in all directions unless guided by a heuristic (e.g., A* search with Euclidean heuristic).",
          "Computes a Shortest-Path Tree (SPT) from one origin, distinct from a Minimum Spanning Tree (MST) which minimizes total graph wire length.",
        ]}
        cppHeader="backend/include/dsa/dijkstra.hpp"
        cppSource="backend/src/dsa/dijkstra.cpp"
        testFile="backend/tests/test_graph_dijkstra.cpp"
      />
    </div>
  );
}
