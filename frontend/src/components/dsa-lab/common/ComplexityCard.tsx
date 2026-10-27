'use client';

import React, { useState } from 'react';

export interface ComplexityMetric {
  operation: string;
  averageTime: string;
  worstTime: string;
  space: string;
  notes?: string;
}

export interface ComplexityCardProps {
  algorithmName: string;
  problemSolved: string;
  whyEmergencyGridUsesIt: string;
  howItWorks: string;
  metrics: ComplexityMetric[];
  strengths: string[];
  limitations: string[];
  cppHeader: string;
  cppSource?: string;
  testFile: string;
  defaultExpanded?: boolean;
}

export default function ComplexityCard({
  algorithmName,
  problemSolved,
  whyEmergencyGridUsesIt,
  howItWorks,
  metrics,
  strengths,
  limitations,
  cppHeader,
  cppSource,
  testFile,
  defaultExpanded = false,
}: ComplexityCardProps) {
  const [isOpen, setIsOpen] = useState(defaultExpanded);

  return (
    <div className="rounded-xl border border-slate-200 bg-white p-4 space-y-3.5 shadow-xs">
      <div className="flex items-center justify-between">
        <div className="flex items-center space-x-2">
          <span className="w-2 h-2 rounded-full bg-blue-600" />
          <h3 className="text-xs font-bold text-slate-800 uppercase tracking-wide">
            Algorithm Details & Viva Reference: {algorithmName}
          </h3>
        </div>
        <button
          type="button"
          onClick={() => setIsOpen(!isOpen)}
          className="text-xs text-blue-600 hover:text-blue-700 font-semibold px-2 py-1 rounded hover:bg-blue-50 transition-colors"
        >
          {isOpen ? 'Hide Details ▲' : 'Show Details ▼'}
        </button>
      </div>

      {isOpen && (
        <div className="space-y-4 pt-2 border-t border-slate-100 text-xs text-slate-600">
          {/* Conceptual Summary */}
          <div className="grid grid-cols-1 md:grid-cols-3 gap-3">
            <div className="p-3 rounded-lg bg-slate-50 border border-slate-200 space-y-1">
              <span className="text-[10px] uppercase font-bold text-slate-500 tracking-wider">
                1. Problem Solved
              </span>
              <p className="text-slate-800 leading-relaxed">{problemSolved}</p>
            </div>

            <div className="p-3 rounded-lg bg-blue-50/50 border border-blue-100 space-y-1">
              <span className="text-[10px] uppercase font-bold text-blue-700 tracking-wider">
                2. Why EmergencyGrid Uses It
              </span>
              <p className="text-slate-800 leading-relaxed">{whyEmergencyGridUsesIt}</p>
            </div>

            <div className="p-3 rounded-lg bg-teal-50/50 border border-teal-100 space-y-1">
              <span className="text-[10px] uppercase font-bold text-teal-700 tracking-wider">
                3. How It Works
              </span>
              <p className="text-slate-800 leading-relaxed">{howItWorks}</p>
            </div>
          </div>

          {/* Big-O Complexity Table */}
          <div className="rounded-lg border border-slate-200 overflow-hidden">
            <div className="bg-slate-50 px-3 py-2 border-b border-slate-200 flex items-center justify-between">
              <span className="font-bold text-slate-800 text-[11px] uppercase tracking-wider">
                Time & Space Complexity (Matching C++20 Implementation)
              </span>
              <span className="text-[10px] text-slate-500 font-mono">
                Academic Rigor
              </span>
            </div>
            <div className="overflow-x-auto">
              <table className="w-full text-left font-mono text-xs">
                <thead>
                  <tr className="bg-slate-100/70 text-slate-700 border-b border-slate-200">
                    <th className="py-2 px-3 font-semibold">Operation</th>
                    <th className="py-2 px-3 font-semibold">Average Case</th>
                    <th className="py-2 px-3 font-semibold">Worst Case</th>
                    <th className="py-2 px-3 font-semibold">Space</th>
                    <th className="py-2 px-3 font-semibold font-sans">Implementation Notes</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-slate-100">
                  {metrics.map((m, idx) => (
                    <tr key={idx} className="hover:bg-slate-50 transition-colors">
                      <td className="py-2 px-3 font-semibold text-slate-900">{m.operation}</td>
                      <td className="py-2 px-3 text-emerald-700 font-bold">{m.averageTime}</td>
                      <td className="py-2 px-3 text-amber-700 font-bold">{m.worstTime}</td>
                      <td className="py-2 px-3 text-blue-700 font-bold">{m.space}</td>
                      <td className="py-2 px-3 font-sans text-slate-600">{m.notes || '—'}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          </div>

          {/* Strengths & Limitations */}
          <div className="grid grid-cols-1 md:grid-cols-2 gap-3">
            <div className="p-3 rounded-lg bg-emerald-50/50 border border-emerald-100 space-y-1">
              <span className="text-[10px] font-bold uppercase text-emerald-700 tracking-wider">
                Strengths & Guarantees
              </span>
              <ul className="list-disc list-inside space-y-0.5 text-slate-700 text-[11px]">
                {strengths.map((s, idx) => (
                  <li key={idx}>{s}</li>
                ))}
              </ul>
            </div>

            <div className="p-3 rounded-lg bg-rose-50/50 border border-rose-100 space-y-1">
              <span className="text-[10px] font-bold uppercase text-rose-700 tracking-wider">
                Limitations & Trade-offs
              </span>
              <ul className="list-disc list-inside space-y-0.5 text-slate-700 text-[11px]">
                {limitations.map((l, idx) => (
                  <li key={idx}>{l}</li>
                ))}
              </ul>
            </div>
          </div>

          {/* C++ Source Code References */}
          <div className="p-2.5 rounded-lg bg-slate-50 border border-slate-200 flex flex-wrap items-center justify-between gap-2 text-[11px]">
            <div className="flex flex-wrap items-center gap-2">
              <span className="font-semibold text-slate-600">C++ Source:</span>
              <span className="px-2 py-0.5 rounded bg-white text-blue-700 border border-slate-200 font-mono">
                {cppHeader}
              </span>
              {cppSource && (
                <span className="px-2 py-0.5 rounded bg-white text-blue-700 border border-slate-200 font-mono">
                  {cppSource}
                </span>
              )}
            </div>

            <div className="flex items-center space-x-2">
              <span className="font-semibold text-slate-600">Test Suite:</span>
              <span className="px-2 py-0.5 rounded bg-white text-emerald-700 border border-slate-200 font-mono">
                {testFile}
              </span>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
