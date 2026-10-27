import { NextResponse } from 'next/server';
import { ApiResponse, BackendHealthData, BackendStatusResult, SimulationStateData } from '@/lib/types';


export async function GET() {
  const rawBackendUrl = process.env.BACKEND_URL || 'http://localhost:8080';
  // Normalize URL (strip trailing slash)
  const backendUrl = rawBackendUrl.replace(/\/+$/, '');
  const startTime = Date.now();

  try {
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), 2500);

    const healthPromise = fetch(`${backendUrl}/api/v1/health`, {
      method: 'GET',
      headers: { Accept: 'application/json' },
      cache: 'no-store',
      signal: controller.signal,
    });

    const statePromise = fetch(`${backendUrl}/api/v1/simulation/state`, {
      method: 'GET',
      headers: { Accept: 'application/json' },
      cache: 'no-store',
      signal: controller.signal,
    });

    const [healthRes, stateRes] = await Promise.all([healthPromise, statePromise]);
    clearTimeout(timeoutId);

    const latencyMs = Date.now() - startTime;

    if (!healthRes.ok) {
      return NextResponse.json<BackendStatusResult>({
        reachable: false,
        checkedAt: new Date().toISOString(),
        backendUrl,
        error: `C++ Backend returned HTTP ${healthRes.status}: ${healthRes.statusText}`,
        latencyMs,
      });
    }

    const healthJson: ApiResponse<BackendHealthData> = await healthRes.json();
    let stateData: SimulationStateData | undefined;

    if (stateRes.ok) {
      const stateJson: ApiResponse<SimulationStateData> = await stateRes.json();
      stateData = stateJson.data;
    }

    return NextResponse.json<BackendStatusResult>({
      reachable: true,
      checkedAt: new Date().toISOString(),
      backendUrl,
      health: healthJson.data,
      state: stateData,
      latencyMs,
    });
  } catch (err: unknown) {
    const latencyMs = Date.now() - startTime;
    const isAbort = err instanceof Error && err.name === 'AbortError';
    const message = isAbort
      ? 'Connection timed out after 2500ms'
      : err instanceof Error
      ? err.message
      : 'Network connection failed';

    return NextResponse.json<BackendStatusResult>({
      reachable: false,
      checkedAt: new Date().toISOString(),
      backendUrl,
      error: `C++ Engine unreachable: ${message}`,
      latencyMs,
    });
  }
}
