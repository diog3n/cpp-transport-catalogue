#pragma once
#include <optional>
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

using BusPtr = Bus*;
using StopPtr = Stop*;

using BusInfoOpt = std::optional<BusInfo>;
using StopInfoOpt = std::optional<StopInfo>;

// Callable comparator for BusPtrs used in maps and sets
class BusCompare {
public:
    bool operator() (const BusPtr lb, const BusPtr rb) const;
};

enum QueryType {
    STOP, BUS, MAP
};

struct Bus {
    Bus(std::string name, 
        std::vector<Stop*> route,
        bool is_round = false)
        : name(name)
        , route(route)
        , is_roundtrip(is_round) {}

    std::string name;
    std::vector<Stop*> route;
    bool is_roundtrip;
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

struct Info {
    std::string name;
};

struct StopInfo : public Info {
    geo::Coordinates coordinates;
    std::vector<std::string_view> bus_names;
};

struct BusInfo : public Info {
    size_t stops_on_route;
    size_t unique_stops;
    double route_length;
    double curvature;
};

struct InputQuery {
    std::string_view name;
};

struct OutputQuery {
    int id;
    QueryType type;
};

struct BusInputQuery : public InputQuery {
    std::vector<std::string_view> stop_names;
    bool is_roundtrip = false;
};

struct StopInputQuery : public InputQuery {
    geo::Coordinates coordinates;
    std::unordered_map<std::string_view, int> distances;
};

struct MapOutputQuery : public OutputQuery {
    MapOutputQuery(int id)
        : OutputQuery{ id, QueryType::MAP } {}
};

struct BusOutputQuery : public OutputQuery { 
    BusOutputQuery(int id, std::string_view name)
        : OutputQuery{ id, QueryType::BUS }, bus_name(name) {}
    std::string_view bus_name;
};

struct StopOutputQuery : public OutputQuery { 
    StopOutputQuery(int id, std::string_view name)
        : OutputQuery{ id, QueryType::STOP }, stop_name(name) {} 
    std::string_view stop_name;
};

} // namespace domain