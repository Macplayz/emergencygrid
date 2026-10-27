'use client';

import Link from 'next/link';
import { usePathname } from 'next/navigation';
import { BackendStatusResult } from '@/lib/types';

interface HeaderProps {
  status?: BackendStatusResult | null;
  loading?: boolean;
}

export default function Header({ status, loading }: HeaderProps) {
  const pathname = usePathname();

  const isOpsActive = pathname === '/';
  const isDsaActive = pathname.startsWith('/dsa-lab');

  return (
    <header className="border-b border-slate-200 bg-white/95 backdrop-blur-md sticky top-0 z-50 shadow-xs">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 h-16 flex items-center justify-between">
        {/* Brand & Subtitle */}
        <div className="flex items-center space-x-3">
          <div className="flex items-center justify-center w-9 h-9 rounded-lg bg-blue-600 text-white font-extrabold text-base shadow-sm">
            EG
          </div>
          <div>
            <div className="flex items-center space-x-2">
              <span className="font-extrabold text-slate-900 text-lg tracking-tight">
                Emergency<span className="text-blue-600">Grid</span>
              </span>
              <span className="px-2 py-0.5 text-[10px] font-semibold uppercase tracking-wider rounded bg-slate-100 text-slate-600 border border-slate-200">
                Simulation
              </span>
            </div>
            <p className="text-[11px] text-slate-500 hidden sm:block">
              Multi-Agency Dispatch & Data Structures Lab
            </p>
          </div>
        </div>

        {/* Navigation Tabs */}
        <nav className="flex items-center space-x-1 sm:space-x-1.5 p-1 bg-slate-100 rounded-xl border border-slate-200">
          <Link
            href="/"
            className={`px-3.5 py-1.5 rounded-lg text-sm font-medium transition-all ${
              isOpsActive
                ? 'bg-white text-slate-900 shadow-xs font-semibold'
                : 'text-slate-600 hover:text-slate-900 hover:bg-white/50'
            }`}
          >
            Operations Mode
          </Link>
          <Link
            href="/dsa-lab"
            className={`px-3.5 py-1.5 rounded-lg text-sm font-medium transition-all flex items-center space-x-1.5 ${
              isDsaActive
                ? 'bg-white text-blue-700 shadow-xs font-semibold'
                : 'text-slate-600 hover:text-slate-900 hover:bg-white/50'
            }`}
          >
            <span>DSA Lab</span>
            <span className="px-1.5 py-0.2 text-[9px] font-bold rounded bg-blue-50 text-blue-600 border border-blue-200">
              Lab
            </span>
          </Link>
        </nav>

        {/* Backend Connectivity Status Pill */}
        <div className="flex items-center space-x-2">
          {loading ? (
            <div className="flex items-center space-x-2 px-3 py-1 rounded-full bg-slate-100 border border-slate-200 text-xs text-slate-600">
              <span className="w-2 h-2 rounded-full bg-amber-500 animate-pulse" />
              <span className="hidden sm:inline">Checking Engine...</span>
            </div>
          ) : status?.reachable ? (
            <div className="flex items-center space-x-2 px-3 py-1 rounded-full bg-emerald-50 border border-emerald-200 text-xs text-emerald-800 shadow-xs">
              <span className="w-2 h-2 rounded-full bg-emerald-600" />
              <span className="font-medium">System Online</span>
              {status.latencyMs !== undefined && (
                <span className="text-[10px] text-emerald-600">({status.latencyMs}ms)</span>
              )}
            </div>
          ) : (
            <div className="flex items-center space-x-2 px-3 py-1 rounded-full bg-rose-50 border border-rose-200 text-xs text-rose-800 shadow-xs">
              <span className="w-2 h-2 rounded-full bg-rose-600" />
              <span className="font-medium">System Offline</span>
            </div>
          )}
        </div>
      </div>
    </header>
  );
}
