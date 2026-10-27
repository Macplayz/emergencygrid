import urllib.request
import json
import sys

def test_demo(rot_type, action='rotation'):
    url = 'http://127.0.0.1:8080/api/v1/dsa/avl/demo'
    payload = json.dumps({'action': action, 'rotationType': rot_type}).encode('utf-8')
    req = urllib.request.Request(url, data=payload, headers={'Content-Type': 'application/json'})
    
    with urllib.request.urlopen(req) as response:
        assert response.status == 200, f"Expected 200, got {response.status}"
        data = json.loads(response.read().decode('utf-8'))
        
    assert data['success'] is True, "Success envelope must be true"
    trace = data['trace']
    final_root = data['data']['snapshot']['root']
    
    print(f"\n==========================================")
    print(f"Testing Demo: {rot_type} (action: {action})")
    print(f"Trace events count: {len(trace)}")
    
    # Check step 1
    step1 = trace[0]
    print(f"Step 1: op={step1.get('operation')} explanation='{step1.get('explanation')}'")
    assert step1['operation'] == 'initial_state', f"Step 1 must be initial_state, got {step1['operation']}"
    
    if action == 'inorder':
        assert step1['metadata']['empty'] is False
        assert step1['metadata']['treeSnapshot'] is not None
    else:
        assert step1['metadata']['empty'] is True
        assert step1['metadata']['treeSnapshot'] is None
        
        # Check step 2
        step2 = trace[1]
        print(f"Step 2: op={step2.get('operation')} entityId={step2.get('entityId')} snap_root={step2.get('metadata', {}).get('treeSnapshot', {}).get('key')}")
        assert step2['operation'] == 'insert'
        assert step2['metadata']['treeSnapshot'] is not None
        assert step2['metadata']['treeSnapshot']['key'] == step2['entityId']
    
    # Check final tree root and children
    root_key = final_root['key']
    left_key = final_root['left']['key'] if final_root['left'] else None
    right_key = final_root['right']['key'] if final_root['right'] else None
    height = final_root['height']
    bf = final_root['balanceFactor']
    
    print(f"Final tree: root={root_key}, left={left_key}, right={right_key}, height={height}, BF={bf}")
    assert root_key == 'INC-20', f"Expected root INC-20, got {root_key}"
    assert left_key == 'INC-10', f"Expected left child INC-10, got {left_key}"
    assert right_key == 'INC-30', f"Expected right child INC-30, got {right_key}"
    assert height == 2, f"Expected height 2, got {height}"
    assert bf == 0, f"Expected BF 0, got {bf}"
    
    # Check operations in trace
    ops = [ev['operation'] for ev in trace]
    if action == 'inorder':
        inorder_visits = [ev['entityId'] for ev in trace if ev['operation'] == 'inorder_visit']
        print(f"Inorder visits: {inorder_visits}")
        assert inorder_visits == ['INC-10', 'INC-20', 'INC-30'], f"Inorder sequence incorrect: {inorder_visits}"
    elif rot_type == 'LL':
        assert 'rotate_ll' in ops, "LL demo must have rotate_ll"
        assert 'rotate_rr' not in ops, "LL demo must NOT have rotate_rr"
    elif rot_type == 'RR':
        assert 'rotate_rr' in ops, "RR demo must have rotate_rr"
        assert 'rotate_ll' not in ops, "RR demo must NOT have rotate_ll"
    elif rot_type == 'LR':
        assert 'rotate_lr_start' in ops
        assert 'rotate_lr_intermediate' in ops
        assert 'rotate_rr' in ops
        assert 'rotate_ll' in ops
    elif rot_type == 'RL':
        assert 'rotate_rl_start' in ops
        assert 'rotate_rl_intermediate' in ops
        assert 'rotate_ll' in ops
        assert 'rotate_rr' in ops
    elif rot_type == 'BALANCED':
        assert not any('rotate' in op for op in ops), "BALANCED demo must NOT trigger any rotation"
        print("Verified: BALANCED example executed with 0 rotations.")
        
    print(f"PASSED: {rot_type} ({action})")

for rot in ['LL', 'RR', 'LR', 'RL', 'BALANCED']:
    test_demo(rot, 'rotation')

test_demo('LL', 'inorder')

print("\n==========================================")
print("ALL AVL DEMO TESTS PASSED PERFECTLY!")
print("==========================================")
