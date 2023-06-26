#pragma once

#include <string>
#include <deque>
#include <unordered_map>
#include <set>
#include <vector>

#include "geo.hpp"

namespace transport_catalogue {

struct Bus;
struct Stop;
struct BusInfo;
struct StopInfo;

enum InfoType {
    VALID,
    EMPTY,
    NOT_FOUND,
};


class TransportCatalogue {
public:
    TransportCatalogue() {};

    void AddBus(const std::string_view& name, const std::vector<std::string_view>& stop_names);

    void AddStop(const std::string_view& name, const Coordinates& coordinates);

    void AddDistance(const std::string_view& stop_from, const std::string_view& stop_to, const int distance);

    const Bus& FindBus(const std::string_view& name) const;

    const Stop& FindStop(const std::string_view& name) const;

    int GetDistance(const std::string_view& stop_from, const std::string_view& stop_to) const;

    BusInfo GetBusInfo(const std::string_view& name) const;

    StopInfo GetStopInfo(const std::string_view& name) const;

private:

    struct StopPtrPairHasher {
        size_t operator() (const std::pair<Stop*, Stop*>& stop_pair) const {
            std::hash<const void *> hasher;
            size_t hash1 = hasher(stop_pair.first);
            size_t hash2 = hasher(stop_pair.second);
            return hash1 + hash2 * 37;
        }
    };

    static size_t CountUniqueStops(const Bus& bus);
    
    static double ComputeRouteDistance(const Bus& bus);

    double ComputeCurvedRouteDistance(const Bus& bus) const;

    static double ComputeCurvature(const double curved_distance, const double geo_distance);

    std::deque<Stop> stops_;

    std::unordered_map<std::string_view, Stop*> names_to_stops_;

    std::deque<Bus> buses_;
    
    std::unordered_map<std::string_view, Bus*> names_to_buses_; 

    std::unordered_map<std::pair<Stop*, Stop*>, int, StopPtrPairHasher> stop_distances_;
    
};

class BusCompare {
public:
    bool operator() (const Bus* lb, const Bus* rb) const;
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
         Coordinates coordinates, 
         std::set<Bus*, BusCompare> buses)
         : name(name)
         , coordinates(coordinates)
         , buses(buses) {}

    std::string name;
    Coordinates coordinates;
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

namespace tests {

void TestAddFindMethods();
void TestGetBusInfo();
void TestGetStopInfo();
void TestDistances();


} // namespace transport_catalogue::tests

} // namespace transport_catalogue 
