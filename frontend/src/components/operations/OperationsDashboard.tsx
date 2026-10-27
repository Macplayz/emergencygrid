'use client';

import { useState, useEffect, useCallback } from 'react';
import { api } from '@/lib/api';
import {
  CityGraph,
  CreateIncidentInput,
  DispatchRecord,
  EmergencyUnit,
  Incident,
  RoadSegment,
  RouteResult,
  SimulationStateData,
} from '@/lib/types';
import SummaryMetricsCards from './SummaryMetricsCards';
import CityMapCanvas from './CityMapCanvas';
import IncidentListPanel from './IncidentListPanel';
import FleetUnitsPanel from './FleetUnitsPanel';
import DispatchDetailsPanel from './DispatchDetailsPanel';
import RoadControlsModal from './RoadControlsModal';
import CreateIncidentModal from './CreateIncidentModal';

interface OperationsDashboardProps {
  initialState?: SimulationStateData | null;
  onRefreshHealth?: () => void;
}

export default function OperationsDashboard({
  initialState,
  onRefreshHealth,
}: OperationsDashboardProps) {
  // Core Data States
  const [simulationState, setSimulationState] = useState<SimulationStateData | null>(initialState || null);
  const [graph, setGraph] = useState<CityGraph | null>(null);
  const [incidents, setIncidents] = useState<Incident[]>([]);
  const [units, setUnits] = useState<EmergencyUnit[]>([]);
  const [dispatches, setDispatches] = useState<DispatchRecord[]>([]);

  // Selection & Route States
  const [selectedIncident, setSelectedIncident] = useState<Incident | null>(null);
  const [selectedUnit, setSelectedUnit] = useState<EmergencyUnit | null>(null);
  const [selectedRoad, setSelectedRoad] = useState<RoadSegment | null>(null);
  const [activeDispatch, setActiveDispatch] = useState<DispatchRecord | null>(null);
  const [activeRoute, setActiveRoute] = useState<RouteResult | null>(null);

  // UI Modals & Active Tab States
  const [activeTab, setActiveTab] = useState<'INCIDENTS' | 'FLEET' | 'DISPATCHES'>('INCIDENTS');
  const [isCreateModalOpen, setIsCreateModalOpen] = useState<boolean>(false);
  const [isRoadModalOpen, setIsRoadModalOpen] = useState<boolean>(false);
  const [isResetConfirmOpen, setIsResetConfirmOpen] = useState<boolean>(false);

  // Loading & Notification Feedback
  const [loading, setLoading] = useState<boolean>(true);
  const [dispatchLoading, setDispatchLoading] = useState<boolean>(false);
  const [bannerAlert, setBannerAlert] = useState<{
    type: 'success' | 'error' | 'info';
    message: string;
  } | null>(null);

  // Helper to show dismissible toast / banner
  const notify = useCallback((type: 'success' | 'error' | 'info', message: string) => {
    setBannerAlert({ type, message });
    setTimeout(() => {
      setBannerAlert((prev) => (prev?.message === message ? null : prev));
    }, 6000);
  }, []);

  // Comprehensive Data Fetcher
  const refreshAllData = useCallback(async () => {
    setLoading(true);
    try {
      const [stateRes, graphRes, incRes, unitRes, dispRes] = await Promise.all([
        api.getSimulationState(),
        api.getCityGraph(),
        api.getIncidents(),
        api.getUnits(),
        api.getDispatches(),
      ]);

      if (stateRes.success && stateRes.data) {
        setSimulationState(stateRes.data);
      }
      if (graphRes.success && graphRes.data) {
        setGraph(graphRes.data);
      }
      if (incRes.success && incRes.data) {
        setIncidents(incRes.data.incidents || []);
      }
      if (unitRes.success && unitRes.data) {
        setUnits(unitRes.data.units || []);
      }
      if (dispRes.success && dispRes.data) {
        const dispList = dispRes.data.dispatches || [];
        setDispatches(dispList);

        // Keep active dispatch up to date if one was selected
        if (activeDispatch) {
          const updated = dispList.find((d) => d.id === activeDispatch.id);
          if (updated) {
            setActiveDispatch(updated);
            if (updated.route) setActiveRoute(updated.route);
          }
        }
      }
    } catch (err: unknown) {
      notify(
        'error',
        err instanceof Error ? err.message : 'Failed to connect to backend simulation engine'
      );
    } finally {
      setLoading(false);
    }
  }, [activeDispatch, notify]);

  // Initial fetch on mount
  useEffect(() => {
    let ignore = false;
    async function loadInitialData() {
      await Promise.resolve();
      if (!ignore) {
        await refreshAllData();
      }
    }
    loadInitialData();
    return () => {
      ignore = true;
    };
  }, [refreshAllData]);

  // --- WORKFLOW 1: Create Incident ---
  const handleCreateIncident = async (data: CreateIncidentInput) => {
    const res = await api.createIncident(data);
    if (!res.success) {
      throw new Error(res.error?.message || 'Failed to create incident');
    }
    notify(
      'success',
      `Incident ${res.data?.id || ''} created and prioritized in triage queue!`
    );
    await refreshAllData();
    if (onRefreshHealth) onRefreshHealth();
  };

  // --- WORKFLOW 2: Dispatch Specific Incident ---
  const handleDispatchSelected = async (incident: Incident) => {
    setDispatchLoading(true);
    try {
      const res = await api.dispatchIncident(incident.id);
      if (!res.success) {
        notify('error', res.error?.message || 'Dispatch failed for selected incident');
        return;
      }
      const record = res.data?.dispatch;
      if (record) {
        setActiveDispatch(record);
        if (record.route) setActiveRoute(record.route);
        setActiveTab('DISPATCHES');
        notify(
          'success',
          `Unit ${record.unitId} dispatched to incident ${incident.id} (ETA: ${record.travelTimeMinutes.toFixed(2)} min)!`
        );
      }
      await refreshAllData();
      if (onRefreshHealth) onRefreshHealth();
    } catch (err: unknown) {
      notify('error', err instanceof Error ? err.message : 'Dispatch request failed');
    } finally {
      setDispatchLoading(false);
    }
  };

  // --- WORKFLOW 3: Dispatch Next (Highest Priority from Heap) ---
  const handleDispatchNext = async () => {
    setDispatchLoading(true);
    try {
      const res = await api.dispatchNext();
      if (!res.success) {
        notify('error', res.error?.message || 'Dispatch next failed');
        return;
      }
      const record = res.data?.dispatch;
      if (record) {
        setActiveDispatch(record);
        if (record.route) setActiveRoute(record.route);
        setActiveTab('DISPATCHES');
        notify(
          'success',
          `Optimal Dispatch: Unit ${record.unitId} assigned to incident ${record.incidentId} with optimal route!`
        );
      }
      await refreshAllData();
      if (onRefreshHealth) onRefreshHealth();
    } catch (err: unknown) {
      notify('error', err instanceof Error ? err.message : 'Dispatch request failed');
    } finally {
      setDispatchLoading(false);
    }
  };

  // --- WORKFLOW 4: Cancel Incident ---
  const handleCancelIncident = async (incident: Incident) => {
    try {
      const res = await api.cancelIncident(incident.id);
      if (!res.success) {
        notify('error', res.error?.message || 'Failed to cancel incident');
        return;
      }
      if (selectedIncident?.id === incident.id) {
        setSelectedIncident(null);
      }
      notify('info', `Incident ${incident.id} was marked CANCELLED.`);
      await refreshAllData();
      if (onRefreshHealth) onRefreshHealth();
    } catch (err: unknown) {
      notify('error', err instanceof Error ? err.message : 'Cancel request failed');
    }
  };

  // --- WORKFLOW 5: Complete Dispatch Response ---
  const handleCompleteDispatch = async (dispatchId: string) => {
    const res = await api.completeDispatch(dispatchId);
    if (!res.success) {
      throw new Error(res.error?.message || 'Failed to complete dispatch');
    }
    notify(
      'success',
      `Dispatch ${dispatchId} completed! Incident archived and emergency unit released back to pool.`
    );
    setActiveRoute(null);
    await refreshAllData();
    if (onRefreshHealth) onRefreshHealth();
  };

  // --- WORKFLOW 6: Recalculate Dispatch Route ---
  const handleRecalculateDispatch = async (dispatchId: string) => {
    const res = await api.recalculateDispatch(dispatchId);
    if (!res.success || !res.data) {
      throw new Error(res.error?.message || 'Failed to recalculate shortest path');
    }
    const updated = res.data.dispatch;
    setActiveDispatch(updated);
    if (updated.route) setActiveRoute(updated.route);
    notify(
      'success',
      `Route recalculated! New travel time: ${updated.travelTimeMinutes.toFixed(2)} min.`
    );
    await refreshAllData();
  };

  // --- WORKFLOW 7: Road Status & Congestion Update ---
  const handleUpdateRoad = async (
    roadId: string,
    data: { isClosed?: boolean; congestionFactor?: number }
  ) => {
    const res = await api.updateRoad(roadId, data);
    if (!res.success) {
      throw new Error(res.error?.message || 'Road update rejected by backend');
    }
    await refreshAllData();
    if (onRefreshHealth) onRefreshHealth();
  };

  // --- WORKFLOW 8: Reset Simulation Network ---
  const handleResetSimulation = async () => {
    setIsResetConfirmOpen(false);
    try {
      const res = await api.resetSimulation();
      if (!res.success) {
        notify('error', res.error?.message || 'Reset failed');
        return;
      }
      setSelectedIncident(null);
      setSelectedUnit(null);
      setSelectedRoad(null);
      setActiveDispatch(null);
      setActiveRoute(null);
      notify('success', 'Simulation network reset to canonical MetroGrid-16 topology.');
      await refreshAllData();
      if (onRefreshHealth) onRefreshHealth();
    } catch (err: unknown) {
      notify('error', err instanceof Error ? err.message : 'Reset failed');
    }
  };

  return (
    <div className="space-y-6">
      {/* Toast / Banner Notification */}
      {bannerAlert && (
        <div
          className={`p-3.5 rounded-xl border flex items-center justify-between text-xs animate-fade-in shadow-xs ${
            bannerAlert.type === 'success'
              ? 'bg-emerald-50 border-emerald-200 text-emerald-900'
              : bannerAlert.type === 'error'
              ? 'bg-red-50 border-red-200 text-red-900'
              : 'bg-blue-50 border-blue-200 text-blue-900'
          }`}
        >
          <div className="flex items-center space-x-2 font-medium">
            <span>
              {bannerAlert.type === 'success' ? '✓' : bannerAlert.type === 'error' ? '⚠' : 'ℹ'}
            </span>
            <span>{bannerAlert.message}</span>
          </div>
          <button
            type="button"
            onClick={() => setBannerAlert(null)}
            className="text-[#718096] hover:text-[#172033] px-2 font-bold cursor-pointer"
          >
            ✕
          </button>
        </div>
      )}

      {/* Primary Command Toolbar */}
      <div className="bg-white border border-[#DCE3EC] rounded-xl p-3.5 flex flex-wrap items-center justify-between gap-3 text-xs shadow-xs">
        <div className="flex items-center space-x-2">
          <span className="text-[#526176] font-medium">Simulation Tick:</span>
          <span className="font-mono font-bold text-[#172033] bg-[#F5F7FA] px-2 py-0.5 rounded border border-[#DCE3EC]">
            #{simulationState?.tick ?? 0}
          </span>
          <span className="text-[#DCE3EC]">•</span>
          <span className="text-[#15803D] font-medium">Deterministic Engine Active</span>
        </div>

        {/* Global Action Buttons */}
        <div className="flex items-center space-x-2 flex-wrap">
          <button
            type="button"
            onClick={() => setIsCreateModalOpen(true)}
            className="px-3 py-1.5 rounded-lg bg-[#3563D9] hover:bg-[#284eb3] text-white font-semibold text-xs shadow-xs transition-colors flex items-center space-x-1 cursor-pointer"
          >
            <span>+</span>
            <span>New Incident</span>
          </button>

          <button
            type="button"
            onClick={() => setIsRoadModalOpen(true)}
            className="px-3 py-1.5 rounded-lg bg-white hover:bg-slate-50 text-[#D97706] border border-[#DCE3EC] text-xs font-semibold shadow-xs transition-colors flex items-center space-x-1 cursor-pointer"
          >
            <span>🚧</span>
            <span>Road &amp; Traffic Controls</span>
          </button>

          <button
            type="button"
            onClick={() => setIsResetConfirmOpen(true)}
            className="px-3 py-1.5 rounded-lg bg-white hover:bg-slate-50 text-[#526176] border border-[#DCE3EC] text-xs font-medium shadow-xs transition-colors cursor-pointer"
            title="Reset city network back to initial MetroGrid-16 seed topology"
          >
            Reset Network
          </button>

          <button
            type="button"
            disabled={loading}
            onClick={refreshAllData}
            className="px-2.5 py-1.5 rounded-lg bg-white hover:bg-slate-50 text-[#526176] border border-[#DCE3EC] text-xs font-medium shadow-xs transition-colors flex items-center space-x-1 cursor-pointer"
            title="Refresh backend simulation telemetry"
          >
            <span className={loading ? 'animate-spin' : ''}>🔄</span>
            <span className="hidden sm:inline">Refresh</span>
          </button>
        </div>
      </div>

      {/* KPI Overview Summary Cards */}
      <SummaryMetricsCards state={simulationState} loading={loading} />

      {/* Central Prominent City Map Visualization */}
      <CityMapCanvas
        graph={graph}
        incidents={incidents}
        units={units}
        activeRoute={activeRoute}
        selectedIncident={selectedIncident}
        selectedUnit={selectedUnit}
        selectedRoad={selectedRoad}
        onSelectIncident={(inc) => {
          setSelectedIncident(inc);
          if (inc) setActiveTab('INCIDENTS');
        }}
        onSelectUnit={(unit) => {
          setSelectedUnit(unit);
          if (unit) setActiveTab('FLEET');
        }}
        onSelectRoad={(road) => {
          setSelectedRoad(road);
          if (road) setIsRoadModalOpen(true);
        }}
        onSelectIntersection={(node) => {
          if (node) {
            const inc = incidents.find((i) => i.locationId === node.id && i.status === 'PENDING');
            if (inc) setSelectedIncident(inc);
          }
        }}
      />

      {/* Operational Panels Navigation Tabs */}
      <div className="border-b border-[#DCE3EC] flex items-center space-x-2 text-xs font-semibold">
        <button
          type="button"
          onClick={() => setActiveTab('INCIDENTS')}
          className={`px-4 py-2 border-b-2 transition-all flex items-center space-x-2 cursor-pointer ${
            activeTab === 'INCIDENTS'
              ? 'border-[#3563D9] text-[#3563D9] bg-blue-50/50'
              : 'border-transparent text-[#526176] hover:text-[#172033]'
          }`}
        >
          <span>🚨 Incidents Triage</span>
          <span className="px-1.5 py-0.2 rounded-full text-[10px] bg-slate-100 text-[#526176]">
            {incidents.filter((i) => i.status === 'PENDING').length}
          </span>
        </button>

        <button
          type="button"
          onClick={() => setActiveTab('FLEET')}
          className={`px-4 py-2 border-b-2 transition-all flex items-center space-x-2 cursor-pointer ${
            activeTab === 'FLEET'
              ? 'border-[#15803D] text-[#15803D] bg-emerald-50/50'
              : 'border-transparent text-[#526176] hover:text-[#172033]'
          }`}
        >
          <span>🚑 Fleet Readiness</span>
          <span className="px-1.5 py-0.2 rounded-full text-[10px] bg-slate-100 text-[#526176]">
            {units.filter((u) => u.status === 'AVAILABLE').length}
          </span>
        </button>

        <button
          type="button"
          onClick={() => setActiveTab('DISPATCHES')}
          className={`px-4 py-2 border-b-2 transition-all flex items-center space-x-2 cursor-pointer ${
            activeTab === 'DISPATCHES'
              ? 'border-[#087E8B] text-[#087E8B] bg-sky-50/50'
              : 'border-transparent text-[#526176] hover:text-[#172033]'
          }`}
        >
          <span>⚡ Dispatch Telemetry</span>
          {dispatches.filter((d) => d.status === 'ACTIVE').length > 0 && (
            <span className="px-1.5 py-0.2 rounded-full text-[10px] bg-amber-100 text-amber-800 font-mono">
              {dispatches.filter((d) => d.status === 'ACTIVE').length} active
            </span>
          )}
        </button>
      </div>

      {/* Tab Panels */}
      <div>
        {activeTab === 'INCIDENTS' && (
          <IncidentListPanel
            incidents={incidents}
            selectedIncident={selectedIncident}
            onSelectIncident={setSelectedIncident}
            onOpenCreateModal={() => setIsCreateModalOpen(true)}
            onDispatchSelected={handleDispatchSelected}
            onDispatchNext={handleDispatchNext}
            onCancelIncident={handleCancelIncident}
            dispatchLoading={dispatchLoading}
          />
        )}

        {activeTab === 'FLEET' && (
          <FleetUnitsPanel
            units={units}
            selectedUnit={selectedUnit}
            onSelectUnit={setSelectedUnit}
          />
        )}

        {activeTab === 'DISPATCHES' && (
          <DispatchDetailsPanel
            dispatches={dispatches}
            activeDispatch={activeDispatch}
            onSelectDispatch={(disp) => {
              setActiveDispatch(disp);
              if (disp.route) setActiveRoute(disp.route);
            }}
            onCompleteDispatch={handleCompleteDispatch}
            onRecalculateDispatch={handleRecalculateDispatch}
          />
        )}
      </div>

      {/* Modal: Create Incident Dialog */}
      <CreateIncidentModal
        isOpen={isCreateModalOpen}
        intersections={graph?.intersections || []}
        onClose={() => setIsCreateModalOpen(false)}
        onSubmit={handleCreateIncident}
      />

      {/* Modal: Road Controls Dialog */}
      <RoadControlsModal
        isOpen={isRoadModalOpen}
        roads={graph?.roads || []}
        selectedRoad={selectedRoad}
        onClose={() => setIsRoadModalOpen(false)}
        onUpdateRoad={handleUpdateRoad}
      />

      {/* Confirmation Modal: Reset Simulation */}
      {isResetConfirmOpen && (
        <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-slate-900/40 backdrop-blur-xs">
          <div className="bg-white border border-[#DCE3EC] rounded-2xl p-5 max-w-sm w-full space-y-4 shadow-xl">
            <h3 className="text-sm font-bold text-[#172033] flex items-center space-x-2">
              <span className="text-amber-500">⚠</span>
              <span>Reset Simulation Network?</span>
            </h3>
            <p className="text-xs text-[#526176] leading-relaxed">
              This will restore MetroGrid-16 to default seed data: all road closures and congestion modifiers will reset, and all active/pending incidents will be cleared.
            </p>
            <div className="flex items-center justify-end space-x-2 pt-2 border-t border-[#DCE3EC]">
              <button
                type="button"
                onClick={() => setIsResetConfirmOpen(false)}
                className="px-3 py-1.5 rounded-lg bg-white hover:bg-slate-50 text-[#526176] border border-[#DCE3EC] text-xs font-medium cursor-pointer shadow-xs"
              >
                Cancel
              </button>
              <button
                type="button"
                onClick={handleResetSimulation}
                className="px-3.5 py-1.5 rounded-lg bg-[#DC2626] hover:bg-[#b91c1c] text-white text-xs font-semibold shadow-xs cursor-pointer"
              >
                Confirm Reset
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
