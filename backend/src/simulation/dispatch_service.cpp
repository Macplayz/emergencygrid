#include "simulation/dispatch_service.hpp"
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace emergencygrid {

DispatchService::DispatchService(
    const RoadGraph* graph,
    IncidentService* incidentService,
    UnitService* unitService
) : graph_(graph), incidentService_(incidentService), unitService_(unitService) {}

void DispatchService::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    dispatches_.clear();
    dispatchOrder_.clear();
    dispatchCounter_ = 0;
}

DispatchServiceResult DispatchService::dispatchIncident(
    const std::string& incidentId,
    TraceCollector* trace
) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!incidentService_ || !unitService_ || !graph_) {
        return {
            false,
            "SERVICE_UNINITIALIZED",
            "Simulation services or city graph not initialized.",
            std::nullopt,
            ""
        };
    }

    // 1. Verify incident exists
    auto incOpt = incidentService_->getIncident(incidentId, trace);
    if (!incOpt.has_value()) {
        return {
            false,
            "INCIDENT_NOT_FOUND",
            "Incident '" + incidentId + "' does not exist.",
            std::nullopt,
            ""
        };
    }

    const Incident& incident = *incOpt;

    // 2. Validate incident is in an eligible pending state
    if (incident.status != IncidentStatus::PENDING) {
        if (incident.status == IncidentStatus::ASSIGNED || incident.status == IncidentStatus::DISPATCHED) {
            return {
                false,
                "ALREADY_ASSIGNED",
                "Incident '" + incidentId + "' is already assigned to an active dispatch.",
                std::nullopt,
                ""
            };
        }
        if (incident.status == IncidentStatus::RESOLVED) {
            return {
                false,
                "INCIDENT_ALREADY_RESOLVED",
                "Incident '" + incidentId + "' is already resolved.",
                std::nullopt,
                ""
            };
        }
        if (incident.status == IncidentStatus::CANCELLED) {
            return {
                false,
                "INCIDENT_CANCELLED",
                "Cannot dispatch cancelled incident '" + incidentId + "'.",
                std::nullopt,
                ""
            };
        }
        return {
            false,
            "INVALID_INCIDENT_STATE",
            "Incident '" + incidentId + "' is not eligible for dispatch (status: " +
            incidentStatusToString(incident.status) + ").",
            std::nullopt,
            ""
        };
    }

    if (trace && trace->isEnabled()) {
        TraceEvent ev;
        ev.algorithm = "DispatchEngine";
        ev.operation = "validate_incident";
        ev.entityId = incidentId;
        ev.metadata = {
            {"incidentId", incidentId},
            {"serviceType", incidentTypeToString(incident.type)},
            {"severity", static_cast<int>(incident.severity)},
            {"locationId", incident.locationId},
            {"requiredCapabilities", incident.requiredCapabilities}
        };
        ev.explanation = "Validated incident " + incidentId + " at " + incident.locationId +
                         " as eligible for emergency unit allocation.";
        trace->addEvent(std::move(ev));
    }

    // 3. Discover candidates and evaluate eligibility
    auto allUnits = unitService_->listUnits();
    std::vector<EmergencyUnit> eligibleUnits;

    for (const auto& u : allUnits) {
        if (u.status != UnitStatus::AVAILABLE) {
            if (trace && trace->isEnabled()) {
                TraceEvent ev;
                ev.algorithm = "DispatchEngine";
                ev.operation = "candidate_rejected";
                ev.entityId = u.id;
                ev.metadata = {
                    {"unitId", u.id},
                    {"reason", "unit_unavailable"},
                    {"currentStatus", unitStatusToString(u.status)}
                };
                ev.explanation = "Candidate " + u.id + " rejected: unit is currently " +
                                 unitStatusToString(u.status) + ".";
                trace->addEvent(std::move(ev));
            }
            continue;
        }

        if (u.serviceType != incident.type) {
            if (trace && trace->isEnabled()) {
                TraceEvent ev;
                ev.algorithm = "DispatchEngine";
                ev.operation = "candidate_rejected";
                ev.entityId = u.id;
                ev.metadata = {
                    {"unitId", u.id},
                    {"reason", "wrong_service_type"},
                    {"unitServiceType", incidentTypeToString(u.serviceType)},
                    {"requiredServiceType", incidentTypeToString(incident.type)}
                };
                ev.explanation = "Candidate " + u.id + " rejected: service type " +
                                 incidentTypeToString(u.serviceType) + " does not match required type " +
                                 incidentTypeToString(incident.type) + ".";
                trace->addEvent(std::move(ev));
            }
            continue;
        }

        if (!u.satisfiesCapabilities(incident.requiredCapabilities)) {
            if (trace && trace->isEnabled()) {
                TraceEvent ev;
                ev.algorithm = "DispatchEngine";
                ev.operation = "candidate_rejected";
                ev.entityId = u.id;
                ev.metadata = {
                    {"unitId", u.id},
                    {"reason", "missing_capabilities"},
                    {"unitCapabilities", u.capabilities},
                    {"requiredCapabilities", incident.requiredCapabilities}
                };
                ev.explanation = "Candidate " + u.id + " rejected: unit lacks one or more required capabilities.";
                trace->addEvent(std::move(ev));
            }
            continue;
        }

        eligibleUnits.push_back(u);
    }

    if (eligibleUnits.empty()) {
        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "DispatchEngine";
            ev.operation = "dispatch_failed";
            ev.entityId = incidentId;
            ev.metadata = {{"reason", "no_eligible_units"}};
            ev.explanation = "Dispatch failed: no available units meet the service type and capability requirements.";
            trace->addEvent(std::move(ev));
        }
        return {
            false,
            "NO_ELIGIBLE_UNITS",
            "No available units match the required service type (" + incidentTypeToString(incident.type) + ") and capabilities.",
            std::nullopt,
            ""
        };
    }

    // 4. Calculate shortest path routes using Dijkstra
    struct EvaluatedCandidate {
        EmergencyUnit unit;
        RouteResult route;
    };

    std::vector<EvaluatedCandidate> reachableCandidates;

    for (const auto& u : eligibleUnits) {
        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "DispatchEngine";
            ev.operation = "route_calculation_requested";
            ev.entityId = u.id;
            ev.from = u.locationId;
            ev.to = incident.locationId;
            ev.metadata = {
                {"unitId", u.id},
                {"origin", u.locationId},
                {"destination", incident.locationId}
            };
            ev.explanation = "Computing shortest-path travel time from " + u.id + " at " +
                             u.locationId + " to incident at " + incident.locationId + ".";
            trace->addEvent(std::move(ev));
        }

        RouteResult route = DijkstraRouter::findShortestPath(*graph_, u.locationId, incident.locationId, trace);

        if (!route.routeFound) {
            if (trace && trace->isEnabled()) {
                TraceEvent ev;
                ev.algorithm = "DispatchEngine";
                ev.operation = "candidate_rejected";
                ev.entityId = u.id;
                ev.metadata = {
                    {"unitId", u.id},
                    {"reason", "no_route_found"},
                    {"origin", u.locationId},
                    {"destination", incident.locationId}
                };
                ev.explanation = "Candidate " + u.id + " rejected: destination is unreachable from " +
                                 u.locationId + " due to road closures.";
                trace->addEvent(std::move(ev));
            }
            continue;
        }

        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "DispatchEngine";
            ev.operation = "candidate_route_evaluated";
            ev.entityId = u.id;
            ev.cost = route.totalTravelTimeMinutes;
            ev.metadata = {
                {"unitId", u.id},
                {"travelTimeMinutes", route.totalTravelTimeMinutes},
                {"totalDistanceKm", route.totalDistanceKm},
                {"path", route.path}
            };
            ev.explanation = "Candidate " + u.id + " route evaluated: estimated travel time " +
                             std::to_string(route.totalTravelTimeMinutes) + " min.";
            trace->addEvent(std::move(ev));
        }

        reachableCandidates.push_back({u, route});
    }

    if (reachableCandidates.empty()) {
        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "DispatchEngine";
            ev.operation = "dispatch_failed";
            ev.entityId = incidentId;
            ev.metadata = {{"reason", "no_reachable_units"}};
            ev.explanation = "Dispatch failed: eligible units exist, but none can reach incident due to road closures.";
            trace->addEvent(std::move(ev));
        }
        return {
            false,
            "NO_REACHABLE_UNITS",
            "No reachable units found: eligible units exist, but none can reach incident location '" + incident.locationId + "' due to road closures.",
            std::nullopt,
            ""
        };
    }

    // 5. Select unit with minimum estimated travel time (deterministic tie-breaking by unitId)
    std::sort(reachableCandidates.begin(), reachableCandidates.end(), [](const auto& a, const auto& b) {
        if (std::abs(a.route.totalTravelTimeMinutes - b.route.totalTravelTimeMinutes) > 1e-6) {
            return a.route.totalTravelTimeMinutes < b.route.totalTravelTimeMinutes;
        }
        return a.unit.id < b.unit.id;
    });

    const EvaluatedCandidate& selected = reachableCandidates.front();

    std::ostringstream ratOss;
    ratOss << "Unit " << selected.unit.id << " (" << selected.unit.name << ") was available, "
           << "satisfied required capabilities, and had the lowest valid travel time ("
           << std::fixed << std::setprecision(2) << selected.route.totalTravelTimeMinutes
           << " min) among " << reachableCandidates.size() << " eligible evaluated candidates.";
    std::string rationale = ratOss.str();

    if (trace && trace->isEnabled()) {
        TraceEvent ev;
        ev.algorithm = "DispatchEngine";
        ev.operation = "candidate_selected";
        ev.entityId = selected.unit.id;
        ev.cost = selected.route.totalTravelTimeMinutes;
        ev.metadata = {
            {"selectedUnitId", selected.unit.id},
            {"incidentId", incidentId},
            {"travelTimeMinutes", selected.route.totalTravelTimeMinutes},
            {"candidateCount", reachableCandidates.size()},
            {"decisionRationale", rationale}
        };
        ev.explanation = "Selected unit " + selected.unit.id + " with optimal travel time " +
                         std::to_string(selected.route.totalTravelTimeMinutes) + " min.";
        trace->addEvent(std::move(ev));
    }

    // 6. Atomic state mutation across UnitService, IncidentService, and DispatchService
    std::ostringstream dispOss;
    dispOss << "DISP-" << std::setw(3) << std::setfill('0') << (++dispatchCounter_);
    std::string dispatchId = dispOss.str();

    // Mark unit DISPATCHED
    unitService_->updateStatus(selected.unit.id, UnitStatus::DISPATCHED, incidentId, trace);

    // Mark incident ASSIGNED
    incidentService_->updateIncidentStatus(incidentId, IncidentStatus::ASSIGNED, trace);

    DispatchRecord record;
    record.id = dispatchId;
    record.incidentId = incidentId;
    record.unitId = selected.unit.id;
    record.startLocationId = selected.unit.locationId;
    record.targetLocationId = incident.locationId;
    record.route = selected.route;
    record.travelTimeMinutes = selected.route.totalTravelTimeMinutes;
    record.dispatchedTick = dispatchCounter_;
    record.status = DispatchStatus::ACTIVE;
    record.decisionRationale = rationale;

    dispatches_.insert(dispatchId, record, trace);
    dispatchOrder_.push_back(dispatchId);

    if (trace && trace->isEnabled()) {
        TraceEvent ev;
        ev.algorithm = "DispatchEngine";
        ev.operation = "dispatch_created";
        ev.entityId = dispatchId;
        ev.metadata = {
            {"dispatchId", dispatchId},
            {"unitId", selected.unit.id},
            {"incidentId", incidentId},
            {"travelTimeMinutes", record.travelTimeMinutes}
        };
        ev.explanation = "Dispatch " + dispatchId + " active: unit " + selected.unit.id +
                         " dispatched to incident " + incidentId + ".";
        trace->addEvent(std::move(ev));
    }

    return {true, "", "", record, rationale};
}

DispatchServiceResult DispatchService::dispatchNext(
    TraceCollector* trace
) {
    if (!incidentService_) {
        return {
            false,
            "SERVICE_UNINITIALIZED",
            "Incident service not initialized.",
            std::nullopt,
            ""
        };
    }

    auto nextOpt = incidentService_->peekNext(trace);
    if (!nextOpt.has_value()) {
        return {
            false,
            "EMPTY_QUEUE",
            "No pending incidents in triage queue.",
            std::nullopt,
            ""
        };
    }

    // Attempt dispatch
    auto dispRes = dispatchIncident(nextOpt->id, trace);
    if (dispRes.success) {
        // Pop the incident from the heap
        incidentService_->selectNext(trace);
    }

    return dispRes;
}

DispatchServiceResult DispatchService::completeDispatch(
    const std::string& dispatchId,
    TraceCollector* trace
) {
    std::lock_guard<std::mutex> lock(mutex_);

    DispatchRecord* disp = dispatches_.search(dispatchId, trace);
    if (!disp) {
        return {
            false,
            "DISPATCH_NOT_FOUND",
            "Dispatch record '" + dispatchId + "' does not exist.",
            std::nullopt,
            ""
        };
    }

    if (disp->status != DispatchStatus::ACTIVE) {
        return {
            false,
            "DISPATCH_ALREADY_COMPLETED",
            "Dispatch '" + dispatchId + "' is already completed or inactive.",
            std::nullopt,
            ""
        };
    }

    // 1. Mark dispatch completed
    disp->status = DispatchStatus::COMPLETED;
    disp->completedTick = ++dispatchCounter_;
    DispatchRecord completedCopy = *disp;

    // 2. Release emergency unit back to AVAILABLE
    if (unitService_) {
        unitService_->updateStatus(completedCopy.unitId, UnitStatus::AVAILABLE, "", trace);
    }

    // 3. Resolve incident in IncidentService across all indexes
    if (incidentService_) {
        incidentService_->resolveIncident(completedCopy.incidentId, trace);
    }

    if (trace && trace->isEnabled()) {
        TraceEvent ev;
        ev.algorithm = "DispatchEngine";
        ev.operation = "dispatch_completed";
        ev.entityId = dispatchId;
        ev.metadata = {
            {"dispatchId", dispatchId},
            {"unitId", completedCopy.unitId},
            {"incidentId", completedCopy.incidentId}
        };
        ev.explanation = "Dispatch " + dispatchId + " completed: unit " + completedCopy.unitId +
                         " released and incident " + completedCopy.incidentId + " marked RESOLVED.";
        trace->addEvent(std::move(ev));
    }

    return {true, "", "", completedCopy, "Dispatch " + dispatchId + " successfully completed."};
}

DispatchServiceResult DispatchService::recalculateRoute(
    const std::string& dispatchId,
    TraceCollector* trace
) {
    std::lock_guard<std::mutex> lock(mutex_);

    DispatchRecord* disp = dispatches_.search(dispatchId, trace);
    if (!disp) {
        return {
            false,
            "DISPATCH_NOT_FOUND",
            "Dispatch record '" + dispatchId + "' does not exist.",
            std::nullopt,
            ""
        };
    }

    if (disp->status != DispatchStatus::ACTIVE) {
        return {
            false,
            "INVALID_DISPATCH_STATE",
            "Cannot recalculate route for inactive dispatch '" + dispatchId + "'.",
            std::nullopt,
            ""
        };
    }

    if (!graph_) {
        return {
            false,
            "GRAPH_UNINITIALIZED",
            "City graph not available.",
            std::nullopt,
            ""
        };
    }

    RouteResult newRoute = DijkstraRouter::findShortestPath(*graph_, disp->startLocationId, disp->targetLocationId, trace);

    if (!newRoute.routeFound) {
        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "DispatchEngine";
            ev.operation = "route_recalculation_failed";
            ev.entityId = dispatchId;
            ev.metadata = {
                {"dispatchId", dispatchId},
                {"reason", "no_passable_route"}
            };
            ev.explanation = "Route recalculation failed: destination unreachable under current road conditions.";
            trace->addEvent(std::move(ev));
        }
        return {
            false,
            "ROUTE_UNAVAILABLE",
            "No route exists from " + disp->startLocationId + " to " + disp->targetLocationId +
            " under current road network conditions.",
            std::nullopt,
            ""
        };
    }

    double oldTime = disp->travelTimeMinutes;
    disp->route = newRoute;
    disp->travelTimeMinutes = newRoute.totalTravelTimeMinutes;

    std::ostringstream oss;
    oss << "Route recalculated due to updated road conditions: travel time changed from "
        << std::fixed << std::setprecision(2) << oldTime << " to "
        << newRoute.totalTravelTimeMinutes << " min (" << newRoute.path.size() << " nodes).";
    disp->decisionRationale = oss.str();

    DispatchRecord updatedCopy = *disp;

    if (trace && trace->isEnabled()) {
        TraceEvent ev;
        ev.algorithm = "DispatchEngine";
        ev.operation = "route_recalculated";
        ev.entityId = dispatchId;
        ev.previousValue = oldTime;
        ev.newValue = newRoute.totalTravelTimeMinutes;
        ev.metadata = {
            {"dispatchId", dispatchId},
            {"oldTravelTimeMinutes", oldTime},
            {"newTravelTimeMinutes", newRoute.totalTravelTimeMinutes},
            {"newPath", newRoute.path}
        };
        ev.explanation = updatedCopy.decisionRationale;
        trace->addEvent(std::move(ev));
    }

    return {true, "", "", updatedCopy, updatedCopy.decisionRationale};
}

std::optional<DispatchRecord> DispatchService::getDispatch(
    const std::string& dispatchId
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const DispatchRecord* d = dispatches_.search(dispatchId);
    if (d) {
        return *d;
    }
    return std::nullopt;
}

std::vector<DispatchRecord> DispatchService::listDispatches(
    std::optional<DispatchStatus> filterStatus
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<DispatchRecord> result;
    result.reserve(dispatchOrder_.size());

    for (const auto& id : dispatchOrder_) {
        const DispatchRecord* d = dispatches_.search(id);
        if (!d) continue;

        if (filterStatus.has_value() && d->status != *filterStatus) {
            continue;
        }

        result.push_back(*d);
    }

    return result;
}

size_t DispatchService::activeCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& id : dispatchOrder_) {
        const DispatchRecord* d = dispatches_.search(id);
        if (d && d->status == DispatchStatus::ACTIVE) count++;
    }
    return count;
}

size_t DispatchService::completedCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& id : dispatchOrder_) {
        const DispatchRecord* d = dispatches_.search(id);
        if (d && d->status == DispatchStatus::COMPLETED) count++;
    }
    return count;
}

size_t DispatchService::totalCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return dispatches_.size();
}

} // namespace emergencygrid
