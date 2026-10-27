'use client';

export default function DsaLabPlaceholder() {
  const dsaModules = [
    {
      title: 'Graph & Dijkstra Shortest Path',
      structure: 'AdjacencyListGraph + Min-Heap',
      complexity: 'O((V + E) log V)',
      description:
        'Step-by-step node visitations, edge relaxations, and tentative distance updates across MetroGrid-16.',
      phase: 'Phase 2',
      badgeColor: 'border-cyan-500/30 text-cyan-400 bg-cyan-500/10',
    },
    {
      title: 'Incident Triage Priority Queue',
      structure: 'BinaryHeap (Max-Heap Array & Tree)',
      complexity: 'O(log N) swim / sink',
      description:
        'Visualizes swim and sink swaps as incident severity and wait times escalate emergency priorities.',
      phase: 'Phase 3',
      badgeColor: 'border-amber-500/30 text-amber-400 bg-amber-500/10',
    },
    {
      title: 'Chronological Resolution Log',
      structure: 'AVLTree (Self-Balancing BST)',
      complexity: 'O(log N) search / insert',
      description:
        'Audits resolved incidents ordered by (Timestamp, Severity). Shows balance factors and LL, RR, LR, RL rotations.',
      phase: 'Phase 4',
      badgeColor: 'border-indigo-500/30 text-indigo-400 bg-indigo-500/10',
    },
    {
      title: 'Entity Lookup Registry',
      structure: 'HashTable (Separate Chaining)',
      complexity: 'O(1) average lookup',
      description:
        'Indexes responders and incidents by ID with DJB2 hashing, collision chain inspection, and dynamic rehashing.',
      phase: 'Phase 5',
      badgeColor: 'border-emerald-500/30 text-emerald-400 bg-emerald-500/10',
    },
  ];

  return (
    <div className="space-y-6">
      <div className="bg-slate-900/60 border border-slate-800 rounded-2xl p-6 shadow-xl">
        <div className="flex flex-col md:flex-row md:items-center md:justify-between gap-4">
          <div>
            <div className="flex items-center space-x-2">
              <h2 className="text-xl font-bold text-slate-100">
                DSA Lab — Algorithmic Inspector
              </h2>
              <span className="px-2.5 py-0.5 text-xs font-semibold rounded-full bg-indigo-500/15 text-indigo-300 border border-indigo-500/30">
                Academic Inspector
              </span>
            </div>
            <p className="text-xs text-slate-400 mt-1 max-w-2xl leading-relaxed">
              The academic differentiator of EmergencyGrid. Instead of textbook mockups, every visualizer steps through genuine trace events streamed from the C++20 engine.
            </p>
          </div>

          <div className="flex items-center space-x-2 bg-slate-950/80 px-4 py-2 rounded-xl border border-slate-800 text-xs text-slate-300 font-mono">
            <span className="w-2 h-2 rounded-full bg-indigo-400 animate-pulse" />
            <span>Trace Schema: JSON Event Stream</span>
          </div>
        </div>

        {/* DSA Modules Grid */}
        <div className="grid grid-cols-1 md:grid-cols-2 gap-4 mt-6">
          {dsaModules.map((m) => (
            <div
              key={m.title}
              className="bg-slate-950/60 border border-slate-800/90 rounded-xl p-5 hover:border-slate-700/80 transition shadow-sm"
            >
              <div className="flex items-start justify-between">
                <div>
                  <h4 className="text-sm font-bold text-slate-100">{m.title}</h4>
                  <p className="text-xs font-mono text-slate-400 mt-0.5">{m.structure}</p>
                </div>
                <span
                  className={`px-2 py-0.5 text-[10px] font-bold rounded-md border ${m.badgeColor}`}
                >
                  {m.phase}
                </span>
              </div>
              <p className="text-xs text-slate-400 mt-3 leading-relaxed">
                {m.description}
              </p>
              <div className="mt-4 pt-3 border-t border-slate-900 flex items-center justify-between text-[11px]">
                <span className="text-slate-400">Theoretical Bound:</span>
                <span className="font-mono font-semibold text-slate-300">
                  {m.complexity}
                </span>
              </div>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}
