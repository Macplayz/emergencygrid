#pragma once

#include <string>
#include <vector>
#include <optional>
#include <mutex>
#include <memory>
#include "core/types.hpp"
#include "core/trace.hpp"
#include "dsa/graph.hpp"
#include "dsa/dijkstra.hpp"
#include "dsa/hash_table.hpp"
#include "simulation/incident_service.hpp"
#include "simulation/unit_service.hpp"

namespace emergencygrid {

struct DispatchServiceResult {
    bool success{false};
    std::string errorCode;
    std::string errorMessage;
    std::string error;
    std::optional<DispatchRecord> dispatch{std::nullopt};
    std::optional<DispatchRecord> record{std::nullopt};
    std::string decisionRationale;
    std::string explanation;

    DispatchServiceResult() = default;

    DispatchServiceResult(
        bool s,
        std::string code,
        std::string msg,
        std::optional<DispatchRecord> rec = std::nullopt,
        std::string rationale = ""
    ) : success(s),
        errorCode(std::move(code)),
        errorMessage(msg),
        error(msg),
        dispatch(rec),
        record(rec),
        decisionRationale(rationale),
        explanation(std::move(rationale)) {}
};

using DispatchResult = DispatchServiceResult;

/**
 * @brief Dispatch selection and allocation engine.
 *
 * Coordinates candidate unit eligibility evaluation, Dijkstra travel-time computations,
 * deterministic tie-breaking, transactional state transitions, active route management,
 * and comprehensive operational trace emission.
 */
class DispatchService {
public:
    DispatchService(
        const RoadGraph* graph = nullptr,
        IncidentService* incidentService = nullptr,
        UnitService* unitService = nullptr
    );

    void setGraph(const RoadGraph* graph) noexcept { graph_ = graph; }
    void setIncidentService(IncidentService* svc) noexcept { incidentService_ = svc; }
    void setUnitService(UnitService* svc) noexcept { unitService_ = svc; }

    void init(
        IncidentService* incidentService,
        UnitService* unitService,
        const RoadGraph* graph
    ) noexcept {
        incidentService_ = incidentService;
        unitService_ = unitService;
        graph_ = graph;
    }

    // 1. Dispatch a specific pending incident
    DispatchServiceResult dispatchIncident(
        const std::string& incidentId,
        TraceCollector* trace = nullptr
    );

    // 2. Dispatch the highest-priority eligible pending incident from heap
    DispatchServiceResult dispatchNext(
        TraceCollector* trace = nullptr
    );

    // 3. Complete an active dispatch: resolves incident and releases unit
    DispatchServiceResult completeDispatch(
        const std::string& dispatchId,
        TraceCollector* trace = nullptr
    );

    // 4. Recalculate route on-demand for an active dispatch using current road conditions
    DispatchServiceResult recalculateRoute(
        const std::string& dispatchId,
        TraceCollector* trace = nullptr
    );

    // 5. Retrieve dispatch by ID
    std::optional<DispatchRecord> getDispatch(
        const std::string& dispatchId
    ) const;

    // 6. List all dispatches with optional status filter
    std::vector<DispatchRecord> listDispatches(
        std::optional<DispatchStatus> filterStatus = std::nullopt
    ) const;

    // Metrics
    size_t activeCount() const;
    size_t completedCount() const;
    size_t totalCount() const;

    void reset();

private:
    const RoadGraph* graph_{nullptr};
    IncidentService* incidentService_{nullptr};
    UnitService* unitService_{nullptr};
    mutable std::mutex mutex_;

    // Custom Hash Table for O(1) dispatch lookup
    HashTable<std::string, DispatchRecord> dispatches_;

    // Insertion order tracking for deterministic chronological listing
    std::vector<std::string> dispatchOrder_;

    uint64_t dispatchCounter_{0};
};

} // namespace emergencygrid
