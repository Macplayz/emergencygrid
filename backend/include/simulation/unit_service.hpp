#pragma once

#include <string>
#include <vector>
#include <optional>
#include <mutex>
#include "core/types.hpp"
#include "core/trace.hpp"
#include "dsa/graph.hpp"
#include "dsa/hash_table.hpp"

namespace emergencygrid {

struct UnitServiceResult {
    bool success{false};
    std::string errorCode;
    std::string errorMessage;
    std::optional<EmergencyUnit> unit{std::nullopt};
};

/**
 * @brief Resource-management service for emergency response fleet units.
 *
 * Reuses the custom educational HashTable<std::string, EmergencyUnit> for O(1) lookups
 * and enforces strict lifecycle validation, location checks against RoadGraph, and capability matching.
 */
class UnitService {
public:
    explicit UnitService(const RoadGraph* graph = nullptr);

    void setGraph(const RoadGraph* graph) noexcept { graph_ = graph; }

    // 1. Register a new emergency unit
    UnitServiceResult registerUnit(
        const EmergencyUnit& unit,
        TraceCollector* trace = nullptr
    );

    // 2. Retrieve unit by ID using custom HashTable
    std::optional<EmergencyUnit> getUnit(
        const std::string& unitId,
        TraceCollector* trace = nullptr
    ) const;

    // 3. List units with optional serviceType and status filters
    std::vector<EmergencyUnit> listUnits(
        std::optional<IncidentType> filterType = std::nullopt,
        std::optional<UnitStatus> filterStatus = std::nullopt,
        bool availableOnly = false
    ) const;

    // 4. Update unit location (prevented if unit is busy/dispatched)
    UnitServiceResult updateLocation(
        const std::string& unitId,
        const std::string& newLocationId,
        TraceCollector* trace = nullptr
    );

    // 5. Update unit status and optional assigned incident ID
    UnitServiceResult updateStatus(
        const std::string& unitId,
        UnitStatus newStatus,
        const std::string& activeIncidentId = "",
        TraceCollector* trace = nullptr
    );

    // Convenience methods
    bool hasUnit(const std::string& unitId) const {
        return getUnit(unitId).has_value();
    }

    bool updateUnitLocation(const std::string& unitId, const std::string& newLocationId) {
        return updateLocation(unitId, newLocationId).success;
    }

    bool updateUnitStatus(const std::string& unitId, UnitStatus newStatus, const std::string& activeIncidentId = "") {
        return updateStatus(unitId, newStatus, activeIncidentId).success;
    }

    // 6. Find currently AVAILABLE units satisfying serviceType and required capabilities
    std::vector<EmergencyUnit> findEligibleUnits(
        IncidentType serviceType,
        const std::vector<std::string>& requiredCaps = {}
    ) const;

    // Counts & metrics
    size_t totalCount() const;
    size_t availableCount() const;
    size_t busyCount() const;

    const HashTable<std::string, EmergencyUnit>& getTable() const noexcept { return table_; }

    void reset();

private:
    const RoadGraph* graph_{nullptr};
    mutable std::mutex mutex_;

    // Custom Hash Table for O(1) unit lookups
    HashTable<std::string, EmergencyUnit> table_;

    // Insertion order tracking for deterministic chronological listing
    std::vector<std::string> unitOrder_;
};

} // namespace emergencygrid
