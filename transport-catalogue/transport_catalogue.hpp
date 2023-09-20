#pragma once

#include <map>
#include <deque>

#include "geo.hpp"
#include "domain.hpp"

namespace transport_catalogue {

using namespace domain;

class TransportCatalogue {
public:
    TransportCatalogue() {};

    /* Adds a stop to the transport catalogue. This operation involves population 
     * of buses_ deque as well as population of names_to_buses_ index */
    void AddBus(const std::string_view name, const std::vector<std::string_view>& stop_names, bool is_round = false);

    /* Adds a stop to the transport catalogue. This operation involves population 
     * of stops_ deque as well as population of names_to_stops_ index */
    void AddStop(const std::string_view name, const geo::Coordinates& coordinates);

    // Adds a distance between stops. Stop's existence is required
    void AddDistance(const std::string_view stop_from, const std::string_view stop_to, const int distance);

    /* Finds a bus by name and returns a reference to its struct 
     * contained in buses_ deque */
    BusPtr FindBus(const std::string_view name) const;

    /* Finds a stop by name and returns a reference to its struct
     * contained in stops_ deque */
    StopPtr FindStop(const std::string_view name) const;

    /* Returns a distance between stops. Distances between same stops may be 
     * different depending on direction. If a distance between stops in a given
     * direction is not found, function will try to look for a distance in the
     * opposite direction */
    int GetDistance(const std::string_view stop_from, const std::string_view stop_to) const;

    // Returns info on a given bus in a specific format
    BusInfoOpt GetBusInfo(const std::string_view name) const;

    // Returns info on a given stop in a specific format
    StopInfoOpt GetStopInfo(const std::string_view name) const;

    std::vector<std::string_view> GetStopNames() const;

    std::vector<std::string_view> GetBusNames() const;

    size_t GetStopCount() const;

    size_t GetBusCount() const;

private:

    struct StopPtrPairHasher {
        size_t operator() (const std::pair<StopPtr, StopPtr>& stop_pair) const {
            std::hash<const void *> hasher;
            size_t hash1 = hasher(stop_pair.first);
            size_t hash2 = hasher(stop_pair.second);
            return hash1 + hash2 * 37;
        }
    };

    // Counts unique stops in given bus's route
    static size_t CountUniqueStops(const Bus& bus);
    
    // Computes a route distance for a given bus
    static double ComputeRouteDistance(const Bus& bus);

    // Computes bus route's length based on given distances
    double ComputeCurvedRouteDistance(const Bus& bus) const;

    // Computes route's curvature
    static double ComputeCurvature(const double curved_distance, const double geo_distance);

    std::deque<Stop> stops_;

    std::map<std::string_view, StopPtr> names_to_stops_;

    std::deque<Bus> buses_;
    
    std::map<std::string_view, BusPtr> names_to_buses_; 

    std::unordered_map<std::pair<StopPtr, StopPtr>, int, StopPtrPairHasher> stop_distances_;
    
};

namespace tests {

void TestAddFindMethods();
void TestGetBusInfo();
void TestGetStopInfo();
void TestDistances();


} // namespace transport_catalogue::tests

namespace util {

namespace view {

// Returns a substring [start_pos, end_pos) of a given string_view 
std::string_view Substr(std::string_view view, size_t start_pos, size_t end_pos);

/* Moves the start of given view to the first non-to_remove character
 * and moves the end of given view to the last non-to_remove character */
std::string_view Trim(std::string_view view, char to_remove = ' ');

// Splits a string_view into a vector of string_views, delimited by a delim char
std::vector<std::string_view> SplitBy(std::string_view view, const char delim = ' ');

} // namespace transport_catalogue::util::view

} // namespace transport_catalogue::util

} // namespace transport_catalogue 
