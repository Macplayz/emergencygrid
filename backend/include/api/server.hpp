#pragma once

#include <string>
#include <memory>
#include <mutex>
#include "httplib.h"
#include "json.hpp"
#include "core/types.hpp"
#include "core/trace.hpp"
#include "dsa/graph.hpp"
#include "dsa/dijkstra.hpp"
#include "simulation/seed_data.hpp"
#include "simulation/incident_service.hpp"
#include "simulation/unit_service.hpp"
#include "simulation/dispatch_service.hpp"

namespace emergencygrid {

class HttpServer {
public:
    HttpServer(std::string host = "127.0.0.1", int port = 8080);
    ~HttpServer();

    void registerRoutes();
    bool start();
    void stop();

    std::string getHost() const { return host_; }
    int getPort() const { return port_; }

    // Direct access for testing
    RoadGraph& getGraph() noexcept { return graph_; }
    const RoadGraph& getGraph() const noexcept { return graph_; }
    void resetGraph();

    IncidentService& getIncidentService() noexcept { return incidentService_; }
    const IncidentService& getIncidentService() const noexcept { return incidentService_; }

    UnitService& getUnitService() noexcept { return unitService_; }
    const UnitService& getUnitService() const noexcept { return unitService_; }

    DispatchService& getDispatchService() noexcept { return dispatchService_; }
    const DispatchService& getDispatchService() const noexcept { return dispatchService_; }

private:
    std::string host_;
    int port_;
    std::unique_ptr<httplib::Server> server_;
    SimulationStateDTO state_;
    RoadGraph graph_;
    IncidentService incidentService_;
    UnitService unitService_;
    DispatchService dispatchService_;
    mutable std::mutex graphMutex_;

    // Helpers
    void setupCors();
    void setupErrorHandlers();
    
    static nlohmann::json makeSuccessEnvelope(
        const nlohmann::json& data,
        const nlohmann::json& trace = nlohmann::json::array(),
        uint64_t tick = 0,
        double executionTimeMs = 0.15
    );
    static nlohmann::json makeErrorEnvelope(
        const std::string& code,
        const std::string& message,
        const nlohmann::json& details = nlohmann::json::object()
    );
};

} // namespace emergencygrid
