cmake_minimum_required(VERSION 3.20)
project(EmergencyGridBackend VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /EHsc /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Core DSA & Simulation source files
set(CORE_SOURCES
    src/dsa/graph.cpp
    src/dsa/dijkstra.cpp
    src/dsa/binary_heap.cpp
    src/simulation/incident_service.cpp
    src/simulation/unit_service.cpp
    src/simulation/dispatch_service.cpp
)

# Backend server sources
set(SERVER_SOURCES
    src/main.cpp
    src/api/server.cpp
    ${CORE_SOURCES}
)

# Header files
set(HEADERS
    include/core/types.hpp
    include/core/trace.hpp
    include/dsa/graph.hpp
    include/dsa/dijkstra.hpp
    include/dsa/binary_heap.hpp
    include/dsa/avl_tree.hpp
    include/dsa/hash_table.hpp
    include/simulation/seed_data.hpp
    include/simulation/incident_service.hpp
    include/simulation/unit_service.hpp
    include/simulation/dispatch_service.hpp
    include/api/server.hpp
    include/external/httplib.h
    include/external/json.hpp
)

# Main HTTP Server Executable
add_executable(emergency_grid_backend ${SERVER_SOURCES} ${HEADERS})

target_include_directories(emergency_grid_backend PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/external
)

# On Windows, cpp-httplib requires Winsock2
if(WIN32)
    target_link_libraries(emergency_grid_backend PRIVATE ws2_32 crypt32)
endif()

set_target_properties(emergency_grid_backend PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Testing Executable
enable_testing()

add_executable(test_graph_dijkstra
    tests/test_graph_dijkstra.cpp
    ${CORE_SOURCES}
    ${HEADERS}
)

target_include_directories(test_graph_dijkstra PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/external
)

set_target_properties(test_graph_dijkstra PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME TestGraphDijkstra COMMAND test_graph_dijkstra)

# Incident Heap & Service Test Executable
add_executable(test_incident_heap
    tests/test_incident_heap.cpp
    ${CORE_SOURCES}
    ${HEADERS}
)

target_include_directories(test_incident_heap PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/external
)

set_target_properties(test_incident_heap PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME TestIncidentHeap COMMAND test_incident_heap)

# AVL Tree & Hash Table Test Executable
add_executable(test_avl_hash
    tests/test_avl_hash.cpp
    ${CORE_SOURCES}
    ${HEADERS}
)

target_include_directories(test_avl_hash PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/external
)

set_target_properties(test_avl_hash PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME TestAvlHash COMMAND test_avl_hash)

# Dispatch & Unit Allocation Test Executable
add_executable(test_dispatch_allocation
    tests/test_dispatch_allocation.cpp
    ${CORE_SOURCES}
    ${HEADERS}
)

target_include_directories(test_dispatch_allocation PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/external
)

set_target_properties(test_dispatch_allocation PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME TestDispatchAllocation COMMAND test_dispatch_allocation)


