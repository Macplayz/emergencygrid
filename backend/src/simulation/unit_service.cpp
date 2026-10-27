#include "simulation/unit_service.hpp"

namespace emergencygrid {

UnitService::UnitService(const RoadGraph* graph)
    : graph_(graph) {}

void UnitService::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    table_.clear();
    unitOrder_.clear();
}

UnitServiceResult UnitService::registerUnit(
    const EmergencyUnit& unit,
    TraceCollector* trace
) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (unit.id.empty()) {
        return {
            false,
            "INVALID_UNIT_ID",
            "Emergency unit ID cannot be empty.",
            std::nullopt
        };
    }

    if (graph_ && !graph_->hasLocation(unit.locationId)) {
        return {
            false,
            "LOCATION_NOT_FOUND",
            "Location ID '" + unit.locationId + "' does not exist in city graph.",
            std::nullopt
        };
    }

    if (table_.contains(unit.id)) {
        return {
            false,
            "DUPLICATE_UNIT_ID",
            "An emergency unit with ID '" + unit.id + "' already exists.",
            std::nullopt
        };
    }

    table_.insert(unit.id, unit, trace);
    unitOrder_.push_back(unit.id);

    if (trace && trace->isEnabled()) {
        TraceEvent ev;
        ev.algorithm = "ResourceAllocation";
        ev.operation = "unit_registered";
        ev.entityId = unit.id;
        ev.metadata = {
            {"unitId", unit.id},
            {"name", unit.name},
            {"serviceType", incidentTypeToString(unit.serviceType)},
            {"locationId", unit.locationId},
            {"status", unitStatusToString(unit.status)},
            {"capabilities", unit.capabilities}
        };
        ev.explanation = "Registered emergency unit " + unit.id + " (" + unit.name +
                         ") at location " + unit.locationId + ".";
        trace->addEvent(std::move(ev));
    }

    return {true, "", "", unit};
}

std::optional<EmergencyUnit> UnitService::getUnit(
    const std::string& unitId,
    TraceCollector* trace
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const EmergencyUnit* u = table_.search(unitId, trace);
    if (u) {
        return *u;
    }
    return std::nullopt;
}

std::vector<EmergencyUnit> UnitService::listUnits(
    std::optional<IncidentType> filterType,
    std::optional<UnitStatus> filterStatus,
    bool availableOnly
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<EmergencyUnit> result;
    result.reserve(unitOrder_.size());

    for (const auto& id : unitOrder_) {
        const EmergencyUnit* u = table_.search(id);
        if (!u) continue;

        if (availableOnly && u->status != UnitStatus::AVAILABLE) {
            continue;
        }
        if (filterType.has_value() && u->serviceType != *filterType) {
            continue;
        }
        if (filterStatus.has_value() && u->status != *filterStatus) {
            continue;
        }

        result.push_back(*u);
    }

    return result;
}

UnitServiceResult UnitService::updateLocation(
    const std::string& unitId,
    const std::string& newLocationId,
    TraceCollector* trace
) {
    std::lock_guard<std::mutex> lock(mutex_);

    EmergencyUnit* u = table_.search(unitId, trace);
    if (!u) {
        return {
            false,
            "UNIT_NOT_FOUND",
            "Emergency unit '" + unitId + "' does not exist.",
            std::nullopt
        };
    }

    if (graph_ && !graph_->hasLocation(newLocationId)) {
        return {
            false,
            "LOCATION_NOT_FOUND",
            "Destination location '" + newLocationId + "' does not exist in graph.",
            std::nullopt
        };
    }

    // Lifecycle invariant: Dispatched, Responding, or Busy units cannot be arbitrarily moved
    if (u->status == UnitStatus::DISPATCHED || u->status == UnitStatus::RESPONDING || u->status == UnitStatus::BUSY) {
        return {
            false,
            "BUSY_UNIT_CANNOT_MOVE",
            "Unit '" + unitId + "' is currently " + unitStatusToString(u->status) +
            " on incident " + u->activeIncidentId + " and cannot manually change locations.",
            std::nullopt
        };
    }

    std::string oldLoc = u->locationId;
    u->locationId = newLocationId;
    EmergencyUnit updatedCopy = *u;

    if (trace && trace->isEnabled()) {
        TraceEvent ev;
        ev.algorithm = "ResourceAllocation";
        ev.operation = "unit_location_updated";
        ev.entityId = unitId;
        ev.from = oldLoc;
        ev.to = newLocationId;
        ev.metadata = {
            {"unitId", unitId},
            {"fromLocation", oldLoc},
            {"toLocation", newLocationId}
        };
        ev.explanation = "Relocated unit " + unitId + " from " + oldLoc + " to " + newLocationId + ".";
        trace->addEvent(std::move(ev));
    }

    return {true, "", "", updatedCopy};
}

UnitServiceResult UnitService::updateStatus(
    const std::string& unitId,
    UnitStatus newStatus,
    const std::string& activeIncidentId,
    TraceCollector* trace
) {
    std::lock_guard<std::mutex> lock(mutex_);

    EmergencyUnit* u = table_.search(unitId, trace);
    if (!u) {
        return {
            false,
            "UNIT_NOT_FOUND",
            "Emergency unit '" + unitId + "' does not exist.",
            std::nullopt
        };
    }

    UnitStatus oldStatus = u->status;
    if (oldStatus != newStatus) {
        // Invariant: An OUT_OF_SERVICE unit cannot be dispatched or set responding directly
        if (oldStatus == UnitStatus::OUT_OF_SERVICE && newStatus != UnitStatus::AVAILABLE) {
            return {
                false,
                "INVALID_STATE_TRANSITION",
                "Unit '" + unitId + "' is OUT_OF_SERVICE and must be marked AVAILABLE before dispatching or responding.",
                std::nullopt
            };
        }

        // Invariant: An AVAILABLE unit cannot directly become RESPONDING or BUSY without being DISPATCHED
        if (oldStatus == UnitStatus::AVAILABLE && (newStatus == UnitStatus::RESPONDING || newStatus == UnitStatus::BUSY)) {
            return {
                false,
                "INVALID_STATE_TRANSITION",
                "Unit '" + unitId + "' is AVAILABLE and must be DISPATCHED before becoming RESPONDING or BUSY.",
                std::nullopt
            };
        }

        u->status = newStatus;
    }

    if (newStatus == UnitStatus::AVAILABLE) {
        u->activeIncidentId.clear();
    } else if (!activeIncidentId.empty()) {
        u->activeIncidentId = activeIncidentId;
    }

    EmergencyUnit updatedCopy = *u;

    if (trace && trace->isEnabled()) {
        TraceEvent ev;
        ev.algorithm = "ResourceAllocation";
        ev.operation = "unit_status_updated";
        ev.entityId = unitId;
        ev.metadata = {
            {"unitId", unitId},
            {"oldStatus", unitStatusToString(oldStatus)},
            {"newStatus", unitStatusToString(newStatus)},
            {"activeIncidentId", u->activeIncidentId}
        };
        ev.explanation = "Transitioned status of unit " + unitId + " from " +
                         unitStatusToString(oldStatus) + " to " + unitStatusToString(newStatus) + ".";
        trace->addEvent(std::move(ev));
    }

    return {true, "", "", updatedCopy};
}

std::vector<EmergencyUnit> UnitService::findEligibleUnits(
    IncidentType serviceType,
    const std::vector<std::string>& requiredCaps
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<EmergencyUnit> eligible;

    for (const auto& id : unitOrder_) {
        const EmergencyUnit* u = table_.search(id);
        if (!u) continue;

        if (u->status == UnitStatus::AVAILABLE &&
            u->serviceType == serviceType &&
            u->satisfiesCapabilities(requiredCaps)) {
            eligible.push_back(*u);
        }
    }

    return eligible;
}

size_t UnitService::totalCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return table_.size();
}

size_t UnitService::availableCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& id : unitOrder_) {
        const EmergencyUnit* u = table_.search(id);
        if (u && u->status == UnitStatus::AVAILABLE) count++;
    }
    return count;
}

size_t UnitService::busyCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& id : unitOrder_) {
        const EmergencyUnit* u = table_.search(id);
        if (u && (u->status == UnitStatus::DISPATCHED || u->status == UnitStatus::RESPONDING || u->status == UnitStatus::BUSY)) {
            count++;
        }
    }
    return count;
}

} // namespace emergencygrid
