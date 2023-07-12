#pragma once
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "geo.hpp"

namespace domain {

struct Bus;
struct Stop;
struct BusInfo;
struct StopInfo;

class BusCompare {
public:
    bool operator() (const Bus* lb, const Bus* rb) const;
};

enum InfoType {
    VALID,
    EMPTY,
    NOT_FOUND,
};

struct Bus {
    Bus(std::string name, 
        std::vector<Stop*> route)
        : name(name)
        , route(route) {}

    std::string name;
    std::vector<Stop*> route;
};

struct Stop {
    Stop(std::string name, 
         geo::Coordinates coordinates, 
         std::set<Bus*, BusCompare> buses)
         : name(name)
         , coordinates(coordinates)
         , buses(buses) {}

    std::string name;
    geo::Coordinates coordinates;
    std::set<Bus*, BusCompare> buses;
};

struct StopInfo {
    InfoType type;
    std::string name;
    std::vector<std::string_view> bus_names;
};

struct BusInfo {
    InfoType type;
    std::string name;
    size_t stops_on_route;
    size_t unique_stops;
    double route_length;
    double curvature;
};

struct BusInputQuery {
    std::string_view bus_name;
    std::vector<std::string_view> stop_names;
};

struct StopInputQuery {
    std::string_view stop_name;
    geo::Coordinates coordinates;
    std::unordered_map<std::string_view, int> distances;
};

struct BusOutputQuery {
    int id;
    std::string_view bus_name;
};

struct StopOutputQuery {
    int id;
    std::string_view stop_name;
};

} // namespace domain