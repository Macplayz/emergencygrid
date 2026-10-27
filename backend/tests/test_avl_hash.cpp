#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <algorithm>
#include <random>

#include "core/types.hpp"
#include "core/trace.hpp"
#include "dsa/graph.hpp"
#include "dsa/dijkstra.hpp"
#include "dsa/binary_heap.hpp"
#include "dsa/avl_tree.hpp"
#include "dsa/hash_table.hpp"
#include "simulation/seed_data.hpp"
#include "simulation/incident_service.hpp"

using namespace emergencygrid;

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "  [FAIL] Line " << __LINE__ << ": " << (msg) << "\n"; \
            g_testsFailed++; \
            return; \
        } \
    } while (0)

#define RUN_TEST(fn) \
    do { \
        std::cout << "[RUNNING] " << #fn << " ...\n"; \
        int failedBefore = g_testsFailed; \
        fn(); \
        if (g_testsFailed == failedBefore) { \
            std::cout << "  [PASS] " << #fn << "\n"; \
            g_testsPassed++; \
        } \
    } while (0)

static Incident makeTestInc(
    const std::string& id,
    IncidentSeverity severity = IncidentSeverity::HIGH,
    const std::string& loc = "J1",
    IncidentType type = IncidentType::MEDICAL
) {
    Incident inc;
    inc.id = id;
    inc.severity = severity;
    inc.locationId = loc;
    inc.type = type;
    inc.status = IncidentStatus::PENDING;
    inc.computedPriority = static_cast<double>(severity) * 1000.0;
    return inc;
}

// =============================================================
// SECTION 1: AVL TREE TESTS (1 - 17)
// =============================================================

// 1. Inserting keys in ascending order (triggers repeated RR rotations)
void test_avl_insert_ascending_order() {
    AVLTree<int, int> tree;
    for (int i = 1; i <= 7; ++i) {
        bool ok = tree.insert(i, i * 10);
        TEST_ASSERT(ok, "Insert should succeed");
    }
    TEST_ASSERT(tree.size() == 7, "Tree size must be 7");
    TEST_ASSERT(tree.height() <= 4, "Height of balanced 7-node AVL tree must be <= 4 (log2(7) ~ 3)");
    
    // Inorder must be strictly ascending
    auto keys = tree.inorderKeys();
    std::vector<int> expected = {1, 2, 3, 4, 5, 6, 7};
    TEST_ASSERT(keys == expected, "Inorder traversal must match sorted expected array");
}

// 2. Inserting keys in descending order (triggers repeated LL rotations)
void test_avl_insert_descending_order() {
    AVLTree<int, int> tree;
    for (int i = 7; i >= 1; --i) {
        bool ok = tree.insert(i, i * 10);
        TEST_ASSERT(ok, "Insert should succeed");
    }
    TEST_ASSERT(tree.size() == 7, "Tree size must be 7");
    TEST_ASSERT(tree.height() <= 4, "Height must be <= 4");

    auto keys = tree.inorderKeys();
    std::vector<int> expected = {1, 2, 3, 4, 5, 6, 7};
    TEST_ASSERT(keys == expected, "Inorder traversal must match sorted expected array");
}

// 3. LL rotation (insert 30, 20, 10 -> Right rotation on 30)
void test_avl_ll_rotation() {
    AVLTree<int, std::string> tree;
    TraceCollector trace(true);

    tree.insert(30, "thirty", &trace);
    tree.insert(20, "twenty", &trace);
    trace.clear();

    tree.insert(10, "ten", &trace);

    // After inserting 10, balance factor of 30 becomes +2, left child 20 has bf +1 -> LL rotation
    TEST_ASSERT(tree.height() == 2, "Height should be 2 after LL rotation");
    TEST_ASSERT(tree.size() == 3, "Size must be 3");

    bool foundLL = false;
    for (const auto& ev : trace.getEvents()) {
        if (ev.operation == "rotate_ll") {
            foundLL = true;
            TEST_ASSERT(ev.entityId == "30", "Pivot must be 30");
            TEST_ASSERT(ev.to == "20", "Promoted root must be 20");
        }
    }
    TEST_ASSERT(foundLL, "Must record rotate_ll trace event");
}

// 4. RR rotation (insert 10, 20, 30 -> Left rotation on 10)
void test_avl_rr_rotation() {
    AVLTree<int, std::string> tree;
    TraceCollector trace(true);

    tree.insert(10, "ten", &trace);
    tree.insert(20, "twenty", &trace);
    trace.clear();

    tree.insert(30, "thirty", &trace);

    TEST_ASSERT(tree.height() == 2, "Height should be 2 after RR rotation");
    TEST_ASSERT(tree.size() == 3, "Size must be 3");

    bool foundRR = false;
    for (const auto& ev : trace.getEvents()) {
        if (ev.operation == "rotate_rr") {
            foundRR = true;
            TEST_ASSERT(ev.entityId == "10", "Pivot must be 10");
            TEST_ASSERT(ev.to == "20", "Promoted root must be 20");
        }
    }
    TEST_ASSERT(foundRR, "Must record rotate_rr trace event");
}

// 5. LR rotation (insert 30, 10, 20 -> Left on 10, Right on 30)
void test_avl_lr_rotation() {
    AVLTree<int, std::string> tree;
    TraceCollector trace(true);

    tree.insert(30, "thirty", &trace);
    tree.insert(10, "ten", &trace);
    trace.clear();

    tree.insert(20, "twenty", &trace);

    TEST_ASSERT(tree.height() == 2, "Height should be 2 after LR rotation");
    TEST_ASSERT(tree.size() == 3, "Size must be 3");

    bool foundLRStart = false;
    bool foundRR = false;
    bool foundLL = false;
    for (const auto& ev : trace.getEvents()) {
        if (ev.operation == "rotate_lr_start") foundLRStart = true;
        if (ev.operation == "rotate_rr") foundRR = true;
        if (ev.operation == "rotate_ll") foundLL = true;
    }
    TEST_ASSERT(foundLRStart, "Must log rotate_lr_start");
    TEST_ASSERT(foundRR && foundLL, "LR must perform RR child rotation followed by LL root rotation");
}

// 6. RL rotation (insert 10, 30, 20 -> Right on 30, Left on 10)
void test_avl_rl_rotation() {
    AVLTree<int, std::string> tree;
    TraceCollector trace(true);

    tree.insert(10, "ten", &trace);
    tree.insert(30, "thirty", &trace);
    trace.clear();

    tree.insert(20, "twenty", &trace);

    TEST_ASSERT(tree.height() == 2, "Height should be 2 after RL rotation");
    TEST_ASSERT(tree.size() == 3, "Size must be 3");

    bool foundRLStart = false;
    bool foundLL = false;
    bool foundRR = false;
    for (const auto& ev : trace.getEvents()) {
        if (ev.operation == "rotate_rl_start") foundRLStart = true;
        if (ev.operation == "rotate_ll") foundLL = true;
        if (ev.operation == "rotate_rr") foundRR = true;
    }
    TEST_ASSERT(foundRLStart, "Must log rotate_rl_start");
    TEST_ASSERT(foundLL && foundRR, "RL must perform LL child rotation followed by RR root rotation");
}

// 7. Search for existing and missing keys
void test_avl_search() {
    AVLTree<std::string, int> tree;
    tree.insert("INC-10", 100);
    tree.insert("INC-20", 200);
    tree.insert("INC-05", 50);

    TEST_ASSERT(tree.contains("INC-20"), "Must contain INC-20");
    int* val = tree.search("INC-20");
    TEST_ASSERT(val != nullptr && *val == 200, "Search INC-20 must return 200");

    TEST_ASSERT(!tree.contains("INC-99"), "Must not contain missing key INC-99");
    int* missing = tree.search("INC-99");
    TEST_ASSERT(missing == nullptr, "Search for missing key must return nullptr");
}

// 8. Deleting a leaf
void test_avl_delete_leaf() {
    AVLTree<int, int> tree;
    tree.insert(20, 200);
    tree.insert(10, 100);
    tree.insert(30, 300);

    bool del = tree.remove(10);
    TEST_ASSERT(del, "Leaf deletion must succeed");
    TEST_ASSERT(tree.size() == 2, "Size must be 2");
    TEST_ASSERT(!tree.contains(10), "10 must not be present");
    TEST_ASSERT(tree.contains(20) && tree.contains(30), "Other nodes must remain intact");
}

// 9. Deleting a node with one child
void test_avl_delete_one_child() {
    AVLTree<int, int> tree;
    tree.insert(20, 200);
    tree.insert(10, 100);
    tree.insert(30, 300);
    tree.insert(5, 50); // 10 now has one left child (5)

    bool del = tree.remove(10);
    TEST_ASSERT(del, "Deletion of single-child node must succeed");
    TEST_ASSERT(tree.size() == 3, "Size must be 3");
    TEST_ASSERT(!tree.contains(10), "10 deleted");
    TEST_ASSERT(tree.contains(5), "Child 5 must be preserved");

    auto keys = tree.inorderKeys();
    std::vector<int> expected = {5, 20, 30};
    TEST_ASSERT(keys == expected, "Inorder must match expected");
}

// 10. Deleting a node with two children
void test_avl_delete_two_children() {
    AVLTree<int, int> tree;
    tree.insert(20, 200);
    tree.insert(10, 100);
    tree.insert(30, 300);
    tree.insert(25, 250);
    tree.insert(40, 400);

    // 30 has two children: 25 and 40. Successor is 25.
    bool del = tree.remove(30);
    TEST_ASSERT(del, "Deletion of two-child node must succeed");
    TEST_ASSERT(tree.size() == 4, "Size must be 4");
    TEST_ASSERT(!tree.contains(30), "30 must be deleted");
    TEST_ASSERT(tree.contains(25) && tree.contains(40), "Both children must be preserved");

    auto keys = tree.inorderKeys();
    std::vector<int> expected = {10, 20, 25, 40};
    TEST_ASSERT(keys == expected, "Inorder must remain sorted after two-child deletion");
}

// 11. Deleting the root
void test_avl_delete_root() {
    AVLTree<int, int> tree;
    tree.insert(50, 500);
    tree.insert(30, 300);
    tree.insert(70, 700);

    bool del = tree.remove(50);
    TEST_ASSERT(del, "Root deletion must succeed");
    TEST_ASSERT(tree.size() == 2, "Size must be 2");
    TEST_ASSERT(!tree.contains(50), "50 must be gone");

    auto keys = tree.inorderKeys();
    std::vector<int> expected = {30, 70};
    TEST_ASSERT(keys == expected, "Remaining elements must be 30, 70");
}

// 12. Rebalancing after deletion
void test_avl_rebalance_after_deletion() {
    AVLTree<int, int> tree;
    // Build tree where deleting a node triggers a rotation
    tree.insert(20, 20);
    tree.insert(10, 10);
    tree.insert(30, 30);
    tree.insert(40, 40);

    // Delete 10 -> right subtree becomes heavier -> triggers rebalance
    tree.remove(10);
    TEST_ASSERT(tree.size() == 3, "Size must be 3");
    TEST_ASSERT(tree.height() == 2, "Tree must be rebalanced to height 2");

    auto keys = tree.inorderKeys();
    std::vector<int> expected = {20, 30, 40};
    TEST_ASSERT(keys == expected, "Sorted keys preserved");
}

// 13. Duplicate-key behavior
void test_avl_duplicate_key_rejected() {
    AVLTree<std::string, int> tree;
    bool ok1 = tree.insert("INC-1", 100);
    TEST_ASSERT(ok1, "First insert must succeed");

    bool ok2 = tree.insert("INC-1", 999);
    TEST_ASSERT(!ok2, "Duplicate insert must be rejected");
    TEST_ASSERT(tree.size() == 1, "Size must remain 1");

    int* val = tree.search("INC-1");
    TEST_ASSERT(val != nullptr && *val == 100, "Original value must not be corrupted");
}

// 14. Sorted inorder traversal
void test_avl_sorted_inorder_traversal() {
    AVLTree<std::string, int> tree;
    std::vector<std::string> ids = {"INC-05", "INC-01", "INC-09", "INC-03", "INC-07", "INC-02", "INC-08"};
    for (const auto& id : ids) {
        tree.insert(id, 1);
    }

    auto keys = tree.inorderKeys();
    std::vector<std::string> sortedIds = ids;
    std::sort(sortedIds.begin(), sortedIds.end());
    TEST_ASSERT(keys == sortedIds, "Inorder traversal keys must match standard sort order");
}

// 15. Correct node heights and balance factors
void test_avl_node_heights_and_balance_factors() {
    AVLTree<int, int> tree;
    tree.insert(20, 20);
    tree.insert(10, 10);
    tree.insert(30, 30);

    TEST_ASSERT(tree.height() == 2, "Tree height must be 2");
    auto snap = tree.toJsonSnapshot();
    TEST_ASSERT(snap["size"] == 3, "Snapshot size must be 3");
    TEST_ASSERT(snap["height"] == 2, "Snapshot height must be 2");
    TEST_ASSERT(snap["root"]["key"] == "20", "Root must be 20");
    TEST_ASSERT(snap["root"]["balanceFactor"] == 0, "Root balance factor must be 0");
}

// 16. Tree remains balanced after many operations
void test_avl_remains_balanced_large_random_ops() {
    AVLTree<int, int> tree;
    const int count = 100;
    std::vector<int> inserted;

    for (int i = 0; i < count; ++i) {
        int k = (i * 37) % 250;
        if (tree.insert(k, i)) {
            inserted.push_back(k);
        }
    }

    // AVL height upper bound: 1.44 * log2(N + 2)
    int maxAllowedHeight = 12;
    TEST_ASSERT(tree.height() <= maxAllowedHeight, "AVL tree height must respect logarithmic balance bound");

    // Delete half
    for (size_t i = 0; i < inserted.size() / 2; ++i) {
        tree.remove(inserted[i]);
    }
    TEST_ASSERT(tree.height() <= maxAllowedHeight, "Tree must remain balanced after deletions");

    // Inorder traversal must still be strictly sorted
    auto keys = tree.inorderKeys();
    for (size_t i = 1; i < keys.size(); ++i) {
        TEST_ASSERT(keys[i - 1] < keys[i], "Keys must remain strictly sorted");
    }
}

// 17. Trace events accurately reflect rotations and operations
void test_avl_trace_events_fidelity() {
    AVLTree<int, int> tree;
    TraceCollector trace(true);

    tree.insert(10, 100, &trace);
    tree.insert(20, 200, &trace);
    tree.insert(30, 300, &trace); // Triggers RR rotation

    const auto& events = trace.getEvents();
    bool foundInsert = false;
    bool foundCompare = false;
    bool foundRotate = false;
    bool foundHeightUpdate = false;

    for (const auto& ev : events) {
        if (ev.operation == "insert") foundInsert = true;
        if (ev.operation == "compare") foundCompare = true;
        if (ev.operation == "rotate_rr") foundRotate = true;
        if (ev.operation == "height_update") foundHeightUpdate = true;
    }

    TEST_ASSERT(foundInsert, "Trace must log insert");
    TEST_ASSERT(foundCompare, "Trace must log compare");
    TEST_ASSERT(foundRotate, "Trace must log rotate_rr");
    TEST_ASSERT(foundHeightUpdate, "Trace must log height_update");
}

// =============================================================
// SECTION 2: HASH TABLE TESTS (18 - 27)
// =============================================================

// 18. Insert and retrieve an entry
void test_hash_table_insert_retrieve() {
    HashTable<std::string, Incident> table;
    Incident inc = makeTestInc("INC-100", IncidentSeverity::CRITICAL);

    bool ok = table.insert("INC-100", inc);
    TEST_ASSERT(ok, "Insert must succeed");
    TEST_ASSERT(table.size() == 1, "Size must be 1");
    TEST_ASSERT(table.contains("INC-100"), "Table must contain key");

    Incident* found = table.search("INC-100");
    TEST_ASSERT(found != nullptr, "Search must return pointer");
    TEST_ASSERT(found->severity == IncidentSeverity::CRITICAL, "Severity must match");
}

// 19. Search for a missing key
void test_hash_table_search_missing() {
    HashTable<std::string, int> table;
    table.insert("EXISTING", 42);

    TEST_ASSERT(!table.contains("NON_EXISTENT"), "Must not contain missing key");
    int* missing = table.search("NON_EXISTENT");
    TEST_ASSERT(missing == nullptr, "Search for missing key must return nullptr");
}

// 20. Update an existing entry
void test_hash_table_update_entry() {
    HashTable<std::string, int> table;
    table.insert("KEY-1", 100);

    bool up = table.update("KEY-1", 999);
    TEST_ASSERT(up, "Update must succeed for existing key");
    TEST_ASSERT(*table.search("KEY-1") == 999, "Value must be updated to 999");
    TEST_ASSERT(table.size() == 1, "Size must remain 1");

    bool badUp = table.update("MISSING", 50);
    TEST_ASSERT(!badUp, "Update for non-existent key must return false");
}

// 21. Delete an entry
void test_hash_table_delete_entry() {
    HashTable<std::string, int> table;
    table.insert("K1", 10);
    table.insert("K2", 20);

    bool del = table.remove("K1");
    TEST_ASSERT(del, "Delete must return true");
    TEST_ASSERT(table.size() == 1, "Size must decrease to 1");
    TEST_ASSERT(!table.contains("K1"), "K1 must no longer exist");
    TEST_ASSERT(table.contains("K2"), "K2 must remain");

    bool delAgain = table.remove("K1");
    TEST_ASSERT(!delAgain, "Second delete of same key must return false");
}

// 22. Collision handling (force entries into same bucket by using fixed capacity)
void test_hash_table_collision_handling() {
    // 4 buckets to guarantee collisions
    HashTable<std::string, int> table(4, 5.0); // high load factor threshold so it won't rehash

    table.insert("alpha", 1);
    table.insert("beta", 2);
    table.insert("gamma", 3);
    table.insert("delta", 4);
    table.insert("epsilon", 5);

    TEST_ASSERT(table.size() == 5, "All 5 items must be inserted");
    TEST_ASSERT(table.collisionCount() > 0, "Must have collisions with 5 items in 4 buckets");

    TEST_ASSERT(*table.search("alpha") == 1, "alpha found");
    TEST_ASSERT(*table.search("beta") == 2, "beta found");
    TEST_ASSERT(*table.search("gamma") == 3, "gamma found");
    TEST_ASSERT(*table.search("delta") == 4, "delta found");
    TEST_ASSERT(*table.search("epsilon") == 5, "epsilon found");
}

// 23. Multiple entries in the same bucket
void test_hash_table_multiple_entries_same_bucket() {
    HashTable<std::string, int> table(8, 10.0);

    // Find keys that hash to the same bucket
    std::string keyA = "testA";
    uint64_t hashA = HashTable<std::string, int>::hashKey(keyA);
    size_t targetBucket = hashA % 8;

    std::vector<std::string> collidingKeys = {keyA};
    for (int i = 0; i < 500 && collidingKeys.size() < 3; ++i) {
        std::string cand = "candidate_" + std::to_string(i);
        if (HashTable<std::string, int>::hashKey(cand) % 8 == targetBucket) {
            collidingKeys.push_back(cand);
        }
    }

    TEST_ASSERT(collidingKeys.size() >= 3, "Must find 3 colliding keys");

    for (size_t i = 0; i < collidingKeys.size(); ++i) {
        table.insert(collidingKeys[i], static_cast<int>(i + 1));
    }

    TEST_ASSERT(table.maxBucketDepth() >= 3, "Bucket depth must be at least 3");

    for (size_t i = 0; i < collidingKeys.size(); ++i) {
        int* v = table.search(collidingKeys[i]);
        TEST_ASSERT(v != nullptr && *v == static_cast<int>(i + 1), "Every colliding key must be retrievable");
    }
}

// 24. Deleting one colliding entry preserves the others
void test_hash_table_delete_colliding_entry_preserves_others() {
    HashTable<std::string, int> table(8, 10.0);

    std::string k1 = "item1";
    size_t b1 = HashTable<std::string, int>::hashKey(k1) % 8;

    std::string k2;
    for (int i = 0; i < 500; ++i) {
        std::string cand = "coll_" + std::to_string(i);
        if (HashTable<std::string, int>::hashKey(cand) % 8 == b1) {
            k2 = cand;
            break;
        }
    }
    TEST_ASSERT(!k2.empty(), "Must find colliding key k2");

    table.insert(k1, 100);
    table.insert(k2, 200);
    TEST_ASSERT(table.size() == 2, "Size is 2");

    // Remove k1
    bool del = table.remove(k1);
    TEST_ASSERT(del, "Delete k1 succeeds");
    TEST_ASSERT(!table.contains(k1), "k1 is gone");
    TEST_ASSERT(table.contains(k2), "k2 is preserved in bucket");
    TEST_ASSERT(*table.search(k2) == 200, "k2 value remains 200");
}

// 25. Correct entry count after operations
void test_hash_table_entry_count_consistency() {
    HashTable<int, int> table;
    TEST_ASSERT(table.size() == 0, "Initial size 0");

    for (int i = 0; i < 20; ++i) {
        table.insert(i, i * 10);
    }
    TEST_ASSERT(table.size() == 20, "Size is 20");

    for (int i = 0; i < 5; ++i) {
        table.remove(i);
    }
    TEST_ASSERT(table.size() == 15, "Size is 15 after removing 5");

    table.clear();
    TEST_ASSERT(table.size() == 0, "Size is 0 after clear");
}

// 26. Rehashing when load factor is exceeded
void test_hash_table_automatic_rehashing() {
    // Initial bucket count 4, max load factor 0.75 -> threshold is 4 * 0.75 = 3
    HashTable<std::string, int> table(4, 0.75);
    TEST_ASSERT(table.bucketCount() == 4, "Initial bucket count 4");

    table.insert("A", 1);
    table.insert("B", 2);
    table.insert("C", 3);
    TEST_ASSERT(table.bucketCount() == 4, "Still 4 buckets at threshold");

    // 4th insert will exceed 3 -> triggers rehash to 8 buckets
    table.insert("D", 4);
    TEST_ASSERT(table.bucketCount() >= 8, "Bucket count must double after threshold exceeded");
    TEST_ASSERT(table.size() == 4, "Size must be 4");

    // All elements must still be retrievable
    TEST_ASSERT(*table.search("A") == 1, "A retrievable after rehash");
    TEST_ASSERT(*table.search("B") == 2, "B retrievable after rehash");
    TEST_ASSERT(*table.search("C") == 3, "C retrievable after rehash");
    TEST_ASSERT(*table.search("D") == 4, "D retrievable after rehash");
}

// 27. Trace events reflect actual bucket traversal and collisions
void test_hash_table_trace_events() {
    HashTable<std::string, int> table(4, 10.0);
    TraceCollector trace(true);

    table.insert("KEY-1", 10, &trace);
    table.search("KEY-1", &trace);

    const auto& events = trace.getEvents();
    bool foundHashKey = false;
    bool foundEntryInserted = false;
    bool foundEntryFound = false;

    for (const auto& ev : events) {
        if (ev.operation == "hash_key") foundHashKey = true;
        if (ev.operation == "entry_inserted") foundEntryInserted = true;
        if (ev.operation == "entry_found") foundEntryFound = true;
    }

    TEST_ASSERT(foundHashKey, "Trace must log hash_key");
    TEST_ASSERT(foundEntryInserted, "Trace must log entry_inserted");
    TEST_ASSERT(foundEntryFound, "Trace must log entry_found");
}

// =============================================================
// SECTION 3: INTEGRATION TESTS (28 - 36)
// =============================================================

// 28. Creating an incident updates both indexes (HashTable and AVLTree)
void test_integration_create_updates_both_indexes() {
    RoadGraph graph = createMetroGrid16();
    IncidentService service(&graph);

    CreateIncidentRequest req{
        .id = "INC-801",
        .locationId = "J1",
        .type = "FIRE",
        .severity = 3,
        .description = "Transformer fire"
    };

    auto res = service.createIncident(req);
    TEST_ASSERT(res.success, "Incident creation must succeed");

    // 1. Check custom Hash Table
    const auto& table = service.getHashTable();
    TEST_ASSERT(table.contains("INC-801"), "HashTable must contain INC-801");
    const Incident* inTable = table.search("INC-801");
    TEST_ASSERT(inTable != nullptr && inTable->id == "INC-801", "HashTable data matches");

    // 2. Check custom AVL Tree
    const auto& avl = service.getAvlTree();
    TEST_ASSERT(avl.contains("INC-801"), "AVLTree must contain INC-801");
    const Incident* inAvl = avl.search("INC-801");
    TEST_ASSERT(inAvl != nullptr && inAvl->id == "INC-801", "AVLTree data matches");

    // 3. Check Heap
    TEST_ASSERT(service.heapSize() == 1, "Pending heap size must be 1");
}

// 29. ID lookup returns the correct incident
void test_integration_id_lookup_uses_hash_table() {
    RoadGraph graph = createMetroGrid16();
    IncidentService service(&graph);

    service.createIncident({.id = "INC-AAA", .locationId = "J2", .type = "MEDICAL", .severity = 4, .description = "Cardiac arrest"});
    service.createIncident({.id = "INC-BBB", .locationId = "J3", .type = "POLICE", .severity = 2, .description = "Traffic dispute"});

    TraceCollector trace(true);
    auto found = service.getIncident("INC-AAA", &trace);
    TEST_ASSERT(found.has_value(), "Must find INC-AAA");
    TEST_ASSERT(found->description == "Cardiac arrest", "Description must match");

    // Verify lookup generated HashTable trace
    bool foundHashTrace = false;
    for (const auto& ev : trace.getEvents()) {
        if (ev.algorithm == "HashTable" && ev.operation == "entry_found") {
            foundHashTrace = true;
        }
    }
    TEST_ASSERT(foundHashTrace, "Must log HashTable entry_found event during ID lookup");
}

// 30. Sorted listing matches AVL inorder order
void test_integration_sorted_listing_matches_avl_inorder() {
    RoadGraph graph = createMetroGrid16();
    IncidentService service(&graph);

    // Insert out of lexical order
    service.createIncident({.id = "INC-050", .locationId = "J1", .type = "FIRE", .severity = 3});
    service.createIncident({.id = "INC-010", .locationId = "J2", .type = "MEDICAL", .severity = 2});
    service.createIncident({.id = "INC-090", .locationId = "J3", .type = "POLICE", .severity = 4});
    service.createIncident({.id = "INC-030", .locationId = "J4", .type = "FIRE", .severity = 1});

    auto sorted = service.listIncidentsSorted();
    TEST_ASSERT(sorted.size() == 4, "Must list all 4 incidents");

    std::vector<std::string> sortedIds;
    for (const auto& inc : sorted) {
        sortedIds.push_back(inc.id);
    }

    std::vector<std::string> expected = {"INC-010", "INC-030", "INC-050", "INC-090"};
    TEST_ASSERT(sortedIds == expected, "Incidents must be strictly ordered by ID via AVL inorder traversal");
}

// 31. Incident lifecycle changes remain consistent across indexes
void test_integration_lifecycle_consistency_across_indexes() {
    RoadGraph graph = createMetroGrid16();
    IncidentService service(&graph);

    service.createIncident({.id = "INC-LIFE", .locationId = "J4", .type = "MEDICAL", .severity = 4});

    // Initial state: PENDING in HashTable and AVLTree
    TEST_ASSERT(service.getIncident("INC-LIFE")->status == IncidentStatus::PENDING, "Initial status PENDING in HashTable");
    TEST_ASSERT(service.getAvlTree().search("INC-LIFE")->status == IncidentStatus::PENDING, "Initial status PENDING in AVLTree");

    // Select for dispatch -> marks ASSIGNED
    auto sel = service.selectNext();
    TEST_ASSERT(sel.success, "Select next succeeds");
    TEST_ASSERT(sel.incident->status == IncidentStatus::ASSIGNED, "Selected status is ASSIGNED");

    // Check consistency in HashTable
    auto inTable = service.getIncident("INC-LIFE");
    TEST_ASSERT(inTable.has_value() && inTable->status == IncidentStatus::ASSIGNED, "Status updated to ASSIGNED in HashTable");

    // Check consistency in AVLTree
    const Incident* inAvl = service.getAvlTree().search("INC-LIFE");
    TEST_ASSERT(inAvl != nullptr && inAvl->status == IncidentStatus::ASSIGNED, "Status updated to ASSIGNED in AVLTree");

    // Resolve incident -> marks RESOLVED
    auto res = service.resolveIncident("INC-LIFE");
    TEST_ASSERT(res.success, "Resolve succeeds");
    TEST_ASSERT(service.getIncident("INC-LIFE")->status == IncidentStatus::RESOLVED, "Status RESOLVED in HashTable");
    TEST_ASSERT(service.getAvlTree().search("INC-LIFE")->status == IncidentStatus::RESOLVED, "Status RESOLVED in AVLTree");
}

// 32. Cancellation does not permit later dispatch selection
void test_integration_cancellation_lazy_heap_purge() {
    RoadGraph graph = createMetroGrid16();
    IncidentService service(&graph);

    service.createIncident({.id = "INC-TOP", .locationId = "J1", .type = "FIRE", .severity = 4});
    service.createIncident({.id = "INC-NEXT", .locationId = "J2", .type = "MEDICAL", .severity = 3});

    // Cancel INC-TOP
    auto canRes = service.cancelIncident("INC-TOP");
    TEST_ASSERT(canRes.success, "Cancellation succeeds");
    TEST_ASSERT(service.getIncident("INC-TOP")->status == IncidentStatus::CANCELLED, "Status CANCELLED in HashTable");
    TEST_ASSERT(service.getAvlTree().search("INC-TOP")->status == IncidentStatus::CANCELLED, "Status CANCELLED in AVLTree");

    // selectNext must skip cancelled INC-TOP and return INC-NEXT
    auto sel = service.selectNext();
    TEST_ASSERT(sel.success, "Select next succeeds");
    TEST_ASSERT(sel.incident->id == "INC-NEXT", "INC-NEXT selected, skipping cancelled INC-TOP");

    // No pending left
    auto selEmpty = service.selectNext();
    TEST_ASSERT(!selEmpty.success && selEmpty.errorCode == "EMPTY_QUEUE", "Queue is now empty");
}

// 33. Duplicate incident IDs do not corrupt either index
void test_integration_duplicate_id_preserves_indexes() {
    RoadGraph graph = createMetroGrid16();
    IncidentService service(&graph);

    service.createIncident({.id = "INC-ORIG", .locationId = "J1", .type = "FIRE", .severity = 3, .description = "Original"});

    // Attempt duplicate creation
    auto dupRes = service.createIncident({.id = "INC-ORIG", .locationId = "J2", .type = "POLICE", .severity = 4, .description = "Duplicate attempt"});
    TEST_ASSERT(!dupRes.success, "Duplicate must be rejected");
    TEST_ASSERT(dupRes.errorCode == "DUPLICATE_INCIDENT_ID", "Error code must be DUPLICATE_INCIDENT_ID");

    // Both indexes must retain original data
    TEST_ASSERT(service.totalCount() == 1, "Total count must still be 1");
    auto fromTable = service.getIncident("INC-ORIG");
    TEST_ASSERT(fromTable->description == "Original" && fromTable->severity == IncidentSeverity::HIGH, "HashTable untouched");

    const auto* fromAvl = service.getAvlTree().search("INC-ORIG");
    TEST_ASSERT(fromAvl != nullptr && fromAvl->description == "Original", "AVLTree untouched");
}

// 34. Existing incident-queue behavior still works
void test_integration_heap_priority_ordering() {
    RoadGraph graph = createMetroGrid16();
    IncidentService service(&graph);

    service.createIncident({.id = "INC-LOW", .locationId = "J1", .type = "MEDICAL", .severity = 1});
    service.createIncident({.id = "INC-CRIT", .locationId = "J2", .type = "FIRE", .severity = 4});
    service.createIncident({.id = "INC-MED", .locationId = "J3", .type = "POLICE", .severity = 2});

    auto peek = service.peekNext();
    TEST_ASSERT(peek.has_value() && peek->id == "INC-CRIT", "Root must be CRITICAL");

    auto sel1 = service.selectNext();
    TEST_ASSERT(sel1.incident->id == "INC-CRIT", "INC-CRIT selected first");

    auto sel2 = service.selectNext();
    TEST_ASSERT(sel2.incident->id == "INC-MED", "INC-MED selected second");

    auto sel3 = service.selectNext();
    TEST_ASSERT(sel3.incident->id == "INC-LOW", "INC-LOW selected third");
}

// 35. Existing graph and routing tests still pass
void test_integration_graph_routing_remains_intact() {
    RoadGraph graph = createMetroGrid16();
    RouteResult res = DijkstraRouter::findShortestPath(graph, "J1", "J5");
    TEST_ASSERT(res.routeFound, "Shortest path J1 -> J5 must be found");
    TEST_ASSERT(res.totalTravelTimeMinutes == 6.0, "Expected travel time 6.0 min");
}

// 36. API responses correctly expose sorted incidents and structure snapshots
void test_integration_snapshots_exposed() {
    RoadGraph graph = createMetroGrid16();
    IncidentService service(&graph);

    service.createIncident({.id = "INC-10", .locationId = "J1", .type = "FIRE", .severity = 3});
    service.createIncident({.id = "INC-20", .locationId = "J2", .type = "MEDICAL", .severity = 4});

    // 1. AVL Snapshot
    nlohmann::json avlSnap = service.getAvlSnapshot();
    TEST_ASSERT(avlSnap.contains("root"), "AVL snapshot contains root");
    TEST_ASSERT(avlSnap["size"] == 2, "AVL snapshot size is 2");
    TEST_ASSERT(avlSnap["height"] >= 1, "AVL height >= 1");

    // 2. Hash Table Snapshot
    nlohmann::json htSnap = service.getHashTableSnapshot();
    TEST_ASSERT(htSnap["size"] == 2, "HashTable snapshot size is 2");
    TEST_ASSERT(htSnap.contains("buckets"), "HashTable contains buckets array");
    TEST_ASSERT(htSnap.contains("loadFactor"), "HashTable contains loadFactor");

    // 3. Heap Snapshot
    nlohmann::json heapSnap = service.getHeapSnapshot();
    TEST_ASSERT(heapSnap["size"] == 2, "Heap snapshot size is 2");
}

// =============================================================
// MAIN TEST RUNNER
// =============================================================

int main() {
    std::cout << "======================================================\n";
    std::cout << " EmergencyGrid: AVL Tree & Hash Table Test Suite      \n";
    std::cout << " Advanced Data Structures & Algorithms Capstone Project\n";
    std::cout << "======================================================\n\n";

    std::cout << "--- Section A: AVL Tree Correctness & Balancing Tests ---\n";
    RUN_TEST(test_avl_insert_ascending_order);
    RUN_TEST(test_avl_insert_descending_order);
    RUN_TEST(test_avl_ll_rotation);
    RUN_TEST(test_avl_rr_rotation);
    RUN_TEST(test_avl_lr_rotation);
    RUN_TEST(test_avl_rl_rotation);
    RUN_TEST(test_avl_search);
    RUN_TEST(test_avl_delete_leaf);
    RUN_TEST(test_avl_delete_one_child);
    RUN_TEST(test_avl_delete_two_children);
    RUN_TEST(test_avl_delete_root);
    RUN_TEST(test_avl_rebalance_after_deletion);
    RUN_TEST(test_avl_duplicate_key_rejected);
    RUN_TEST(test_avl_sorted_inorder_traversal);
    RUN_TEST(test_avl_node_heights_and_balance_factors);
    RUN_TEST(test_avl_remains_balanced_large_random_ops);
    RUN_TEST(test_avl_trace_events_fidelity);

    std::cout << "\n--- Section B: Custom Hash Table Tests ---\n";
    RUN_TEST(test_hash_table_insert_retrieve);
    RUN_TEST(test_hash_table_search_missing);
    RUN_TEST(test_hash_table_update_entry);
    RUN_TEST(test_hash_table_delete_entry);
    RUN_TEST(test_hash_table_collision_handling);
    RUN_TEST(test_hash_table_multiple_entries_same_bucket);
    RUN_TEST(test_hash_table_delete_colliding_entry_preserves_others);
    RUN_TEST(test_hash_table_entry_count_consistency);
    RUN_TEST(test_hash_table_automatic_rehashing);
    RUN_TEST(test_hash_table_trace_events);

    std::cout << "\n--- Section C: Incident Service Integration Tests ---\n";
    RUN_TEST(test_integration_create_updates_both_indexes);
    RUN_TEST(test_integration_id_lookup_uses_hash_table);
    RUN_TEST(test_integration_sorted_listing_matches_avl_inorder);
    RUN_TEST(test_integration_lifecycle_consistency_across_indexes);
    RUN_TEST(test_integration_cancellation_lazy_heap_purge);
    RUN_TEST(test_integration_duplicate_id_preserves_indexes);
    RUN_TEST(test_integration_heap_priority_ordering);
    RUN_TEST(test_integration_graph_routing_remains_intact);
    RUN_TEST(test_integration_snapshots_exposed);

    std::cout << "\n======================================================\n";
    std::cout << " Test Summary: " << g_testsPassed << " passed, " << g_testsFailed << " failed.\n";
    std::cout << "======================================================\n";

    return (g_testsFailed == 0) ? 0 : 1;
}
