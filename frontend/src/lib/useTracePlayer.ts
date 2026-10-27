'use client';

import { useState, useEffect, useCallback, useRef } from 'react';
import { TraceEvent } from './types';

export interface UseTracePlayerOptions {
  defaultSpeedMs?: number;
  autoPlay?: boolean;
}

export interface TracePlayerState {
  currentStepIndex: number;
  isPlaying: boolean;
  speedMs: number;
  totalSteps: number;
  currentEvent: TraceEvent | null;
  visibleEvents: TraceEvent[];
  isAtStart: boolean;
  isAtEnd: boolean;
  progressPercent: number;
  play: () => void;
  pause: () => void;
  togglePlay: () => void;
  stepForward: () => void;
  stepBackward: () => void;
  goToStep: (index: number) => void;
  reset: () => void;
  replay: () => void;
  setSpeed: (speedMs: number) => void;
}

export function useTracePlayer(
  events: TraceEvent[],
  options?: UseTracePlayerOptions
): TracePlayerState {
  const [currentStepIndex, setCurrentStepIndex] = useState<number>(0);
  const [isPlaying, setIsPlaying] = useState<boolean>(options?.autoPlay ?? false);
  const [speedMs, setSpeedMs] = useState<number>(options?.defaultSpeedMs ?? 750);

  const totalSteps = events.length;
  const timerRef = useRef<NodeJS.Timeout | null>(null);

  const [prevEvents, setPrevEvents] = useState(events);
  if (events !== prevEvents) {
    setPrevEvents(events);
    setCurrentStepIndex(0);
    setIsPlaying(false);
  }

  const pause = useCallback(() => {
    setIsPlaying(false);
    if (timerRef.current) {
      clearInterval(timerRef.current);
      timerRef.current = null;
    }
  }, []);

  const play = useCallback(() => {
    if (totalSteps === 0) return;
    // If at end, loop to start on play
    if (currentStepIndex >= totalSteps - 1) {
      setCurrentStepIndex(0);
    }
    setIsPlaying(true);
  }, [totalSteps, currentStepIndex]);

  const togglePlay = useCallback(() => {
    if (isPlaying) {
      pause();
    } else {
      play();
    }
  }, [isPlaying, pause, play]);

  const stepForward = useCallback(() => {
    pause();
    setCurrentStepIndex((prev) => Math.min(prev + 1, Math.max(0, totalSteps - 1)));
  }, [pause, totalSteps]);

  const stepBackward = useCallback(() => {
    pause();
    setCurrentStepIndex((prev) => Math.max(0, prev - 1));
  }, [pause]);

  const goToStep = useCallback(
    (index: number) => {
      pause();
      const clamped = Math.max(0, Math.min(index, totalSteps - 1));
      setCurrentStepIndex(clamped);
    },
    [pause, totalSteps]
  );

  const reset = useCallback(() => {
    pause();
    setCurrentStepIndex(0);
  }, [pause]);

  const replay = useCallback(() => {
    pause();
    setCurrentStepIndex(0);
    setTimeout(() => {
      setIsPlaying(true);
    }, 50);
  }, [pause]);

  const setSpeed = useCallback((speed: number) => {
    setSpeedMs(speed);
  }, []);

  // Timer loop for auto-play
  useEffect(() => {
    if (!isPlaying) {
      if (timerRef.current) {
        clearInterval(timerRef.current);
        timerRef.current = null;
      }
      return;
    }

    timerRef.current = setInterval(() => {
      setCurrentStepIndex((prev) => {
        if (prev >= totalSteps - 1) {
          setIsPlaying(false);
          return prev;
        }
        return prev + 1;
      });
    }, speedMs);

    return () => {
      if (timerRef.current) {
        clearInterval(timerRef.current);
        timerRef.current = null;
      }
    };
  }, [isPlaying, speedMs, totalSteps]);

  const currentEvent = totalSteps > 0 && currentStepIndex < totalSteps ? events[currentStepIndex] : null;
  const visibleEvents = totalSteps > 0 ? events.slice(0, currentStepIndex + 1) : [];
  const isAtStart = currentStepIndex === 0;
  const isAtEnd = totalSteps === 0 || currentStepIndex >= totalSteps - 1;
  const progressPercent = totalSteps > 0 ? ((currentStepIndex + 1) / totalSteps) * 100 : 0;

  return {
    currentStepIndex,
    isPlaying,
    speedMs,
    totalSteps,
    currentEvent,
    visibleEvents,
    isAtStart,
    isAtEnd,
    progressPercent,
    play,
    pause,
    togglePlay,
    stepForward,
    stepBackward,
    goToStep,
    reset,
    replay,
    setSpeed,
  };
}
