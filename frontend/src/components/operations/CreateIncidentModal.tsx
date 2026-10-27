'use client';

import { useState } from 'react';
import { CreateIncidentInput, Intersection } from '@/lib/types';

interface CreateIncidentModalProps {
  isOpen: boolean;
  intersections: Intersection[];
  onClose: () => void;
  onSubmit: (data: CreateIncidentInput) => Promise<void>;
}

const COMMON_CAPABILITIES = [
  { id: 'BLS', label: 'Basic Life Support (BLS)' },
  { id: 'ALS', label: 'Advanced Life Support (ALS)' },
  { id: 'HAZMAT', label: 'Hazardous Materials (HAZMAT)' },
  { id: 'HEAVY_RESCUE', label: 'Heavy Extrication / Rescue' },
  { id: 'TACTICAL', label: 'Tactical Intervention' },
];

export default function CreateIncidentModal({
  isOpen,
  intersections,
  onClose,
  onSubmit,
}: CreateIncidentModalProps) {
  const [incidentType, setIncidentType] = useState<string>('MEDICAL');
  const [severity, setSeverity] = useState<number>(3);
  const [locationId, setLocationId] = useState<string>('');
  const [description, setDescription] = useState<string>('');
  const [capabilities, setCapabilities] = useState<string[]>([]);
  const [customId, setCustomId] = useState<string>('');
  const [loading, setLoading] = useState<boolean>(false);
  const [error, setError] = useState<string | null>(null);

  if (!isOpen) return null;

  const handleToggleCapability = (capId: string) => {
    setCapabilities((prev) =>
      prev.includes(capId) ? prev.filter((c) => c !== capId) : [...prev, capId]
    );
  };

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError(null);

    const selectedLoc = locationId || (intersections.length > 0 ? intersections[0].id : '');
    if (!selectedLoc) {
      setError('Please select a valid city intersection location.');
      return;
    }

    setLoading(true);

    try {
      await onSubmit({
        id: customId.trim() ? customId.trim() : undefined,
        type: incidentType,
        severity: Number(severity),
        locationId: selectedLoc,
        description: description.trim() || undefined,
        requiredCapabilities: capabilities.length > 0 ? capabilities : undefined,
      });

      // Reset form on success
      setDescription('');
      setCustomId('');
      setCapabilities([]);
      onClose();
    } catch (err: unknown) {
      setError(err instanceof Error ? err.message : 'Backend incident creation failed');
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-slate-900/40 backdrop-blur-xs animate-fade-in">
      <div className="bg-white border border-[#DCE3EC] rounded-2xl w-full max-w-lg shadow-xl overflow-hidden flex flex-col text-[#172033]">
        {/* Modal Header */}
        <div className="px-5 py-4 border-b border-[#DCE3EC] bg-white flex items-center justify-between">
          <div className="flex items-center space-x-2">
            <span className="w-2.5 h-2.5 rounded-full bg-[#3563D9]" />
            <h3 className="text-base font-bold text-[#172033] tracking-tight">
              Create Emergency Incident
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

        {/* Form Body */}
        <form onSubmit={handleSubmit} className="p-5 space-y-4 text-xs">
          {error && (
            <div className="p-3 rounded-lg bg-red-50 border border-red-200 text-red-800">
              {error}
            </div>
          )}

          {/* Service Agency / Incident Type */}
          <div>
            <label className="block text-[#172033] font-semibold mb-1">
              Incident Service Category: <span className="text-red-600">*</span>
            </label>
            <div className="grid grid-cols-3 gap-2">
              {[
                { id: 'MEDICAL', label: '🚑 Medical (EMS)', color: 'border-sky-400 text-sky-900 bg-sky-50' },
                { id: 'FIRE', label: '🚒 Fire & Rescue', color: 'border-red-400 text-red-900 bg-red-50' },
                { id: 'POLICE', label: '🚓 Police Dept', color: 'border-amber-400 text-amber-900 bg-amber-50' },
              ].map((type) => (
                <button
                  key={type.id}
                  type="button"
                  onClick={() => setIncidentType(type.id)}
                  className={`p-2.5 rounded-lg border text-center font-bold text-xs transition-all cursor-pointer ${
                    incidentType === type.id
                      ? `${type.color} ring-1 ring-[#3563D9]`
                      : 'border-[#DCE3EC] bg-white text-[#526176] hover:text-[#172033] hover:bg-slate-50'
                  }`}
                >
                  {type.label}
                </button>
              ))}
            </div>
          </div>

          {/* Severity Selector */}
          <div>
            <div className="flex items-center justify-between mb-1">
              <label className="text-[#172033] font-semibold">
                Severity Level: <span className="text-red-600">*</span>
              </label>
              <span className="font-mono font-bold text-xs text-red-700">
                {severity === 4
                  ? '4 — Critical (Life Threat)'
                  : severity === 3
                  ? '3 — High Priority'
                  : severity === 2
                  ? '2 — Medium Severity'
                  : '1 — Low / Routine'}
              </span>
            </div>
            <div className="grid grid-cols-4 gap-2">
              {[
                { val: 1, label: '1 - Low', desc: 'Routine' },
                { val: 2, label: '2 - Med', desc: 'Standard' },
                { val: 3, label: '3 - High', desc: 'Urgent' },
                { val: 4, label: '4 - Crit', desc: 'Immediate' },
              ].map((s) => (
                <button
                  key={s.val}
                  type="button"
                  onClick={() => setSeverity(s.val)}
                  className={`py-2 px-1 rounded-lg border text-center font-bold transition-all cursor-pointer ${
                    severity === s.val
                      ? s.val === 4
                        ? 'bg-red-50 text-red-800 border-red-300 ring-1 ring-red-400'
                        : s.val === 3
                        ? 'bg-amber-50 text-amber-800 border-amber-300 ring-1 ring-amber-400'
                        : 'bg-blue-50 text-blue-800 border-blue-300 ring-1 ring-blue-400'
                      : 'border-[#DCE3EC] bg-white text-[#526176] hover:text-[#172033] hover:bg-slate-50'
                  }`}
                >
                  <div className="text-xs">{s.label}</div>
                  <div className="text-[10px] opacity-75 font-normal">{s.desc}</div>
                </button>
              ))}
            </div>
          </div>

          {/* Location Selector (Dynamic from CityGraph) */}
          <div>
            <label className="block text-[#172033] font-semibold mb-1">
              Intersection Location: <span className="text-red-600">*</span>
            </label>
            <select
              value={locationId || (intersections.length > 0 ? intersections[0].id : '')}
              onChange={(e) => setLocationId(e.target.value)}
              className="w-full bg-white border border-[#DCE3EC] rounded-lg px-3 py-2 text-[#172033] font-mono text-xs focus:outline-none focus:ring-1 focus:ring-[#3563D9]"
            >
              {intersections.map((node) => (
                <option key={node.id} value={node.id}>
                  {node.id} — {node.name} ({node.zone} Zone)
                </option>
              ))}
            </select>
          </div>

          {/* Optional Custom Incident ID */}
          <div>
            <label className="block text-[#526176] text-[11px] mb-1">
              Custom Incident ID (Optional — Leave blank for auto-generation):
            </label>
            <input
              type="text"
              placeholder="e.g. INC-201"
              value={customId}
              onChange={(e) => setCustomId(e.target.value)}
              className="w-full bg-white border border-[#DCE3EC] rounded-lg px-3 py-1.5 text-[#172033] font-mono text-xs focus:outline-none focus:ring-1 focus:ring-[#3563D9]"
            />
          </div>

          {/* Optional Short Description */}
          <div>
            <label className="block text-[#172033] font-semibold mb-1">
              Incident Description (Optional):
            </label>
            <textarea
              rows={2}
              placeholder="e.g. Multi-vehicle collision with fuel leak near east avenue..."
              value={description}
              onChange={(e) => setDescription(e.target.value)}
              className="w-full bg-white border border-[#DCE3EC] rounded-lg p-2.5 text-[#172033] text-xs focus:outline-none focus:ring-1 focus:ring-[#3563D9] resize-none"
            />
          </div>

          {/* Required Capabilities Multi-Select */}
          <div>
            <label className="block text-[#172033] font-semibold mb-1">
              Required Unit Capabilities (Optional):
            </label>
            <div className="grid grid-cols-1 sm:grid-cols-2 gap-1.5 pt-1">
              {COMMON_CAPABILITIES.map((cap) => {
                const checked = capabilities.includes(cap.id);
                return (
                  <label
                    key={cap.id}
                    className={`flex items-center space-x-2 p-2 rounded-lg border cursor-pointer transition-colors ${
                      checked
                        ? 'bg-blue-50 border-blue-200 text-blue-900 font-medium'
                        : 'bg-white border-[#DCE3EC] text-[#526176] hover:text-[#172033]'
                    }`}
                  >
                    <input
                      type="checkbox"
                      checked={checked}
                      onChange={() => handleToggleCapability(cap.id)}
                      className="accent-[#3563D9] rounded"
                    />
                    <span className="text-[11px] font-mono">{cap.label}</span>
                  </label>
                );
              })}
            </div>
          </div>

          <div className="text-[11px] text-[#526176] bg-[#F8FAFC] p-2.5 rounded-lg border border-[#DCE3EC] leading-relaxed">
            Submitting queues this emergency call in the prioritized triage queue and indexes it for rapid dispatch.
          </div>

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
              {loading ? 'Creating Incident...' : 'Add Call to Triage Queue'}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
}
