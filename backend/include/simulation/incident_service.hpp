#pragma once

#include <string>
#include <vector>
#include <optional>
#include <mutex>
#include <memory>
#include "core/types.hpp"
#include "core/trace.hpp"
#include "dsa/graph.hpp"
#include "dsa/binary_heap.hpp"
#include "dsa/avl_tree.hpp"
#include "dsa/hash_table.hpp"

namespace emergencygrid {

struct CreateIncidentRequest {
    std::string id; // Optional: auto-generated if empty
    std::string locationId;
    std::string type; // "MEDICAL", "FIRE", "POLICE"
    int severity{2};  // 1: Low, 2: Medium, 3: High, 4: Critical
    std::string description;
    std::vector<std::string> requiredCapabilities;
    bool enableTrace{false};
};

struct IncidentServiceResult {
    bool success{false};
    std::string errorCode;
    std::string errorMessage;
    std::optional<Incident> incident;
};

class IncidentService {
public:
    explicit IncidentService(const RoadGraph* graph = nullptr);

    void setGraph(const RoadGraph* graph) noexcept { graph_ = graph; }

    // 1. Create incident (inserts into HashTable, AVLTree, and BinaryHeap)
    IncidentServiceResult createIncident(
        const CreateIncidentRequest& req,
        TraceCollector* trace = nullptr
    );

    // 2. List incidents in arrival order (optionally filtered by status)
    std::vector<Incident> listIncidents(
        std::optional<IncidentStatus> filterStatus = std::nullopt
    ) const;

    // 3. List incidents in sorted ID order using custom AVL Tree inorder traversal
    std::vector<Incident> listIncidentsSorted(
        std::optional<IncidentStatus> filterStatus = std::nullopt,
        TraceCollector* trace = nullptr
    ) const;

    // 4. Inspect the next eligible pending incident without extracting
    std::optional<Incident> peekNext(TraceCollector* trace = nullptr);

    // 5. Select the next eligible pending incident and mark it ASSIGNED across all indexes
    IncidentServiceResult selectNext(TraceCollector* trace = nullptr);

    // 6. Cancel an incident (updates HashTable and AVLTree; lazily invalidates Heap)
    IncidentServiceResult cancelIncident(
        const std::string& incidentId,
        TraceCollector* trace = nullptr
    );

    // 7. Retrieve incident by ID using custom Hash Table
    std::optional<Incident> getIncident(
        const std::string& incidentId,
        TraceCollector* trace = nullptr
    ) const;

    // 8. Resolve an incident (lifecycle management across all indexes)
    IncidentServiceResult resolveIncident(
        const std::string& incidentId,
        TraceCollector* trace = nullptr
    );

    // 9. Update an incident's lifecycle status across all indexes
    IncidentServiceResult updateIncidentStatus(
        const std::string& incidentId,
        IncidentStatus newStatus,
        TraceCollector* trace = nullptr
    );

    // Counts & metrics
    size_t totalCount() const;
    size_t pendingCount() const;
    size_t assignedCount() const;
    size_t resolvedCount() const;
    size_t cancelledCount() const;
    size_t heapSize() const;

    // Direct data structure inspector snapshots for DSA Lab
    nlohmann::json getHeapSnapshot() const;
    nlohmann::json getAvlSnapshot() const;
    nlohmann::json getHashTableSnapshot() const;

    const IncidentHashTable& getHashTable() const noexcept { return table_; }
    const IncidentAVLTree& getAvlTree() const noexcept { return avlTree_; }
    const BinaryHeap& getHeap() const noexcept { return pendingHeap_; }

    // Reset service state
    void reset();

private:
    // Helper to discard stale entries from the top of the heap
    void purgeStaleTop(TraceCollector* trace);

    const RoadGraph* graph_{nullptr};
    mutable std::mutex mutex_;

    // Custom Hash Table for O(1) ID lookups
    IncidentHashTable table_;

    // Custom AVL Tree for O(log N) ordered traversal & balanced indexing
    IncidentAVLTree avlTree_;

    // Custom Binary Max-Heap for pending incident prioritization
    BinaryHeap pendingHeap_;

    // Insertion order tracking for deterministic chronological listing
    std::vector<std::string> incidentOrder_;

    uint64_t arrivalCounter_{0};
};

} // namespace emergencygrid
