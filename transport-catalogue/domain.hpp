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

using BusPtr = Bus*;
using StopPtr = Stop*;

enum InfoType {
    VALID,
    EMPTY,
    NOT_FOUND,
};

enum QueryType {
    STOP, BUS
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
    InfoType type;
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
    std::string_view name;
};

struct BusInputQuery : public InputQuery {
    std::vector<std::string_view> stop_names;
    bool is_roundtrip = false;
};

struct StopInputQuery : public InputQuery {
    geo::Coordinates coordinates;
    std::unordered_map<std::string_view, int> distances;
};

struct BusOutputQuery : public OutputQuery { 
    BusOutputQuery(int id, std::string_view name)
        : OutputQuery{id, QueryType::BUS, name} {}
};

struct StopOutputQuery : public OutputQuery { 
    StopOutputQuery(int id, std::string_view name)
        : OutputQuery{id, QueryType::STOP, name} {} 
};

} // namespace domain