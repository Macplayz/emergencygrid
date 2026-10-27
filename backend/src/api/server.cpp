#include "api/server.hpp"
#include <iostream>
#include <chrono>

namespace emergencygrid {

HttpServer::HttpServer(std::string host, int port)
    : host_(std::move(host)), port_(port), server_(std::make_unique<httplib::Server>()) {
    // Initialize default seed graph and services
    graph_ = createMetroGrid16();
    incidentService_.setGraph(&graph_);
    unitService_.setGraph(&graph_);
    dispatchService_.init(&incidentService_, &unitService_, &graph_);

    // Register default seed units
    for (const auto& u : createDefaultUnits()) {
        unitService_.registerUnit(u);
    }

    state_.tick = 0;
    state_.status = "INITIALIZED";
    state_.metrics = {
        .totalIncidents = 0,
        .pendingIncidents = 0,
        .activeResponses = 0,
        .completedIncidents = 0,
        .availableUnits = static_cast<uint32_t>(unitService_.availableCount()),
        .totalUnits = static_cast<uint32_t>(unitService_.totalCount())
    };
    state_.roadsSummary = graph_.getRoadsSummary();
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::resetGraph() {
    std::lock_guard<std::mutex> lock(graphMutex_);
    graph_ = createMetroGrid16();
    incidentService_.reset();
    incidentService_.setGraph(&graph_);
    unitService_.reset();
    unitService_.setGraph(&graph_);
    dispatchService_.reset();
    dispatchService_.init(&incidentService_, &unitService_, &graph_);

    for (const auto& u : createDefaultUnits()) {
        unitService_.registerUnit(u);
    }

    state_.roadsSummary = graph_.getRoadsSummary();
    state_.metrics.totalIncidents = 0;
    state_.metrics.pendingIncidents = 0;
    state_.metrics.activeResponses = 0;
    state_.metrics.completedIncidents = 0;
    state_.metrics.availableUnits = static_cast<uint32_t>(unitService_.availableCount());
    state_.metrics.totalUnits = static_cast<uint32_t>(unitService_.totalCount());
}

nlohmann::json HttpServer::makeSuccessEnvelope(
    const nlohmann::json& data,
    const nlohmann::json& trace,
    uint64_t tick,
    double executionTimeMs
) {
    return {
        {"success", true},
        {"data", data},
        {"trace", trace},
        {"meta", {
            {"simulationTick", tick},
            {"executionTimeMs", executionTimeMs}
        }}
    };
}

nlohmann::json HttpServer::makeErrorEnvelope(
    const std::string& code,
    const std::string& message,
    const nlohmann::json& details
) {
    nlohmann::json err = {
        {"code", code},
        {"message", message}
    };
    if (!details.empty()) {
        err["details"] = details;
    }
    return {
        {"success", false},
        {"error", err}
    };
}

void HttpServer::setupCors() {
    server_->set_post_routing_handler([](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
    });

    server_->Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("", "text/plain");
        res.status = 204;
    });
}

void HttpServer::setupErrorHandlers() {
    server_->set_error_handler([](const httplib::Request&, httplib::Response& res) {
        if (!res.body.empty()) {
            return;
        }
        std::string code = "INTERNAL_ERROR";
        std::string msg = "An internal server error occurred.";
        if (res.status == 404) {
            code = "NOT_FOUND";
            msg = "The requested resource or endpoint was not found.";
        } else if (res.status == 400) {
            code = "BAD_REQUEST";
            msg = "The request payload or arguments were invalid.";
        }
        res.set_content(makeErrorEnvelope(code, msg).dump(2), "application/json");
    });
}

void HttpServer::registerRoutes() {
    setupCors();
    setupErrorHandlers();

    // 1. Health check: GET /api/v1/health
    server_->Get("/api/v1/health", [](const httplib::Request&, httplib::Response& res) {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

        nlohmann::json healthData = {
            {"status", "UP"},
            {"service", "EmergencyGrid Simulation Core"},
            {"version", "1.1.0"},
            {"standard", "C++20"},
            {"timestampMs", ms}
        };

        res.set_content(makeSuccessEnvelope(healthData, nlohmann::json::array(), 0, 0.05).dump(2), "application/json");
        res.status = 200;
    });

    // 2. Simulation state snapshot: GET /api/v1/simulation/state
    server_->Get("/api/v1/simulation/state", [this](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(graphMutex_);
        auto summary = graph_.getRoadsSummary();
        state_.roadsSummary = summary;
        state_.metrics.totalIncidents = static_cast<uint32_t>(incidentService_.totalCount());
        state_.metrics.pendingIncidents = static_cast<uint32_t>(incidentService_.pendingCount());
        state_.metrics.activeResponses = static_cast<uint32_t>(dispatchService_.activeCount());
        state_.metrics.completedIncidents = static_cast<uint32_t>(incidentService_.resolvedCount());
        state_.metrics.availableUnits = static_cast<uint32_t>(unitService_.availableCount());
        state_.metrics.totalUnits = static_cast<uint32_t>(unitService_.totalCount());

        nlohmann::json stateData = {
            {"tick", state_.tick},
            {"status", state_.status},
            {"metrics", {
                {"totalIncidents", state_.metrics.totalIncidents},
                {"pendingIncidents", state_.metrics.pendingIncidents},
                {"activeResponses", state_.metrics.activeResponses},
                {"completedIncidents", state_.metrics.completedIncidents},
                {"availableUnits", state_.metrics.availableUnits},
                {"totalUnits", state_.metrics.totalUnits}
            }},
            {"roadsSummary", {
                {"totalRoads", summary.totalRoads},
                {"closedRoads", summary.closedRoads},
                {"congestedRoads", summary.congestedRoads}
            }}
        };

        res.set_content(makeSuccessEnvelope(stateData, nlohmann::json::array(), state_.tick, 0.08).dump(2), "application/json");
        res.status = 200;
    });

    // 3. Reset simulation and graph: POST /api/v1/simulation/reset & POST /api/v1/city/reset
    auto handleReset = [this](const httplib::Request&, httplib::Response& res) {
        resetGraph();
        std::lock_guard<std::mutex> lock(graphMutex_);
        auto summary = graph_.getRoadsSummary();

        nlohmann::json resetData = {
            {"message", "Simulation network successfully reset to MetroGrid-16 seed topology."},
            {"intersections", graph_.locationCount()},
            {"roads", summary.totalRoads},
            {"closedRoads", summary.closedRoads}
        };

        res.set_content(makeSuccessEnvelope(resetData, nlohmann::json::array(), state_.tick, 0.12).dump(2), "application/json");
        res.status = 200;
    };
    server_->Post("/api/v1/simulation/reset", handleReset);
    server_->Post("/api/v1/city/reset", handleReset);

    // 4. Retrieve city graph: GET /api/v1/city/graph
    server_->Get("/api/v1/city/graph", [this](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(graphMutex_);
        nlohmann::json graphData = graph_.toJson();
        res.set_content(makeSuccessEnvelope(graphData, nlohmann::json::array(), state_.tick, 0.15).dump(2), "application/json");
        res.status = 200;
    });

    // 5. Calculate shortest route: POST /api/v1/routing/calculate & POST /api/v1/route
    auto handleRouting = [this](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json reqJson;
        try {
            reqJson = nlohmann::json::parse(req.body);
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(makeErrorEnvelope("INVALID_JSON", std::string("Malformed JSON body: ") + e.what()).dump(2), "application/json");
            return;
        }

        if (!reqJson.contains("sourceId") || !reqJson.contains("destinationId") ||
            !reqJson["sourceId"].is_string() || !reqJson["destinationId"].is_string()) {
            res.status = 400;
            res.set_content(makeErrorEnvelope("INVALID_ARGUMENT", "Fields 'sourceId' and 'destinationId' are required string properties.").dump(2), "application/json");
            return;
        }

        std::string sourceId = reqJson["sourceId"].get<std::string>();
        std::string destinationId = reqJson["destinationId"].get<std::string>();
        bool enableTrace = reqJson.value("enableTrace", false);

        auto startTime = std::chrono::high_resolution_clock::now();

        std::lock_guard<std::mutex> lock(graphMutex_);

        if (!graph_.hasLocation(sourceId)) {
            res.status = 404;
            res.set_content(makeErrorEnvelope("LOCATION_NOT_FOUND", "Source location '" + sourceId + "' does not exist in graph.").dump(2), "application/json");
            return;
        }

        if (!graph_.hasLocation(destinationId)) {
            res.status = 404;
            res.set_content(makeErrorEnvelope("LOCATION_NOT_FOUND", "Destination location '" + destinationId + "' does not exist in graph.").dump(2), "application/json");
            return;
        }

        TraceCollector trace(enableTrace);
        RouteResult route = DijkstraRouter::findShortestPath(graph_, sourceId, destinationId, enableTrace ? &trace : nullptr);

        auto endTime = std::chrono::high_resolution_clock::now();
        double elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        nlohmann::json responseData = {
            {"route", route.toJson()}
        };

        res.status = 200;
        res.set_content(makeSuccessEnvelope(responseData, trace.toJsonArray(), state_.tick, elapsedMs).dump(2), "application/json");
    };

    server_->Post("/api/v1/routing/calculate", handleRouting);
    server_->Post("/api/v1/route", handleRouting);

    // 6. Update road status / travel time: PUT & POST /api/v1/city/roads/:id
    auto handleRoadUpdate = [this](const httplib::Request& req, httplib::Response& res) {
        if (req.matches.size() < 2) {
            res.status = 400;
            res.set_content(makeErrorEnvelope("INVALID_PATH", "Road ID parameter missing from request path.").dump(2), "application/json");
            return;
        }

        std::string roadId = req.matches[1];

        nlohmann::json reqJson;
        try {
            reqJson = nlohmann::json::parse(req.body);
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(makeErrorEnvelope("INVALID_JSON", std::string("Malformed JSON body: ") + e.what()).dump(2), "application/json");
            return;
        }

        bool enableTrace = reqJson.value("enableTrace", false);

        std::lock_guard<std::mutex> lock(graphMutex_);

        if (!graph_.hasRoad(roadId)) {
            res.status = 404;
            res.set_content(makeErrorEnvelope("ROAD_NOT_FOUND", "Road with ID '" + roadId + "' does not exist.").dump(2), "application/json");
            return;
        }

        TraceCollector trace(enableTrace);

        // Closure update
        if (reqJson.contains("isClosed")) {
            if (!reqJson["isClosed"].is_boolean()) {
                res.status = 400;
                res.set_content(makeErrorEnvelope("INVALID_ARGUMENT", "Field 'isClosed' must be a boolean.").dump(2), "application/json");
                return;
            }
            bool isClosed = reqJson["isClosed"].get<bool>();
            graph_.setRoadClosed(roadId, isClosed, enableTrace ? &trace : nullptr);
        }

        // Congestion factor update
        if (reqJson.contains("congestionFactor")) {
            if (!reqJson["congestionFactor"].is_number() || reqJson["congestionFactor"].get<double>() <= 0.0) {
                res.status = 400;
                res.set_content(makeErrorEnvelope("INVALID_WEIGHT", "Field 'congestionFactor' must be a positive number (> 0.0).").dump(2), "application/json");
                return;
            }
            double factor = reqJson["congestionFactor"].get<double>();
            graph_.setRoadCongestion(roadId, factor, enableTrace ? &trace : nullptr);
        }
        // Direct travel time update
        else if (reqJson.contains("travelTimeMinutes")) {
            if (!reqJson["travelTimeMinutes"].is_number() || reqJson["travelTimeMinutes"].get<double>() <= 0.0) {
                res.status = 400;
                res.set_content(makeErrorEnvelope("INVALID_WEIGHT", "Field 'travelTimeMinutes' must be a positive number (> 0.0).").dump(2), "application/json");
                return;
            }
            double timeMins = reqJson["travelTimeMinutes"].get<double>();
            graph_.updateRoadTravelTime(roadId, timeMins, enableTrace ? &trace : nullptr);
        }

        state_.roadsSummary = graph_.getRoadsSummary();
        const auto* updated = graph_.getRoad(roadId);

        res.status = 200;
        res.set_content(makeSuccessEnvelope(updated->toJson(), trace.toJsonArray(), state_.tick).dump(2), "application/json");
    };

    server_->Put(R"(/api/v1/city/roads/([^/]+))", handleRoadUpdate);
    server_->Post(R"(/api/v1/city/roads/([^/]+))", handleRoadUpdate);

    // --- Incident Priority & Heap Management Endpoints ---

    // 7. Create incident: POST /api/v1/incidents & POST /api/incidents
    auto handleCreateIncident = [this](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json reqJson;
        try {
            reqJson = nlohmann::json::parse(req.body);
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(makeErrorEnvelope("INVALID_JSON", std::string("Malformed JSON body: ") + e.what()).dump(2), "application/json");
            return;
        }

        if (!reqJson.contains("locationId") || !reqJson["locationId"].is_string()) {
            res.status = 400;
            res.set_content(makeErrorEnvelope("INVALID_ARGUMENT", "Field 'locationId' is required and must be a string.").dump(2), "application/json");
            return;
        }

        std::string locationId = reqJson["locationId"].get<std::string>();
        std::string incidentId = reqJson.value("id", "");
        std::string description = reqJson.value("description", "");
        bool enableTrace = reqJson.value("enableTrace", false);

        std::string typeStr = reqJson.value("type", reqJson.value("serviceType", ""));
        if (typeStr.empty()) {
            res.status = 400;
            res.set_content(makeErrorEnvelope("INVALID_ARGUMENT", "Field 'type' or 'serviceType' is required (e.g. MEDICAL, FIRE, POLICE).").dump(2), "application/json");
            return;
        }

        int severityVal = 2;
        if (reqJson.contains("severity")) {
            if (reqJson["severity"].is_number_integer()) {
                severityVal = reqJson["severity"].get<int>();
            } else if (reqJson["severity"].is_string()) {
                auto parsedSev = parseSeverity(reqJson["severity"].get<std::string>());
                if (parsedSev.has_value()) {
                    severityVal = severityToInt(*parsedSev);
                } else {
                    res.status = 400;
                    res.set_content(makeErrorEnvelope("INVALID_SEVERITY", "Invalid severity string '" + reqJson["severity"].get<std::string>() + "'.").dump(2), "application/json");
                    return;
                }
            } else {
                res.status = 400;
                res.set_content(makeErrorEnvelope("INVALID_SEVERITY", "Field 'severity' must be an integer (1-4) or severity name.").dump(2), "application/json");
                return;
            }
        }

        std::vector<std::string> reqCaps;
        if (reqJson.contains("requiredCapabilities") && reqJson["requiredCapabilities"].is_array()) {
            for (const auto& cap : reqJson["requiredCapabilities"]) {
                if (cap.is_string()) {
                    reqCaps.push_back(cap.get<std::string>());
                }
            }
        }

        CreateIncidentRequest incReq{
            .id = incidentId,
            .locationId = locationId,
            .type = typeStr,
            .severity = severityVal,
            .description = description,
            .requiredCapabilities = reqCaps,
            .enableTrace = enableTrace
        };

        TraceCollector trace(enableTrace);
        auto result = incidentService_.createIncident(incReq, enableTrace ? &trace : nullptr);

        if (!result.success) {
            if (result.errorCode == "LOCATION_NOT_FOUND") {
                res.status = 404;
            } else if (result.errorCode == "DUPLICATE_INCIDENT_ID") {
                res.status = 409;
            } else {
                res.status = 400;
            }
            res.set_content(makeErrorEnvelope(result.errorCode, result.errorMessage).dump(2), "application/json");
            return;
        }

        res.status = 201;
        res.set_content(makeSuccessEnvelope(result.incident->toJson(), trace.toJsonArray(), state_.tick, 0.20).dump(2), "application/json");
    };

    server_->Post("/api/v1/incidents", handleCreateIncident);
    server_->Post("/api/incidents", handleCreateIncident);

    // 8. Inspect next pending incident: GET /api/v1/incidents/next & GET /api/incidents/next
    auto handleInspectNext = [this](const httplib::Request& req, httplib::Response& res) {
        bool enableTrace = (req.get_param_value("enableTrace") == "true");

        TraceCollector trace(enableTrace);
        auto nextOpt = incidentService_.peekNext(enableTrace ? &trace : nullptr);

        if (!nextOpt.has_value()) {
            res.status = 404;
            res.set_content(makeErrorEnvelope("EMPTY_QUEUE", "No pending incidents in triage queue.").dump(2), "application/json");
            return;
        }

        res.status = 200;
        res.set_content(makeSuccessEnvelope(nextOpt->toJson(), trace.toJsonArray(), state_.tick, 0.10).dump(2), "application/json");
    };

    server_->Get("/api/v1/incidents/next", handleInspectNext);
    server_->Get("/api/incidents/next", handleInspectNext);

    // 9. Select next pending incident for dispatch: POST /api/v1/incidents/next/select & POST /api/incidents/next/select
    auto handleSelectNext = [this](const httplib::Request& req, httplib::Response& res) {
        bool enableTrace = false;
        if (!req.body.empty()) {
            try {
                auto j = nlohmann::json::parse(req.body);
                enableTrace = j.value("enableTrace", false);
            } catch (...) {}
        }
        if (req.get_param_value("enableTrace") == "true") {
            enableTrace = true;
        }

        TraceCollector trace(enableTrace);
        auto result = incidentService_.selectNext(enableTrace ? &trace : nullptr);

        if (!result.success) {
            res.status = 404;
            res.set_content(makeErrorEnvelope(result.errorCode, result.errorMessage).dump(2), "application/json");
            return;
        }

        res.status = 200;
        res.set_content(makeSuccessEnvelope(result.incident->toJson(), trace.toJsonArray(), state_.tick, 0.15).dump(2), "application/json");
    };

    server_->Post("/api/v1/incidents/next/select", handleSelectNext);
    server_->Post("/api/incidents/next/select", handleSelectNext);

    // 10. Cancel incident: POST /api/v1/incidents/:id/cancel & POST /api/incidents/:id/cancel
    auto handleCancelIncident = [this](const httplib::Request& req, httplib::Response& res) {
        if (req.matches.size() < 2) {
            res.status = 400;
            res.set_content(makeErrorEnvelope("INVALID_PATH", "Incident ID parameter missing from path.").dump(2), "application/json");
            return;
        }

        std::string incidentId = req.matches[1];
        bool enableTrace = false;
        if (!req.body.empty()) {
            try {
                auto j = nlohmann::json::parse(req.body);
                enableTrace = j.value("enableTrace", false);
            } catch (...) {}
        }
        if (req.get_param_value("enableTrace") == "true") {
            enableTrace = true;
        }

        TraceCollector trace(enableTrace);
        auto result = incidentService_.cancelIncident(incidentId, enableTrace ? &trace : nullptr);

        if (!result.success) {
            if (result.errorCode == "INCIDENT_NOT_FOUND") {
                res.status = 404;
            } else if (result.errorCode == "ALREADY_CANCELLED" || result.errorCode == "INVALID_STATE_TRANSITION") {
                res.status = 409;
            } else {
                res.status = 400;
            }
            res.set_content(makeErrorEnvelope(result.errorCode, result.errorMessage).dump(2), "application/json");
            return;
        }

        res.status = 200;
        res.set_content(makeSuccessEnvelope(result.incident->toJson(), trace.toJsonArray(), state_.tick, 0.12).dump(2), "application/json");
    };

    server_->Post(R"(/api/v1/incidents/([^/]+)/cancel)", handleCancelIncident);
    server_->Post(R"(/api/incidents/([^/]+)/cancel)", handleCancelIncident);

    // 11. List incidents: GET /api/v1/incidents & GET /api/incidents
    auto handleListIncidents = [this](const httplib::Request& req, httplib::Response& res) {
        std::optional<IncidentStatus> statusFilter = std::nullopt;
        if (req.has_param("status")) {
            std::string statusStr = req.get_param_value("status");
            statusFilter = parseIncidentStatus(statusStr);
            if (!statusFilter.has_value()) {
                res.status = 400;
                res.set_content(makeErrorEnvelope("INVALID_ARGUMENT", "Invalid status filter '" + statusStr + "'.").dump(2), "application/json");
                return;
            }
        }

        auto incidents = incidentService_.listIncidents(statusFilter);
        nlohmann::json listArr = nlohmann::json::array();
        for (const auto& inc : incidents) {
            listArr.push_back(inc.toJson());
        }

        nlohmann::json responseData = {
            {"incidents", listArr},
            {"count", listArr.size()}
        };

        res.status = 200;
        res.set_content(makeSuccessEnvelope(responseData, nlohmann::json::array(), state_.tick, 0.10).dump(2), "application/json");
    };

    server_->Get("/api/v1/incidents", handleListIncidents);
    server_->Get("/api/incidents", handleListIncidents);

    // 11b. List incidents in sorted ID order (AVL Tree Inorder): GET /api/v1/incidents/sorted & GET /api/incidents/sorted
    auto handleListIncidentsSorted = [this](const httplib::Request& req, httplib::Response& res) {
        bool enableTrace = (req.get_param_value("enableTrace") == "true");
        std::optional<IncidentStatus> statusFilter = std::nullopt;
        if (req.has_param("status")) {
            std::string statusStr = req.get_param_value("status");
            statusFilter = parseIncidentStatus(statusStr);
            if (!statusFilter.has_value()) {
                res.status = 400;
                res.set_content(makeErrorEnvelope("INVALID_ARGUMENT", "Invalid status filter '" + statusStr + "'.").dump(2), "application/json");
                return;
            }
        }

        TraceCollector trace(enableTrace);
        auto incidents = incidentService_.listIncidentsSorted(statusFilter, enableTrace ? &trace : nullptr);
        nlohmann::json listArr = nlohmann::json::array();
        for (const auto& inc : incidents) {
            listArr.push_back(inc.toJson());
        }

        nlohmann::json responseData = {
            {"sortedIncidents", listArr},
            {"count", listArr.size()},
            {"orderingKey", "id"}
        };

        res.status = 200;
        res.set_content(makeSuccessEnvelope(responseData, trace.toJsonArray(), state_.tick, 0.10).dump(2), "application/json");
    };

    server_->Get("/api/v1/incidents/sorted", handleListIncidentsSorted);
    server_->Get("/api/incidents/sorted", handleListIncidentsSorted);

    // 12. Retrieve incident by ID (Custom HashTable lookup): GET /api/v1/incidents/:id & GET /api/incidents/:id
    auto handleGetIncidentById = [this](const httplib::Request& req, httplib::Response& res) {
        if (req.matches.size() < 2) {
            res.status = 400;
            res.set_content(makeErrorEnvelope("INVALID_PATH", "Incident ID parameter missing from path.").dump(2), "application/json");
            return;
        }

        std::string incidentId = req.matches[1];
        if (incidentId == "next" || incidentId == "sorted") {
            res.status = 400;
            res.set_content(makeErrorEnvelope("INVALID_ID", "Reserved route '" + incidentId + "' should be accessed directly.").dump(2), "application/json");
            return;
        }

        bool enableTrace = (req.get_param_value("enableTrace") == "true");
        TraceCollector trace(enableTrace);
        auto incOpt = incidentService_.getIncident(incidentId, enableTrace ? &trace : nullptr);
        if (!incOpt.has_value()) {
            res.status = 404;
            res.set_content(makeErrorEnvelope("INCIDENT_NOT_FOUND", "Incident with ID '" + incidentId + "' not found in Hash Table.").dump(2), "application/json");
            return;
        }

        res.status = 200;
        res.set_content(makeSuccessEnvelope(incOpt->toJson(), trace.toJsonArray(), state_.tick, 0.05).dump(2), "application/json");
    };

    server_->Get(R"(/api/v1/incidents/([^/]+))", handleGetIncidentById);
    server_->Get(R"(/api/incidents/([^/]+))", handleGetIncidentById);

    // 13. DSA Lab Heap Inspector Snapshot: GET /api/v1/dsa/heap/snapshot & GET /api/dsa/heap/snapshot
    auto handleHeapSnapshot = [this](const httplib::Request&, httplib::Response& res) {
        nlohmann::json snapshot = incidentService_.getHeapSnapshot();
        res.status = 200;
        res.set_content(makeSuccessEnvelope(snapshot, nlohmann::json::array(), state_.tick, 0.08).dump(2), "application/json");
    };

    server_->Get("/api/v1/dsa/heap/snapshot", handleHeapSnapshot);
    server_->Get("/api/dsa/heap/snapshot", handleHeapSnapshot);

    // 14. DSA Lab AVL Tree Inspector Snapshot: GET /api/v1/dsa/avl & GET /api/dsa/avl
    auto handleAvlSnapshot = [this](const httplib::Request&, httplib::Response& res) {
        nlohmann::json snapshot = incidentService_.getAvlSnapshot();
        res.status = 200;
        res.set_content(makeSuccessEnvelope(snapshot, nlohmann::json::array(), state_.tick, 0.08).dump(2), "application/json");
    };

    server_->Get("/api/v1/dsa/avl", handleAvlSnapshot);
    server_->Get("/api/dsa/avl", handleAvlSnapshot);
    server_->Get("/api/v1/dsa/avl/snapshot", handleAvlSnapshot);

    // 15. DSA Lab Hash Table Inspector Snapshot: GET /api/v1/dsa/hash-table & GET /api/dsa/hash-table
    auto handleHashTableStats = [this](const httplib::Request&, httplib::Response& res) {
        nlohmann::json stats = incidentService_.getHashTableSnapshot();
        res.status = 200;
        res.set_content(makeSuccessEnvelope(stats, nlohmann::json::array(), state_.tick, 0.08).dump(2), "application/json");
    };

    server_->Get("/api/v1/dsa/hash-table", handleHashTableStats);
    server_->Get("/api/dsa/hash-table", handleHashTableStats);
    server_->Get("/api/v1/dsa/hashtable/snapshot", handleHashTableStats);

    // 15b. Educational Isolated Binary Heap Sandbox: POST /api/v1/dsa/heap/demo
    server_->Post("/api/v1/dsa/heap/demo", [](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json reqJson = nlohmann::json::object();
        if (!req.body.empty()) {
            try { reqJson = nlohmann::json::parse(req.body); } catch (...) {}
        }

        std::string action = reqJson.value("action", "sample_sequence");
        BinaryHeap heap;
        TraceCollector trace(true);

        if (action == "sample_sequence") {
            Incident inc1{.id = "INC-101", .type = IncidentType::MEDICAL, .severity = IncidentSeverity::LOW, .locationId = "J1", .arrivalSequence = 1, .status = IncidentStatus::PENDING, .computedPriority = 1000.0, .description = "Minor medical call"};
            Incident inc2{.id = "INC-102", .type = IncidentType::FIRE, .severity = IncidentSeverity::HIGH, .locationId = "J5", .arrivalSequence = 2, .status = IncidentStatus::PENDING, .computedPriority = 3000.0, .description = "Structure fire"};
            Incident inc3{.id = "INC-103", .type = IncidentType::POLICE, .severity = IncidentSeverity::MEDIUM, .locationId = "J4", .arrivalSequence = 3, .status = IncidentStatus::PENDING, .computedPriority = 2000.0, .description = "Traffic disturbance"};
            Incident inc4{.id = "INC-104", .type = IncidentType::MEDICAL, .severity = IncidentSeverity::CRITICAL, .locationId = "J2", .arrivalSequence = 4, .status = IncidentStatus::PENDING, .computedPriority = 4000.0, .description = "Cardiac arrest"};

            heap.insert(inc1, &trace);
            heap.insert(inc2, &trace);
            heap.insert(inc3, &trace);
            heap.insert(inc4, &trace);
        } else if (action == "extract_max") {
            Incident inc1{.id = "INC-101", .type = IncidentType::MEDICAL, .severity = IncidentSeverity::LOW, .locationId = "J1", .arrivalSequence = 1, .status = IncidentStatus::PENDING, .computedPriority = 1000.0};
            Incident inc2{.id = "INC-102", .type = IncidentType::FIRE, .severity = IncidentSeverity::HIGH, .locationId = "J5", .arrivalSequence = 2, .status = IncidentStatus::PENDING, .computedPriority = 3000.0};
            Incident inc3{.id = "INC-103", .type = IncidentType::POLICE, .severity = IncidentSeverity::MEDIUM, .locationId = "J4", .arrivalSequence = 3, .status = IncidentStatus::PENDING, .computedPriority = 2000.0};
            Incident inc4{.id = "INC-104", .type = IncidentType::MEDICAL, .severity = IncidentSeverity::CRITICAL, .locationId = "J2", .arrivalSequence = 4, .status = IncidentStatus::PENDING, .computedPriority = 4000.0};
            heap.insert(inc1);
            heap.insert(inc2);
            heap.insert(inc3);
            heap.insert(inc4);
            heap.extractMax(&trace);
        } else if (action == "insert") {
            int sev = reqJson.value("severity", 3);
            Incident customInc{
                .id = reqJson.value("id", "INC-DEMO"),
                .type = IncidentType::FIRE,
                .severity = static_cast<IncidentSeverity>(sev),
                .locationId = reqJson.value("locationId", "J1"),
                .arrivalSequence = 99,
                .status = IncidentStatus::PENDING,
                .computedPriority = sev * 1000.0
            };
            heap.insert(customInc, &trace);
        }

        nlohmann::json responseData = {
            {"snapshot", heap.toJsonSnapshot()},
            {"action", action}
        };
        res.status = 200;
        res.set_content(makeSuccessEnvelope(responseData, trace.toJsonArray(), 0, 0.10).dump(2), "application/json");
    });

    // 15c. Educational Isolated AVL Tree Sandbox: POST /api/v1/dsa/avl/demo
    server_->Post("/api/v1/dsa/avl/demo", [](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json reqJson = nlohmann::json::object();
        if (!req.body.empty()) {
            try { reqJson = nlohmann::json::parse(req.body); } catch (...) {}
        }

        std::string action = reqJson.value("action", "rotation");
        std::string rotType = reqJson.value("rotationType", "LL");
        IncidentAVLTree tree;
        TraceCollector trace(true);

        auto makeInc = [](const std::string& id) {
            return Incident{.id = id, .type = IncidentType::MEDICAL, .severity = IncidentSeverity::HIGH, .locationId = "J1", .status = IncidentStatus::PENDING};
        };

        if (action == "inorder") {
            tree.insert("INC-20", makeInc("INC-20"));
            tree.insert("INC-10", makeInc("INC-10"));
            tree.insert("INC-30", makeInc("INC-30"));
            TraceEvent initEv;
            initEv.algorithm = "AVLTree";
            initEv.operation = "initial_state";
            initEv.entityId = "TREE_READY";
            initEv.explanation = "Initial state: balanced AVL tree (INC-20, INC-10, INC-30) ready for inorder traversal.";
            initEv.metadata = {
                {"empty", false},
                {"treeSnapshot", tree.toJsonSnapshot()["root"]}
            };
            trace.addEvent(std::move(initEv));
            tree.inorderTraversal(&trace);
        } else {
            TraceEvent initEv;
            initEv.algorithm = "AVLTree";
            initEv.operation = "initial_state";
            initEv.entityId = "EMPTY_TREE";
            initEv.explanation = "Initial state: AVL tree is empty before inserting keys.";
            initEv.metadata = {
                {"empty", true},
                {"treeSnapshot", nullptr}
            };
            trace.addEvent(std::move(initEv));

            if (rotType == "LL") {
                // Left-Left (Single Right Rotation): insert 30, 20, 10
                tree.insert("INC-30", makeInc("INC-30"), &trace);
                tree.insert("INC-20", makeInc("INC-20"), &trace);
                tree.insert("INC-10", makeInc("INC-10"), &trace);
            } else if (rotType == "RR") {
                // Right-Right (Single Left Rotation): insert 10, 20, 30
                tree.insert("INC-10", makeInc("INC-10"), &trace);
                tree.insert("INC-20", makeInc("INC-20"), &trace);
                tree.insert("INC-30", makeInc("INC-30"), &trace);
            } else if (rotType == "LR") {
                // Left-Right (Double Rotation): insert 30, 10, 20
                tree.insert("INC-30", makeInc("INC-30"), &trace);
                tree.insert("INC-10", makeInc("INC-10"), &trace);
                tree.insert("INC-20", makeInc("INC-20"), &trace);
            } else if (rotType == "RL") {
                // Right-Left (Double Rotation): insert 10, 30, 20
                tree.insert("INC-10", makeInc("INC-10"), &trace);
                tree.insert("INC-30", makeInc("INC-30"), &trace);
                tree.insert("INC-20", makeInc("INC-20"), &trace);
            } else if (rotType == "BALANCED" || rotType == "NONE") {
                // Balanced Insertion Sequence (No Rotations Required): insert 20, 10, 30
                tree.insert("INC-20", makeInc("INC-20"), &trace);
                tree.insert("INC-10", makeInc("INC-10"), &trace);
                tree.insert("INC-30", makeInc("INC-30"), &trace);
            }
        }

        nlohmann::json responseData = {
            {"snapshot", tree.toJsonSnapshot()},
            {"action", action},
            {"rotationType", rotType}
        };
        res.status = 200;
        res.set_content(makeSuccessEnvelope(responseData, trace.toJsonArray(), 0, 0.10).dump(2), "application/json");
    });

    // 15d. Educational Isolated Hash Table Sandbox: POST /api/v1/dsa/hash-table/demo
    server_->Post("/api/v1/dsa/hash-table/demo", [](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json reqJson = nlohmann::json::object();
        if (!req.body.empty()) {
            try { reqJson = nlohmann::json::parse(req.body); } catch (...) {}
        }

        std::string action = reqJson.value("action", "lookup");
        std::string searchKey = reqJson.value("key", "INC-101");
        IncidentHashTable table;
        TraceCollector trace(true);

        auto makeInc = [](const std::string& id) {
            return Incident{.id = id, .type = IncidentType::FIRE, .severity = IncidentSeverity::CRITICAL, .locationId = "J5", .status = IncidentStatus::PENDING};
        };

        if (action == "lookup") {
            table.insert("INC-101", makeInc("INC-101"));
            table.insert("INC-102", makeInc("INC-102"));
            table.insert("INC-103", makeInc("INC-103"));
            table.search(searchKey, &trace);
        } else if (action == "collision") {
            // Keys chosen so FNV-1a hashes produce deliberate collisions into buckets 4 and 7:
            // "INC-2", "INC-11", "INC-28" -> bucket 4
            // "INC-3", "INC-10"           -> bucket 7
            table.insert("INC-2", makeInc("INC-2"), &trace);
            table.insert("INC-11", makeInc("INC-11"), &trace);
            table.insert("INC-28", makeInc("INC-28"), &trace);
            table.insert("INC-3", makeInc("INC-3"), &trace);
            table.insert("INC-10", makeInc("INC-10"), &trace);
        } else if (action == "rehash") {
            // Table starts with 16 buckets. Threshold is 0.75 * 16 = 12.
            // Inserting 13 items triggers doubling to 32 buckets.
            for (int i = 1; i <= 14; ++i) {
                std::string k = "INC-" + std::to_string(i * 10);
                table.insert(k, makeInc(k), &trace);
            }
        }

        nlohmann::json responseData = {
            {"snapshot", table.getStatsJson()},
            {"action", action}
        };
        res.status = 200;
        res.set_content(makeSuccessEnvelope(responseData, trace.toJsonArray(), 0, 0.10).dump(2), "application/json");
    });

    // 15e. Educational Isolated Dispatch Sandbox: POST /api/v1/dsa/dispatch/demo
    server_->Post("/api/v1/dsa/dispatch/demo", [this](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json reqJson = nlohmann::json::object();
        if (!req.body.empty()) {
            try { reqJson = nlohmann::json::parse(req.body); } catch (...) {}
        }

        std::string incidentId = reqJson.value("incidentId", "");
        Incident targetIncident;
        bool found = false;

        {
            std::lock_guard<std::mutex> lock(graphMutex_);
            if (!incidentId.empty()) {
                auto opt = incidentService_.getIncident(incidentId);
                if (opt.has_value()) {
                    targetIncident = *opt;
                    found = true;
                }
            }
            if (!found) {
                auto pending = incidentService_.listIncidents(IncidentStatus::PENDING);
                if (!pending.empty()) {
                    targetIncident = pending.front();
                    found = true;
                }
            }
        }

        if (!found) {
            targetIncident = Incident{
                .id = "INC-DEMO",
                .type = IncidentType::MEDICAL,
                .severity = IncidentSeverity::HIGH,
                .locationId = "J5",
                .status = IncidentStatus::PENDING
            };
        }

        TraceCollector trace(true);
        auto startTime = std::chrono::high_resolution_clock::now();

        TraceEvent valEv;
        valEv.algorithm = "DispatchEngine";
        valEv.operation = "validate_incident";
        valEv.entityId = targetIncident.id;
        valEv.metadata = {
            {"incidentId", targetIncident.id},
            {"serviceType", incidentTypeToString(targetIncident.type)},
            {"severity", static_cast<int>(targetIncident.severity)},
            {"locationId", targetIncident.locationId}
        };
        valEv.explanation = "Simulating dispatch evaluation for incident " + targetIncident.id +
                            " (" + incidentTypeToString(targetIncident.type) + " at " + targetIncident.locationId + ").";
        trace.addEvent(std::move(valEv));

        std::vector<EmergencyUnit> allUnits = unitService_.listUnits();
        struct CandidateEval {
            EmergencyUnit unit;
            RouteResult route;
        };
        std::vector<CandidateEval> candidates;

        std::lock_guard<std::mutex> lock(graphMutex_);
        for (const auto& u : allUnits) {
            if (u.serviceType != targetIncident.type) {
                TraceEvent rejEv;
                rejEv.algorithm = "DispatchEngine";
                rejEv.operation = "candidate_rejected";
                rejEv.entityId = u.id;
                rejEv.metadata = {
                    {"unitId", u.id},
                    {"reason", "service_type_mismatch"},
                    {"unitServiceType", incidentTypeToString(u.serviceType)},
                    {"requiredServiceType", incidentTypeToString(targetIncident.type)}
                };
                rejEv.explanation = "Candidate " + u.id + " rejected: service type mismatch.";
                trace.addEvent(std::move(rejEv));
                continue;
            }

            if (u.status != UnitStatus::AVAILABLE) {
                TraceEvent rejEv;
                rejEv.algorithm = "DispatchEngine";
                rejEv.operation = "candidate_rejected";
                rejEv.entityId = u.id;
                rejEv.metadata = {
                    {"unitId", u.id},
                    {"reason", "unit_unavailable"},
                    {"currentStatus", unitStatusToString(u.status)}
                };
                rejEv.explanation = "Candidate " + u.id + " rejected: unit status is " + unitStatusToString(u.status) + ".";
                trace.addEvent(std::move(rejEv));
                continue;
            }

            RouteResult r = DijkstraRouter::findShortestPath(graph_, u.locationId, targetIncident.locationId, &trace);
            if (!r.routeFound) {
                TraceEvent rejEv;
                rejEv.algorithm = "DispatchEngine";
                rejEv.operation = "candidate_rejected";
                rejEv.entityId = u.id;
                rejEv.metadata = {
                    {"unitId", u.id},
                    {"reason", "unreachable"},
                    {"origin", u.locationId},
                    {"destination", targetIncident.locationId}
                };
                rejEv.explanation = "Candidate " + u.id + " rejected: destination is unreachable.";
                trace.addEvent(std::move(rejEv));
                continue;
            }

            TraceEvent evalEv;
            evalEv.algorithm = "DispatchEngine";
            evalEv.operation = "candidate_route_evaluated";
            evalEv.entityId = u.id;
            evalEv.cost = r.totalTravelTimeMinutes;
            evalEv.metadata = {
                {"unitId", u.id},
                {"travelTimeMinutes", r.totalTravelTimeMinutes},
                {"totalDistanceKm", r.totalDistanceKm},
                {"path", r.path}
            };
            evalEv.explanation = "Candidate " + u.id + " route evaluated: travel time " +
                                 std::to_string(r.totalTravelTimeMinutes) + " min.";
            trace.addEvent(std::move(evalEv));

            candidates.push_back({u, r});
        }

        if (candidates.empty()) {
            res.status = 422;
            res.set_content(makeErrorEnvelope("NO_ELIGIBLE_UNITS", "No eligible reachable units available for simulation.").dump(2), "application/json");
            return;
        }

        std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
            if (std::abs(a.route.totalTravelTimeMinutes - b.route.totalTravelTimeMinutes) > 1e-6) {
                return a.route.totalTravelTimeMinutes < b.route.totalTravelTimeMinutes;
            }
            return a.unit.id < b.unit.id;
        });

        const auto& winner = candidates.front();
        std::ostringstream ratOss;
        ratOss << "Unit " << winner.unit.id << " (" << winner.unit.name << ") was selected with lowest travel time ("
               << std::fixed << std::setprecision(2) << winner.route.totalTravelTimeMinutes << " min) among "
               << candidates.size() << " eligible candidates.";
        std::string rationale = ratOss.str();

        TraceEvent selEv;
        selEv.algorithm = "DispatchEngine";
        selEv.operation = "candidate_selected";
        selEv.entityId = winner.unit.id;
        selEv.cost = winner.route.totalTravelTimeMinutes;
        selEv.metadata = {
            {"selectedUnitId", winner.unit.id},
            {"incidentId", targetIncident.id},
            {"travelTimeMinutes", winner.route.totalTravelTimeMinutes},
            {"decisionRationale", rationale}
        };
        selEv.explanation = "Selected unit " + winner.unit.id + " with optimal travel time.";
        trace.addEvent(std::move(selEv));

        DispatchRecord demoRecord;
        demoRecord.id = "DISP-DEMO";
        demoRecord.incidentId = targetIncident.id;
        demoRecord.unitId = winner.unit.id;
        demoRecord.startLocationId = winner.unit.locationId;
        demoRecord.targetLocationId = targetIncident.locationId;
        demoRecord.route = winner.route;
        demoRecord.travelTimeMinutes = winner.route.totalTravelTimeMinutes;
        demoRecord.status = DispatchStatus::ACTIVE;
        demoRecord.decisionRationale = rationale;

        auto endTime = std::chrono::high_resolution_clock::now();
        double elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        nlohmann::json responseData = {
            {"dispatch", demoRecord.toJson()},
            {"explanation", rationale},
            {"isolated", true}
        };

        res.status = 200;
        res.set_content(makeSuccessEnvelope(responseData, trace.toJsonArray(), state_.tick, elapsedMs).dump(2), "application/json");
    });

    // ==========================================
    // EMERGENCY UNITS MANAGEMENT ENDPOINTS
    // ==========================================

    // 16. Register emergency unit: POST /api/v1/units
    server_->Post("/api/v1/units", [this](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json reqJson;
        try {
            reqJson = nlohmann::json::parse(req.body);
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(makeErrorEnvelope("INVALID_JSON", std::string("Malformed JSON body: ") + e.what()).dump(2), "application/json");
            return;
        }

        if (!reqJson.contains("id") || !reqJson["id"].is_string() || reqJson["id"].get<std::string>().empty()) {
            res.status = 400;
            res.set_content(makeErrorEnvelope("INVALID_ARGUMENT", "Field 'id' is required and must be a non-empty string.").dump(2), "application/json");
            return;
        }

        if (!reqJson.contains("serviceType") || !reqJson["serviceType"].is_string()) {
            res.status = 400;
            res.set_content(makeErrorEnvelope("INVALID_ARGUMENT", "Field 'serviceType' is required (AMBULANCE, FIRE_RESCUE, POLICE).").dump(2), "application/json");
            return;
        }

        if (!reqJson.contains("locationId") || !reqJson["locationId"].is_string()) {
            res.status = 400;
            res.set_content(makeErrorEnvelope("INVALID_ARGUMENT", "Field 'locationId' is required.").dump(2), "application/json");
            return;
        }

        std::string unitId = reqJson["id"].get<std::string>();
        std::string sTypeStr = reqJson["serviceType"].get<std::string>();
        std::string locId = reqJson["locationId"].get<std::string>();

        std::lock_guard<std::mutex> lock(graphMutex_);

        if (!graph_.hasLocation(locId)) {
            res.status = 400;
            res.set_content(makeErrorEnvelope("INVALID_LOCATION", "Location ID '" + locId + "' does not exist in the city graph.").dump(2), "application/json");
            return;
        }

        if (unitService_.hasUnit(unitId)) {
            res.status = 409;
            res.set_content(makeErrorEnvelope("UNIT_ALREADY_EXISTS", "A unit with ID '" + unitId + "' already exists.").dump(2), "application/json");
            return;
        }

        IncidentType sType = IncidentType::MEDICAL;
        if (sTypeStr == "AMBULANCE" || sTypeStr == "MEDICAL" || sTypeStr == "EMS") sType = IncidentType::MEDICAL;
        else if (sTypeStr == "FIRE_RESCUE" || sTypeStr == "FIRE" || sTypeStr == "Fire") sType = IncidentType::FIRE;
        else if (sTypeStr == "POLICE" || sTypeStr == "Police") sType = IncidentType::POLICE;
        else {
            res.status = 400;
            res.set_content(makeErrorEnvelope("INVALID_ARGUMENT", "Unknown serviceType '" + sTypeStr + "'.").dump(2), "application/json");
            return;
        }

        EmergencyUnit unit;
        unit.id = unitId;
        unit.name = reqJson.value("name", reqJson.value("callSign", unitId));
        unit.callSign = reqJson.value("callSign", unit.name);
        unit.serviceType = sType;
        unit.locationId = locId;
        unit.status = UnitStatus::AVAILABLE;

        if (reqJson.contains("capabilities") && reqJson["capabilities"].is_array()) {
            for (const auto& cap : reqJson["capabilities"]) {
                if (cap.is_string()) {
                    unit.capabilities.push_back(cap.get<std::string>());
                }
            }
        }

        auto regResult = unitService_.registerUnit(unit);
        if (!regResult.success) {
            res.status = 400;
            res.set_content(makeErrorEnvelope(regResult.errorCode.empty() ? "REGISTRATION_FAILED" : regResult.errorCode, regResult.errorMessage).dump(2), "application/json");
            return;
        }

        res.status = 201;
        res.set_content(makeSuccessEnvelope(unit.toJson(), nlohmann::json::array(), state_.tick, 0.05).dump(2), "application/json");
    });

    // 17. List emergency units: GET /api/v1/units
    server_->Get("/api/v1/units", [this](const httplib::Request& req, httplib::Response& res) {
        std::optional<IncidentType> sTypeFilter;
        if (req.has_param("serviceType")) {
            sTypeFilter = parseIncidentType(req.get_param_value("serviceType"));
        }

        std::optional<UnitStatus> statusFilter;
        if (req.has_param("status")) {
            statusFilter = parseUnitStatus(req.get_param_value("status"));
        }

        bool availableOnly = (req.get_param_value("availableOnly") == "true");

        auto units = unitService_.listUnits(sTypeFilter, statusFilter, availableOnly);
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& u : units) {
            arr.push_back(u.toJson());
        }

        nlohmann::json responseData = {
            {"total", units.size()},
            {"units", arr}
        };

        res.status = 200;
        res.set_content(makeSuccessEnvelope(responseData, nlohmann::json::array(), state_.tick, 0.05).dump(2), "application/json");
    });

    // 18. Retrieve unit by ID: GET /api/v1/units/:id
    server_->Get(R"(/api/v1/units/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        if (req.matches.size() < 2) {
            res.status = 400;
            res.set_content(makeErrorEnvelope("INVALID_PATH", "Unit ID parameter missing from path.").dump(2), "application/json");
            return;
        }

        std::string unitId = req.matches[1];
        auto unitOpt = unitService_.getUnit(unitId);
        if (!unitOpt.has_value()) {
            res.status = 404;
            res.set_content(makeErrorEnvelope("UNIT_NOT_FOUND", "Unit with ID '" + unitId + "' not found.").dump(2), "application/json");
            return;
        }

        res.status = 200;
        res.set_content(makeSuccessEnvelope(unitOpt->toJson(), nlohmann::json::array(), state_.tick, 0.04).dump(2), "application/json");
    });

    // 19. Update unit status / location: PUT & PATCH /api/v1/units/:id
    auto handleUpdateUnit = [this](const httplib::Request& req, httplib::Response& res) {
        if (req.matches.size() < 2) {
            res.status = 400;
            res.set_content(makeErrorEnvelope("INVALID_PATH", "Unit ID parameter missing from path.").dump(2), "application/json");
            return;
        }

        std::string unitId = req.matches[1];
        auto unitOpt = unitService_.getUnit(unitId);
        if (!unitOpt.has_value()) {
            res.status = 404;
            res.set_content(makeErrorEnvelope("UNIT_NOT_FOUND", "Unit with ID '" + unitId + "' not found.").dump(2), "application/json");
            return;
        }

        nlohmann::json reqJson;
        try {
            reqJson = nlohmann::json::parse(req.body);
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(makeErrorEnvelope("INVALID_JSON", std::string("Malformed JSON body: ") + e.what()).dump(2), "application/json");
            return;
        }

        std::lock_guard<std::mutex> lock(graphMutex_);

        // Location update
        if (reqJson.contains("locationId") && reqJson["locationId"].is_string()) {
            std::string newLoc = reqJson["locationId"].get<std::string>();
            if (!graph_.hasLocation(newLoc)) {
                res.status = 400;
                res.set_content(makeErrorEnvelope("INVALID_LOCATION", "Location ID '" + newLoc + "' does not exist.").dump(2), "application/json");
                return;
            }
            if (!unitService_.updateUnitLocation(unitId, newLoc)) {
                res.status = 400;
                res.set_content(makeErrorEnvelope("INVALID_STATE_TRANSITION", "Cannot update location of unit '" + unitId + "' while assigned or busy.").dump(2), "application/json");
                return;
            }
        }

        // Status update
        if (reqJson.contains("status") && reqJson["status"].is_string()) {
            UnitStatus newStatus = stringToUnitStatus(reqJson["status"].get<std::string>());
            if (!unitService_.updateUnitStatus(unitId, newStatus)) {
                res.status = 400;
                res.set_content(makeErrorEnvelope("INVALID_STATE_TRANSITION", "Invalid status transition for unit '" + unitId + "'.").dump(2), "application/json");
                return;
            }
        }

        auto updatedUnit = unitService_.getUnit(unitId);
        res.status = 200;
        res.set_content(makeSuccessEnvelope(updatedUnit->toJson(), nlohmann::json::array(), state_.tick, 0.05).dump(2), "application/json");
    };

    server_->Put(R"(/api/v1/units/([^/]+))", handleUpdateUnit);
    server_->Patch(R"(/api/v1/units/([^/]+))", handleUpdateUnit);

    // ==========================================
    // DISPATCH & RESOURCE ALLOCATION ENDPOINTS
    // ==========================================

    // 20. Dispatch incident: POST /api/v1/dispatch & POST /api/v1/dispatch/next
    auto handleDispatch = [this](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json reqJson = nlohmann::json::object();
        if (!req.body.empty()) {
            try {
                reqJson = nlohmann::json::parse(req.body);
            } catch (const std::exception& e) {
                res.status = 400;
                res.set_content(makeErrorEnvelope("INVALID_JSON", std::string("Malformed JSON body: ") + e.what()).dump(2), "application/json");
                return;
            }
        }

        bool enableTrace = reqJson.value("enableTrace", false);
        TraceCollector trace(enableTrace);
        std::lock_guard<std::mutex> lock(graphMutex_);

        DispatchResult result;
        bool isNext = (req.path == "/api/v1/dispatch/next") || reqJson.value("highestPriority", false);

        if (isNext) {
            result = dispatchService_.dispatchNext(enableTrace ? &trace : nullptr);
        } else {
            if (!reqJson.contains("incidentId") || !reqJson["incidentId"].is_string()) {
                res.status = 400;
                res.set_content(makeErrorEnvelope("INVALID_ARGUMENT", "Field 'incidentId' or 'highestPriority: true' is required.").dump(2), "application/json");
                return;
            }
            std::string incidentId = reqJson["incidentId"].get<std::string>();
            result = dispatchService_.dispatchIncident(incidentId, enableTrace ? &trace : nullptr);
        }

        if (!result.success) {
            std::string code = "DISPATCH_FAILED";
            int statusCode = 422;

            if (result.error.find("not found") != std::string::npos) {
                code = "INCIDENT_NOT_FOUND";
                statusCode = 404;
            } else if (result.error.find("not in PENDING") != std::string::npos ||
                       result.error.find("already") != std::string::npos) {
                code = "INVALID_STATE_TRANSITION";
                statusCode = 409;
            } else if (result.error.find("No pending incidents") != std::string::npos) {
                code = "NO_PENDING_INCIDENTS";
                statusCode = 404;
            } else if (result.error.find("No available units") != std::string::npos) {
                code = "NO_ELIGIBLE_UNITS";
                statusCode = 422;
            } else if (result.error.find("No reachable units") != std::string::npos) {
                code = "NO_REACHABLE_UNITS";
                statusCode = 422;
            }

            res.status = statusCode;
            res.set_content(makeErrorEnvelope(code, result.error, {{"trace", trace.toJsonArray()}}).dump(2), "application/json");
            return;
        }

        nlohmann::json responseData = {
            {"dispatch", result.record->toJson()},
            {"explanation", result.explanation}
        };

        res.status = 201;
        res.set_content(makeSuccessEnvelope(responseData, trace.toJsonArray(), state_.tick, 0.15).dump(2), "application/json");
    };

    server_->Post("/api/v1/dispatch", handleDispatch);
    server_->Post("/api/v1/dispatch/next", handleDispatch);

    // 21. List dispatch records: GET /api/v1/dispatches
    server_->Get("/api/v1/dispatches", [this](const httplib::Request& req, httplib::Response& res) {
        std::optional<DispatchStatus> statusFilter;
        if (req.has_param("status")) {
            std::string st = req.get_param_value("status");
            if (st == "ACTIVE") statusFilter = DispatchStatus::ACTIVE;
            else if (st == "COMPLETED") statusFilter = DispatchStatus::COMPLETED;
            else if (st == "CANCELLED") statusFilter = DispatchStatus::CANCELLED;
        }

        auto dispatches = dispatchService_.listDispatches(statusFilter);
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& d : dispatches) {
            arr.push_back(d.toJson());
        }

        nlohmann::json responseData = {
            {"total", dispatches.size()},
            {"dispatches", arr}
        };

        res.status = 200;
        res.set_content(makeSuccessEnvelope(responseData, nlohmann::json::array(), state_.tick, 0.05).dump(2), "application/json");
    });

    // 22. Retrieve dispatch record: GET /api/v1/dispatches/:id
    server_->Get(R"(/api/v1/dispatches/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        if (req.matches.size() < 2) {
            res.status = 400;
            res.set_content(makeErrorEnvelope("INVALID_PATH", "Dispatch ID parameter missing from path.").dump(2), "application/json");
            return;
        }

        std::string dispatchId = req.matches[1];
        auto dispOpt = dispatchService_.getDispatch(dispatchId);
        if (!dispOpt.has_value()) {
            res.status = 404;
            res.set_content(makeErrorEnvelope("DISPATCH_NOT_FOUND", "Dispatch record with ID '" + dispatchId + "' not found.").dump(2), "application/json");
            return;
        }

        res.status = 200;
        res.set_content(makeSuccessEnvelope(dispOpt->toJson(), nlohmann::json::array(), state_.tick, 0.04).dump(2), "application/json");
    });

    // 23. Complete dispatch: POST /api/v1/dispatches/:id/complete
    server_->Post(R"(/api/v1/dispatches/([^/]+)/complete)", [this](const httplib::Request& req, httplib::Response& res) {
        if (req.matches.size() < 2) {
            res.status = 400;
            res.set_content(makeErrorEnvelope("INVALID_PATH", "Dispatch ID parameter missing from path.").dump(2), "application/json");
            return;
        }

        std::string dispatchId = req.matches[1];
        bool enableTrace = false;
        if (!req.body.empty()) {
            try {
                auto bodyJson = nlohmann::json::parse(req.body);
                enableTrace = bodyJson.value("enableTrace", false);
            } catch (...) {}
        }

        TraceCollector trace(enableTrace);
        std::lock_guard<std::mutex> lock(graphMutex_);

        auto result = dispatchService_.completeDispatch(dispatchId, enableTrace ? &trace : nullptr);
        if (!result.success) {
            std::string code = "COMPLETION_FAILED";
            int statusCode = 400;
            if (result.error.find("not found") != std::string::npos) {
                code = "DISPATCH_NOT_FOUND";
                statusCode = 404;
            } else if (result.error.find("already completed") != std::string::npos) {
                code = "DISPATCH_ALREADY_COMPLETED";
                statusCode = 409;
            }
            res.status = statusCode;
            res.set_content(makeErrorEnvelope(code, result.error).dump(2), "application/json");
            return;
        }

        nlohmann::json responseData = {
            {"dispatch", result.record->toJson()},
            {"message", "Dispatch " + dispatchId + " completed. Unit released and incident resolved."}
        };

        res.status = 200;
        res.set_content(makeSuccessEnvelope(responseData, trace.toJsonArray(), state_.tick, 0.08).dump(2), "application/json");
    });

    // 24. Recalculate route for active dispatch: POST /api/v1/dispatches/:id/recalculate
    server_->Post(R"(/api/v1/dispatches/([^/]+)/recalculate)", [this](const httplib::Request& req, httplib::Response& res) {
        if (req.matches.size() < 2) {
            res.status = 400;
            res.set_content(makeErrorEnvelope("INVALID_PATH", "Dispatch ID parameter missing from path.").dump(2), "application/json");
            return;
        }

        std::string dispatchId = req.matches[1];
        bool enableTrace = false;
        if (!req.body.empty()) {
            try {
                auto bodyJson = nlohmann::json::parse(req.body);
                enableTrace = bodyJson.value("enableTrace", false);
            } catch (...) {}
        }

        TraceCollector trace(enableTrace);
        std::lock_guard<std::mutex> lock(graphMutex_);

        auto result = dispatchService_.recalculateRoute(dispatchId, enableTrace ? &trace : nullptr);
        if (!result.success) {
            std::string code = "RECALCULATION_FAILED";
            int statusCode = 400;
            if (result.error.find("not found") != std::string::npos) {
                code = "DISPATCH_NOT_FOUND";
                statusCode = 404;
            } else if (result.error.find("not active") != std::string::npos) {
                code = "INVALID_STATE_TRANSITION";
                statusCode = 409;
            } else if (result.error.find("No valid route") != std::string::npos) {
                code = "NO_REACHABLE_UNITS";
                statusCode = 422;
            }
            res.status = statusCode;
            res.set_content(makeErrorEnvelope(code, result.error).dump(2), "application/json");
            return;
        }

        nlohmann::json responseData = {
            {"dispatch", result.record->toJson()},
            {"message", "Route successfully recalculated under current road conditions."}
        };

        res.status = 200;
        res.set_content(makeSuccessEnvelope(responseData, trace.toJsonArray(), state_.tick, 0.10).dump(2), "application/json");
    });

    // 25. Root info fallback: GET /
    server_->Get("/", [](const httplib::Request&, httplib::Response& res) {
        nlohmann::json rootInfo = {
            {"name", "EmergencyGrid Simulation Core"},
            {"description", "Educational ADS Capstone C++20 Engine"},
            {"endpoints", {
                {"health", "/api/v1/health"},
                {"simulationState", "/api/v1/simulation/state"},
                {"cityGraph", "/api/v1/city/graph"},
                {"routing", "/api/v1/routing/calculate"},
                {"roadUpdate", "/api/v1/city/roads/{id}"},
                {"reset", "/api/v1/city/reset"},
                {"createIncident", "/api/v1/incidents"},
                {"listIncidents", "/api/v1/incidents"},
                {"listIncidentsSorted", "/api/v1/incidents/sorted"},
                {"inspectNextIncident", "/api/v1/incidents/next"},
                {"selectNextIncident", "/api/v1/incidents/next/select"},
                {"getIncident", "/api/v1/incidents/{id}"},
                {"cancelIncident", "/api/v1/incidents/{id}/cancel"},
                {"heapSnapshot", "/api/v1/dsa/heap/snapshot"},
                {"avlSnapshot", "/api/v1/dsa/avl"},
                {"hashTableStats", "/api/v1/dsa/hash-table"},
                {"units", "/api/v1/units"},
                {"unitById", "/api/v1/units/{id}"},
                {"dispatch", "/api/v1/dispatch"},
                {"dispatchNext", "/api/v1/dispatch/next"},
                {"dispatches", "/api/v1/dispatches"},
                {"dispatchById", "/api/v1/dispatches/{id}"},
                {"completeDispatch", "/api/v1/dispatches/{id}/complete"},
                {"recalculateDispatch", "/api/v1/dispatches/{id}/recalculate"}
            }}
        };
        res.set_content(makeSuccessEnvelope(rootInfo, nlohmann::json::array(), 0).dump(2), "application/json");
        res.status = 200;
    });
}

bool HttpServer::start() {
    std::cout << "[EmergencyGrid Backend] Binding to http://" << host_ << ":" << port_ << " ...\n";
    std::cout << "[EmergencyGrid Backend] Endpoints registered:\n";
    std::cout << "  - GET  http://" << host_ << ":" << port_ << "/api/v1/health\n";
    std::cout << "  - GET  http://" << host_ << ":" << port_ << "/api/v1/simulation/state\n";
    std::cout << "  - GET  http://" << host_ << ":" << port_ << "/api/v1/city/graph\n";
    std::cout << "  - POST http://" << host_ << ":" << port_ << "/api/v1/routing/calculate\n";
    std::cout << "  - PUT  http://" << host_ << ":" << port_ << "/api/v1/city/roads/{id}\n";
    std::cout << "  - POST http://" << host_ << ":" << port_ << "/api/v1/city/reset\n";
    std::cout << "  - POST http://" << host_ << ":" << port_ << "/api/v1/incidents\n";
    std::cout << "  - GET  http://" << host_ << ":" << port_ << "/api/v1/incidents\n";
    std::cout << "  - GET  http://" << host_ << ":" << port_ << "/api/v1/incidents/next\n";
    std::cout << "  - POST http://" << host_ << ":" << port_ << "/api/v1/incidents/next/select\n";
    std::cout << "  - GET  http://" << host_ << ":" << port_ << "/api/v1/incidents/{id}\n";
    std::cout << "  - POST http://" << host_ << ":" << port_ << "/api/v1/incidents/{id}/cancel\n";
    std::cout << "  - GET  http://" << host_ << ":" << port_ << "/api/v1/dsa/heap/snapshot\n";
    std::cout << "[EmergencyGrid Backend] Server starting (blocking call) ...\n" << std::flush;
    return server_->listen(host_.c_str(), port_);
}

void HttpServer::stop() {
    if (server_ && server_->is_running()) {
        std::cout << "[EmergencyGrid Backend] Stopping server ...\n";
        server_->stop();
    }
}

} // namespace emergencygrid
