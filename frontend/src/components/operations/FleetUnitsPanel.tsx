'use client';

import { useState, useMemo } from 'react';
import { EmergencyUnit, UnitStatus } from '@/lib/types';

interface FleetUnitsPanelProps {
  units: EmergencyUnit[];
  selectedUnit: EmergencyUnit | null;
  onSelectUnit: (unit: EmergencyUnit | null) => void;
}

const AGENCY_BADGES: Record<string, { label: string; bg: string; text: string; border: string }> = {
  MEDICAL: { label: 'EMS', bg: 'bg-sky-50', text: 'text-sky-800', border: 'border-sky-200' },
  FIRE: { label: 'FIRE', bg: 'bg-red-50', text: 'text-red-800', border: 'border-red-200' },
  POLICE: { label: 'POLICE', bg: 'bg-amber-50', text: 'text-amber-800', border: 'border-amber-200' },
};

const UNIT_STATUS_STYLES: Record<UnitStatus, { bg: string; text: string; border: string; dot: string }> = {
  AVAILABLE: { bg: 'bg-emerald-50', text: 'text-emerald-800', border: 'border-emerald-200', dot: 'bg-[#15803D]' },
  DISPATCHED: { bg: 'bg-amber-50', text: 'text-amber-800', border: 'border-amber-200', dot: 'bg-[#D97706]' },
  RESPONDING: { bg: 'bg-sky-50', text: 'text-sky-800', border: 'border-sky-200', dot: 'bg-[#0284C7]' },
  BUSY: { bg: 'bg-red-50', text: 'text-red-800', border: 'border-red-200', dot: 'bg-[#DC2626]' },
  OUT_OF_SERVICE: { bg: 'bg-slate-100', text: 'text-slate-600', border: 'border-slate-200', dot: 'bg-slate-400' },
};

export default function FleetUnitsPanel({
  units,
  selectedUnit,
  onSelectUnit,
}: FleetUnitsPanelProps) {
  const [agencyFilter, setAgencyFilter] = useState<string>('ALL');
  const [statusFilter, setStatusFilter] = useState<string>('ALL');

  const filteredUnits = useMemo(() => {
    return units.filter((u) => {
      if (agencyFilter !== 'ALL') {
        const agency = u.agency || u.serviceType;
        if (agency !== agencyFilter) return false;
      }
      if (statusFilter === 'AVAILABLE' && u.status !== 'AVAILABLE') return false;
      if (statusFilter === 'BUSY' && u.status === 'AVAILABLE') return false;
      return true;
    });
  }, [units, agencyFilter, statusFilter]);

  const availableCount = useMemo(() => {
    return units.filter((u) => u.status === 'AVAILABLE').length;
  }, [units]);

  return (
    <div className="bg-white border border-[#DCE3EC] rounded-xl flex flex-col shadow-xs overflow-hidden">
      {/* Header */}
      <div className="p-4 border-b border-[#DCE3EC] bg-white flex flex-wrap items-center justify-between gap-3">
        <div>
          <div className="flex items-center space-x-2">
            <span className="w-2.5 h-2.5 rounded-full bg-[#15803D]" />
            <h2 className="text-sm font-bold text-[#172033] uppercase tracking-wide">
              Emergency Fleet Status
            </h2>
            <span className="px-2 py-0.5 rounded-full text-[11px] font-mono bg-slate-100 text-[#526176] border border-slate-200">
              {availableCount} / {units.length} Available
            </span>
          </div>
          <p className="text-[11px] text-[#526176] mt-0.5">
            Multi-agency responder capabilities, station locations, and readiness telemetry
          </p>
        </div>

        {/* Filters */}
        <div className="flex items-center space-x-2 text-xs">
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

          <select
            value={statusFilter}
            onChange={(e) => setStatusFilter(e.target.value)}
            className="bg-white border border-[#DCE3EC] text-[#172033] rounded-md px-2 py-1 text-xs focus:outline-none focus:ring-1 focus:ring-[#3563D9]"
          >
            <option value="ALL">All Statuses</option>
            <option value="AVAILABLE">Available Only</option>
            <option value="BUSY">Active/Busy Only</option>
          </select>
        </div>
      </div>

      {/* Units Grid */}
      <div className="p-3 sm:p-4 grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-3 overflow-y-auto max-h-[380px]">
        {filteredUnits.length === 0 ? (
          <div className="col-span-full py-12 text-center text-[#718096] text-xs">
            No units match the selected filters.
          </div>
        ) : (
          filteredUnits.map((unit) => {
            const isSelected = selectedUnit?.id === unit.id;
            const agencyBadge = AGENCY_BADGES[unit.agency || unit.serviceType] || AGENCY_BADGES.MEDICAL;
            const statusStyle = UNIT_STATUS_STYLES[unit.status] || UNIT_STATUS_STYLES.AVAILABLE;

            return (
              <div
                key={unit.id}
                onClick={() => onSelectUnit(isSelected ? null : unit)}
                className={`p-3 rounded-xl border transition-all cursor-pointer flex flex-col justify-between ${
                  isSelected
                    ? 'bg-emerald-50/70 border-emerald-400 ring-1 ring-emerald-300 shadow-sm'
                    : 'bg-white border-[#DCE3EC] hover:border-slate-300 hover:bg-[#F8FAFC]'
                }`}
              >
                {/* Top Row: Unit ID, Agency Badge, Status Dot */}
                <div className="flex items-start justify-between">
                  <div>
                    <div className="flex items-center space-x-1.5">
                      <span className="font-mono font-bold text-[#172033] text-sm">
                        {unit.id}
                      </span>
                      <span
                        className={`px-1.5 py-0.2 rounded text-[9px] font-bold border ${agencyBadge.bg} ${agencyBadge.text} ${agencyBadge.border}`}
                      >
                        {agencyBadge.label}
                      </span>
                    </div>
                    <div className="text-[11px] text-[#526176] mt-0.5">
                      {unit.callSign || unit.name}
                    </div>
                  </div>

                  {/* Status Pill */}
                  <div
                    className={`flex items-center space-x-1.5 px-2 py-0.5 rounded-full text-[10px] font-semibold border ${statusStyle.bg} ${statusStyle.text} ${statusStyle.border}`}
                  >
                    <span className={`w-1.5 h-1.5 rounded-full ${statusStyle.dot}`} />
                    <span>{unit.status}</span>
                  </div>
                </div>

                {/* Middle Info: Station Location & Capabilities */}
                <div className="mt-3 space-y-1.5 text-[11px]">
                  <div className="flex items-center justify-between text-[#526176]">
                    <span>Station Location:</span>
                    <span className="font-mono font-bold text-[#172033]">
                      {unit.locationId}
                    </span>
                  </div>

                  {/* Capabilities Tags */}
                  {unit.capabilities && unit.capabilities.length > 0 && (
                    <div className="flex items-center gap-1 flex-wrap pt-0.5">
                      {unit.capabilities.map((cap) => (
                        <span
                          key={cap}
                          className="px-1.5 py-0.2 rounded bg-[#F8FAFC] text-[9px] font-mono text-[#526176] border border-[#E2E8F0]"
                        >
                          {cap}
                        </span>
                      ))}
                    </div>
                  )}

                  {/* Active Incident if Dispatched */}
                  {unit.activeIncidentId && (
                    <div className="pt-1 border-t border-[#E2E8F0] flex items-center justify-between text-[10px]">
                      <span className="text-[#D97706] font-medium">Assigned Call:</span>
                      <span className="font-mono font-bold text-[#B45309]">
                        {unit.activeIncidentId}
                      </span>
                    </div>
                  )}
                </div>
              </div>
            );
          })
        )}
      </div>
    </div>
  );
}
