import { NextRequest, NextResponse } from 'next/server';

const getBackendBaseUrl = () => {
  const rawUrl = process.env.BACKEND_URL || 'http://localhost:8080';
  return rawUrl.replace(/\/+$/, '');
};

async function handleProxy(req: NextRequest, { params }: { params: Promise<{ path: string[] }> }) {
  const resolvedParams = await params;
  const pathSegment = resolvedParams.path.join('/');
  const backendBase = getBackendBaseUrl();
  const search = req.nextUrl.search;
  const targetUrl = `${backendBase}/${pathSegment}${search}`;

  const method = req.method;
  const headers: Record<string, string> = {
    Accept: 'application/json',
  };

  const contentType = req.headers.get('content-type');
  if (contentType) {
    headers['Content-Type'] = contentType;
  }

  let body: string | undefined = undefined;
  if (method !== 'GET' && method !== 'HEAD') {
    try {
      body = await req.text();
    } catch {
      // No body or failed to read
    }
  }

  const controller = new AbortController();
  const timeoutId = setTimeout(() => controller.abort(), 6000);

  try {
    const backendRes = await fetch(targetUrl, {
      method,
      headers,
      body: body || undefined,
      cache: 'no-store',
      signal: controller.signal,
    });
    clearTimeout(timeoutId);

    const responseText = await backendRes.text();
    let responseData: unknown;
    try {
      responseData = JSON.parse(responseText);
    } catch {
      responseData = {
        success: false,
        error: {
          code: 'MALFORMED_BACKEND_RESPONSE',
          message: 'The C++ backend response could not be parsed as JSON.',
          details: { raw: responseText.slice(0, 500) },
        },
      };
    }

    return NextResponse.json(responseData, {
      status: backendRes.status,
      headers: {
        'Cache-Control': 'no-store, max-age=0',
      },
    });
  } catch (err: unknown) {
    clearTimeout(timeoutId);
    const isTimeout = err instanceof Error && err.name === 'AbortError';
    const message = isTimeout
      ? 'Connection to C++ simulation engine timed out (6000ms).'
      : err instanceof Error
      ? err.message
      : 'Network connection to C++ backend failed.';

    return NextResponse.json(
      {
        success: false,
        error: {
          code: 'BACKEND_UNAVAILABLE',
          message: `C++ Engine unreachable: ${message}`,
        },
      },
      { status: 503 }
    );
  }
}

export const GET = handleProxy;
export const POST = handleProxy;
export const PUT = handleProxy;
export const PATCH = handleProxy;
export const DELETE = handleProxy;
