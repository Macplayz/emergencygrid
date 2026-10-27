'use client';

import React from 'react';
import { TracePlayerState } from '@/lib/useTracePlayer';

interface TracePlayerControlsProps {
  player: TracePlayerState;
  title?: string;
  sourceFile?: string;
}

export default function TracePlayerControls({
  player,
  title,
  sourceFile,
}: TracePlayerControlsProps) {
  const {
    currentStepIndex,
    isPlaying,
    speedMs,
    totalSteps,
    currentEvent,
    isAtStart,
    isAtEnd,
    progressPercent,
    togglePlay,
    stepForward,
    stepBackward,
    goToStep,
    reset,
    replay,
    setSpeed,
  } = player;

  if (totalSteps === 0) {
    return (
      <div className="p-4 rounded-xl border border-slate-200 bg-white text-center text-xs text-slate-500 shadow-xs">
        No active execution trace. Run an algorithmic operation above to inspect step-by-step telemetry.
      </div>
    );
  }

  // Summary line: Step X of Y — Operation (Entity or Route)
  const operationSummary = currentEvent
    ? `${currentEvent.operation}${currentEvent.from && currentEvent.to ? ` (${currentEvent.from} → ${currentEvent.to})` : currentEvent.entityId ? ` (${currentEvent.entityId})` : ''}`
    : 'Ready';

  return (
    <div className="rounded-xl border border-slate-200 bg-white p-4 space-y-3.5 shadow-xs">
      {/* Header bar: Title, Source file link, Speed selector */}
      <div className="flex flex-wrap items-center justify-between gap-2 border-b border-slate-100 pb-3">
        <div className="flex items-center space-x-2">
          <span className="w-2 h-2 rounded-full bg-blue-600" />
          <span className="text-xs font-bold text-slate-900 uppercase tracking-wider">
            {title || 'Algorithm Execution Trace'}
          </span>
          <span className="px-2 py-0.5 rounded text-[11px] font-mono font-medium bg-slate-100 text-slate-700 border border-slate-200">
            Step {currentStepIndex + 1} of {totalSteps}
          </span>
        </div>

        <div className="flex items-center space-x-3 text-xs">
          {sourceFile && (
            <div className="hidden sm:flex items-center space-x-1.5 font-mono text-[11px] text-slate-500">
              <span>Source:</span>
              <span className="text-blue-700 bg-blue-50 px-2 py-0.5 rounded border border-blue-200">
                {sourceFile}
              </span>
            </div>
          )}

          {/* Compact speed selector */}
          <div className="flex items-center space-x-1 bg-slate-50 px-1.5 py-0.5 rounded-lg border border-slate-200 text-[11px] font-mono">
            <span className="text-slate-500 mr-1">Speed:</span>
            {[
              { label: '0.5x', ms: 1400 },
              { label: '1x', ms: 750 },
              { label: '2x', ms: 350 },
            ].map((item) => (
              <button
                key={item.label}
                type="button"
                onClick={() => setSpeed(item.ms)}
                className={`px-1.5 py-0.5 rounded transition-colors ${
                  speedMs === item.ms
                    ? 'bg-blue-600 text-white font-bold'
                    : 'text-slate-600 hover:text-slate-900 hover:bg-slate-200'
                }`}
              >
                {item.label}
              </button>
            ))}
          </div>
        </div>
      </div>

      {/* Primary Status Line */}
      <div className="bg-slate-50 border border-slate-200 rounded-lg px-3 py-2 flex items-center justify-between text-xs">
        <div className="flex items-center space-x-2 truncate">
          <span className="font-semibold text-slate-800 font-mono">
            Step {currentStepIndex + 1} of {totalSteps}:
          </span>
          <span className="text-slate-600 truncate">{operationSummary}</span>
        </div>
        <span className="text-[11px] font-mono text-slate-500 shrink-0 ml-2">
          {Math.round(progressPercent)}%
        </span>
      </div>

      {/* Scrub Slider */}
      <div className="space-y-1">
        <input
          type="range"
          min={0}
          max={Math.max(0, totalSteps - 1)}
          value={currentStepIndex}
          onChange={(e) => goToStep(parseInt(e.target.value, 10))}
          className="w-full h-1.5 bg-slate-200 rounded-lg appearance-none cursor-pointer accent-blue-600 focus:outline-none"
        />
      </div>

      {/* Playback action buttons */}
      <div className="flex flex-wrap items-center justify-between gap-3 pt-1">
        <div className="flex items-center space-x-2">
          <button
            type="button"
            onClick={reset}
            disabled={isAtStart}
            title="Reset to beginning"
            className="px-2.5 py-1.5 rounded-lg border border-slate-200 bg-white text-slate-700 hover:bg-slate-50 disabled:opacity-40 disabled:cursor-not-allowed transition-colors text-xs font-medium"
          >
            ⏮ Reset
          </button>

          <button
            type="button"
            onClick={stepBackward}
            disabled={isAtStart}
            title="Step backward"
            className="px-3 py-1.5 rounded-lg border border-slate-200 bg-white text-slate-700 hover:bg-slate-50 disabled:opacity-40 disabled:cursor-not-allowed transition-colors text-xs font-medium"
          >
            ◀ Previous
          </button>

          <button
            type="button"
            onClick={togglePlay}
            className={`px-4 py-1.5 rounded-lg font-semibold text-xs transition-all shadow-xs flex items-center space-x-1.5 ${
              isPlaying
                ? 'bg-amber-600 text-white hover:bg-amber-700'
                : 'bg-blue-600 text-white hover:bg-blue-700'
            }`}
          >
            <span>{isPlaying ? '⏸ Pause' : '▶ Play'}</span>
          </button>

          <button
            type="button"
            onClick={stepForward}
            disabled={isAtEnd}
            title="Step forward"
            className="px-3 py-1.5 rounded-lg border border-slate-200 bg-white text-slate-700 hover:bg-slate-50 disabled:opacity-40 disabled:cursor-not-allowed transition-colors text-xs font-medium"
          >
            Next ▶
          </button>

          <button
            type="button"
            onClick={replay}
            title="Replay from start"
            className="px-2.5 py-1.5 rounded-lg border border-slate-200 bg-white text-slate-700 hover:bg-slate-50 transition-colors text-xs font-medium"
          >
            🔄 Replay
          </button>
        </div>

        {/* Algorithm Badge */}
        {currentEvent && (
          <div className="flex items-center space-x-2 text-xs font-mono">
            <span className="px-2 py-0.5 rounded bg-blue-50 text-blue-700 border border-blue-200 font-semibold">
              {currentEvent.algorithm}
            </span>
            <span className="px-2 py-0.5 rounded bg-slate-100 text-slate-700 border border-slate-200">
              {currentEvent.operation}
            </span>
          </div>
        )}
      </div>

      {/* Active Event Explanation Card */}
      {currentEvent && (
        <div className="rounded-lg border border-blue-100 bg-blue-50/50 p-3 text-xs space-y-1.5">
          <p className="text-slate-800 leading-relaxed font-sans">
            {currentEvent.explanation}
          </p>

          {currentEvent.metadata && Object.keys(currentEvent.metadata).length > 0 && (
            <div className="pt-1.5 border-t border-blue-100 flex flex-wrap gap-1.5 text-[11px] font-mono">
              {Object.entries(currentEvent.metadata).map(([k, v]) => (
                <span key={k} className="px-1.5 py-0.5 rounded bg-white text-slate-600 border border-slate-200">
                  <span className="text-slate-400">{k}:</span> {typeof v === 'object' ? JSON.stringify(v) : String(v)}
                </span>
              ))}
            </div>
          )}
        </div>
      )}
    </div>
  );
}
