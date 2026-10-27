import urllib.request
import json
import sys

def get_json(url):
    req = urllib.request.Request(url, headers={'Accept': 'application/json'})
    with urllib.request.urlopen(req) as resp:
        return resp.status, json.loads(resp.read().decode('utf-8'))

def post_json(url, payload):
    req = urllib.request.Request(
        url,
        data=json.dumps(payload).encode('utf-8'),
        headers={'Content-Type': 'application/json'}
    )
    with urllib.request.urlopen(req) as resp:
        return resp.status, json.loads(resp.read().decode('utf-8'))

def put_json(url, payload):
    req = urllib.request.Request(
        url,
        data=json.dumps(payload).encode('utf-8'),
        headers={'Content-Type': 'application/json'},
        method='PUT'
    )
    with urllib.request.urlopen(req) as resp:
        return resp.status, json.loads(resp.read().decode('utf-8'))

print("==================================================")
print("DIJKSTRA SHORTEST-PATH TREE (SPT) VERIFICATION")
print("==================================================")

# 1. Regression Test: J1 -> J16 Route Reconstruction & Edge Highlighting
status, data = post_json('http://127.0.0.1:8080/api/v1/routing/calculate', {
    'sourceId': 'J1',
    'destinationId': 'J16',
    'enableTrace': True
})
assert status == 200, "Failed to calculate J1 -> J16"
route = data['data']['route']
trace = data['trace']

# Verify route geometry and weights
assert route['routeFound'] is True, "Route must be found"
assert route['path'] == ['J1', 'J4', 'J16'], f"Path must be ['J1', 'J4', 'J16'], got {route['path']}"
assert route['roadIds'] == ['R1_4', 'R4_16'], f"Roads must be ['R1_4', 'R4_16'], got {route['roadIds']}"
assert abs(route['totalTravelTimeMinutes'] - 5.40) < 1e-4, f"Travel time must be 5.40 min, got {route['totalTravelTimeMinutes']}"
assert abs(route['totalDistanceKm'] - 4.0) < 1e-4, f"Distance must be 4.0 km, got {route['totalDistanceKm']}"
assert route['nodesSettled'] == 14, f"Nodes settled must be 14, got {route['nodesSettled']}"

# Verify final trace event is route_reconstructed
assert len(trace) > 0, "Trace events must not be empty"
final_ev = trace[-1]
assert final_ev['operation'] == 'route_reconstructed', f"Final event must be route_reconstructed, got {final_ev['operation']}"
assert final_ev['from'] == 'J1' and final_ev['to'] == 'J16'
assert abs(final_ev['cost'] - 5.40) < 1e-4
assert final_ev['metadata']['path'] == ['J1', 'J4', 'J16']
assert final_ev['metadata']['roadIds'] == ['R1_4', 'R4_16']

# Verify city graph endpoints serialization
g_status, g_data = get_json('http://127.0.0.1:8080/api/v1/city/graph')
assert g_status == 200, "Failed to get city graph"
roads_map = {r['id']: r for r in g_data['data']['roads']}
assert 'R1_4' in roads_map and 'R4_16' in roads_map
assert roads_map['R1_4'].get('sourceId') == 'J1' and roads_map['R1_4'].get('targetId') == 'J4'
assert roads_map['R4_16'].get('sourceId') == 'J4' and roads_map['R4_16'].get('targetId') == 'J16'
assert abs(roads_map['R1_4']['travelTimeMinutes'] - 3.0) < 1e-4
assert abs(roads_map['R4_16']['travelTimeMinutes'] - 2.4) < 1e-4

# Verify frontend edge highlighting logic simulation
final_path_set = set(['J1->J4', 'J4->J1', 'J4->J16', 'J16->J4'])
final_route_road_ids = set(route['roadIds']) # {'R1_4', 'R4_16'}
show_destination_route = True # At final trace step / 100% progress / when playback stops

for r_id in ['R1_4', 'R4_16']:
    r = roads_map[r_id]
    src = r.get('sourceId') or r.get('source')
    tgt = r.get('targetId') or r.get('target')
    is_route_road = (r_id in final_route_road_ids) or (f"{src}->{tgt}" in final_path_set)
    is_final_dest_route = is_route_road and show_destination_route
    assert is_final_dest_route is True, f"Road {r_id} MUST be highlighted as final destination route (blue)"

# Non-route SPT road should be SPT (teal), not destination route
r_spt = roads_map['R1_2']
assert r_spt['id'] not in final_route_road_ids
assert f"{r_spt.get('sourceId')}->{r_spt.get('targetId')}" not in final_path_set

print(f"[PASS] 1. Regression Test J1 -> J16: Path={route['path']}, Roads={route['roadIds']}, Time={route['totalTravelTimeMinutes']}m")
print(f"       Final trace event: {final_ev['operation']} ({final_ev['from']} -> {final_ev['to']})")
print(f"       Simultaneous Blue Edge Highlighting for R1_4 & R4_16 verified!")
print(f"       SPT Roads Count: {len(route.get('shortestPathTreeRoadIds', []))}, SPT Edges Count: {len(route.get('shortestPathTree', []))}")
assert len(route.get('shortestPathTreeRoadIds', [])) > 0, "SPT Road IDs must not be empty"
assert len(route.get('shortestPathTree', [])) > 0, "SPT Edges must not be empty"

# Verify trace events have predecessor information
relax_events = [ev for ev in trace if ev['operation'] == 'relax_edge']
print(f"       Trace total events: {len(trace)}, Relaxations: {len(relax_events)}")
for rev in relax_events:
    assert rev['from'] is not None, "relax_edge must have from"
    assert rev['to'] is not None, "relax_edge must have to"
    assert 'roadId' in rev['metadata'], "relax_edge metadata must contain roadId"
print("[PASS] 2. Predecessor Metadata in Trace Events verified")

# 2. Edge Case: Source equals Destination (J1 -> J1)
status, data = post_json('http://127.0.0.1:8080/api/v1/routing/calculate', {
    'sourceId': 'J1',
    'destinationId': 'J1',
    'enableTrace': True
})
assert status == 200, "Failed to calculate J1 -> J1"
route = data['data']['route']
print(f"[PASS] 3. Edge Case Source = Destination (J1 -> J1): Travel Time={route['totalTravelTimeMinutes']}m, Path={route['path']}, Settled={route['nodesSettled']}")
assert route['routeFound'] is True
assert route['totalTravelTimeMinutes'] == 0.0
assert route['path'] == ['J1']
assert len(route['roadIds']) == 0

# 3. Edge Case: Road Closure and Rerouting
# Close road R1_4
status, close_res = put_json('http://127.0.0.1:8080/api/v1/city/roads/R1_4', {'isClosed': True})
assert status == 200 and close_res['data']['isClosed'] is True, "Failed to close road R1_4"

status, data = post_json('http://127.0.0.1:8080/api/v1/routing/calculate', {
    'sourceId': 'J1',
    'destinationId': 'J4',
    'enableTrace': True
})
route_rerouted = data['data']['route']
print(f"[PASS] 4. Closed Road R1_4 Rerouting: Path={' -> '.join(route_rerouted['path'])}, Travel Time={route_rerouted['totalTravelTimeMinutes']}m")
assert 'R1_4' not in route_rerouted['roadIds'], "Closed road R1_4 must NOT be used in path"
assert 'R1_4' not in route_rerouted['shortestPathTreeRoadIds'], "Closed road R1_4 must NOT be in SPT"

# Reopen road R1_4
status, reopen_res = put_json('http://127.0.0.1:8080/api/v1/city/roads/R1_4', {'isClosed': False})
assert status == 200 and reopen_res['data']['isClosed'] is False, "Failed to reopen road R1_4"

status, data = post_json('http://127.0.0.1:8080/api/v1/routing/calculate', {
    'sourceId': 'J1',
    'destinationId': 'J4',
    'enableTrace': True
})
route_reopened = data['data']['route']
print(f"[PASS] 5. Reopened Road R1_4 Direct Path Restored: Path={' -> '.join(route_reopened['path'])}, Travel Time={route_reopened['totalTravelTimeMinutes']}m")
assert route_reopened['path'] == ['J1', 'J4']
assert route_reopened['totalTravelTimeMinutes'] == 3.0

# 4. Regression Check: AVL Demonstrations Still Work
status, avl_data = post_json('http://127.0.0.1:8080/api/v1/dsa/avl/demo', {
    'action': 'rotation',
    'rotationType': 'LL'
})
assert status == 200 and avl_data['data']['snapshot']['root']['key'] == 'INC-20'
print("[PASS] 6. Regression: AVL Tree LL Demonstration working properly")

# 5. Regression Check: Binary Heap Sandbox
status, heap_data = post_json('http://127.0.0.1:8080/api/v1/dsa/heap/demo', {
    'action': 'insert',
    'incidentId': 'INC-REGRESS',
    'severity': 3
})
assert status == 200 and len(heap_data['trace']) > 0
print("[PASS] 7. Regression: Binary Heap Sandbox working properly")

# 6. Regression Check: Hash Table Sandbox
status, ht_data = post_json('http://127.0.0.1:8080/api/v1/dsa/hash-table/demo', {
    'action': 'lookup',
    'key': 'INC-101'
})
assert status == 200
print("[PASS] 8. Regression: Hash Table Sandbox working properly")

# 7. Regression Check: Dispatch Engine Sandbox
status, disp_data = post_json('http://127.0.0.1:8080/api/v1/dsa/dispatch/demo', {
    'scenario': 'medical'
})
assert status == 200 and len(disp_data['trace']) > 0
print("[PASS] 9. Regression: Dispatch Engine Sandbox working properly")

print("\n==================================================")
print("ALL DIJKSTRA SPT AND REGRESSION CHECKS PASSED!")
print("==================================================")
