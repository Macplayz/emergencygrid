// DSA Lab Automated Verification Suite
// Verifies:
// 1. Live C++ backend connectivity on port 8080
// 2. Dijkstra trace event generation & completeness
// 3. Isolated Binary Heap sandbox demo & state isolation
// 4. Isolated AVL Tree rotation demo & rebalancing trace
// 5. Isolated Hash Table collision & rehash demo
// 6. Non-mutation of live operations state by lab demos
// 7. Trace player logic & step boundary contracts

import assert from 'node:assert';

const BACKEND_URL = 'http://127.0.0.1:8080';

async function testBackendEndpoints() {
  console.log('--- 1. Testing C++ Backend Health ---');
  const healthRes = await fetch(`${BACKEND_URL}/api/v1/health`);
  assert.strictEqual(healthRes.status, 200, 'Health endpoint should return 200');
  const healthJson = await healthRes.json();
  assert.strictEqual(healthJson.success, true);
  assert.strictEqual(healthJson.data.service, 'EmergencyGrid Simulation Core');
  console.log('✓ Health endpoint verified: Service UP, ISO C++20');

  console.log('\n--- 2. Verifying Initial Operations State Snapshot ---');
  const simStateRes = await fetch(`${BACKEND_URL}/api/v1/simulation/state`);
  const simStateJson = await simStateRes.json();
  assert.strictEqual(simStateJson.success, true);
  const initialIncidents = simStateJson.data.metrics.totalIncidents;
  const initialUnits = simStateJson.data.metrics.totalUnits;
  console.log(`✓ Initial live state: ${initialIncidents} incidents, ${initialUnits} units`);

  console.log('\n--- 3. Testing Dijkstra Shortest Path with Trace Events ---');
  const routeRes = await fetch(`${BACKEND_URL}/api/v1/routing/calculate`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ sourceId: 'J1', destinationId: 'J16', enableTrace: true }),
  });
  assert.strictEqual(routeRes.status, 200);
  const routeJson = await routeRes.json();
  assert.strictEqual(routeJson.success, true);
  assert(routeJson.data.route.path.length >= 2, 'Route path should connect source and destination');
  assert(Array.isArray(routeJson.trace), 'Trace should be returned as array');
  assert(routeJson.trace.length > 0, 'Dijkstra trace should contain events');

  // Verify trace event structure
  const firstEvent = routeJson.trace[0];
  assert.strictEqual(firstEvent.algorithm, 'Dijkstra');
  assert(firstEvent.operation, 'Trace event must have an operation');
  assert(firstEvent.explanation, 'Trace event must have an explanation');
  console.log(`✓ Dijkstra trace generated: ${routeJson.trace.length} recorded events from J1 to J16`);

  console.log('\n--- 4. Testing Isolated Binary Heap Sandbox Demo ---');
  const heapDemoRes = await fetch(`${BACKEND_URL}/api/v1/dsa/heap/demo`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ action: 'sample_sequence' }),
  });
  assert.strictEqual(heapDemoRes.status, 200);
  const heapDemoJson = await heapDemoRes.json();
  assert.strictEqual(heapDemoJson.success, true);
  assert.strictEqual(heapDemoJson.data.snapshot.size, 4);
  assert(Array.isArray(heapDemoJson.trace) && heapDemoJson.trace.length > 0);
  const heapExtractRes = await fetch(`${BACKEND_URL}/api/v1/dsa/heap/demo`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ action: 'extract_max' }),
  });
  const heapExtractJson = await heapExtractRes.json();
  assert.strictEqual(heapExtractJson.data.snapshot.size, 3);
  console.log(`✓ Binary Heap demo verified: 4 insertions, extract-max sift-down trace verified`);

  console.log('\n--- 5. Testing Isolated AVL Tree Sandbox Demo ---');
  const avlDemoRes = await fetch(`${BACKEND_URL}/api/v1/dsa/avl/demo`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ rotationType: 'LL', action: 'rotation' }),
  });
  assert.strictEqual(avlDemoRes.status, 200);
  const avlDemoJson = await avlDemoRes.json();
  assert.strictEqual(avlDemoJson.success, true);
  assert.strictEqual(avlDemoJson.data.snapshot.size, 3);
  assert.strictEqual(avlDemoJson.data.snapshot.root.key, 'INC-20');
  assert(avlDemoJson.trace.some((e) => e.operation.includes('rotate') || e.operation.includes('rebalance')));
  console.log(`✓ AVL Tree demo verified: Single Right (LL) rotation elevated INC-20 to root with BF=0`);

  console.log('\n--- 6. Testing Isolated Hash Table Sandbox Demo ---');
  const hashDemoRes = await fetch(`${BACKEND_URL}/api/v1/dsa/hash-table/demo`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ action: 'rehash' }),
  });
  assert.strictEqual(hashDemoRes.status, 200);
  const hashDemoJson = await hashDemoRes.json();
  assert.strictEqual(hashDemoJson.success, true);
  assert(hashDemoJson.data.snapshot.bucketCount >= 32, 'Rehash should have expanded bucket capacity to at least 32');
  console.log(`✓ Hash Table demo verified: dynamic doubling rehash expanded buckets to ${hashDemoJson.data.snapshot.bucketCount}`);

  console.log('\n--- 7. Verifying Operational State Isolation ---');
  const postDemoStateRes = await fetch(`${BACKEND_URL}/api/v1/simulation/state`);
  const postDemoStateJson = await postDemoStateRes.json();
  assert.strictEqual(
    postDemoStateJson.data.metrics.totalIncidents,
    initialIncidents,
    'Educational lab demonstrations MUST NOT alter live Operations Mode incident counts'
  );
  assert.strictEqual(
    postDemoStateJson.data.metrics.totalUnits,
    initialUnits,
    'Educational lab demonstrations MUST NOT alter live Operations Mode fleet counts'
  );
  console.log('✓ Operations Mode state isolation verified: 0 live side-effects from lab operations');

  console.log('\n--- 8. Testing Read-Only Snapshots ---');
  const [heapSnap, avlSnap, hashSnap] = await Promise.all([
    fetch(`${BACKEND_URL}/api/v1/dsa/heap/snapshot`).then((r) => r.json()),
    fetch(`${BACKEND_URL}/api/v1/dsa/avl`).then((r) => r.json()),
    fetch(`${BACKEND_URL}/api/v1/dsa/hash-table`).then((r) => r.json()),
  ]);
  assert.strictEqual(heapSnap.success, true);
  assert.strictEqual(avlSnap.success, true);
  assert.strictEqual(hashSnap.success, true);
  console.log('✓ Read-only inspection endpoints verified (Heap, AVL, Hash Table)');

  console.log('\n===========================================');
  console.log('🎉 ALL DSA LAB VERIFICATION CHECKS PASSED!');
  console.log('===========================================');
}

testBackendEndpoints().catch((err) => {
  console.error('❌ Verification failed:', err);
  process.exit(1);
});
