#include "dsa/binary_heap.hpp"
#include <utility>
#include <sstream>

namespace emergencygrid {

bool BinaryHeap::hasHigherPriority(const Incident& a, const Incident& b) const noexcept {
    // 1. Higher severity comes first (4: Critical > 3: High > 2: Medium > 1: Low)
    if (a.severity != b.severity) {
        return static_cast<int>(a.severity) > static_cast<int>(b.severity);
    }
    // 2. If severity is equal, earlier arrival sequence comes first (FIFO)
    if (a.arrivalSequence != b.arrivalSequence) {
        return a.arrivalSequence < b.arrivalSequence;
    }
    // 3. Deterministic lexical tie-breaker by unique incident ID
    return a.id < b.id;
}

void BinaryHeap::insert(const Incident& incident, TraceCollector* trace) {
    size_t index = heap_.size();
    heap_.push_back(incident);

    if (trace && trace->isEnabled()) {
        TraceEvent ev;
        ev.algorithm = "BinaryHeap";
        ev.operation = "insert";
        ev.entityId = incident.id;
        ev.metadata = {
            {"index", index},
            {"severity", static_cast<int>(incident.severity)},
            {"severityLabel", severityToString(incident.severity)},
            {"arrivalSequence", incident.arrivalSequence},
            {"computedPriority", incident.computedPriority}
        };
        ev.explanation = "Inserted incident " + incident.id + " (severity: " +
                         severityToString(incident.severity) + ") at array index " +
                         std::to_string(index) + ".";
        trace->addEvent(std::move(ev));
    }

    if (index > 0) {
        heapifyUp(index, trace);
    }
}

std::optional<Incident> BinaryHeap::peek(TraceCollector* trace) const {
    if (heap_.empty()) {
        return std::nullopt;
    }

    if (trace && trace->isEnabled()) {
        TraceEvent ev;
        ev.algorithm = "BinaryHeap";
        ev.operation = "peek";
        ev.entityId = heap_[0].id;
        ev.metadata = {
            {"rootId", heap_[0].id},
            {"severity", static_cast<int>(heap_[0].severity)},
            {"severityLabel", severityToString(heap_[0].severity)},
            {"arrivalSequence", heap_[0].arrivalSequence},
            {"heapSize", heap_.size()}
        };
        ev.explanation = "Inspected root incident " + heap_[0].id + " (severity: " +
                         severityToString(heap_[0].severity) + ", arrival: " +
                         std::to_string(heap_[0].arrivalSequence) + ") without removing from heap.";
        trace->addEvent(std::move(ev));
    }

    return heap_[0];
}

std::optional<Incident> BinaryHeap::extractMax(TraceCollector* trace) {
    if (heap_.empty()) {
        return std::nullopt;
    }

    if (heap_.size() == 1) {
        Incident root = std::move(heap_.back());
        heap_.pop_back();

        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "BinaryHeap";
            ev.operation = "extract_max";
            ev.entityId = root.id;
            ev.metadata = {
                {"extractedId", root.id},
                {"severity", static_cast<int>(root.severity)},
                {"severityLabel", severityToString(root.severity)},
                {"remainingSize", 0}
            };
            ev.explanation = "Extracted root incident " + root.id + " from heap (heap is now empty).";
            trace->addEvent(std::move(ev));
        }

        return root;
    }

    Incident root = heap_[0];
    Incident last = heap_.back();

    // Swap root with last element
    std::swap(heap_[0], heap_.back());

    if (trace && trace->isEnabled()) {
        TraceEvent evSwap;
        evSwap.algorithm = "BinaryHeap";
        evSwap.operation = "swap";
        evSwap.entityId = root.id;
        evSwap.from = "0";
        evSwap.to = std::to_string(heap_.size() - 1);
        evSwap.metadata = {
            {"fromIndex", 0},
            {"toIndex", heap_.size() - 1},
            {"rootId", root.id},
            {"lastId", last.id}
        };
        evSwap.explanation = "Swapped root incident " + root.id + " with last element " +
                             last.id + " at index " + std::to_string(heap_.size() - 1) +
                             " prior to removal.";
        trace->addEvent(std::move(evSwap));
    }

    heap_.pop_back();

    if (trace && trace->isEnabled()) {
        TraceEvent evExt;
        evExt.algorithm = "BinaryHeap";
        evExt.operation = "extract_max";
        evExt.entityId = root.id;
        evExt.metadata = {
            {"extractedId", root.id},
            {"severity", static_cast<int>(root.severity)},
            {"severityLabel", severityToString(root.severity)},
            {"remainingSize", heap_.size()}
        };
        evExt.explanation = "Extracted highest-priority incident " + root.id + " from root of heap.";
        trace->addEvent(std::move(evExt));
    }

    heapifyDown(0, trace);
    return root;
}

void BinaryHeap::heapifyUp(size_t index, TraceCollector* trace) {
    if (trace && trace->isEnabled() && index > 0) {
        TraceEvent ev;
        ev.algorithm = "BinaryHeap";
        ev.operation = "heapify_up";
        ev.entityId = heap_[index].id;
        ev.metadata = {
            {"startIndex", index},
            {"incidentId", heap_[index].id}
        };
        ev.explanation = "Starting heapify-up for incident " + heap_[index].id +
                         " from index " + std::to_string(index) + ".";
        trace->addEvent(std::move(ev));
    }

    while (index > 0) {
        size_t p = parentIndex(index);
        bool childHasHigher = hasHigherPriority(heap_[index], heap_[p]);

        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "BinaryHeap";
            ev.operation = "compare";
            ev.entityId = heap_[index].id;
            ev.metadata = {
                {"childIndex", index},
                {"parentIndex", p},
                {"childId", heap_[index].id},
                {"parentId", heap_[p].id},
                {"childSeverity", static_cast<int>(heap_[index].severity)},
                {"parentSeverity", static_cast<int>(heap_[p].severity)},
                {"childArrivalSequence", heap_[index].arrivalSequence},
                {"parentArrivalSequence", heap_[p].arrivalSequence},
                {"childHasHigherPriority", childHasHigher}
            };
            ev.explanation = "Comparing child " + heap_[index].id + " (sev: " +
                             severityToString(heap_[index].severity) + ", seq: " +
                             std::to_string(heap_[index].arrivalSequence) + ") with parent " +
                             heap_[p].id + " (sev: " +
                             severityToString(heap_[p].severity) + ", seq: " +
                             std::to_string(heap_[p].arrivalSequence) + ").";
            trace->addEvent(std::move(ev));
        }

        if (childHasHigher) {
            std::string childId = heap_[index].id;
            std::string parentId = heap_[p].id;

            std::swap(heap_[index], heap_[p]);

            if (trace && trace->isEnabled()) {
                TraceEvent evSwap;
                evSwap.algorithm = "BinaryHeap";
                evSwap.operation = "swap";
                evSwap.entityId = childId;
                evSwap.from = std::to_string(index);
                evSwap.to = std::to_string(p);
                evSwap.metadata = {
                    {"fromIndex", index},
                    {"toIndex", p},
                    {"childId", childId},
                    {"parentId", parentId}
                };
                evSwap.explanation = "Swapped child " + childId + " at index " +
                                     std::to_string(index) + " with parent " + parentId +
                                     " at index " + std::to_string(p) + " to restore max-heap invariant.";
                trace->addEvent(std::move(evSwap));
            }

            index = p;
        } else {
            break;
        }
    }
}

void BinaryHeap::heapifyDown(size_t index, TraceCollector* trace) {
    if (heap_.empty()) return;

    if (trace && trace->isEnabled()) {
        TraceEvent ev;
        ev.algorithm = "BinaryHeap";
        ev.operation = "heapify_down";
        ev.entityId = heap_[index].id;
        ev.metadata = {
            {"startIndex", index},
            {"incidentId", heap_[index].id}
        };
        ev.explanation = "Starting heapify-down for incident " + heap_[index].id +
                         " from index " + std::to_string(index) + ".";
        trace->addEvent(std::move(ev));
    }

    while (true) {
        size_t left = leftChildIndex(index);
        size_t right = rightChildIndex(index);
        size_t largest = index;

        if (left < heap_.size()) {
            bool leftHigher = hasHigherPriority(heap_[left], heap_[largest]);
            if (trace && trace->isEnabled()) {
                TraceEvent ev;
                ev.algorithm = "BinaryHeap";
                ev.operation = "compare";
                ev.entityId = heap_[left].id;
                ev.metadata = {
                    {"candidateIndex", left},
                    {"currentLargestIndex", largest},
                    {"candidateId", heap_[left].id},
                    {"currentLargestId", heap_[largest].id},
                    {"candidateSeverity", static_cast<int>(heap_[left].severity)},
                    {"currentLargestSeverity", static_cast<int>(heap_[largest].severity)},
                    {"candidateHasHigherPriority", leftHigher}
                };
                ev.explanation = "Comparing left child " + heap_[left].id + " against current largest " +
                                 heap_[largest].id + ".";
                trace->addEvent(std::move(ev));
            }
            if (leftHigher) {
                largest = left;
            }
        }

        if (right < heap_.size()) {
            bool rightHigher = hasHigherPriority(heap_[right], heap_[largest]);
            if (trace && trace->isEnabled()) {
                TraceEvent ev;
                ev.algorithm = "BinaryHeap";
                ev.operation = "compare";
                ev.entityId = heap_[right].id;
                ev.metadata = {
                    {"candidateIndex", right},
                    {"currentLargestIndex", largest},
                    {"candidateId", heap_[right].id},
                    {"currentLargestId", heap_[largest].id},
                    {"candidateSeverity", static_cast<int>(heap_[right].severity)},
                    {"currentLargestSeverity", static_cast<int>(heap_[largest].severity)},
                    {"candidateHasHigherPriority", rightHigher}
                };
                ev.explanation = "Comparing right child " + heap_[right].id + " against current largest " +
                                 heap_[largest].id + ".";
                trace->addEvent(std::move(ev));
            }
            if (rightHigher) {
                largest = right;
            }
        }

        if (largest != index) {
            std::string curId = heap_[index].id;
            std::string targetId = heap_[largest].id;

            std::swap(heap_[index], heap_[largest]);

            if (trace && trace->isEnabled()) {
                TraceEvent evSwap;
                evSwap.algorithm = "BinaryHeap";
                evSwap.operation = "swap";
                evSwap.entityId = curId;
                evSwap.from = std::to_string(index);
                evSwap.to = std::to_string(largest);
                evSwap.metadata = {
                    {"fromIndex", index},
                    {"toIndex", largest},
                    {"displacedId", curId},
                    {"promotedId", targetId}
                };
                evSwap.explanation = "Swapped incident " + curId + " at index " +
                                     std::to_string(index) + " with child " + targetId +
                                     " at index " + std::to_string(largest) + " to restore max-heap invariant.";
                trace->addEvent(std::move(evSwap));
            }

            index = largest;
        } else {
            break;
        }
    }
}

nlohmann::json BinaryHeap::toJsonSnapshot() const {
    nlohmann::json arr = nlohmann::json::array();
    for (size_t i = 0; i < heap_.size(); ++i) {
        nlohmann::json item = heap_[i].toJson();
        item["heapIndex"] = i;
        if (i > 0) {
            item["parentIndex"] = parentIndex(i);
        } else {
            item["parentIndex"] = nullptr;
        }
        size_t left = leftChildIndex(i);
        size_t right = rightChildIndex(i);
        item["leftChildIndex"] = (left < heap_.size()) ? nlohmann::json(left) : nlohmann::json(nullptr);
        item["rightChildIndex"] = (right < heap_.size()) ? nlohmann::json(right) : nlohmann::json(nullptr);
        arr.push_back(item);
    }
    return {
        {"size", heap_.size()},
        {"isEmpty", heap_.empty()},
        {"elements", arr}
    };
}

} // namespace emergencygrid
