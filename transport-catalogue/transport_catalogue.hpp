#pragma once

#include <string>
#include <deque>
#include <unordered_map>
#include <set>
#include <vector>

#include "geo.hpp"
#include "domain.hpp"

namespace transport_catalogue {

using namespace domain;

class TransportCatalogue {
public:
    TransportCatalogue() {};

    void AddBus(const std::string_view name, const std::vector<std::string_view>& stop_names);

    void AddStop(const std::string_view name, const geo::Coordinates& coordinates);

    void AddDistance(const std::string_view stop_from, const std::string_view stop_to, const int distance);

    const Bus& FindBus(const std::string_view name) const;

    const Stop& FindStop(const std::string_view name) const;

    int GetDistance(const std::string_view stop_from, const std::string_view stop_to) const;

    BusInfo GetBusInfo(const std::string_view name) const;

    StopInfo GetStopInfo(const std::string_view name) const;

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

namespace tests {

void TestAddFindMethods();
void TestGetBusInfo();
void TestGetStopInfo();
void TestDistances();


} // namespace transport_catalogue::tests

namespace util {

namespace view {

std::string_view Substr(std::string_view view, size_t start_pos, size_t end_pos);

std::string_view Trim(std::string_view view, char to_remove = ' ');

std::vector<std::string_view> SplitBy(std::string_view view, const char delim = ' ');

} // namespace transport_catalogue::util::view

} // namespace transport_catalogue::util

} // namespace transport_catalogue 
