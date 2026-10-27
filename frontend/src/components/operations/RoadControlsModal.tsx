'use client';

import { useState } from 'react';
import { RoadSegment } from '@/lib/types';

interface RoadControlsModalProps {
  isOpen: boolean;
  roads: RoadSegment[];
  selectedRoad: RoadSegment | null;
  onClose: () => void;
  onUpdateRoad: (
    roadId: string,
    data: { isClosed?: boolean; congestionFactor?: number }
  ) => Promise<void>;
}

export default function RoadControlsModal({
  isOpen,
  roads,
  selectedRoad,
  onClose,
  onUpdateRoad,
}: RoadControlsModalProps) {
  const [prevSelectedRoad, setPrevSelectedRoad] = useState<RoadSegment | null>(selectedRoad);
  const [currentRoadId, setCurrentRoadId] = useState<string>(
    selectedRoad?.id || (roads.length > 0 ? roads[0].id : '')
  );
  const [isClosed, setIsClosed] = useState<boolean>(selectedRoad?.isClosed ?? false);
  const [congestionFactor, setCongestionFactor] = useState<number>(
    selectedRoad?.congestionFactor || 1.0
  );
  const [loading, setLoading] = useState<boolean>(false);
  const [error, setError] = useState<string | null>(null);
  const [successMsg, setSuccessMsg] = useState<string | null>(null);

  // Sync state when selectedRoad changes (React render-time state adjustment)
  if (selectedRoad !== prevSelectedRoad) {
    setPrevSelectedRoad(selectedRoad);
    if (selectedRoad) {
      setCurrentRoadId(selectedRoad.id);
      setIsClosed(selectedRoad.isClosed);
      setCongestionFactor(selectedRoad.congestionFactor || 1.0);
    }
  }

  if (!isOpen) return null;

  const activeRoad = roads.find((r) => r.id === currentRoadId) || selectedRoad;

  const handleRoadChange = (roadId: string) => {
    setCurrentRoadId(roadId);
    setError(null);
    setSuccessMsg(null);
    const r = roads.find((x) => x.id === roadId);
    if (r) {
      setIsClosed(r.isClosed);
      setCongestionFactor(r.congestionFactor || 1.0);
    }
  };

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!currentRoadId) return;

    setLoading(true);
    setError(null);
    setSuccessMsg(null);

    try {
      await onUpdateRoad(currentRoadId, {
        isClosed,
        congestionFactor: Number(congestionFactor),
      });
      setSuccessMsg(`Road ${currentRoadId} successfully updated in road network!`);
      setTimeout(() => {
        setSuccessMsg(null);
      }, 3000);
    } catch (err: unknown) {
      setError(err instanceof Error ? err.message : 'Failed to update road in backend');
    } finally {
      setLoading(false);
    }
  };

  // Calculate projected travel time based on inputs
  const projectedTravelTime = activeRoad
    ? (activeRoad.baseLengthKm / activeRoad.speedLimitKmh) * congestionFactor * 60
    : 0;

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-slate-900/40 backdrop-blur-xs animate-fade-in">
      <div className="bg-white border border-[#DCE3EC] rounded-2xl w-full max-w-lg shadow-xl overflow-hidden flex flex-col text-[#172033]">
        {/* Modal Header */}
        <div className="px-5 py-4 border-b border-[#DCE3EC] bg-white flex items-center justify-between">
          <div className="flex items-center space-x-2">
            <span className="w-2.5 h-2.5 rounded-full bg-[#D97706]" />
            <h3 className="text-base font-bold text-[#172033] tracking-tight">
              Road Network &amp; Traffic Controls
            </h3>
          </div>
          <button
            type="button"
            onClick={onClose}
            className="text-[#718096] hover:text-[#172033] text-lg px-2 py-0.5 rounded-md hover:bg-slate-100 transition-colors cursor-pointer"
          >
            ✕
          </button>
        </div>

        {/* Modal Body */}
        <form onSubmit={handleSubmit} className="p-5 space-y-4 text-xs">
          {error && (
            <div className="p-3 rounded-lg bg-red-50 border border-red-200 text-red-800">
              {error}
            </div>
          )}

          {successMsg && (
            <div className="p-3 rounded-lg bg-emerald-50 border border-emerald-200 text-emerald-800 font-medium">
              ✓ {successMsg}
            </div>
          )}

          {/* Road Selector */}
          <div>
            <label className="block text-[#172033] font-semibold mb-1">
              Select Road Segment: <span className="text-red-600">*</span>
            </label>
            <select
              value={currentRoadId}
              onChange={(e) => handleRoadChange(e.target.value)}
              className="w-full bg-white border border-[#DCE3EC] rounded-lg px-3 py-2 text-[#172033] font-mono text-xs focus:outline-none focus:ring-1 focus:ring-[#3563D9]"
            >
              {roads.map((road) => (
                <option key={road.id} value={road.id}>
                  {road.id} ({road.sourceId} ⟷ {road.targetId}) — {road.baseLengthKm}km @{' '}
                  {road.speedLimitKmh}km/h {road.isClosed ? '[CLOSED]' : ''}
                </option>
              ))}
            </select>
          </div>

          {/* Current Road Attributes Card */}
          {activeRoad && (
            <div className="bg-[#F8FAFC] border border-[#DCE3EC] rounded-xl p-3.5 space-y-2 text-xs">
              <div className="flex items-center justify-between font-mono">
                <span className="font-bold text-[#172033]">
                  {activeRoad.id} ({activeRoad.sourceId} ⟷ {activeRoad.targetId})
                </span>
                <span
                  className={`px-2 py-0.5 rounded text-[10px] font-bold border ${
                    activeRoad.isClosed
                      ? 'bg-red-50 text-red-800 border-red-200'
                      : 'bg-emerald-50 text-emerald-800 border-emerald-200'
                  }`}
                >
                  {activeRoad.isClosed ? 'STATUS: CLOSED' : 'STATUS: OPEN'}
                </span>
              </div>

              <div className="grid grid-cols-2 sm:grid-cols-4 gap-2 text-[11px] text-[#526176] font-mono pt-1">
                <div>
                  <span className="text-[#718096]">Base Length:</span>
                  <div className="font-bold text-[#172033]">{activeRoad.baseLengthKm} km</div>
                </div>
                <div>
                  <span className="text-[#718096]">Speed Limit:</span>
                  <div className="font-bold text-[#172033]">{activeRoad.speedLimitKmh} km/h</div>
                </div>
                <div>
                  <span className="text-[#718096]">Current Congestion:</span>
                  <div className="font-bold text-[#D97706]">{activeRoad.congestionFactor}x</div>
                </div>
                <div>
                  <span className="text-[#718096]">Travel Time:</span>
                  <div className="font-bold text-[#15803D]">
                    {activeRoad.travelTimeMinutes.toFixed(2)} min
                  </div>
                </div>
              </div>
            </div>
          )}

          {/* Closure Toggle */}
          <div className="pt-1">
            <label className="text-[#172033] font-semibold block mb-1.5">
              Road Closure State (Hazard / Obstruction):
            </label>
            <div className="grid grid-cols-2 gap-2">
              <button
                type="button"
                onClick={() => setIsClosed(false)}
                className={`py-2 px-3 rounded-lg border font-semibold text-xs flex items-center justify-center space-x-2 transition-all cursor-pointer ${
                  !isClosed
                    ? 'bg-emerald-50 text-emerald-800 border-emerald-300 ring-1 ring-emerald-400'
                    : 'bg-white text-[#526176] border-[#DCE3EC] hover:bg-slate-50'
                }`}
              >
                <span>✓</span>
                <span>Open for Transit</span>
              </button>

              <button
                type="button"
                onClick={() => setIsClosed(true)}
                className={`py-2 px-3 rounded-lg border font-semibold text-xs flex items-center justify-center space-x-2 transition-all cursor-pointer ${
                  isClosed
                    ? 'bg-red-50 text-red-800 border-red-300 ring-1 ring-red-400'
                    : 'bg-white text-[#526176] border-[#DCE3EC] hover:bg-slate-50'
                }`}
              >
                <span>✕</span>
                <span>Closed (No Entry)</span>
              </button>
            </div>
            <p className="text-[10px] text-[#718096] mt-1">
              Closed edges receive infinite weight (w = ∞) during Dijkstra graph relaxations.
            </p>
          </div>

          {/* Congestion Multiplier Slider */}
          <div className="space-y-1.5 pt-1">
            <div className="flex items-center justify-between">
              <label className="text-[#172033] font-semibold">
                Traffic Congestion Factor:
              </label>
              <span className="font-mono font-bold text-xs text-[#D97706]">
                {Number(congestionFactor).toFixed(1)}x Multiplier
              </span>
            </div>

            <input
              type="range"
              min="1.0"
              max="5.0"
              step="0.1"
              value={congestionFactor}
              disabled={isClosed}
              onChange={(e) => setCongestionFactor(parseFloat(e.target.value))}
              className="w-full h-2 bg-slate-200 rounded-lg appearance-none cursor-pointer accent-[#D97706] disabled:opacity-40"
            />

            <div className="flex justify-between text-[10px] text-[#718096] font-mono">
              <span>1.0x (Free Flow)</span>
              <span>2.5x (Moderate Delay)</span>
              <span>5.0x (Severe Gridlock)</span>
            </div>
          </div>

          {/* Dynamic Projected Travel Time Banner */}
          {!isClosed && activeRoad && (
            <div className="p-3 rounded-xl bg-blue-50/70 border border-blue-200 text-xs flex items-center justify-between">
              <div>
                <span className="text-blue-900 font-semibold block">Projected Travel Time:</span>
                <span className="text-[10px] text-[#526176]">
                  ({activeRoad.baseLengthKm}km / {activeRoad.speedLimitKmh}km/h) × {Number(congestionFactor).toFixed(1)}x × 60
                </span>
              </div>
              <span className="font-mono font-bold text-base text-[#3563D9]">
                {projectedTravelTime.toFixed(2)} min
              </span>
            </div>
          )}

          {/* Action Buttons */}
          <div className="pt-2 flex items-center justify-end space-x-2 border-t border-[#DCE3EC]">
            <button
              type="button"
              onClick={onClose}
              className="px-4 py-2 rounded-lg text-[#526176] hover:text-[#172033] bg-white hover:bg-slate-50 border border-[#DCE3EC] font-medium transition-colors cursor-pointer shadow-xs"
            >
              Cancel
            </button>
            <button
              type="submit"
              disabled={loading}
              className="px-4 py-2 rounded-lg bg-[#3563D9] hover:bg-[#284eb3] text-white font-semibold transition-all shadow-xs cursor-pointer disabled:opacity-50"
            >
              {loading ? 'Applying...' : 'Apply Road Updates'}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
}
