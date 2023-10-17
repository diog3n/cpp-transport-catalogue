#include "serialization.h"
#include "domain.h"
#include "geo.h"
#include "transport_catalogue.h"
#include "transport_catalogue.pb.h"
#include <algorithm>
#include <iterator>
#include <sstream>

namespace serialization {

namespace transport_catalogue {

std::vector<domain::BusPtr> TransportCatalogueSerializer::GetBusPtrs(
                                          const TransportCatalogue& catalogue) {
    const std::vector<std::string_view>& bus_names = catalogue.GetBusNames();
    std::vector<domain::BusPtr> bus_ptrs;

    bus_ptrs.reserve(bus_names.size());

    for (const auto bus_name : bus_names) {
        bus_ptrs.push_back(catalogue.FindBus(bus_name));
    }

    return bus_ptrs;
}

std::vector<domain::StopPtr> TransportCatalogueSerializer::GetStopPtrs(
                                          const TransportCatalogue& catalogue) {
    const std::vector<std::string_view>& stop_names = catalogue.GetStopNames();

    std::vector<domain::StopPtr> stop_ptrs;

    stop_ptrs.reserve(stop_names.size());

    for (const auto stop_name : stop_names) {
        stop_ptrs.push_back(catalogue.FindStop(stop_name));
    }

    return stop_ptrs;
}

size_t TransportCatalogueSerializer::GetStopIndex(
                    const std::vector<domain::StopPtr>& stop_ptrs,
                    const std::string& stop_name) {
    auto stop_iter = std::lower_bound(stop_ptrs.begin(),
                                      stop_ptrs.end(), 
                                      stop_name,
    [](const domain::StopPtr val, const std::string& stop_name) {
        return val->name < stop_name;
    });

    return std::distance(stop_ptrs.begin(), stop_iter);
}

serialize::Bus TransportCatalogueSerializer::BuildSerializedBus(
                                const domain::Bus& bus,
                                const std::vector<domain::StopPtr> stop_ptrs) {
    serialize::Bus serialized_bus;

    serialized_bus.set_name(bus.name);
    serialized_bus.set_is_roundtrip(bus.is_roundtrip);
    
    for (const domain::StopPtr stop_ptr : bus.route) {
        serialized_bus.add_stop_indexes(GetStopIndex(stop_ptrs, stop_ptr->name));
    }

    return serialized_bus;
}

serialize::Stop TransportCatalogueSerializer::BuildSerializedStop(
                                                     const domain::Stop& stop) {
    serialize::Stop serialized_stop;

    serialize::Coordinates serialized_coords;
    serialized_coords.set_lat(stop.coordinates.lat);
    serialized_coords.set_lng(stop.coordinates.lng);

    serialized_stop.set_name(stop.name);
    *serialized_stop.mutable_coordinates() = serialized_coords;

    return serialized_stop;
}

serialize::StopDistance TransportCatalogueSerializer::BuildSerializedDistance(
                   const std::pair<domain::StopPtr, domain::StopPtr>& stop_pair,
                   int distance, 
                   const std::vector<domain::StopPtr>& stop_ptrs) {
    serialize::StopDistance serialized_distance;

    serialized_distance.set_l_stop_index(GetStopIndex(stop_ptrs, 
                                                      stop_pair.first->name));
    serialized_distance.set_r_stop_index(GetStopIndex(stop_ptrs, 
                                                      stop_pair.second->name));
    serialized_distance.set_distance(distance);

    return serialized_distance;
}

void TransportCatalogueSerializer::SerializeTransportCatalogue(
                                const TransportCatalogue& catalogue, 
                                std::ostream &out) {
    serialize::TransportCatalogue tc;
    using DistanceMap = TransportCatalogue::DistanceMap;

    const std::vector<domain::StopPtr> stop_ptrs = GetStopPtrs(catalogue);
    const std::vector<domain::BusPtr> bus_ptrs = GetBusPtrs(catalogue);
    const DistanceMap& distance_map = catalogue.GetDistanceMap();

    for (const domain::StopPtr stop_ptr : stop_ptrs) {
        *tc.add_stops() = BuildSerializedStop(*stop_ptr);
    }

    for (const domain::BusPtr bus_ptr : bus_ptrs) {
        *tc.add_buses() = BuildSerializedBus(*bus_ptr, stop_ptrs);
    }

    for (const auto& [stop_pair, distance] : distance_map) {
        *tc.add_distances() = BuildSerializedDistance(stop_pair, 
                                                      distance, 
                                                      stop_ptrs);
    }

    tc.SerializeToOstream(&out);
}

void TransportCatalogueSerializer::DeserializeAndAddStop(
                                    const serialize::Stop serialized_stop,
                                    TransportCatalogue& catalogue) {
    geo::Coordinates coords;

    coords.lat = serialized_stop.coordinates().lat();
    coords.lng = serialized_stop.coordinates().lng();

    catalogue.AddStop(serialized_stop.name(), coords);
}

void TransportCatalogueSerializer::DeserializeAndAddBus(
                                const serialize::Bus serialized_bus, 
                                const std::vector<domain::StopPtr>& stop_ptrs,
                                TransportCatalogue& catalogue) {
    std::vector<std::string_view> stop_names;

    for (const auto& stop_index : serialized_bus.stop_indexes()) {
        stop_names.push_back(stop_ptrs.at(stop_index)->name);
    }

    catalogue.AddBus(serialized_bus.name(), 
                     stop_names, 
                     serialized_bus.is_roundtrip());
}

void TransportCatalogueSerializer::DeserializeAndAddDistance(
                            const serialize::StopDistance serialized_distance,
                            const std::vector<domain::StopPtr>& stop_ptrs,
                            TransportCatalogue& catalogue) {
    catalogue.AddDistance(
        stop_ptrs.at(serialized_distance.l_stop_index())->name,
        stop_ptrs.at(serialized_distance.r_stop_index())->name,
        serialized_distance.distance()
    );
}

TransportCatalogueSerializer::TransportCatalogue 
TransportCatalogueSerializer::DeserializeTransportCatalogue(std::istream &in) {
    TransportCatalogue catalogue;

    serialize::TransportCatalogue serialized_catalogue;

    serialized_catalogue.ParseFromIstream(&in);

    for (const auto& serialized_stop : serialized_catalogue.stops()) {
        DeserializeAndAddStop(serialized_stop, catalogue);
    }

    const std::vector<domain::StopPtr> stop_ptrs = GetStopPtrs(catalogue);

    for (const auto& serialized_bus : serialized_catalogue.buses()) {
        DeserializeAndAddBus(serialized_bus, stop_ptrs, catalogue);
    }

    for (const auto& serialized_distance : serialized_catalogue.distances()) {
        DeserializeAndAddDistance(serialized_distance, stop_ptrs, catalogue);
    }

    return catalogue;
}

namespace tests {

void TestTransportCatalogueSerialization() {
    using namespace std::literals;
    using TransportCatalogue = ::transport_catalogue::TransportCatalogue;

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

    TestBusInfo bus1{ "256"sv, 
                        { 
                            "Marushkino"sv, 
                            "Tolstopaltsevo"sv, 
                            "Marushkino"sv,
                            "Biryusinka Miryusinka"sv
                        } };
    TestBusInfo bus2{ "11"sv, 
                        { 
                            "Tolstopaltsevo"sv, 
                            "Biryusinka Miryusinka"sv, 
                            "Tolstopaltsevo"sv 
                        } };

    tc.AddStop(stop1.name, stop1.coordinates);
    tc.AddStop(stop2.name, stop2.coordinates);
    tc.AddStop(stop3.name, stop3.coordinates);

    tc.AddBus(bus1.name, bus1.stop_names, false);
    tc.AddBus(bus2.name, bus2.stop_names, true);
    
    for (const auto& [stop_name, distance] : stop1.distances) {
        tc.AddDistance(stop1.name, stop_name, distance);
    }

    for (const auto& [stop_name, distance] : stop3.distances) {
        tc.AddDistance(stop3.name, stop_name, distance);
    }

    std::ostringstream output(std::ios::binary);

    TransportCatalogueSerializer::SerializeTransportCatalogue(tc, output);

    std::istringstream input(output.str(), std::ios::binary);

    TransportCatalogue deserialized_tc = 
             TransportCatalogueSerializer::DeserializeTransportCatalogue(input);

    assert(deserialized_tc.GetStopNames() == tc.GetStopNames());

    assert(deserialized_tc.GetBusNames() == tc.GetBusNames());

    assert(deserialized_tc.GetDistance("Biryusinka Miryusinka"sv, 
                                       "Marushkino"sv) == 1500);
    assert(deserialized_tc.GetDistance("Marushkino"sv,
                                       "Tolstopaltsevo"sv) == 200);
       
}

} // namespace serialization::transport_catalogue::tests

} // namespace serialization::transport_catalogue

} // namespace serialization