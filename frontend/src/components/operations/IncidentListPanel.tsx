'use client';

import { useState, useMemo } from 'react';
import { Incident, IncidentStatus } from '@/lib/types';

interface IncidentListPanelProps {
  incidents: Incident[];
  selectedIncident: Incident | null;
  onSelectIncident: (incident: Incident | null) => void;
  onOpenCreateModal: () => void;
  onDispatchSelected: (incident: Incident) => void;
  onDispatchNext: () => void;
  onCancelIncident: (incident: Incident) => void;
  dispatchLoading?: boolean;
}

const SEVERITY_COLORS: Record<string, { bg: string; text: string; border: string }> = {
  CRITICAL: { bg: 'bg-red-50', text: 'text-red-700', border: 'border-red-200' },
  HIGH: { bg: 'bg-amber-50', text: 'text-amber-800', border: 'border-amber-200' },
  MEDIUM: { bg: 'bg-yellow-50', text: 'text-yellow-800', border: 'border-yellow-200' },
  LOW: { bg: 'bg-slate-100', text: 'text-slate-700', border: 'border-slate-200' },
};

const STATUS_COLORS: Record<IncidentStatus, { bg: string; text: string; border: string }> = {
  PENDING: { bg: 'bg-red-50', text: 'text-red-700', border: 'border-red-200' },
  ASSIGNED: { bg: 'bg-amber-50', text: 'text-amber-800', border: 'border-amber-200' },
  DISPATCHED: { bg: 'bg-sky-50', text: 'text-sky-800', border: 'border-sky-200' },
  ON_SCENE: { bg: 'bg-indigo-50', text: 'text-indigo-800', border: 'border-indigo-200' },
  RESOLVED: { bg: 'bg-emerald-50', text: 'text-emerald-800', border: 'border-emerald-200' },
  CANCELLED: { bg: 'bg-slate-100', text: 'text-slate-500', border: 'border-slate-200' },
};

export default function IncidentListPanel({
  incidents,
  selectedIncident,
  onSelectIncident,
  onOpenCreateModal,
  onDispatchSelected,
  onDispatchNext,
  onCancelIncident,
  dispatchLoading,
}: IncidentListPanelProps) {
  const [statusFilter, setStatusFilter] = useState<string>('ALL');
  const [agencyFilter, setAgencyFilter] = useState<string>('ALL');
  const [severityFilter, setSeverityFilter] = useState<string>('ALL');

  // Filtered incidents
  const filteredIncidents = useMemo(() => {
    return incidents.filter((inc) => {
      if (statusFilter !== 'ALL' && inc.status !== statusFilter) return false;
      if (agencyFilter !== 'ALL') {
        const incType = inc.serviceType || inc.type;
        if (incType !== agencyFilter) return false;
      }
      if (severityFilter !== 'ALL') {
        if (String(inc.severity) !== severityFilter && inc.severityLabel !== severityFilter) {
          return false;
        }
      }
      return true;
    });
  }, [incidents, statusFilter, agencyFilter, severityFilter]);

  const pendingCount = useMemo(() => {
    return incidents.filter((i) => i.status === 'PENDING').length;
  }, [incidents]);

  return (
    <div className="bg-white border border-[#DCE3EC] rounded-xl flex flex-col shadow-xs overflow-hidden">
      {/* Header with Title and Primary Actions */}
      <div className="p-4 border-b border-[#DCE3EC] bg-white flex flex-wrap items-center justify-between gap-3">
        <div>
          <div className="flex items-center space-x-2">
            <span className="w-2.5 h-2.5 rounded-full bg-[#DC2626]" />
            <h2 className="text-sm font-bold text-[#172033] uppercase tracking-wide">
              Active Incidents
            </h2>
            <span className="px-2 py-0.5 rounded-full text-[11px] font-mono bg-slate-100 text-[#526176] border border-slate-200">
              {filteredIncidents.length} / {incidents.length}
            </span>
          </div>
          <p className="text-[11px] text-[#526176] mt-0.5">
            Triage queue ordered by real-time severity and arrival priority
          </p>
        </div>

        {/* Action Buttons */}
        <div className="flex items-center space-x-2">
          {/* Dispatch Next Highest Priority */}
          <button
            type="button"
            disabled={pendingCount === 0 || dispatchLoading}
            onClick={onDispatchNext}
            className={`px-3 py-1.5 rounded-lg text-xs font-semibold transition-all shadow-xs flex items-center space-x-1.5 ${
              pendingCount > 0 && !dispatchLoading
                ? 'bg-[#DC2626] hover:bg-[#b91c1c] text-white cursor-pointer'
                : 'bg-slate-100 text-[#718096] border border-slate-200 cursor-not-allowed opacity-60'
            }`}
            title="Dispatch the highest-priority pending incident from root of heap"
          >
            {dispatchLoading ? (
              <>
                <span className="animate-spin text-xs">⚙</span>
                <span>Dispatching...</span>
              </>
            ) : (
              <>
                <span>⚡</span>
                <span>Dispatch Next ({pendingCount})</span>
              </>
            )}
          </button>

          {/* Create Incident Button */}
          <button
            type="button"
            onClick={onOpenCreateModal}
            className="px-3 py-1.5 rounded-lg text-xs font-medium bg-white hover:bg-slate-50 text-[#172033] border border-[#DCE3EC] shadow-xs transition-colors flex items-center space-x-1 cursor-pointer"
          >
            <span>+</span>
            <span>New Incident</span>
          </button>
        </div>
      </div>

      {/* Filter Bar */}
      <div className="px-4 py-2.5 border-b border-[#DCE3EC] bg-[#F8FAFC] flex flex-wrap items-center gap-2 text-xs">
        {/* Status Filter */}
        <div className="flex items-center space-x-1">
          <span className="text-[11px] text-[#526176] font-medium">Status:</span>
          <select
            value={statusFilter}
            onChange={(e) => setStatusFilter(e.target.value)}
            className="bg-white border border-[#DCE3EC] text-[#172033] rounded-md px-2 py-1 text-xs focus:outline-none focus:ring-1 focus:ring-[#3563D9]"
          >
            <option value="ALL">All Statuses</option>
            <option value="PENDING">Pending Triage</option>
            <option value="ASSIGNED">Assigned</option>
            <option value="DISPATCHED">Dispatched</option>
            <option value="RESOLVED">Resolved</option>
            <option value="CANCELLED">Cancelled</option>
          </select>
        </div>

        {/* Agency Filter */}
        <div className="flex items-center space-x-1">
          <span className="text-[11px] text-[#526176] font-medium">Agency:</span>
          <select
            value={agencyFilter}
            onChange={(e) => setAgencyFilter(e.target.value)}
            className="bg-white border border-[#DCE3EC] text-[#172033] rounded-md px-2 py-1 text-xs focus:outline-none focus:ring-1 focus:ring-[#3563D9]"
          >
            <option value="ALL">All Agencies</option>
            <option value="MEDICAL">Medical (EMS)</option>
            <option value="FIRE">Fire & Rescue</option>
            <option value="POLICE">Police</option>
          </select>
        </div>

        {/* Severity Filter */}
        <div className="flex items-center space-x-1">
          <span className="text-[11px] text-[#526176] font-medium">Severity:</span>
          <select
            value={severityFilter}
            onChange={(e) => setSeverityFilter(e.target.value)}
            className="bg-white border border-[#DCE3EC] text-[#172033] rounded-md px-2 py-1 text-xs focus:outline-none focus:ring-1 focus:ring-[#3563D9]"
          >
            <option value="ALL">All Severities</option>
            <option value="4">4 - Critical</option>
            <option value="3">3 - High</option>
            <option value="2">2 - Medium</option>
            <option value="1">1 - Low</option>
          </select>
        </div>

        {(statusFilter !== 'ALL' || agencyFilter !== 'ALL' || severityFilter !== 'ALL') && (
          <button
            type="button"
            onClick={() => {
              setStatusFilter('ALL');
              setAgencyFilter('ALL');
              setSeverityFilter('ALL');
            }}
            className="text-[11px] text-[#3563D9] hover:underline ml-auto cursor-pointer"
          >
            Reset Filters
          </button>
        )}
      </div>

      {/* Incidents Table / List */}
      <div className="overflow-x-auto flex-1 max-h-[380px] overflow-y-auto divide-y divide-[#E2E8F0]">
        {filteredIncidents.length === 0 ? (
          <div className="py-12 text-center text-[#718096] text-xs">
            <p className="font-medium text-[#526176]">No emergency incidents found matching the filter.</p>
            <p className="text-[11px] text-[#718096] mt-1">
              Click &quot;New Incident&quot; above to simulate an emergency call into the triage queue.
            </p>
          </div>
        ) : (
          <table className="w-full text-left border-collapse text-xs">
            <thead className="bg-[#F8FAFC] sticky top-0 z-10 text-[11px] font-semibold text-[#526176] uppercase tracking-wider border-b border-[#DCE3EC]">
              <tr>
                <th className="py-2.5 px-3">Incident ID</th>
                <th className="py-2.5 px-3">Agency</th>
                <th className="py-2.5 px-3">Severity</th>
                <th className="py-2.5 px-3">Location</th>
                <th className="py-2.5 px-3">Status</th>
                <th className="py-2.5 px-3">Arrival</th>
                <th className="py-2.5 px-3 text-right">Actions</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-[#E2E8F0] font-sans">
              {filteredIncidents.map((inc) => {
                const isSelected = selectedIncident?.id === inc.id;
                const sevStyle = SEVERITY_COLORS[inc.severityLabel] || SEVERITY_COLORS.MEDIUM;
                const statStyle = STATUS_COLORS[inc.status] || STATUS_COLORS.PENDING;
                const canDispatch = inc.status === 'PENDING';
                const canCancel = inc.status === 'PENDING';

                return (
                  <tr
                    key={inc.id}
                    onClick={() => onSelectIncident(isSelected ? null : inc)}
                    className={`cursor-pointer transition-colors ${
                      isSelected
                        ? 'bg-blue-50/70 border-l-2 border-l-[#3563D9]'
                        : 'hover:bg-slate-50'
                    }`}
                  >
                    {/* ID */}
                    <td className="py-2.5 px-3 font-mono font-bold text-[#172033]">
                      {inc.id}
                    </td>

                    {/* Agency */}
                    <td className="py-2.5 px-3">
                      <span className="font-medium text-[#526176]">
                        {inc.serviceType || inc.type}
                      </span>
                    </td>

                    {/* Severity */}
                    <td className="py-2.5 px-3">
                      <span
                        className={`inline-flex items-center px-2 py-0.5 rounded text-[10px] font-bold border ${sevStyle.bg} ${sevStyle.text} ${sevStyle.border}`}
                      >
                        {inc.severity} • {inc.severityLabel}
                      </span>
                    </td>

                    {/* Location */}
                    <td className="py-2.5 px-3 font-mono font-medium text-[#172033]">
                      {inc.locationId}
                    </td>

                    {/* Status */}
                    <td className="py-2.5 px-3">
                      <span
                        className={`inline-flex items-center px-2 py-0.5 rounded text-[10px] font-semibold border ${statStyle.bg} ${statStyle.text} ${statStyle.border}`}
                      >
                        {inc.status}
                      </span>
                    </td>

                    {/* Arrival order */}
                    <td className="py-2.5 px-3 font-mono text-[#718096] text-[11px]">
                      #{inc.arrivalSequence || 1}
                    </td>

                    {/* Actions */}
                    <td className="py-2.5 px-3 text-right">
                      <div className="flex items-center justify-end space-x-1.5" onClick={(e) => e.stopPropagation()}>
                        {canDispatch && (
                          <button
                            type="button"
                            disabled={dispatchLoading}
                            onClick={() => onDispatchSelected(inc)}
                            className="px-2.5 py-1 rounded bg-[#3563D9] hover:bg-[#284eb3] text-white font-medium text-[11px] shadow-xs transition-colors cursor-pointer"
                            title="Dispatch unit for this incident"
                          >
                            Dispatch
                          </button>
                        )}
                        {canCancel && (
                          <button
                            type="button"
                            onClick={() => onCancelIncident(inc)}
                            className="px-2 py-1 rounded bg-white hover:bg-slate-100 text-[#526176] hover:text-[#DC2626] border border-[#DCE3EC] text-[11px] transition-colors cursor-pointer"
                            title="Cancel pending incident"
                          >
                            Cancel
                          </button>
                        )}
                      </div>
                    </td>
                  </tr>
                );
              })}
            </tbody>
          </table>
        )}
      </div>

      {/* Selected Incident Detail Drawer / Inspector */}
      {selectedIncident && (
        <div className="p-3.5 bg-[#F8FAFC] border-t border-[#DCE3EC] text-xs flex flex-wrap items-center justify-between gap-3 text-[#172033]">
          <div className="flex items-center space-x-3">
            <span className="font-mono font-bold text-[#3563D9] text-sm">
              {selectedIncident.id}
            </span>
            <span className="text-[#526176]">
              Location: <strong className="text-[#172033] font-mono">{selectedIncident.locationId}</strong>
            </span>
            <span className="text-[#DCE3EC] hidden sm:inline">•</span>
            <span className="text-[#526176] hidden sm:inline">
              Priority Score:{' '}
              <span className="font-mono text-[#D97706] font-bold">
                {selectedIncident.computedPriority?.toFixed(0) ?? '--'}
              </span>
            </span>
            {selectedIncident.description && (
              <span className="text-[#718096] text-[11px] italic max-w-xs truncate hidden md:inline">
                &ldquo;{selectedIncident.description}&rdquo;
              </span>
            )}
          </div>

          <div className="flex items-center space-x-2">
            {selectedIncident.status === 'PENDING' && (
              <button
                type="button"
                disabled={dispatchLoading}
                onClick={() => onDispatchSelected(selectedIncident)}
                className="px-3 py-1 rounded-md bg-[#3563D9] hover:bg-[#284eb3] text-white font-semibold text-xs shadow-xs transition-colors cursor-pointer"
              >
                ⚡ Dispatch Nearest Eligible Unit
              </button>
            )}
            <button
              type="button"
              onClick={() => onSelectIncident(null)}
              className="text-[#718096] hover:text-[#172033] text-xs px-2 py-1 cursor-pointer"
            >
              Close
            </button>
          </div>
        </div>
      )}
    </div>
  );
}
