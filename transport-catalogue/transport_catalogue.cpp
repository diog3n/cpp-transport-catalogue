#include <algorithm>
#include <cassert>
#include <iterator>
#include <numeric>
#include <string_view>
#include <set>

#include "geo.hpp"
#include "input_reader.hpp"
#include "transport_catalogue.hpp"

namespace transport_catalogue {

// Adds a stop to the transport catalogue. This operation involves population 
// of stops_ deque as well as population of names_to_stops_ index
void TransportCatalogue::AddStop(const std::string_view name, const geo::Coordinates& coordinates) {
    stops_.emplace_back(std::string(name), coordinates, std::set<Bus*, BusCompare>());
    names_to_stops_[stops_.back().name] = &stops_.back();
}

// Adds a stop to the transport catalogue. This operation involves population 
// of buses_ deque as well as population of names_to_buses_ index
void TransportCatalogue::AddBus(const std::string_view name, const std::vector<std::string_view>& stop_names) {
    buses_.emplace_back(std::string(name), std::vector<Stop*>());
    
    Bus& bus = buses_.back();

    for (const std::string_view stop_name : stop_names) {
        bus.route.push_back(names_to_stops_.at(stop_name));
        names_to_stops_.at(stop_name)->buses.insert(&bus);
    }

    names_to_buses_[buses_.back().name] = &buses_.back();
}

// Adds a distance between stops. Stops existence is required
void TransportCatalogue::AddDistance(const std::string_view stop_from, const std::string_view stop_to, const int distance) {
    std::pair<Stop*, Stop*> stop_pair{ 
        names_to_stops_.at(stop_from),
        names_to_stops_.at(stop_to)
    };
    stop_distances_[stop_pair] = distance;
}

// Finds a bus by name and returns a reference to its struct 
// contained in buses_ deque
const Bus& TransportCatalogue::FindBus(const std::string_view name) const {
    return *names_to_buses_.at(name);
}

// Finds a stop by name and returns a reference to its struct
// contained in stops_ deque
const Stop& TransportCatalogue::FindStop(const std::string_view name) const {
    return *names_to_stops_.at(name);
}

// Returns info on a given bus in a specific format
BusInfo TransportCatalogue::GetBusInfo(const std::string_view name) const {
    if (names_to_buses_.count(name) == 0) return { InfoType::NOT_FOUND, std::string(name), 0, 0, 0.0, 0.0 };

    const Bus& bus = *names_to_buses_.at(name);
    const InfoType type = bus.route.empty() ? InfoType::EMPTY : InfoType::VALID;
    const double route_distance_geo = ComputeRouteDistance(bus);
    const double route_distance_cur = ComputeCurvedRouteDistance(bus);
    return { type, bus.name, bus.route.size(), 
             CountUniqueStops(bus), 
             route_distance_cur,
             ComputeCurvature(route_distance_cur, route_distance_geo) };
}

// Returns info on a given stop in a specific format
StopInfo TransportCatalogue::GetStopInfo(const std::string_view name) const {
    if (names_to_stops_.count(name) == 0) return { InfoType::NOT_FOUND, std::string(name), {} };

    const Stop& stop = *names_to_stops_.at(name);
    const InfoType type = stop.buses.empty() ? InfoType::EMPTY : InfoType::VALID;

    StopInfo stop_info = { type, std::string(name), std::vector<std::string_view>() };
    std::transform(stop.buses.begin(), stop.buses.end(), std::back_inserter(stop_info.bus_names), [](const Bus* bus_ptr) {
        return std::string_view(bus_ptr->name);
    });
    return stop_info;
}

// Returns a distance between stops. Distances between same stops may be different 
// depending on direction. If a distance between stops in a given direction is not found,
// function will try to look for a distance in the opposite direction
int TransportCatalogue::GetDistance(const std::string_view stop_from, const std::string_view stop_to) const {
    if (stop_distances_.count({ names_to_stops_.at(stop_from), names_to_stops_.at(stop_to) }) > 0) return stop_distances_.at({ names_to_stops_.at(stop_from), names_to_stops_.at(stop_to) });
    return stop_distances_.at({ names_to_stops_.at(stop_to), names_to_stops_.at(stop_from) });
}

// Counts unique stops in given bus's route
size_t TransportCatalogue::CountUniqueStops(const Bus& bus) {
    std::set<const Stop*> unique_stops;
    for (const Stop* stop_ptr : bus.route) {
        unique_stops.insert(stop_ptr);
    }
    return unique_stops.size();
}

// Computes a route distance for a given bus
double TransportCatalogue::ComputeRouteDistance(const Bus& bus) {
    const std::vector<Stop*>& route = bus.route;
    double result = 0.0;

    for (auto lhs = route.begin(), rhs = route.begin() + 1; rhs != route.end(); lhs++, rhs++) {
        result += ComputeDistance((*lhs)->coordinates, (*rhs)->coordinates);
    }

    return result;
}

// Computes route's curvature
double TransportCatalogue::ComputeCurvature(const double curved_distance, const double geo_distance) {
    return curved_distance/geo_distance;
}

double TransportCatalogue::ComputeCurvedRouteDistance(const Bus& bus) const {
    double route_length = 0.0;

    for (auto from_iter = bus.route.begin(), to_iter = bus.route.begin() + 1; to_iter != bus.route.end(); to_iter++, from_iter++) {
        route_length += static_cast<double>(GetDistance((*from_iter)->name, (*to_iter)->name));
    }

    return route_length;
}


namespace tests {

using namespace std::literals;

void TestAddFindMethods() {
    struct TestBusInfo { std::string_view name; std::vector<std::string_view> stop_names; };
    struct TestStopInfo { std::string_view name; geo::Coordinates coordinates; };
    
    TransportCatalogue tc;

    TestStopInfo stop1{ "Marushkino"sv, { 55.595884, 37.209755 } };
    TestStopInfo stop2{ "Tolstopaltsevo"sv, { 55.611087, 37.208290 } };
    TestStopInfo stop3{ "Biryusinka Miryusinka"sv, { 55.581065, 37.648390 } };

    TestBusInfo bus1{ "256"sv, { "Marushkino"sv, "Tolstopaltsevo"sv, "Marushkino"sv } };
    TestBusInfo bus2{ "47"sv, { "Tolstopaltsevo"sv, "Biryusinka Miryusinka"sv, "Marushkino"sv, "Biryusinka Miryusinka"sv, "Tolstopaltsevo"sv } };
    TestBusInfo bus3{ "11"sv, { "Tolstopaltsevo"sv, "Marushkino"sv, "Tolstopaltsevo"sv } };

    tc.AddStop(stop1.name, stop1.coordinates);
    tc.AddStop(stop2.name, stop2.coordinates);
    tc.AddStop(stop3.name, stop3.coordinates);

    tc.AddBus(bus1.name, bus1.stop_names); 
    tc.AddBus(bus2.name, bus2.stop_names); 
    tc.AddBus(bus3.name, bus3.stop_names); 

    const Stop& stop1_ref = tc.FindStop("Marushkino"s);
    const Stop& stop2_ref = tc.FindStop("Tolstopaltsevo"s);
    const Stop& stop3_ref = tc.FindStop("Biryusinka Miryusinka"s);

    const Bus& bus1_ref = tc.FindBus("256"s);
    const Bus& bus2_ref = tc.FindBus("47"s);
    const Bus& bus3_ref = tc.FindBus("11"s);

    bool test_ref_1 = !bus1_ref.route.empty();
    bool test_ref_2 = !bus1_ref.route.empty();
    bool test_ref_3 = !bus1_ref.route.empty();

    assert(test_ref_1);
    assert(test_ref_2);
    assert(test_ref_3);

    bool test_route_1 = bus1_ref.route.size() == 3;
    bool test_route_2 = bus2_ref.route.size() == 5;
    bool test_route_3 = bus3_ref.route.size() == 3;

    assert(test_route_1);
    assert(test_route_2);
    assert(test_route_3);

    std::vector<std::string> bus1_stop_names;
    std::transform(bus1_ref.route.begin(), bus1_ref.route.end(), std::back_inserter(bus1_stop_names), [](const Stop* stop_ptr) {
        return stop_ptr->name;
    });

    std::vector<std::string> bus2_stop_names;
    std::transform(bus2_ref.route.begin(), bus2_ref.route.end(), std::back_inserter(bus2_stop_names), [](const Stop* stop_ptr) {
        return stop_ptr->name;
    });

    std::vector<std::string> bus3_stop_names;
    std::transform(bus3_ref.route.begin(), bus3_ref.route.end(), std::back_inserter(bus3_stop_names), [](const Stop* stop_ptr) {
        return stop_ptr->name;
    });

    bool test_names_1 = bus1_stop_names == std::vector{ "Marushkino"s, "Tolstopaltsevo"s, "Marushkino"s };
    bool test_names_2 = bus2_stop_names == std::vector{ "Tolstopaltsevo"s, "Biryusinka Miryusinka"s, "Marushkino"s, "Biryusinka Miryusinka"s, "Tolstopaltsevo"s };
    bool test_names_3 = bus3_stop_names == std::vector{ "Tolstopaltsevo"s, "Marushkino"s, "Tolstopaltsevo"s };

    assert(test_names_1);
    assert(test_names_2);
    assert(test_names_3);

    bool test_stops_1 = stop1_ref.name == "Marushkino"s && stop1_ref.coordinates == geo::Coordinates{ 55.595884, 37.209755 };
    bool test_stops_2 = stop2_ref.name == "Tolstopaltsevo"s && stop2_ref.coordinates == geo::Coordinates{ 55.611087, 37.208290 };
    bool test_stops_3 = stop3_ref.name == "Biryusinka Miryusinka"s && stop3_ref.coordinates == geo::Coordinates{ 55.581065, 37.648390 };

    assert(test_stops_1);
    assert(test_stops_2);
    assert(test_stops_3);
}

void TestGetBusInfo() {
    struct TestBusInfo { std::string_view name; std::vector<std::string_view> stop_names; };
    struct TestStopInfo { std::string_view name; geo::Coordinates coordinates; };
    
    TransportCatalogue tc;

    TestStopInfo stop1{ "Marushkino"sv, { 55.595884, 37.209755 } };
    TestStopInfo stop2{ "Tolstopaltsevo"sv, { 55.611087, 37.208290 } };
    TestStopInfo stop3{ "Biryusinka Miryusinka"sv, { 55.581065, 37.648390 } };

    TestBusInfo bus1{ "256"sv, { "Marushkino"sv, "Tolstopaltsevo"sv, "Marushkino"sv } };

    tc.AddStop(stop1.name, stop1.coordinates);
    tc.AddStop(stop2.name, stop2.coordinates);
    tc.AddStop(stop3.name, stop3.coordinates);

    tc.AddBus(bus1.name, bus1.stop_names);

    tc.AddDistance("Marushkino"sv, "Tolstopaltsevo"sv, 140);
    tc.AddDistance("Tolstopaltsevo"sv, "Marushkino"sv, 280);

    BusInfo bus1_info = tc.GetBusInfo("256"s);
    BusInfo bus2_info = tc.GetBusInfo("240"s);

    bool bus1_test1 = bus1_info.type == InfoType::VALID;
    assert(bus1_test1);
    bool bus1_test2 = bus1_info.name == "256"s;
    assert(bus1_test2);
    bool bus1_test3 = bus1_info.stops_on_route == 3;
    assert(bus1_test3);
    bool bus1_test4 = bus1_info.unique_stops == 2;
    assert(bus1_test4);
    bool bus1_test5 = bus1_info.route_length == 420;
    assert(bus1_test5);

    bool bus2_test1 = bus2_info.type == InfoType::NOT_FOUND;
    assert(bus2_test1);
    bool bus2_test2 = bus2_info.name == "240"s;
    assert(bus2_test2);

}

void TestGetStopInfo() {
    struct TestBusInfo { std::string_view name; std::vector<std::string_view> stop_names; };
    struct TestStopInfo { std::string_view name; geo::Coordinates coordinates; };
    
    TransportCatalogue tc;

    TestStopInfo stop1{ "Marushkino"sv, { 55.595884, 37.209755 } };
    TestStopInfo stop2{ "Tolstopaltsevo"sv, { 55.611087, 37.208290 } };
    TestStopInfo stop3{ "Samara"sv, { 35.611087, 17.208290 } };

    TestBusInfo bus1{ "256"sv, { "Marushkino"sv, "Tolstopaltsevo"sv, "Marushkino"sv } };
    TestBusInfo bus2{ "11"sv, { "Tolstopaltsevo"sv, "Tolstopaltsevo"sv } };

    tc.AddStop(stop1.name, stop1.coordinates);
    tc.AddStop(stop2.name, stop2.coordinates);
    tc.AddStop(stop3.name, stop3.coordinates);

    tc.AddBus(bus1.name, bus1.stop_names);
    tc.AddBus(bus2.name, bus2.stop_names);

    StopInfo stop_info1 = tc.GetStopInfo("Marushkino"s);
    StopInfo stop_info2 = tc.GetStopInfo("Tolstopaltsevo"s);
    StopInfo stop_info3 = tc.GetStopInfo("Samara"s);
    StopInfo stop_info4 = tc.GetStopInfo("Rasskazovka"s);

    bool test_stop_info1 = stop_info1.type == InfoType::VALID && stop_info1.bus_names == std::vector{"256"sv};
    assert(test_stop_info1);
    bool test_stop_info2 = stop_info2.type == InfoType::VALID && (stop_info2.bus_names == std::vector{ "256"sv, "11"sv } || stop_info2.bus_names == std::vector{ "11"sv, "256"sv });
    assert(test_stop_info2);
    bool test_stop_info3 = stop_info3.type == InfoType::EMPTY && stop_info3.bus_names.empty();
    assert(test_stop_info3);
    bool test_stop_info4 = stop_info4.type == InfoType::NOT_FOUND;
    assert(test_stop_info4);

}

void TestDistances() {
    struct TestBusInfo { std::string_view name; std::vector<std::string_view> stop_names; };
    struct TestStopInfo { std::string_view name; geo::Coordinates coordinates; std::unordered_map<std::string_view, int> distances; };
        
    TransportCatalogue tc;

    TestStopInfo stop1{ 
        "Marushkino"sv, 
        { 55.595884, 37.209755 }, 
        { 
            { "Tolstopaltsevo"sv, 200 },
            { "Biryusinka Miryusinka"sv, 500 } 
        }
    };
    
    TestStopInfo stop2{ 
        "Tolstopaltsevo"sv, 
        { 55.611087, 37.208290 }, 
        {}
    };

    TestStopInfo stop3{ 
        "Biryusinka Miryusinka"sv, 
        { 55.581065, 37.648390 },
        {
            { "Marushkino"sv, 1500 }
        }
    };

    tc.AddStop(stop1.name, stop1.coordinates);
    tc.AddStop(stop2.name, stop2.coordinates);
    tc.AddStop(stop3.name, stop3.coordinates);

    for (const auto& [stop_name, distance] : stop1.distances) {
        tc.AddDistance(stop1.name, stop_name, distance);
    }

    for (const auto& [stop_name, distance] : stop3.distances) {
        tc.AddDistance(stop1.name, stop_name, distance);
    }

    bool test1 = tc.GetDistance(stop1.name, stop2.name) == tc.GetDistance(stop2.name, stop1.name);
    assert(test1);

    bool test2 = tc.GetDistance(stop1.name, stop3.name) != tc.GetDistance(stop2.name, stop1.name); 
    assert(test2);
}

} // namespace transport_catalogue::tests


} // namespace transport_catalogue
