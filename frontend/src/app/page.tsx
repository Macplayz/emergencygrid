'use client';

import { useState, useEffect, useCallback } from 'react';
import Header from '@/components/common/Header';
import BackendStatusCard from '@/components/common/BackendStatusCard';
import OperationsDashboard from '@/components/operations/OperationsDashboard';
import { BackendStatusResult } from '@/lib/types';

export default function OperationsPage() {
  const [status, setStatus] = useState<BackendStatusResult | null>(null);
  const [loading, setLoading] = useState<boolean>(true);
  const [showDiagnostics, setShowDiagnostics] = useState<boolean>(false);

  const fetchStatus = useCallback(async () => {
    setLoading(true);
    try {
      const res = await fetch('/api/backend-status', { cache: 'no-store' });
      if (res.ok) {
        const data: BackendStatusResult = await res.json();
        setStatus(data);
      } else {
        setStatus({
          reachable: false,
          checkedAt: new Date().toISOString(),
          backendUrl: 'http://localhost:8080',
          error: `HTTP ${res.status}: Failed to reach backend proxy`,
        });
      }
    } catch (err: unknown) {
      setStatus({
        reachable: false,
        checkedAt: new Date().toISOString(),
        backendUrl: 'http://localhost:8080',
        error: err instanceof Error ? err.message : 'Unknown client fetch error',
      });
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    let isMounted = true;

    async function probeBackend() {
      try {
        const res = await fetch('/api/backend-status', { cache: 'no-store' });
        if (res.ok) {
          const data: BackendStatusResult = await res.json();
          if (isMounted) setStatus(data);
        } else {
          if (isMounted) {
            setStatus({
              reachable: false,
              checkedAt: new Date().toISOString(),
              backendUrl: 'http://localhost:8080',
              error: `HTTP ${res.status}: Failed to reach backend proxy`,
            });
          }
        }
      } catch (err: unknown) {
        if (isMounted) {
          setStatus({
            reachable: false,
            checkedAt: new Date().toISOString(),
            backendUrl: 'http://localhost:8080',
            error: err instanceof Error ? err.message : 'Unknown client fetch error',
          });
        }
      } finally {
        if (isMounted) setLoading(false);
      }
    }

    probeBackend();
    const interval = setInterval(probeBackend, 15000);
    return () => {
      isMounted = false;
      clearInterval(interval);
    };
  }, []);

  const isOnline = Boolean(status?.reachable);

  return (
    <div className="min-h-screen bg-[#F5F7FA] text-[#172033] flex flex-col font-sans">
      <Header status={status} loading={loading} />

      <main className="flex-1 max-w-7xl w-full mx-auto px-4 sm:px-6 lg:px-8 py-6 space-y-6">
        {/* Page Title & Status Header */}
        <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-4">
          <div>
            <div className="flex items-center space-x-2">
              <span className="px-2.5 py-0.5 text-xs font-bold uppercase tracking-wider rounded-md bg-blue-50 text-[#3563D9] border border-blue-200">
                Live Operations
              </span>
              <span className="text-xs text-[#526176]">Multi-Agency Response Grid</span>
            </div>
            <h1 className="text-2xl sm:text-3xl font-bold text-[#172033] tracking-tight mt-1">
              Operations Center
            </h1>
            <p className="text-xs sm:text-sm text-[#526176] mt-0.5 max-w-2xl">
              Real-time multi-agency emergency response and road network coordination console.
            </p>
          </div>

          <div className="flex items-center space-x-2">
            <button
              type="button"
              onClick={() => setShowDiagnostics(!showDiagnostics)}
              className="px-3 py-1.5 rounded-lg text-xs font-medium bg-white hover:bg-slate-50 text-[#526176] border border-[#DCE3EC] shadow-xs transition-colors cursor-pointer"
            >
              {showDiagnostics ? 'Hide Status' : 'System Status'}
            </button>
          </div>
        </div>

        {/* Diagnostic Status Card (Always shown if offline, collapsible if online) */}
        {(!isOnline || showDiagnostics) && (
          <BackendStatusCard
            status={status}
            loading={loading}
            onRefresh={fetchStatus}
          />
        )}

        {/* Offline Fallback Banner if unreachable */}
        {!isOnline && !loading && (
          <div className="p-4 rounded-xl bg-red-50 border border-red-200 text-red-900 text-xs flex flex-col sm:flex-row items-start sm:items-center justify-between gap-3 shadow-sm">
            <div className="flex items-center space-x-2.5">
              <span className="text-lg">⚠</span>
              <div>
                <strong className="block text-red-950 text-sm">Simulation Core Offline</strong>
                <span>
                  Start the backend engine binary with{' '}
                  <code className="font-mono bg-white px-1.5 py-0.5 rounded border border-red-200 text-red-800">
                    ./backend/build/bin/emergency_grid_backend.exe
                  </code>{' '}
                  or verify port 8080.
                </span>
              </div>
            </div>
            <button
              type="button"
              onClick={fetchStatus}
              className="px-3 py-1.5 rounded-lg bg-[#DC2626] hover:bg-[#b91c1c] text-white font-semibold text-xs whitespace-nowrap cursor-pointer shadow-xs"
            >
              Retry Connection
            </button>
          </div>
        )}

        {/* Primary Operations Dashboard */}
        <OperationsDashboard
          initialState={status?.state || null}
          onRefreshHealth={fetchStatus}
        />
      </main>

      <footer className="border-t border-[#DCE3EC] py-6 text-center text-xs text-[#718096]">
        <div className="max-w-7xl mx-auto px-4 space-y-1">
          <p className="font-medium text-[#526176]">
            EmergencyGrid — Advanced Data Structures and Algorithms Capstone Project
          </p>
          <p className="text-[11px] text-[#718096]">
            Strictly an academic simulation platform. Not affiliated with municipal emergency services.
          </p>
        </div>
      </footer>
    </div>
  );
}
