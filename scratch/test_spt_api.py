import urllib.request
import json

req = urllib.request.Request(
    'http://127.0.0.1:8080/api/v1/routing/calculate',
    data=json.dumps({'sourceId': 'J1', 'destinationId': 'J5', 'enableTrace': True}).encode('utf-8'),
    headers={'Content-Type': 'application/json'}
)
res = json.loads(urllib.request.urlopen(req).read().decode('utf-8'))
route = res['data']['route']
print('Route found:', route['routeFound'])
print('Path:', route['path'])
print('Road IDs:', route['roadIds'])
print('SPT Road IDs count:', len(route.get('shortestPathTreeRoadIds', [])))
print('SPT Road IDs:', route.get('shortestPathTreeRoadIds'))
print('SPT Edges count:', len(route.get('shortestPathTree', [])))
for edge in route.get('shortestPathTree', []):
    print(f"  SPT Edge: {edge['from']} -> {edge['to']} via {edge['roadId']}")

print("\nLast trace event metadata:")
print(json.dumps(res['trace'][-1]['metadata'], indent=2))
