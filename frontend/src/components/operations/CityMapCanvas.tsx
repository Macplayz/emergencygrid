'use client';

import { useState, useMemo } from 'react';
import { CityGraph, Intersection, RoadSegment, Incident, EmergencyUnit, RouteResult } from '@/lib/types';

interface CityMapCanvasProps {
  graph: CityGraph | null;
  incidents: Incident[];
  units: EmergencyUnit[];
  activeRoute: RouteResult | null;
  selectedIncident: Incident | null;
  selectedUnit: EmergencyUnit | null;
  selectedRoad: RoadSegment | null;
  onSelectIncident: (incident: Incident | null) => void;
  onSelectUnit: (unit: EmergencyUnit | null) => void;
  onSelectRoad: (road: RoadSegment | null) => void;
  onSelectIntersection: (intersection: Intersection | null) => void;
}

const ZONE_COLORS: Record<string, { bg: string; border: string; text: string }> = {
  Commercial: { bg: '#EEF2FF', border: '#4F46E5', text: '#312E81' },
  Residential: { bg: '#ECFDF5', border: '#10B981', text: '#064E3B' },
  Industrial: { bg: '#FEF3C7', border: '#F59E0B', text: '#78350F' },
  Civic: { bg: '#E0F2FE', border: '#0284C7', text: '#0C4A6E' },
  Medical: { bg: '#FFE4E6', border: '#F43F5E', text: '#881337' },
};

export default function CityMapCanvas({
  graph,
  incidents,
  units,
  activeRoute,
  selectedIncident,
  selectedUnit,
  selectedRoad,
  onSelectIncident,
  onSelectUnit,
  onSelectRoad,
  onSelectIntersection,
}: CityMapCanvasProps) {
  const [hoveredNode, setHoveredNode] = useState<Intersection | null>(null);
  const [hoveredRoad, setHoveredRoad] = useState<RoadSegment | null>(null);
  const [showTravelTimes, setShowTravelTimes] = useState<boolean>(true);
  const [selectedIntersection, setSelectedIntersectionState] = useState<Intersection | null>(null);

  // Quick lookup maps
  const nodeMap = useMemo(() => {
    const map = new Map<string, Intersection>();
    if (!graph?.intersections) return map;
    for (const node of graph.intersections) {
      map.set(node.id, node);
    }
    return map;
  }, [graph]);

  // Group incidents by location
  const incidentsByLocation = useMemo(() => {
    const map = new Map<string, Incident[]>();
    for (const inc of incidents) {
      if (inc.status === 'RESOLVED' || inc.status === 'CANCELLED') continue;
      const list = map.get(inc.locationId) || [];
      list.push(inc);
      map.set(inc.locationId, list);
    }
    return map;
  }, [incidents]);

  // Group units by location
  const unitsByLocation = useMemo(() => {
    const map = new Map<string, EmergencyUnit[]>();
    for (const u of units) {
      const list = map.get(u.locationId) || [];
      list.push(u);
      map.set(u.locationId, list);
    }
    return map;
  }, [units]);

  // Determine route nodes and edges if available
  const routePathNodes = useMemo(() => {
    if (!activeRoute || !activeRoute.routeFound || !activeRoute.path) return [];
    return activeRoute.path;
  }, [activeRoute]);

  const routeRoadIdsSet = useMemo(() => {
    if (!activeRoute || !activeRoute.roadIds) return new Set<string>();
    return new Set(activeRoute.roadIds);
  }, [activeRoute]);

  // Handle intersection selection
  const handleNodeClick = (node: Intersection) => {
    setSelectedIntersectionState(node);
    onSelectIntersection(node);
    const incs = incidentsByLocation.get(node.id);
    if (incs && incs.length > 0) {
      onSelectIncident(incs[0]);
    }
  };

  return (
    <div className="bg-white border border-[#DCE3EC] rounded-xl overflow-hidden shadow-xs flex flex-col">
      {/* Canvas Header & Toolbar */}
      <div className="px-4 py-3 border-b border-[#DCE3EC] bg-white flex flex-wrap items-center justify-between gap-2 text-xs">
        <div className="flex items-center space-x-2">
          <span className="w-2.5 h-2.5 rounded-full bg-[#087E8B]" />
          <h2 className="font-bold text-[#172033] tracking-wide text-xs">
            MetroGrid-16 City Map
          </h2>
          <span className="text-[11px] text-[#718096] font-mono hidden sm:inline">
            (16 Intersections • 26 Road Segments)
          </span>
        </div>

        {/* Toolbar Toggles */}
        <div className="flex items-center space-x-2">
          <button
            type="button"
            onClick={() => setShowTravelTimes(!showTravelTimes)}
            className={`px-2.5 py-1 rounded-md text-[11px] font-medium border transition-colors cursor-pointer ${
              showTravelTimes
                ? 'bg-sky-50 text-sky-800 border-sky-300'
                : 'bg-white text-[#526176] border-[#DCE3EC] hover:text-[#172033]'
            }`}
          >
            {showTravelTimes ? 'Weights Visible' : 'Weights Hidden'}
          </button>

          {(selectedIncident || selectedUnit || selectedRoad || activeRoute) && (
            <button
              type="button"
              onClick={() => {
                onSelectIncident(null);
                onSelectUnit(null);
                onSelectRoad(null);
                onSelectIntersection(null);
                setSelectedIntersectionState(null);
              }}
              className="px-2.5 py-1 rounded-md text-[11px] font-medium bg-white hover:bg-slate-50 text-[#526176] border border-[#DCE3EC] transition-colors cursor-pointer shadow-xs"
            >
              Clear Focus
            </button>
          )}
        </div>
      </div>

      {/* Main SVG Visualization */}
      <div className="relative w-full aspect-[16/9] min-h-[380px] sm:min-h-[460px] bg-[#F8FAFC] flex items-center justify-center p-2 sm:p-4 select-none">
        {/* Subtle grid pattern background */}
        <div
          className="absolute inset-0 opacity-20 pointer-events-none"
          style={{
            backgroundImage: `radial-gradient(circle at 1px 1px, #CBD5E1 1px, transparent 0)`,
            backgroundSize: '24px 24px',
          }}
        />

        {!graph ? (
          <div className="text-center py-20 text-[#718096] text-sm">
            Loading MetroGrid-16 road network topology...
          </div>
        ) : (
          <svg
            viewBox="0 0 350 185"
            className="w-full h-full max-h-[580px] overflow-visible"
            preserveAspectRatio="xMidYMid meet"
          >
            <defs>
              {/* Route Glow Filter */}
              <filter id="routeGlow" x="-20%" y="-20%" width="140%" height="140%">
                <feGaussianBlur stdDeviation="2.5" result="blur" />
                <feComposite in="SourceGraphic" in2="blur" operator="over" />
              </filter>
            </defs>

            {/* 1. LAYER: ROADS / EDGES */}
            <g id="roads-layer">
              {graph.roads.map((road) => {
                const sId = road.sourceId || road.source || '';
                const tId = road.targetId || road.target || '';
                const source = nodeMap.get(sId);
                const target = nodeMap.get(tId);
                if (!source || !target) return null;

                const isClosed = road.isClosed;
                const isCongested = road.congestionFactor > 1.05;
                const isSelected = selectedRoad?.id === road.id;
                const isHovered = hoveredRoad?.id === road.id;
                const isRouteEdge = routeRoadIdsSet.has(road.id);

                let strokeColor = '#CBD5E1'; // Slate-300
                let strokeWidth = 2.0;
                let strokeDash = undefined;

                if (isClosed) {
                  strokeColor = '#DC2626'; // Red-600
                  strokeWidth = 2.8;
                  strokeDash = '4 3';
                } else if (isRouteEdge) {
                  strokeColor = '#0284C7'; // Sky-600
                  strokeWidth = 3.5;
                } else if (isCongested) {
                  strokeColor = '#D97706'; // Amber-600
                  strokeWidth = 2.4;
                }

                if (isSelected || isHovered) {
                  strokeColor = '#3563D9'; // Blue-600
                  strokeWidth = 3.5;
                }

                const midX = (source.x + target.x) / 2;
                const midY = (source.y + target.y) / 2;

                return (
                  <g key={road.id} className="cursor-pointer group">
                    {/* Invisible Wider Hitbox for Clicking */}
                    <line
                      x1={source.x}
                      y1={source.y}
                      x2={target.x}
                      y2={target.y}
                      stroke="transparent"
                      strokeWidth={14}
                      strokeLinecap="round"
                      onClick={() => onSelectRoad(road)}
                      onMouseEnter={() => setHoveredRoad(road)}
                      onMouseLeave={() => setHoveredRoad(null)}
                    />

                    {/* Actual Road Line */}
                    <line
                      x1={source.x}
                      y1={source.y}
                      x2={target.x}
                      y2={target.y}
                      stroke={strokeColor}
                      strokeWidth={strokeWidth}
                      strokeDasharray={strokeDash}
                      strokeLinecap="round"
                      className="transition-all duration-200"
                    />

                    {/* Closed Road Warning X at Midpoint */}
                    {isClosed && (
                      <g transform={`translate(${midX}, ${midY})`}>
                        <circle r="5" fill="#DC2626" stroke="#991B1B" strokeWidth="1" />
                        <text
                          y="2.5"
                          textAnchor="middle"
                          fill="#FFFFFF"
                          fontSize="6"
                          fontWeight="bold"
                        >
                          ✕
                        </text>
                      </g>
                    )}

                    {/* Travel Time Tag on Congested or Hovered/Selected Road */}
                    {showTravelTimes && !isClosed && (
                      <g
                        transform={`translate(${midX}, ${midY})`}
                        className="pointer-events-none"
                      >
                        <rect
                          x="-8"
                          y="-4"
                          width="16"
                          height="8"
                          rx="2"
                          fill="#FFFFFF"
                          stroke={
                            isCongested
                              ? '#D97706'
                              : isSelected
                              ? '#3563D9'
                              : '#DCE3EC'
                          }
                          strokeWidth="0.8"
                        />
                        <text
                          y="1.8"
                          textAnchor="middle"
                          fill={isCongested ? '#B45309' : '#526176'}
                          fontSize="4.5"
                          fontFamily="monospace"
                          fontWeight="bold"
                        >
                          {road.travelTimeMinutes ? road.travelTimeMinutes.toFixed(1) : '0'}m
                        </text>
                      </g>
                    )}
                  </g>
                );
              })}
            </g>

            {/* 2. LAYER: ACTIVE DIJKSTRA ROUTE OVERLAY */}
            {routePathNodes.length > 1 && (
              <g id="route-highlight-layer" className="pointer-events-none">
                {/* Under-line */}
                <path
                  d={routePathNodes
                    .map((id, index) => {
                      const node = nodeMap.get(id);
                      if (!node) return '';
                      return `${index === 0 ? 'M' : 'L'} ${node.x} ${node.y}`;
                    })
                    .join(' ')}
                  fill="none"
                  stroke="#0284C7"
                  strokeWidth="6"
                  strokeLinecap="round"
                  strokeLinejoin="round"
                  opacity="0.25"
                />

                {/* Core route path */}
                <path
                  d={routePathNodes
                    .map((id, index) => {
                      const node = nodeMap.get(id);
                      if (!node) return '';
                      return `${index === 0 ? 'M' : 'L'} ${node.x} ${node.y}`;
                    })
                    .join(' ')}
                  fill="none"
                  stroke="#0284C7"
                  strokeWidth="3"
                  strokeDasharray="6 3"
                  strokeLinecap="round"
                  strokeLinejoin="round"
                />

                {/* Origin & Destination Rings */}
                {(() => {
                  const originNode = nodeMap.get(routePathNodes[0]);
                  const destNode = nodeMap.get(routePathNodes[routePathNodes.length - 1]);
                  return (
                    <>
                      {originNode && (
                        <circle
                          cx={originNode.x}
                          cy={originNode.y}
                          r="12"
                          fill="none"
                          stroke="#15803D"
                          strokeWidth="1.5"
                          strokeDasharray="2 2"
                        />
                      )}
                      {destNode && (
                        <circle
                          cx={destNode.x}
                          cy={destNode.y}
                          r="12"
                          fill="none"
                          stroke="#DC2626"
                          strokeWidth="1.5"
                          strokeDasharray="2 2"
                        />
                      )}
                    </>
                  );
                })()}
              </g>
            )}

            {/* 3. LAYER: INTERSECTIONS / NODES */}
            <g id="nodes-layer">
              {graph.intersections.map((node) => {
                const zoneStyle = ZONE_COLORS[node.zone] || {
                  bg: '#F8FAFC',
                  border: '#94A3B8',
                  text: '#172033',
                };
                const isSelected = selectedIntersection?.id === node.id;
                const isHovered = hoveredNode?.id === node.id;
                const isRouteNode = routePathNodes.includes(node.id);
                const nodeIncidents = incidentsByLocation.get(node.id) || [];
                const nodeUnits = unitsByLocation.get(node.id) || [];

                return (
                  <g
                    key={node.id}
                    className="cursor-pointer group"
                    onClick={() => handleNodeClick(node)}
                    onMouseEnter={() => setHoveredNode(node)}
                    onMouseLeave={() => setHoveredNode(null)}
                  >
                    {/* Node Circle */}
                    <circle
                      cx={node.x}
                      cy={node.y}
                      r={isSelected ? 10 : 8}
                      fill={isRouteNode ? '#E0F2FE' : '#FFFFFF'}
                      stroke={
                        isSelected
                          ? '#3563D9'
                          : isRouteNode
                          ? '#0284C7'
                          : isHovered
                          ? '#087E8B'
                          : zoneStyle.border
                      }
                      strokeWidth={isSelected ? 2.5 : isRouteNode ? 2.2 : 1.5}
                      className="transition-all duration-150 shadow-xs"
                    />

                    {/* Node ID label inside circle */}
                    <text
                      x={node.x}
                      y={node.y + 2.2}
                      textAnchor="middle"
                      fill={isSelected ? '#3563D9' : '#172033'}
                      fontSize="5"
                      fontFamily="monospace"
                      fontWeight="bold"
                      className="pointer-events-none select-none"
                    >
                      {node.id}
                    </text>

                    {/* Node Name below circle */}
                    <text
                      x={node.x}
                      y={node.y + 13}
                      textAnchor="middle"
                      fill="#526176"
                      fontSize="3.8"
                      fontFamily="sans-serif"
                      fontWeight="500"
                      className="pointer-events-none select-none tracking-tight"
                    >
                      {node.name}
                    </text>

                    {/* 4. MARKERS: INCIDENTS AT THIS INTERSECTION */}
                    {nodeIncidents.length > 0 && (
                      <g transform={`translate(${node.x + 8}, ${node.y - 8})`}>
                        <circle
                          r="5.5"
                          fill="#DC2626"
                          stroke="#FFFFFF"
                          strokeWidth="0.8"
                          className="shadow-sm"
                        />
                        <text
                          y="1.8"
                          textAnchor="middle"
                          fill="#FFFFFF"
                          fontSize="4"
                          fontWeight="900"
                          fontFamily="sans-serif"
                        >
                          !{nodeIncidents.length > 1 ? nodeIncidents.length : ''}
                        </text>
                      </g>
                    )}

                    {/* 5. MARKERS: UNITS AT THIS INTERSECTION */}
                    {nodeUnits.length > 0 && (
                      <g transform={`translate(${node.x - 8}, ${node.y - 8})`}>
                        <circle
                          r="5.2"
                          fill={nodeUnits[0].status === 'AVAILABLE' ? '#15803D' : '#D97706'}
                          stroke="#FFFFFF"
                          strokeWidth="0.8"
                        />
                        <text
                          y="1.8"
                          textAnchor="middle"
                          fill="#FFFFFF"
                          fontSize="3.8"
                          fontWeight="bold"
                          fontFamily="monospace"
                        >
                          {nodeUnits[0].serviceType.slice(0, 1)}
                        </text>
                      </g>
                    )}
                  </g>
                );
              })}
            </g>
          </svg>
        )}

        {/* Hover / Selection Quick Inspector Card (Bottom-Left overlay) */}
        {(hoveredNode || selectedIntersection || hoveredRoad || selectedRoad) && (
          <div className="absolute bottom-3 left-3 bg-white/95 border border-[#DCE3EC] rounded-lg p-3 shadow-lg max-w-xs text-xs pointer-events-none text-[#172033]">
            {hoveredRoad || selectedRoad ? (
              <div>
                <div className="flex items-center justify-between gap-2">
                  <span className="font-mono font-bold text-[#3563D9]">
                    Road: {(hoveredRoad || selectedRoad)?.id}
                  </span>
                  <span
                    className={`px-1.5 py-0.2 rounded text-[10px] font-bold border ${
                      (hoveredRoad || selectedRoad)?.isClosed
                        ? 'bg-red-50 text-red-800 border-red-200'
                        : 'bg-emerald-50 text-emerald-800 border-emerald-200'
                    }`}
                  >
                    {(hoveredRoad || selectedRoad)?.isClosed ? 'CLOSED' : 'OPEN'}
                  </span>
                </div>
                <div className="mt-1 text-[#526176] text-[11px] font-mono">
                  {(hoveredRoad || selectedRoad)?.sourceId} ⟷ {(hoveredRoad || selectedRoad)?.targetId}
                </div>
                <div className="mt-1.5 grid grid-cols-2 gap-x-2 text-[10px] text-[#718096]">
                  <div>
                    Length: <span className="text-[#172033] font-medium">{(hoveredRoad || selectedRoad)?.baseLengthKm} km</span>
                  </div>
                  <div>
                    Speed: <span className="text-[#172033] font-medium">{(hoveredRoad || selectedRoad)?.speedLimitKmh} km/h</span>
                  </div>
                  <div>
                    Congestion: <span className="text-[#D97706] font-medium">{(hoveredRoad || selectedRoad)?.congestionFactor}x</span>
                  </div>
                  <div>
                    Travel Time:{' '}
                    <span className="text-[#15803D] font-bold">
                      {(hoveredRoad || selectedRoad)?.travelTimeMinutes.toFixed(2)} min
                    </span>
                  </div>
                </div>
              </div>
            ) : (
              <div>
                <div className="flex items-center justify-between gap-2">
                  <span className="font-mono font-bold text-[#172033]">
                    {(hoveredNode || selectedIntersection)?.id} — {(hoveredNode || selectedIntersection)?.name}
                  </span>
                  <span className="px-1.5 py-0.2 rounded text-[10px] font-medium bg-slate-100 text-[#526176] border border-slate-200">
                    {(hoveredNode || selectedIntersection)?.zone}
                  </span>
                </div>
                {(() => {
                  const nId = (hoveredNode || selectedIntersection)?.id || '';
                  const incs = incidentsByLocation.get(nId) || [];
                  const unts = unitsByLocation.get(nId) || [];
                  return (
                    <div className="mt-1.5 space-y-0.5 text-[11px]">
                      {incs.length > 0 && (
                        <div className="text-red-700 font-medium">
                          ⚠ {incs.length} active incident(s): {incs.map((i) => `${i.id} (${i.severityLabel})`).join(', ')}
                        </div>
                      )}
                      {unts.length > 0 && (
                        <div className="text-emerald-800 font-medium">
                          🚑 {unts.length} unit(s) stationed: {unts.map((u) => u.id).join(', ')}
                        </div>
                      )}
                      {incs.length === 0 && unts.length === 0 && (
                        <div className="text-[#718096] text-[10px]">Normal traffic • No active incident at intersection</div>
                      )}
                    </div>
                  );
                })()}
              </div>
            )}
          </div>
        )}
      </div>

      {/* Map Legend */}
      <div className="px-4 py-2 border-t border-[#DCE3EC] bg-white flex flex-wrap items-center justify-between gap-3 text-[11px] text-[#526176]">
        <div className="flex flex-wrap items-center gap-3">
          <div className="flex items-center space-x-1.5">
            <span className="w-3.5 h-0.5 bg-[#CBD5E1] rounded" />
            <span>Normal Road</span>
          </div>
          <div className="flex items-center space-x-1.5">
            <span className="w-3.5 h-0.5 bg-[#D97706] rounded" />
            <span>Congested</span>
          </div>
          <div className="flex items-center space-x-1.5">
            <span className="w-3.5 h-0.5 border-t-2 border-dashed border-[#DC2626]" />
            <span className="text-[#DC2626]">Closed Road</span>
          </div>
          <div className="flex items-center space-x-1.5">
            <span className="w-3.5 h-1 bg-[#0284C7] rounded" />
            <span className="text-[#0284C7] font-semibold">Dijkstra Route</span>
          </div>
        </div>

        <div className="flex items-center space-x-3">
          <div className="flex items-center space-x-1">
            <span className="w-2 h-2 rounded-full bg-[#DC2626]" />
            <span>Incident</span>
          </div>
          <div className="flex items-center space-x-1">
            <span className="w-2 h-2 rounded-full bg-[#15803D]" />
            <span>Available Unit</span>
          </div>
          <div className="flex items-center space-x-1">
            <span className="w-2 h-2 rounded-full bg-[#D97706]" />
            <span>Dispatched Unit</span>
          </div>
        </div>
      </div>
    </div>
  );
}
