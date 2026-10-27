Write-Host "=== 1. Testing GET /api/v1/health ==="
$health = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/health" -Method Get
Write-Host "Health Response:" ($health | ConvertTo-Json -Depth 3)

Write-Host "`n=== 2. Testing GET /api/v1/city/graph ==="
$graph = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/city/graph" -Method Get
Write-Host "Total Intersections:" $graph.data.intersections.Count
Write-Host "Total Roads:" $graph.data.roads.Count

Write-Host "`n=== 3. Testing POST /api/v1/routing/calculate (J1 -> J5, no trace) ==="
$bodyNoTrace = @{ sourceId = "J1"; destinationId = "J5"; enableTrace = $false } | ConvertTo-Json
$route1 = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/routing/calculate" -Method Post -Body $bodyNoTrace -ContentType "application/json"
Write-Host "Route found:" $route1.data.route.routeFound
Write-Host "Path:" ($route1.data.route.path -join " -> ")
Write-Host "Total Travel Time (mins):" $route1.data.route.totalTravelTimeMinutes
Write-Host "Nodes Settled:" $route1.data.route.nodesSettled
Write-Host "Trace events count:" $route1.trace.Count

Write-Host "`n=== 4. Testing POST /api/v1/routing/calculate (J1 -> J5, WITH trace) ==="
$bodyWithTrace = @{ sourceId = "J1"; destinationId = "J5"; enableTrace = $true } | ConvertTo-Json
$routeTrace = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/routing/calculate" -Method Post -Body $bodyWithTrace -ContentType "application/json"
Write-Host "Trace events count:" $routeTrace.trace.Count
Write-Host "Step 1:" ($routeTrace.trace[0] | ConvertTo-Json -Compress)
Write-Host "Step 2:" ($routeTrace.trace[1] | ConvertTo-Json -Compress)
Write-Host "Final step:" ($routeTrace.trace[-1] | ConvertTo-Json -Compress)

Write-Host "`n=== 5. Testing PUT /api/v1/city/roads/R1_4 (CLOSE road) ==="
$closeBody = @{ isClosed = $true; enableTrace = $true } | ConvertTo-Json
$closeRes = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/city/roads/R1_4" -Method Put -Body $closeBody -ContentType "application/json"
Write-Host "Road R1_4 isClosed:" $closeRes.data.isClosed
Write-Host "Closure event:" ($closeRes.trace[0] | ConvertTo-Json -Compress)

Write-Host "`n=== 6. Testing POST /api/v1/routing/calculate (Rerouted path avoiding closed R1_4) ==="
$route2 = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/routing/calculate" -Method Post -Body $bodyNoTrace -ContentType "application/json"
Write-Host "New Path:" ($route2.data.route.path -join " -> ")
Write-Host "New Travel Time (mins):" $route2.data.route.totalTravelTimeMinutes

Write-Host "`n=== 7. Testing PUT /api/v1/city/roads/R1_4 (REOPEN road) ==="
$reopenBody = @{ isClosed = $false } | ConvertTo-Json
$reopenRes = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/city/roads/R1_4" -Method Put -Body $reopenBody -ContentType "application/json"
Write-Host "Road R1_4 isClosed:" $reopenRes.data.isClosed

Write-Host "`n=== 8. Testing PUT /api/v1/city/roads/R1_4 (Update congestion / travel time) ==="
$congBody = @{ congestionFactor = 2.5 } | ConvertTo-Json
$congRes = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/city/roads/R1_4" -Method Put -Body $congBody -ContentType "application/json"
Write-Host "Road R1_4 new travelTimeMinutes:" $congRes.data.travelTimeMinutes

Write-Host "`n=== 9. Testing Negative Weight Error Handling ==="
try {
    $badBody = @{ travelTimeMinutes = -10.0 } | ConvertTo-Json
    Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/city/roads/R1_4" -Method Put -Body $badBody -ContentType "application/json"
    Write-Host "ERROR: Should have rejected negative travel time!"
} catch {
    Write-Host "Successfully rejected invalid weight with status:" $_.Exception.Response.StatusCode.value__
}

Write-Host "`n=== 10. Testing Unknown Location Error Handling ==="
try {
    $badRoute = @{ sourceId = "UNKNOWN_NODE"; destinationId = "J5" } | ConvertTo-Json
    Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/routing/calculate" -Method Post -Body $badRoute -ContentType "application/json"
    Write-Host "ERROR: Should have rejected unknown node!"
} catch {
    Write-Host "Successfully rejected unknown location with status:" $_.Exception.Response.StatusCode.value__
}

Write-Host "`n=== 11. Reset City Graph & Incidents ==="
$resetRes = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/city/reset" -Method Post
Write-Host "City reset message:" $resetRes.data.message
Write-Host "Closed roads count after reset:" $resetRes.data.closedRoads

Write-Host "`n=== 12. Testing POST /api/v1/incidents (Create Incident 1: Fire, Critical, with trace) ==="
$inc1Body = @{
    id = "INC-101"
    locationId = "J5"
    type = "FIRE"
    severity = 4
    description = "Commercial kitchen fire with active smoke"
    enableTrace = $true
} | ConvertTo-Json
$inc1Res = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/incidents" -Method Post -Body $inc1Body -ContentType "application/json"
Write-Host "Created Incident:" $inc1Res.data.id "Severity:" $inc1Res.data.severityLabel "Status:" $inc1Res.data.status
Write-Host "Heap Trace events count:" $inc1Res.trace.Count
Write-Host "Trace step 1:" ($inc1Res.trace[0] | ConvertTo-Json -Compress)

Write-Host "`n=== 13. Testing POST /api/v1/incidents (Create Incident 2: Medical, High) ==="
$inc2Body = @{
    id = "INC-102"
    locationId = "J2"
    type = "MEDICAL"
    severity = 3
    description = "Cardiac distress at North Gate"
} | ConvertTo-Json
$inc2Res = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/incidents" -Method Post -Body $inc2Body -ContentType "application/json"
Write-Host "Created Incident:" $inc2Res.data.id "Severity:" $inc2Res.data.severityLabel

Write-Host "`n=== 14. Testing POST /api/v1/incidents (Create Incident 3: Police, Medium) ==="
$inc3Body = @{
    id = "INC-103"
    locationId = "J3"
    type = "POLICE"
    severity = 2
    description = "Traffic incident at Eastern Boulevard"
} | ConvertTo-Json
$inc3Res = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/incidents" -Method Post -Body $inc3Body -ContentType "application/json"
Write-Host "Created Incident:" $inc3Res.data.id "Severity:" $inc3Res.data.severityLabel

Write-Host "`n=== 15. Testing GET /api/v1/incidents (List Incidents) ==="
$listRes = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/incidents" -Method Get
Write-Host "Total Incidents listed:" $listRes.data.count

Write-Host "`n=== 16. Testing GET /api/v1/incidents/next (Inspect highest priority) ==="
$nextRes = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/incidents/next?enableTrace=true" -Method Get
Write-Host "Next eligible incident:" $nextRes.data.id "Severity:" $nextRes.data.severityLabel "Should be INC-101 (CRITICAL)"

Write-Host "`n=== 17. Testing POST /api/v1/incidents/next/select (Select INC-101 for dispatch) ==="
$sel1Res = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/incidents/next/select" -Method Post -Body (@{ enableTrace = $true } | ConvertTo-Json) -ContentType "application/json"
Write-Host "Selected incident:" $sel1Res.data.id "New status:" $sel1Res.data.status

Write-Host "`n=== 18. Testing POST /api/v1/incidents/INC-102/cancel (Cancel INC-102) ==="
$cancRes = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/incidents/INC-102/cancel" -Method Post
Write-Host "Cancelled incident:" $cancRes.data.id "Status:" $cancRes.data.status

Write-Host "`n=== 19. Testing POST /api/v1/incidents/next/select (Select next: must skip cancelled INC-102 and select INC-103) ==="
$sel2Res = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/incidents/next/select" -Method Post -Body (@{ enableTrace = $true } | ConvertTo-Json) -ContentType "application/json"
Write-Host "Selected incident after skipping cancelled:" $sel2Res.data.id "Severity:" $sel2Res.data.severityLabel
$skipEvent = $sel2Res.trace | Where-Object { $_.operation -eq "skip_stale_entry" }
Write-Host "Observed skip_stale_entry trace event:" ($skipEvent | ConvertTo-Json -Compress)

Write-Host "`n=== 20. Testing GET /api/v1/dsa/heap/snapshot (DSA Lab Heap Inspector) ==="
$heapSnap = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/dsa/heap/snapshot" -Method Get
Write-Host "Heap snapshot size:" $heapSnap.data.size "isEmpty:" $heapSnap.data.isEmpty

Write-Host "`n=== 21. Testing Duplicate ID Error (409 Conflict) ==="
try {
    $dupeBody = @{ id = "INC-103"; locationId = "J1"; type = "FIRE"; severity = 3 } | ConvertTo-Json
    Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/incidents" -Method Post -Body $dupeBody -ContentType "application/json"
    Write-Host "ERROR: Should have rejected duplicate ID!"
} catch {
    Write-Host "Successfully rejected duplicate ID with status:" $_.Exception.Response.StatusCode.value__
}

Write-Host "`n=== 22. Testing Empty Queue on next/select (404 Not Found) ==="
try {
    # Queue is now empty
    Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/incidents/next/select" -Method Post
    Write-Host "ERROR: Should have rejected empty queue!"
} catch {
    Write-Host "Successfully rejected empty queue selection with status:" $_.Exception.Response.StatusCode.value__
}

Write-Host "`n=== 23. Testing GET /api/v1/incidents/sorted (AVL Inorder Traversal) ==="
$sortedRes = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/incidents/sorted?enableTrace=true" -Method Get
Write-Host "Total sorted incidents:" $sortedRes.data.count
$sortedIds = $sortedRes.data.sortedIncidents | ForEach-Object { $_.id }
Write-Host "Incidents in AVL sorted order:" ($sortedIds -join ", ")
Write-Host "AVL Inorder trace event count:" $sortedRes.trace.Count

Write-Host "`n=== 24. Testing GET /api/v1/incidents/INC-101 (Custom Hash Table Lookup) ==="
$lookupRes = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/incidents/INC-101?enableTrace=true" -Method Get
Write-Host "Lookup ID:" $lookupRes.data.id "Severity:" $lookupRes.data.severityLabel "Status:" $lookupRes.data.status
Write-Host "Hash Table trace count:" $lookupRes.trace.Count
$hashEvent = $lookupRes.trace | Where-Object { $_.operation -eq "entry_found" }
Write-Host "Hash Table entry_found event:" ($hashEvent | ConvertTo-Json -Compress)

Write-Host "`n=== 25. Testing GET /api/v1/dsa/avl (DSA Lab AVL Tree Snapshot) ==="
$avlSnap = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/dsa/avl" -Method Get
Write-Host "AVL Tree Size:" $avlSnap.data.size "Height:" $avlSnap.data.height "Root Key:" $avlSnap.data.root.key
Write-Host "Root Balance Factor:" $avlSnap.data.root.balanceFactor

Write-Host "`n=== 26. Testing GET /api/v1/dsa/hash-table (DSA Lab Hash Table Statistics) ==="
$htStats = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/dsa/hash-table" -Method Get
Write-Host "Hash Table Size:" $htStats.data.size "Buckets:" $htStats.data.bucketCount "Load Factor:" $htStats.data.loadFactor
Write-Host "Occupied Buckets:" $htStats.data.occupiedBuckets "Collisions:" $htStats.data.collisionCount "Max Depth:" $htStats.data.maxBucketDepth

Write-Host "`n=== 27. Testing 404 for Missing Incident in Hash Table ==="
try {
    Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/incidents/INC-NONEXISTENT" -Method Get
    Write-Host "ERROR: Should have returned 404 for missing incident!"
} catch {
    Write-Host "Successfully returned 404 for missing incident ID in Hash Table with status:" $_.Exception.Response.StatusCode.value__
}

Write-Host "`n=== 28. Testing GET /api/v1/units (List default emergency fleet) ==="
$unitsList = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/units" -Method Get
Write-Host "Total Units registered:" $unitsList.data.total
Write-Host "First unit:" $unitsList.data.units[0].id "CallSign:" $unitsList.data.units[0].callSign "Status:" $unitsList.data.units[0].status

Write-Host "`n=== 29. Testing GET /api/v1/units/AMB-01 (Retrieve unit by ID) ==="
$unitAmb = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/units/AMB-01" -Method Get
Write-Host "Unit AMB-01 location:" $unitAmb.data.locationId "Capabilities:" ($unitAmb.data.capabilities -join ", ")

Write-Host "`n=== 30. Testing POST /api/v1/units (Register simulated unit AMB-99) ==="
$newUnitBody = @{
    id = "AMB-99"
    serviceType = "AMBULANCE"
    locationId = "J1"
    callSign = "Medic 99"
    capabilities = @("BLS", "ALS")
} | ConvertTo-Json
$newUnitRes = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/units" -Method Post -Body $newUnitBody -ContentType "application/json"
Write-Host "Registered unit:" $newUnitRes.data.id "Status:" $newUnitRes.data.status

Write-Host "`n=== 31. Testing PUT /api/v1/units/AMB-99 (Update unit location and status) ==="
$updateUnitBody = @{
    locationId = "J4"
    status = "AVAILABLE"
} | ConvertTo-Json
$updateUnitRes = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/units/AMB-99" -Method Put -Body $updateUnitBody -ContentType "application/json"
Write-Host "Updated unit AMB-99 new location:" $updateUnitRes.data.locationId

Write-Host "`n=== 32. Testing POST /api/v1/incidents (Create ALS Incident at J14) ==="
$alsIncBody = @{
    id = "INC-ALS-1"
    locationId = "J14"
    serviceType = "AMBULANCE"
    severity = 4
    description = "Critical cardiac event requiring ALS paramedic response"
    requiredCapabilities = @("ALS")
} | ConvertTo-Json
$alsIncRes = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/incidents" -Method Post -Body $alsIncBody -ContentType "application/json"
Write-Host "Created ALS incident:" $alsIncRes.data.id "Location:" $alsIncRes.data.locationId

Write-Host "`n=== 33. Testing POST /api/v1/dispatch (Dispatch unit for INC-ALS-1 with trace) ==="
$dispatchBody = @{
    incidentId = "INC-ALS-1"
    enableTrace = $true
} | ConvertTo-Json
$dispRes = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/dispatch" -Method Post -Body $dispatchBody -ContentType "application/json"
$activeDispatchId = $dispRes.data.dispatch.id
Write-Host "Created Dispatch ID:" $activeDispatchId "Assigned Unit:" $dispRes.data.dispatch.unitId
Write-Host "Explanation:" $dispRes.data.explanation
Write-Host "Travel Time (minutes):" $dispRes.data.dispatch.travelTimeMinutes
Write-Host "Path:" ($dispRes.data.dispatch.route.path -join " -> ")
Write-Host "Trace events count:" $dispRes.trace.Count

Write-Host "`n=== 34. Testing GET /api/v1/dispatches (List all dispatches) ==="
$dispList = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/dispatches" -Method Get
Write-Host "Total Dispatches:" $dispList.data.total
Write-Host "Active dispatch status:" $dispList.data.dispatches[0].status

Write-Host "`n=== 35. Testing GET /api/v1/dispatches/$activeDispatchId (Retrieve dispatch record) ==="
$dispRecord = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/dispatches/$activeDispatchId" -Method Get
Write-Host "Retrieved dispatch:" $dispRecord.data.id "Incident:" $dispRecord.data.incidentId "Unit:" $dispRecord.data.unitId

Write-Host "`n=== 36. Testing POST /api/v1/dispatches/$activeDispatchId/recalculate (Recalculate route) ==="
$recalcRes = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/dispatches/$activeDispatchId/recalculate" -Method Post -Body (@{ enableTrace = $true } | ConvertTo-Json) -ContentType "application/json"
Write-Host "Recalculation message:" $recalcRes.data.message
Write-Host "Travel time after recalculation:" $recalcRes.data.dispatch.travelTimeMinutes

Write-Host "`n=== 37. Testing POST /api/v1/dispatches/$activeDispatchId/complete (Complete response) ==="
$completeRes = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/dispatches/$activeDispatchId/complete" -Method Post -Body (@{ enableTrace = $true } | ConvertTo-Json) -ContentType "application/json"
Write-Host "Completion message:" $completeRes.data.message
Write-Host "Dispatch status:" $completeRes.data.dispatch.status

# Verify unit is released and back to AVAILABLE
$unitAfter = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/units/$($dispRes.data.dispatch.unitId)" -Method Get
Write-Host "Unit status after response completed:" $unitAfter.data.status "(Should be AVAILABLE)"

# Verify incident is marked RESOLVED
$incAfter = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/incidents/INC-ALS-1" -Method Get
Write-Host "Incident status after response completed:" $incAfter.data.status "(Should be RESOLVED)"

Write-Host "`n=== 38. Testing Double Completion Rejection (409 Conflict) ==="
try {
    Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/dispatches/$activeDispatchId/complete" -Method Post
    Write-Host "ERROR: Should have rejected duplicate completion!"
} catch {
    Write-Host "Successfully rejected duplicate completion with status:" $_.Exception.Response.StatusCode.value__
}

Write-Host "`n======================================================"
Write-Host " ALL 38 API INTEGRATION TESTS COMPLETED SUCCESSFULLY! "
Write-Host "======================================================"

