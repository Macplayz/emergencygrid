#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <stdexcept>
#include <algorithm>
#include "core/types.hpp"
#include "core/trace.hpp"

namespace emergencygrid {

struct DirectedEdge {
    std::string roadId;
    std::string targetLocationId;
    double travelTimeMinutes{0.0};
    double baseLengthKm{0.0};
    bool isClosed{false};
};

class RoadGraph {
public:
    RoadGraph() = default;

    // Location operations
    bool addLocation(const Location& loc);
    bool hasLocation(const std::string& id) const;
    const Location* getLocation(const std::string& id) const;
    const std::vector<std::string>& getLocationIds() const noexcept { return locationOrder_; }
    size_t locationCount() const noexcept { return locations_.size(); }

    // Road operations
    bool addRoad(const RoadSegment& road);
    bool hasRoad(const std::string& id) const;
    const RoadSegment* getRoad(const std::string& id) const;
    RoadSegment* getRoadMutable(const std::string& id);
    const std::vector<std::string>& getRoadIds() const noexcept { return roadOrder_; }
    size_t roadCount() const noexcept { return roads_.size(); }

    // Dynamic state updates (closures, congestion, travel time)
    bool setRoadClosed(const std::string& roadId, bool isClosed, TraceCollector* trace = nullptr);
    bool setRoadCongestion(const std::string& roadId, double congestionFactor, TraceCollector* trace = nullptr);
    bool updateRoadTravelTime(const std::string& roadId, double travelTimeMinutes, TraceCollector* trace = nullptr);

    // Graph traversal
    std::vector<DirectedEdge> getOutgoingEdges(const std::string& fromLocationId) const;

    // Metrics & Serialization
    RoadsSummary getRoadsSummary() const;
    nlohmann::json toJson() const;
    void clear() noexcept;

private:
    std::unordered_map<std::string, Location> locations_;
    std::vector<std::string> locationOrder_;

    std::unordered_map<std::string, RoadSegment> roads_;
    std::vector<std::string> roadOrder_;

    // Maps locationId -> list of road IDs touching this location
    std::unordered_map<std::string, std::vector<std::string>> adjacency_;
};

} // namespace emergencygrid
