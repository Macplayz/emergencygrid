#include "api/server.hpp"
#include <iostream>
#include <csignal>
#include <cstdlib>

static emergencygrid::HttpServer* g_server = nullptr;

void signalHandler(int signum) {
    std::cout << "\n[EmergencyGrid Backend] Received interrupt signal (" << signum << "). Shutting down...\n";
    if (g_server) {
        g_server->stop();
    }
}

int main(int argc, char* argv[]) {
    std::cout << "========================================================\n";
    std::cout << " EmergencyGrid C++20 Simulation Core (Phase 1 Foundation)\n";
    std::cout << " Advanced Data Structures & Algorithms Capstone Project \n";
    std::cout << "========================================================\n";

    int port = 8080;
    const char* envPort = std::getenv("PORT");
    if (envPort) {
        try {
            port = std::stoi(envPort);
        } catch (...) {
            std::cerr << "[Warning] Invalid PORT env var, using default 8080\n";
        }
    }

    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "[Warning] Invalid port argument, using 8080\n";
        }
    }

    emergencygrid::HttpServer server("127.0.0.1", port);
    g_server = &server;

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    server.registerRoutes();

    if (!server.start()) {
        std::cerr << "[EmergencyGrid Backend Error] Failed to bind or start HTTP server on port " << port << "\n";
        return 1;
    }

    return 0;
}
