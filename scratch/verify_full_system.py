import urllib.request
import json
import sys

def post_json(url, payload):
    req = urllib.request.Request(
        url,
        data=json.dumps(payload).encode('utf-8'),
        headers={'Content-Type': 'application/json'}
    )
    with urllib.request.urlopen(req) as resp:
        return resp.status, json.loads(resp.read().decode('utf-8'))

def get_json(url):
    req = urllib.request.Request(url)
    with urllib.request.urlopen(req) as resp:
        return resp.status, json.loads(resp.read().decode('utf-8'))

def get_status(url):
    req = urllib.request.Request(url)
    with urllib.request.urlopen(req) as resp:
        return resp.status

print("==================================================")
print("EMERGENCYGRID FULL SYSTEM VERIFICATION SUITE")
print("==================================================")

# 1. Backend Health
status, data = get_json('http://127.0.0.1:8080/api/v1/health')
assert status == 200 and data['data']['status'] == 'UP', "Backend health check failed"
print("[PASS] 1. Backend Health (C++20 Engine UP)")

# 2. City Graph
status, data = get_json('http://127.0.0.1:8080/api/v1/city/graph')
assert status == 200 and len(data['data']['intersections']) > 0, "Graph loading failed"
print(f"[PASS] 2. City Graph ({len(data['data']['intersections'])} intersections, {len(data['data']['roads'])} roads)")

# 3. Dijkstra Routing
status, data = post_json('http://127.0.0.1:8080/api/v1/routing/calculate', {
    'sourceId': 'J1',
    'destinationId': 'J5',
    'enableTrace': True
})
assert status == 200 and data['data']['route']['routeFound'] is True, "Dijkstra routing failed"
print(f"[PASS] 3. Dijkstra Shortest Path (Travel Time: {data['data']['route']['totalTravelTimeMinutes']:.1f} mins, Path: {' -> '.join(data['data']['route']['path'])})")

# 4. Heap Inspector & Heap Demo
status, data = get_json('http://127.0.0.1:8080/api/v1/dsa/heap/snapshot')
assert status == 200, "Heap snapshot failed"
status, data = post_json('http://127.0.0.1:8080/api/v1/dsa/heap/demo', {
    'action': 'insert',
    'incidentId': 'INC-TEST',
    'severity': 4
})
assert status == 200 and len(data['trace']) > 0, "Heap demo failed"
print("[PASS] 4. Binary Max-Heap Inspector & Educational Sandbox")

# 5. AVL Tree Inspector & All 5 Demonstrations
status, data = get_json('http://127.0.0.1:8080/api/v1/dsa/avl')
assert status == 200, "AVL snapshot failed"

for rot in ['LL', 'RR', 'LR', 'RL', 'BALANCED']:
    status, data = post_json('http://127.0.0.1:8080/api/v1/dsa/avl/demo', {
        'action': 'rotation',
        'rotationType': rot
    })
    assert status == 200, f"AVL demo {rot} failed"
    step1 = data['trace'][0]
    step2 = data['trace'][1]
    assert step1['operation'] == 'initial_state' and step1['metadata']['empty'] is True, f"AVL {rot} step 1 must be empty initial state"
    assert step2['operation'] == 'insert' and step2['metadata']['treeSnapshot'] is not None, f"AVL {rot} step 2 must have populated snapshot"
    root = data['data']['snapshot']['root']
    assert root['key'] == 'INC-20' and root['left']['key'] == 'INC-10' and root['right']['key'] == 'INC-30', f"AVL {rot} final root mismatch"
print("[PASS] 5. AVL Tree Demonstrations (LL, RR, LR, RL, BALANCED verified with exact snapshots & rotations)")

# 6. Hash Table Inspector & Demo
status, data = get_json('http://127.0.0.1:8080/api/v1/dsa/hash-table')
assert status == 200, "Hash table stats failed"
status, data = post_json('http://127.0.0.1:8080/api/v1/dsa/hash-table/demo', {
    'action': 'lookup',
    'key': 'INC-101'
})
assert status == 200, "Hash table demo failed"
print("[PASS] 6. Custom Hash Table Inspector & Lookup Sandbox")

# 7. Dispatch Allocation Demo
status, data = post_json('http://127.0.0.1:8080/api/v1/dsa/dispatch/demo', {
    'scenario': 'medical'
})
assert status == 200 and len(data['trace']) > 0, "Dispatch demo failed"
print(f"[PASS] 7. Dispatch Allocation Sandbox (Data keys: {list(data['data'].keys())}, Candidate traces: {len(data['trace'])})")

# 8. Frontend Endpoints (Port 3000)
op_status = get_status('http://localhost:3000/')
assert op_status == 200, "Operations Mode failed to serve"
dsa_status = get_status('http://localhost:3000/dsa-lab')
assert dsa_status == 200, "DSA Lab failed to serve"
print("[PASS] 8. Frontend Next.js Pages (Operations Mode & DSA Lab returning HTTP 200)")

print("\n==================================================")
print("ALL SYSTEM VERIFICATION CHECKS PASSED SUCCESSFULLY!")
print("==================================================")
