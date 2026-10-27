#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "json.hpp"

namespace emergencygrid {

enum class Agency {
    EMS,
    FIRE,
    POLICE
};

inline std::string agencyToString(Agency a) {
    switch (a) {
        case Agency::EMS: return "EMS";
        case Agency::FIRE: return "FIRE";
        case Agency::POLICE: return "POLICE";
    }
    return "UNKNOWN";
}

enum class IncidentSeverity {
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3,
    CRITICAL = 4
};

inline std::string severityToString(IncidentSeverity s) {
    switch (s) {
        case IncidentSeverity::LOW: return "LOW";
        case IncidentSeverity::MEDIUM: return "MEDIUM";
        case IncidentSeverity::HIGH: return "HIGH";
        case IncidentSeverity::CRITICAL: return "CRITICAL";
    }
    return "UNKNOWN";
}

inline int severityToInt(IncidentSeverity s) noexcept {
    return static_cast<int>(s);
}

inline std::optional<IncidentSeverity> parseSeverity(const std::string& str) {
    std::string upper = str;
    for (char& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (upper == "LOW" || upper == "1") return IncidentSeverity::LOW;
    if (upper == "MEDIUM" || upper == "2") return IncidentSeverity::MEDIUM;
    if (upper == "HIGH" || upper == "3") return IncidentSeverity::HIGH;
    if (upper == "CRITICAL" || upper == "4") return IncidentSeverity::CRITICAL;
    return std::nullopt;
}

inline std::optional<IncidentSeverity> parseSeverity(int val) {
    if (val == 1) return IncidentSeverity::LOW;
    if (val == 2) return IncidentSeverity::MEDIUM;
    if (val == 3) return IncidentSeverity::HIGH;
    if (val == 4) return IncidentSeverity::CRITICAL;
    return std::nullopt;
}

enum class IncidentType {
    MEDICAL,
    FIRE,
    POLICE
};

inline std::string incidentTypeToString(IncidentType t) {
    switch (t) {
        case IncidentType::MEDICAL: return "MEDICAL";
        case IncidentType::FIRE: return "FIRE";
        case IncidentType::POLICE: return "POLICE";
    }
    return "UNKNOWN";
}

inline std::optional<IncidentType> parseIncidentType(const std::string& str) {
    std::string upper = str;
    for (char& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (upper == "MEDICAL" || upper == "EMS" || upper == "AMBULANCE") return IncidentType::MEDICAL;
    if (upper == "FIRE" || upper == "FIRE_RESCUE") return IncidentType::FIRE;
    if (upper == "POLICE") return IncidentType::POLICE;
    return std::nullopt;
}

using ServiceType = IncidentType;

enum class IncidentStatus {
    PENDING,
    ASSIGNED,
    DISPATCHED,
    ON_SCENE,
    RESOLVED,
    CANCELLED
};

inline std::string incidentStatusToString(IncidentStatus s) {
    switch (s) {
        case IncidentStatus::PENDING: return "PENDING";
        case IncidentStatus::ASSIGNED: return "ASSIGNED";
        case IncidentStatus::DISPATCHED: return "DISPATCHED";
        case IncidentStatus::ON_SCENE: return "ON_SCENE";
        case IncidentStatus::RESOLVED: return "RESOLVED";
        case IncidentStatus::CANCELLED: return "CANCELLED";
    }
    return "UNKNOWN";
}

inline std::optional<IncidentStatus> parseIncidentStatus(const std::string& str) {
    std::string upper = str;
    for (char& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (upper == "PENDING") return IncidentStatus::PENDING;
    if (upper == "ASSIGNED") return IncidentStatus::ASSIGNED;
    if (upper == "DISPATCHED") return IncidentStatus::DISPATCHED;
    if (upper == "ON_SCENE") return IncidentStatus::ON_SCENE;
    if (upper == "RESOLVED") return IncidentStatus::RESOLVED;
    if (upper == "CANCELLED") return IncidentStatus::CANCELLED;
    return std::nullopt;
}

struct Incident {
    std::string id;
    IncidentType type{IncidentType::MEDICAL};
    IncidentSeverity severity{IncidentSeverity::MEDIUM};
    std::string locationId;
    uint64_t arrivalSequence{0};
    uint64_t reportedTick{0};
    IncidentStatus status{IncidentStatus::PENDING};
    double computedPriority{0.0};
    std::string description;

    std::vector<std::string> requiredCapabilities;

    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"type", incidentTypeToString(type)},
            {"serviceType", incidentTypeToString(type)},
            {"severity", static_cast<int>(severity)},
            {"severityLabel", severityToString(severity)},
            {"locationId", locationId},
            {"arrivalSequence", arrivalSequence},
            {"reportedTick", reportedTick},
            {"status", incidentStatusToString(status)},
            {"computedPriority", computedPriority},
            {"description", description},
            {"requiredCapabilities", requiredCapabilities}
        };
    }
};

enum class UnitStatus {
    AVAILABLE,
    DISPATCHED,
    RESPONDING,
    BUSY,
    OUT_OF_SERVICE
};

inline std::string unitStatusToString(UnitStatus s) {
    switch (s) {
        case UnitStatus::AVAILABLE: return "AVAILABLE";
        case UnitStatus::DISPATCHED: return "DISPATCHED";
        case UnitStatus::RESPONDING: return "RESPONDING";
        case UnitStatus::BUSY: return "BUSY";
        case UnitStatus::OUT_OF_SERVICE: return "OUT_OF_SERVICE";
    }
    return "UNKNOWN";
}

inline std::optional<UnitStatus> parseUnitStatus(const std::string& str) {
    std::string upper = str;
    for (char& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (upper == "AVAILABLE") return UnitStatus::AVAILABLE;
    if (upper == "DISPATCHED") return UnitStatus::DISPATCHED;
    if (upper == "RESPONDING") return UnitStatus::RESPONDING;
    if (upper == "BUSY") return UnitStatus::BUSY;
    if (upper == "OUT_OF_SERVICE" || upper == "UNAVAILABLE") return UnitStatus::OUT_OF_SERVICE;
    return std::nullopt;
}

inline UnitStatus stringToUnitStatus(const std::string& str) {
    return parseUnitStatus(str).value_or(UnitStatus::AVAILABLE);
}

struct EmergencyUnit {
    std::string id;
    std::string name;
    std::string callSign;
    IncidentType serviceType{IncidentType::MEDICAL};
    std::string locationId;
    UnitStatus status{UnitStatus::AVAILABLE};
    std::vector<std::string> capabilities;
    std::string activeIncidentId;

    bool hasCapability(const std::string& cap) const {
        std::string capUpper = cap;
        for (char& c : capUpper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        for (const auto& c : capabilities) {
            std::string existingUpper = c;
            for (char& ch : existingUpper) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            if (existingUpper == capUpper) return true;
        }
        return false;
    }

    bool satisfiesCapabilities(const std::vector<std::string>& reqCaps) const {
        for (const auto& req : reqCaps) {
            if (!hasCapability(req)) return false;
        }
        return true;
    }

    nlohmann::json toJson() const {
        std::string displayName = !callSign.empty() ? callSign : (!name.empty() ? name : id);
        return {
            {"id", id},
            {"name", displayName},
            {"callSign", displayName},
            {"serviceType", incidentTypeToString(serviceType)},
            {"agency", incidentTypeToString(serviceType)},
            {"locationId", locationId},
            {"status", unitStatusToString(status)},
            {"capabilities", capabilities},
            {"activeIncidentId", activeIncidentId.empty() ? nullptr : nlohmann::json(activeIncidentId)}
        };
    }
};

struct RouteResult {
    std::string sourceId;
    std::string destinationId;
    bool routeFound{false};
    std::vector<std::string> path; // Ordered location IDs
    std::vector<std::string> roadIds; // Ordered road segment IDs
    std::vector<std::string> shortestPathTreeRoadIds; // All road IDs in the Shortest-Path Tree (SPT)
    std::vector<nlohmann::json> shortestPathTree; // Explicit SPT edges: [{from, to, roadId, travelTimeMinutes, lengthKm}]
    double totalTravelTimeMinutes{0.0};
    double totalDistanceKm{0.0};
    uint32_t nodesSettled{0};
    uint32_t nodesExplored{0};
    uint32_t edgesExamined{0};
    uint32_t relaxations{0};
    std::string algorithm{"Dijkstra"};

    nlohmann::json toJson() const {
        return {
            {"sourceId", sourceId},
            {"destinationId", destinationId},
            {"routeFound", routeFound},
            {"path", path},
            {"roadIds", roadIds},
            {"shortestPathTreeRoadIds", shortestPathTreeRoadIds},
            {"shortestPathTree", shortestPathTree},
            {"totalTravelTimeMinutes", totalTravelTimeMinutes},
            {"totalDistanceKm", totalDistanceKm},
            {"nodesSettled", nodesSettled},
            {"nodesExplored", nodesExplored},
            {"edgesExamined", edgesExamined},
            {"relaxations", relaxations},
            {"algorithm", algorithm}
        };
    }
};

enum class DispatchStatus {
    ACTIVE,
    COMPLETED,
    CANCELLED
};

inline std::string dispatchStatusToString(DispatchStatus s) {
    switch (s) {
        case DispatchStatus::ACTIVE: return "ACTIVE";
        case DispatchStatus::COMPLETED: return "COMPLETED";
        case DispatchStatus::CANCELLED: return "CANCELLED";
    }
    return "UNKNOWN";
}

inline std::optional<DispatchStatus> parseDispatchStatus(const std::string& str) {
    std::string upper = str;
    for (char& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (upper == "ACTIVE") return DispatchStatus::ACTIVE;
    if (upper == "COMPLETED") return DispatchStatus::COMPLETED;
    if (upper == "CANCELLED") return DispatchStatus::CANCELLED;
    return std::nullopt;
}

struct DispatchRecord {
    std::string id;
    std::string incidentId;
    std::string unitId;
    std::string startLocationId;
    std::string targetLocationId;
    RouteResult route;
    double travelTimeMinutes{0.0};
    uint64_t dispatchedTick{0};
    std::optional<uint64_t> completedTick{std::nullopt};
    DispatchStatus status{DispatchStatus::ACTIVE};
    std::string decisionRationale;

    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"incidentId", incidentId},
            {"unitId", unitId},
            {"startLocationId", startLocationId},
            {"targetLocationId", targetLocationId},
            {"route", route.toJson()},
            {"travelTimeMinutes", travelTimeMinutes},
            {"dispatchedTick", dispatchedTick},
            {"completedTick", completedTick.has_value() ? nlohmann::json(*completedTick) : nullptr},
            {"status", dispatchStatusToString(status)},
            {"decisionRationale", decisionRationale}
        };
    }
};

struct Location {
    std::string id;
    std::string name;
    double x{0.0};
    double y{0.0};
    std::string zone;

    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"name", name},
            {"x", x},
            {"y", y},
            {"zone", zone}
        };
    }
};

struct RoadSegment {
    std::string id;
    std::string sourceId;
    std::string targetId;
    double baseLengthKm{1.0};
    double speedLimitKmh{50.0};
    double congestionFactor{1.0};
    bool isClosed{false};
    bool isBidirectional{true};

    double travelTimeMinutes() const noexcept {
        if (speedLimitKmh <= 0.0) return 0.0;
        return (baseLengthKm / speedLimitKmh) * congestionFactor * 60.0;
    }

    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"source", sourceId},
            {"target", targetId},
            {"sourceId", sourceId},
            {"targetId", targetId},
            {"baseLengthKm", baseLengthKm},
            {"speedLimitKmh", speedLimitKmh},
            {"congestionFactor", congestionFactor},
            {"isClosed", isClosed},
            {"isBidirectional", isBidirectional},
            {"travelTimeMinutes", travelTimeMinutes()}
        };
    }
};

struct SimulationMetrics {
    uint32_t totalIncidents{0};
    uint32_t pendingIncidents{0};
    uint32_t activeResponses{0};
    uint32_t completedIncidents{0};
    uint32_t availableUnits{0};
    uint32_t totalUnits{0};
};

struct RoadsSummary {
    uint32_t totalRoads{26};
    uint32_t closedRoads{0};
    uint32_t congestedRoads{0};
};

struct SimulationStateDTO {
    uint64_t tick{0};
    std::string status{"IDLE"};
    SimulationMetrics metrics;
    RoadsSummary roadsSummary;
};

} // namespace emergencygrid
