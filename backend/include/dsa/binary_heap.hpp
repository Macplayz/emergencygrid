#pragma once

#include <vector>
#include <optional>
#include <string>
#include <cstddef>
#include <functional>
#include "core/types.hpp"
#include "core/trace.hpp"
#include "json.hpp"

namespace emergencygrid {

/**
 * @brief Custom array-backed Binary Max-Heap for prioritizing emergency incidents.
 *
 * Implements an educational, genuine binary max-heap in C++20 without relying on
 * std::priority_queue.
 *
 * Priority Ordering Rule:
 * 1. Higher severity comes first (Critical: 4 > High: 3 > Medium: 2 > Low: 1).
 * 2. If severity is equal, earlier arrivalSequence comes first (FIFO among equal severity).
 * 3. If both severity and arrivalSequence are equal, lexicographical incident ID is the deterministic tie-breaker.
 *
 * Array Index Calculations (0-based):
 * - Parent:       (i - 1) / 2
 * - Left Child:   2 * i + 1
 * - Right Child:  2 * i + 2
 */
class BinaryHeap {
public:
    BinaryHeap() = default;
    ~BinaryHeap() = default;

    // Core heap operations
    void insert(const Incident& incident, TraceCollector* trace = nullptr);
    std::optional<Incident> peek(TraceCollector* trace = nullptr) const;
    std::optional<Incident> extractMax(TraceCollector* trace = nullptr);

    void heapifyUp(size_t index, TraceCollector* trace = nullptr);
    void heapifyDown(size_t index, TraceCollector* trace = nullptr);

    // Queries & state
    bool isEmpty() const noexcept { return heap_.empty(); }
    size_t size() const noexcept { return heap_.size(); }
    void clear() noexcept { heap_.clear(); }
    const std::vector<Incident>& data() const noexcept { return heap_; }

    // Index calculation helpers (public for testing & demonstrability)
    static constexpr size_t parentIndex(size_t i) noexcept { return (i - 1) / 2; }
    static constexpr size_t leftChildIndex(size_t i) noexcept { return 2 * i + 1; }
    static constexpr size_t rightChildIndex(size_t i) noexcept { return 2 * i + 2; }

    /**
     * @brief Determines whether incident `a` has strictly higher priority than incident `b`.
     *
     * In the max-heap, if hasHigherPriority(a, b) is true, `a` belongs above `b`.
     */
    bool hasHigherPriority(const Incident& a, const Incident& b) const noexcept;

    // Snapshot representation for DSA Lab inspector
    nlohmann::json toJsonSnapshot() const;

private:
    std::vector<Incident> heap_;
};

} // namespace emergencygrid
