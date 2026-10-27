#pragma once

#include <vector>
#include "core/types.hpp"
#include "dsa/graph.hpp"

namespace emergencygrid {

inline RoadGraph createMetroGrid16() {
    RoadGraph graph;

    // 16 Intersections
    graph.addLocation(Location{.id = "J1", .name = "Central Plaza", .x = 120.0, .y = 80.0, .zone = "Commercial"});
    graph.addLocation(Location{.id = "J2", .name = "North Gate", .x = 120.0, .y = 20.0, .zone = "Residential"});
    graph.addLocation(Location{.id = "J3", .name = "Northeast District", .x = 220.0, .y = 20.0, .zone = "Residential"});
    graph.addLocation(Location{.id = "J4", .name = "East Avenue", .x = 220.0, .y = 80.0, .zone = "Commercial"});
    graph.addLocation(Location{.id = "J5", .name = "Tech Park", .x = 320.0, .y = 80.0, .zone = "Industrial"});
    graph.addLocation(Location{.id = "J6", .name = "Harbor Crossing", .x = 320.0, .y = 160.0, .zone = "Industrial"});
    graph.addLocation(Location{.id = "J7", .name = "South Marina", .x = 220.0, .y = 160.0, .zone = "Commercial"});
    graph.addLocation(Location{.id = "J8", .name = "Civic Center", .x = 120.0, .y = 160.0, .zone = "Civic"});
    graph.addLocation(Location{.id = "J9", .name = "West End", .x = 20.0, .y = 160.0, .zone = "Residential"});
    graph.addLocation(Location{.id = "J10", .name = "Grand Boulevard", .x = 20.0, .y = 80.0, .zone = "Commercial"});
    graph.addLocation(Location{.id = "J11", .name = "Northwest Heights", .x = 20.0, .y = 20.0, .zone = "Residential"});
    graph.addLocation(Location{.id = "J12", .name = "University Quad", .x = 70.0, .y = 50.0, .zone = "Civic"});
    graph.addLocation(Location{.id = "J13", .name = "Medical Center", .x = 170.0, .y = 50.0, .zone = "Medical"});
    graph.addLocation(Location{.id = "J14", .name = "Memorial Hospital", .x = 170.0, .y = 120.0, .zone = "Medical"});
    graph.addLocation(Location{.id = "J15", .name = "Southwest Suburbs", .x = 70.0, .y = 120.0, .zone = "Residential"});
    graph.addLocation(Location{.id = "J16", .name = "Industrial Hub", .x = 270.0, .y = 120.0, .zone = "Industrial"});

    // 26 Bidirectional Road Segments
    // Formula: travelTimeMinutes = (baseLengthKm / speedLimitKmh) * congestionFactor * 60
    graph.addRoad(RoadSegment{.id = "R1_2", .sourceId = "J1", .targetId = "J2", .baseLengthKm = 2.5, .speedLimitKmh = 50.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R2_3", .sourceId = "J2", .targetId = "J3", .baseLengthKm = 3.0, .speedLimitKmh = 60.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R3_4", .sourceId = "J3", .targetId = "J4", .baseLengthKm = 2.5, .speedLimitKmh = 50.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R1_4", .sourceId = "J1", .targetId = "J4", .baseLengthKm = 2.0, .speedLimitKmh = 40.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R4_5", .sourceId = "J4", .targetId = "J5", .baseLengthKm = 2.5, .speedLimitKmh = 50.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R3_5", .sourceId = "J3", .targetId = "J5", .baseLengthKm = 4.0, .speedLimitKmh = 60.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R5_6", .sourceId = "J5", .targetId = "J6", .baseLengthKm = 3.0, .speedLimitKmh = 50.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R4_16", .sourceId = "J4", .targetId = "J16", .baseLengthKm = 2.0, .speedLimitKmh = 50.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R16_6", .sourceId = "J16", .targetId = "J6", .baseLengthKm = 2.5, .speedLimitKmh = 50.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R16_7", .sourceId = "J16", .targetId = "J7", .baseLengthKm = 2.0, .speedLimitKmh = 50.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R4_7", .sourceId = "J4", .targetId = "J7", .baseLengthKm = 3.0, .speedLimitKmh = 50.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R7_8", .sourceId = "J7", .targetId = "J8", .baseLengthKm = 3.0, .speedLimitKmh = 60.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R1_8", .sourceId = "J1", .targetId = "J8", .baseLengthKm = 2.5, .speedLimitKmh = 50.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R8_9", .sourceId = "J8", .targetId = "J9", .baseLengthKm = 3.0, .speedLimitKmh = 50.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R9_10", .sourceId = "J9", .targetId = "J10", .baseLengthKm = 2.5, .speedLimitKmh = 50.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R10_1", .sourceId = "J10", .targetId = "J1", .baseLengthKm = 2.5, .speedLimitKmh = 50.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R10_11", .sourceId = "J10", .targetId = "J11", .baseLengthKm = 2.5, .speedLimitKmh = 50.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R11_2", .sourceId = "J11", .targetId = "J2", .baseLengthKm = 3.0, .speedLimitKmh = 50.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R11_12", .sourceId = "J11", .targetId = "J12", .baseLengthKm = 2.0, .speedLimitKmh = 40.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R12_1", .sourceId = "J12", .targetId = "J1", .baseLengthKm = 1.5, .speedLimitKmh = 45.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R1_13", .sourceId = "J1", .targetId = "J13", .baseLengthKm = 1.5, .speedLimitKmh = 45.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R13_3", .sourceId = "J13", .targetId = "J3", .baseLengthKm = 2.0, .speedLimitKmh = 40.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R1_14", .sourceId = "J1", .targetId = "J14", .baseLengthKm = 1.5, .speedLimitKmh = 45.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R14_7", .sourceId = "J14", .targetId = "J7", .baseLengthKm = 2.0, .speedLimitKmh = 40.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R1_15", .sourceId = "J1", .targetId = "J15", .baseLengthKm = 1.5, .speedLimitKmh = 45.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R15_9", .sourceId = "J15", .targetId = "J9", .baseLengthKm = 2.0, .speedLimitKmh = 40.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});

    return graph;
}

inline std::vector<EmergencyUnit> createDefaultUnits() {
    return {
        EmergencyUnit{
            .id = "AMB-01",
            .name = "Medic 1",
            .callSign = "Medic 1",
            .serviceType = IncidentType::MEDICAL,
            .locationId = "J13", // Medical Center
            .status = UnitStatus::AVAILABLE,
            .capabilities = {"BLS", "ALS"},
            .activeIncidentId = ""
        },
        EmergencyUnit{
            .id = "AMB-02",
            .name = "Medic 2",
            .callSign = "Medic 2",
            .serviceType = IncidentType::MEDICAL,
            .locationId = "J14", // Memorial Hospital
            .status = UnitStatus::AVAILABLE,
            .capabilities = {"BLS"},
            .activeIncidentId = ""
        },
        EmergencyUnit{
            .id = "AMB-03",
            .name = "Medic 3",
            .callSign = "Medic 3",
            .serviceType = IncidentType::MEDICAL,
            .locationId = "J9", // West End
            .status = UnitStatus::AVAILABLE,
            .capabilities = {"BLS", "ALS"},
            .activeIncidentId = ""
        },
        EmergencyUnit{
            .id = "ENG-01",
            .name = "Engine 1",
            .callSign = "Engine 1",
            .serviceType = IncidentType::FIRE,
            .locationId = "J8", // Civic Center
            .status = UnitStatus::AVAILABLE,
            .capabilities = {"FIRE_SUPPRESSION", "RESCUE"},
            .activeIncidentId = ""
        },
        EmergencyUnit{
            .id = "ENG-02",
            .name = "Engine 2",
            .callSign = "Engine 2",
            .serviceType = IncidentType::FIRE,
            .locationId = "J5", // Tech Park
            .status = UnitStatus::AVAILABLE,
            .capabilities = {"FIRE_SUPPRESSION"},
            .activeIncidentId = ""
        },
        EmergencyUnit{
            .id = "TRK-01",
            .name = "Rescue Hazmat 1",
            .callSign = "Rescue Hazmat 1",
            .serviceType = IncidentType::FIRE,
            .locationId = "J16", // Industrial Hub
            .status = UnitStatus::AVAILABLE,
            .capabilities = {"FIRE_SUPPRESSION", "HEAVY_RESCUE", "HAZMAT"},
            .activeIncidentId = ""
        },
        EmergencyUnit{
            .id = "POL-01",
            .name = "Patrol 1",
            .callSign = "Patrol 1",
            .serviceType = IncidentType::POLICE,
            .locationId = "J1", // Central Plaza
            .status = UnitStatus::AVAILABLE,
            .capabilities = {"PATROL", "TRAFFIC_CONTROL"},
            .activeIncidentId = ""
        },
        EmergencyUnit{
            .id = "POL-02",
            .name = "Patrol 2",
            .callSign = "Patrol 2",
            .serviceType = IncidentType::POLICE,
            .locationId = "J11", // Northwest Heights
            .status = UnitStatus::AVAILABLE,
            .capabilities = {"PATROL"},
            .activeIncidentId = ""
        },
        EmergencyUnit{
            .id = "POL-03",
            .name = "Traffic 3",
            .callSign = "Traffic 3",
            .serviceType = IncidentType::POLICE,
            .locationId = "J4", // East Avenue
            .status = UnitStatus::AVAILABLE,
            .capabilities = {"PATROL", "TRAFFIC_CONTROL"},
            .activeIncidentId = ""
        }
    };
}

} // namespace emergencygrid
