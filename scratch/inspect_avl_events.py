import urllib.request
import json

req = urllib.request.Request(
    'http://127.0.0.1:8080/api/v1/dsa/avl/demo',
    data=json.dumps({'action': 'rotation', 'rotationType': 'LL'}).encode('utf-8'),
    headers={'Content-Type': 'application/json'}
)
res = json.loads(urllib.request.urlopen(req).read().decode('utf-8'))
print('Total trace events:', len(res['trace']))
for i, ev in enumerate(res['trace']):
    snap = ev.get('metadata', {}).get('treeSnapshot')
    snap_key = snap.get('key') if snap else None
    print(f"Step {i+1} ({ev.get('operation')}, {ev.get('entityId')}): snap={snap_key}, explanation={ev.get('explanation')}")
