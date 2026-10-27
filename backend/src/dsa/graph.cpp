#include "dsa/graph.hpp"
#include <sstream>
#include <iomanip>

namespace emergencygrid {

bool RoadGraph::addLocation(const Location& loc) {
    if (loc.id.empty()) return false;
    if (locations_.find(loc.id) != locations_.end()) return false;

    locations_[loc.id] = loc;
    locationOrder_.push_back(loc.id);
    adjacency_[loc.id] = {};
    return true;
}

bool RoadGraph::hasLocation(const std::string& id) const {
    return locations_.find(id) != locations_.end();
}

const Location* RoadGraph::getLocation(const std::string& id) const {
    auto it = locations_.find(id);
    if (it != locations_.end()) {
        return &it->second;
    }
    return nullptr;
}

bool RoadGraph::addRoad(const RoadSegment& road) {
    if (road.id.empty()) return false;
    if (roads_.find(road.id) != roads_.end()) return false;
    if (!hasLocation(road.sourceId) || !hasLocation(road.targetId)) return false;
    if (road.baseLengthKm <= 0.0 || road.speedLimitKmh <= 0.0 || road.congestionFactor <= 0.0) return false;

    roads_[road.id] = road;
    roadOrder_.push_back(road.id);

    adjacency_[road.sourceId].push_back(road.id);
    if (road.isBidirectional && road.sourceId != road.targetId) {
        adjacency_[road.targetId].push_back(road.id);
    }
    return true;
}

bool RoadGraph::hasRoad(const std::string& id) const {
    return roads_.find(id) != roads_.end();
}

const RoadSegment* RoadGraph::getRoad(const std::string& id) const {
    auto it = roads_.find(id);
    if (it != roads_.end()) {
        return &it->second;
    }
    return nullptr;
}

RoadSegment* RoadGraph::getRoadMutable(const std::string& id) {
    auto it = roads_.find(id);
    if (it != roads_.end()) {
        return &it->second;
    }
    return nullptr;
}

bool RoadGraph::setRoadClosed(const std::string& roadId, bool isClosed, TraceCollector* trace) {
    auto* road = getRoadMutable(roadId);
    if (!road) return false;

    bool oldClosed = road->isClosed;
    road->isClosed = isClosed;

    if (trace && trace->isEnabled()) {
        TraceEvent ev;
        ev.algorithm = "Graph";
        ev.operation = "set_edge_status";
        ev.entityId = roadId;
        ev.from = road->sourceId;
        ev.to = road->targetId;
        ev.previousValue = oldClosed ? 1.0 : 0.0;
        ev.newValue = isClosed ? 1.0 : 0.0;
        ev.metadata = {
            {"roadId", roadId},
            {"isClosed", isClosed},
            {"baseTravelTimeMinutes", road->travelTimeMinutes()}
        };
        std::ostringstream oss;
        oss << "Road " << roadId << " (" << road->sourceId << " <-> " << road->targetId 
            << ") status updated to " << (isClosed ? "CLOSED" : "OPEN") << ".";
        ev.explanation = oss.str();
        trace->addEvent(std::move(ev));
    }

    return true;
}

bool RoadGraph::setRoadCongestion(const std::string& roadId, double congestionFactor, TraceCollector* trace) {
    if (congestionFactor <= 0.0) return false;
    auto* road = getRoadMutable(roadId);
    if (!road) return false;

    double oldTime = road->travelTimeMinutes();
    double oldFactor = road->congestionFactor;
    road->congestionFactor = congestionFactor;
    double newTime = road->travelTimeMinutes();

    if (trace && trace->isEnabled()) {
        TraceEvent ev;
        ev.algorithm = "Graph";
        ev.operation = "update_congestion";
        ev.entityId = roadId;
        ev.from = road->sourceId;
        ev.to = road->targetId;
        ev.previousValue = oldFactor;
        ev.newValue = congestionFactor;
        ev.metadata = {
            {"roadId", roadId},
            {"previousTravelTimeMinutes", oldTime},
            {"newTravelTimeMinutes", newTime}
        };
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "Road " << roadId << " congestion updated to " << congestionFactor 
            << "x (travel time changed from " << oldTime << "m to " << newTime << "m).";
        ev.explanation = oss.str();
        trace->addEvent(std::move(ev));
    }

    return true;
}

bool RoadGraph::updateRoadTravelTime(const std::string& roadId, double travelTimeMinutes, TraceCollector* trace) {
    if (travelTimeMinutes <= 0.0) return false;
    auto* road = getRoadMutable(roadId);
    if (!road) return false;

    // T = (baseLengthKm / speedLimitKmh) * congestionFactor * 60
    // => congestionFactor = travelTimeMinutes / ((baseLengthKm / speedLimitKmh) * 60)
    double baseMinutes = (road->baseLengthKm / road->speedLimitKmh) * 60.0;
    if (baseMinutes <= 0.0) return false;

    double newCongestion = travelTimeMinutes / baseMinutes;
    return setRoadCongestion(roadId, newCongestion, trace);
}

std::vector<DirectedEdge> RoadGraph::getOutgoingEdges(const std::string& fromLocationId) const {
    std::vector<DirectedEdge> result;
    auto it = adjacency_.find(fromLocationId);
    if (it == adjacency_.end()) return result;

    result.reserve(it->second.size());
    for (const auto& roadId : it->second) {
        auto rIt = roads_.find(roadId);
        if (rIt == roads_.end()) continue;

        const auto& road = rIt->second;
        std::string target;
        if (road.sourceId == fromLocationId) {
            target = road.targetId;
        } else if (road.isBidirectional && road.targetId == fromLocationId) {
            target = road.sourceId;
        } else {
            continue; // Not outgoing from fromLocationId
        }

        result.push_back(DirectedEdge{
            .roadId = road.id,
            .targetLocationId = target,
            .travelTimeMinutes = road.travelTimeMinutes(),
            .baseLengthKm = road.baseLengthKm,
            .isClosed = road.isClosed
        });
    }

    // Sort edges deterministically by targetLocationId, then by roadId
    std::sort(result.begin(), result.end(), [](const DirectedEdge& a, const DirectedEdge& b) {
        if (a.targetLocationId != b.targetLocationId) {
            return a.targetLocationId < b.targetLocationId;
        }
        return a.roadId < b.roadId;
    });

    return result;
}

RoadsSummary RoadGraph::getRoadsSummary() const {
    RoadsSummary summary;
    summary.totalRoads = static_cast<uint32_t>(roads_.size());
    summary.closedRoads = 0;
    summary.congestedRoads = 0;

    for (const auto& [_, road] : roads_) {
        if (road.isClosed) {
            summary.closedRoads++;
        }
        if (road.congestionFactor > 1.05) {
            summary.congestedRoads++;
        }
    }
    return summary;
}

nlohmann::json RoadGraph::toJson() const {
    nlohmann::json j;
    j["intersections"] = nlohmann::json::array();
    j["roads"] = nlohmann::json::array();

    for (const auto& locId : locationOrder_) {
        auto it = locations_.find(locId);
        if (it != locations_.end()) {
            j["intersections"].push_back(it->second.toJson());
        }
    }

    for (const auto& roadId : roadOrder_) {
        auto it = roads_.find(roadId);
        if (it != roads_.end()) {
            j["roads"].push_back(it->second.toJson());
        }
    }

    return j;
}

void RoadGraph::clear() noexcept {
    locations_.clear();
    locationOrder_.clear();
    roads_.clear();
    roadOrder_.clear();
    adjacency_.clear();
}

} // namespace emergencygrid
