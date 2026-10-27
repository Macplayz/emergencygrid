#include "simulation/incident_service.hpp"
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace emergencygrid {

IncidentService::IncidentService(const RoadGraph* graph)
    : graph_(graph) {}

void IncidentService::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    table_.clear();
    avlTree_.clear();
    pendingHeap_.clear();
    incidentOrder_.clear();
    arrivalCounter_ = 0;
}

void IncidentService::purgeStaleTop(TraceCollector* trace) {
    while (!pendingHeap_.isEmpty()) {
        auto topOpt = pendingHeap_.peek(nullptr);
        if (!topOpt.has_value()) {
            break;
        }

        const Incident& top = *topOpt;
        const Incident* authoritative = table_.search(top.id, nullptr);
        bool isStale = (authoritative == nullptr || authoritative->status != IncidentStatus::PENDING);

        if (isStale) {
            // Discard stale entry from root of heap
            pendingHeap_.extractMax(nullptr);

            if (trace && trace->isEnabled()) {
                TraceEvent ev;
                ev.algorithm = "BinaryHeap";
                ev.operation = "skip_stale_entry";
                ev.entityId = top.id;
                std::string statusStr = (authoritative == nullptr) ? "NOT_FOUND" : incidentStatusToString(authoritative->status);
                ev.metadata = {
                    {"incidentId", top.id},
                    {"staleStatus", statusStr},
                    {"reason", "Authoritative record is not PENDING (current status: " + statusStr + ")"}
                };
                ev.explanation = "Discarded stale heap entry " + top.id + " with authoritative status " +
                                 statusStr + " (ineligible for dispatch).";
                trace->addEvent(std::move(ev));
            }
        } else {
            // Valid pending incident at root
            break;
        }
    }
}

IncidentServiceResult IncidentService::createIncident(
    const CreateIncidentRequest& req,
    TraceCollector* trace
) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 1. Validate location against graph
    if (!graph_ || !graph_->hasLocation(req.locationId)) {
        return {
            false,
            "LOCATION_NOT_FOUND",
            "Location ID '" + req.locationId + "' does not exist in graph.",
            std::nullopt
        };
    }

    // 2. Validate severity
    auto sevOpt = parseSeverity(req.severity);
    if (!sevOpt.has_value()) {
        return {
            false,
            "INVALID_SEVERITY",
            "Invalid severity value " + std::to_string(req.severity) +
            ". Allowed range: 1 (LOW), 2 (MEDIUM), 3 (HIGH), 4 (CRITICAL).",
            std::nullopt
        };
    }

    // 3. Validate incident type
    auto typeOpt = parseIncidentType(req.type);
    if (!typeOpt.has_value()) {
        return {
            false,
            "INVALID_TYPE",
            "Invalid incident type '" + req.type + "'. Allowed: MEDICAL, FIRE, POLICE.",
            std::nullopt
        };
    }

    // 4. Validate or generate incident ID
    std::string id = req.id;
    if (id.empty()) {
        std::ostringstream oss;
        oss << "INC-" << std::setw(3) << std::setfill('0') << (arrivalCounter_ + 1);
        id = oss.str();
    }

    // Uniqueness check using custom HashTable
    if (table_.contains(id)) {
        return {
            false,
            "DUPLICATE_INCIDENT_ID",
            "An incident with ID '" + id + "' already exists.",
            std::nullopt
        };
    }

    // 5. Construct new incident record
    uint64_t seq = ++arrivalCounter_;
    double priority = static_cast<double>(*sevOpt) * 1000.0;

    Incident inc;
    inc.id = id;
    inc.type = *typeOpt;
    inc.severity = *sevOpt;
    inc.locationId = req.locationId;
    inc.arrivalSequence = seq;
    inc.reportedTick = seq;
    inc.status = IncidentStatus::PENDING;
    inc.computedPriority = priority;
    inc.description = req.description;
    inc.requiredCapabilities = req.requiredCapabilities;

    // 6. Record in custom Hash Table (for O(1) ID lookup)
    table_.insert(id, inc, trace);

    // 7. Record in custom AVL Tree (for O(log N) sorted ordering by ID)
    avlTree_.insert(id, inc, trace);

    // 8. Record in custom Binary Max-Heap (for priority triage queue)
    pendingHeap_.insert(inc, trace);

    incidentOrder_.push_back(id);

    return {true, "", "", inc};
}

std::vector<Incident> IncidentService::listIncidents(
    std::optional<IncidentStatus> filterStatus
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Incident> list;
    list.reserve(incidentOrder_.size());

    for (const auto& id : incidentOrder_) {
        const Incident* inc = table_.search(id);
        if (inc) {
            if (!filterStatus.has_value() || inc->status == *filterStatus) {
                list.push_back(*inc);
            }
        }
    }
    return list;
}

std::vector<Incident> IncidentService::listIncidentsSorted(
    std::optional<IncidentStatus> filterStatus,
    TraceCollector* trace
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto sorted = avlTree_.inorderTraversal(trace);

    if (!filterStatus.has_value()) {
        return sorted;
    }

    std::vector<Incident> filtered;
    filtered.reserve(sorted.size());
    for (const auto& inc : sorted) {
        if (inc.status == *filterStatus) {
            filtered.push_back(inc);
        }
    }
    return filtered;
}

std::optional<Incident> IncidentService::peekNext(TraceCollector* trace) {
    std::lock_guard<std::mutex> lock(mutex_);
    purgeStaleTop(trace);
    return pendingHeap_.peek(trace);
}

IncidentServiceResult IncidentService::selectNext(TraceCollector* trace) {
    std::lock_guard<std::mutex> lock(mutex_);
    purgeStaleTop(trace);

    auto topOpt = pendingHeap_.extractMax(trace);
    if (!topOpt.has_value()) {
        return {
            false,
            "EMPTY_QUEUE",
            "No eligible pending incidents available for selection.",
            std::nullopt
        };
    }

    Incident selected = *topOpt;
    Incident* authoritative = table_.search(selected.id, nullptr);
    if (authoritative) {
        authoritative->status = IncidentStatus::ASSIGNED;
        selected = *authoritative;
        // Keep AVL Tree index synchronized
        avlTree_.update(selected.id, selected, trace);
    }

    return {true, "", "", selected};
}

IncidentServiceResult IncidentService::cancelIncident(
    const std::string& incidentId,
    TraceCollector* trace
) {
    std::lock_guard<std::mutex> lock(mutex_);

    Incident* authoritative = table_.search(incidentId, trace);
    if (!authoritative) {
        return {
            false,
            "INCIDENT_NOT_FOUND",
            "Incident '" + incidentId + "' does not exist.",
            std::nullopt
        };
    }

    if (authoritative->status == IncidentStatus::CANCELLED) {
        return {
            false,
            "ALREADY_CANCELLED",
            "Incident '" + incidentId + "' is already cancelled.",
            std::nullopt
        };
    }

    if (authoritative->status == IncidentStatus::RESOLVED) {
        return {
            false,
            "INVALID_STATE_TRANSITION",
            "Cannot cancel incident '" + incidentId + "' because it is already resolved.",
            std::nullopt
        };
    }

    if (authoritative->status != IncidentStatus::PENDING) {
        return {
            false,
            "INVALID_STATE_TRANSITION",
            "Cannot cancel incident '" + incidentId + "' in status " +
            incidentStatusToString(authoritative->status) + ".",
            std::nullopt
        };
    }

    // 1. Mark CANCELLED in custom HashTable
    authoritative->status = IncidentStatus::CANCELLED;
    Incident cancelledCopy = *authoritative;

    // 2. Update status in custom AVLTree
    avlTree_.update(incidentId, cancelledCopy, trace);

    // 3. BinaryHeap will lazily discard entry when it surfaces to root via purgeStaleTop()

    if (trace && trace->isEnabled()) {
        TraceEvent ev;
        ev.algorithm = "IncidentService";
        ev.operation = "cancel_incident";
        ev.entityId = incidentId;
        ev.metadata = {
            {"incidentId", incidentId},
            {"newStatus", "CANCELLED"}
        };
        ev.explanation = "Marked incident " + incidentId +
                         " as CANCELLED across HashTable and AVLTree. Heap entry will be lazily purged.";
        trace->addEvent(std::move(ev));
    }

    return {true, "", "", cancelledCopy};
}

std::optional<Incident> IncidentService::getIncident(
    const std::string& incidentId,
    TraceCollector* trace
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const Incident* found = table_.search(incidentId, trace);
    if (found) {
        return *found;
    }
    return std::nullopt;
}

IncidentServiceResult IncidentService::resolveIncident(
    const std::string& incidentId,
    TraceCollector* trace
) {
    std::lock_guard<std::mutex> lock(mutex_);

    Incident* authoritative = table_.search(incidentId, trace);
    if (!authoritative) {
        return {
            false,
            "INCIDENT_NOT_FOUND",
            "Incident '" + incidentId + "' does not exist.",
            std::nullopt
        };
    }

    if (authoritative->status == IncidentStatus::CANCELLED) {
        return {
            false,
            "INVALID_STATE_TRANSITION",
            "Cannot resolve incident '" + incidentId + "' because it is cancelled.",
            std::nullopt
        };
    }

    authoritative->status = IncidentStatus::RESOLVED;
    Incident resolvedCopy = *authoritative;

    // Update custom AVLTree
    avlTree_.update(incidentId, resolvedCopy, trace);

    if (trace && trace->isEnabled()) {
        TraceEvent ev;
        ev.algorithm = "IncidentService";
        ev.operation = "resolve_incident";
        ev.entityId = incidentId;
        ev.metadata = {
            {"incidentId", incidentId},
            {"newStatus", "RESOLVED"}
        };
        ev.explanation = "Marked incident " + incidentId + " as RESOLVED across all indexes.";
        trace->addEvent(std::move(ev));
    }

    return {true, "", "", resolvedCopy};
}

IncidentServiceResult IncidentService::updateIncidentStatus(
    const std::string& incidentId,
    IncidentStatus newStatus,
    TraceCollector* trace
) {
    std::lock_guard<std::mutex> lock(mutex_);

    Incident* authoritative = table_.search(incidentId, trace);
    if (!authoritative) {
        return {
            false,
            "INCIDENT_NOT_FOUND",
            "Incident '" + incidentId + "' does not exist.",
            std::nullopt
        };
    }

    authoritative->status = newStatus;
    Incident updated = *authoritative;

    avlTree_.update(incidentId, updated, trace);

    if (trace && trace->isEnabled()) {
        TraceEvent ev;
        ev.algorithm = "IncidentService";
        ev.operation = "update_incident_status";
        ev.entityId = incidentId;
        ev.metadata = {
            {"incidentId", incidentId},
            {"newStatus", incidentStatusToString(newStatus)}
        };
        ev.explanation = "Updated status of incident " + incidentId +
                         " to " + incidentStatusToString(newStatus) + " across all indexes.";
        trace->addEvent(std::move(ev));
    }

    return {true, "", "", updated};
}

size_t IncidentService::totalCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return table_.size();
}

size_t IncidentService::pendingCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& inc : table_.values()) {
        if (inc.status == IncidentStatus::PENDING) count++;
    }
    return count;
}

size_t IncidentService::assignedCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& inc : table_.values()) {
        if (inc.status == IncidentStatus::ASSIGNED || inc.status == IncidentStatus::DISPATCHED) count++;
    }
    return count;
}

size_t IncidentService::resolvedCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& inc : table_.values()) {
        if (inc.status == IncidentStatus::RESOLVED) count++;
    }
    return count;
}

size_t IncidentService::cancelledCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& inc : table_.values()) {
        if (inc.status == IncidentStatus::CANCELLED) count++;
    }
    return count;
}

size_t IncidentService::heapSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pendingHeap_.size();
}

nlohmann::json IncidentService::getHeapSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pendingHeap_.toJsonSnapshot();
}

nlohmann::json IncidentService::getAvlSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return avlTree_.toJsonSnapshot();
}

nlohmann::json IncidentService::getHashTableSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return table_.getStatsJson();
}

} // namespace emergencygrid
