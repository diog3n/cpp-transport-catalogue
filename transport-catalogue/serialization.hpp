#pragma once
#include "transport_catalogue.hpp"
#include "transport_catalogue.pb.h"
#include <filesystem>
#include <vector>

namespace serialization {

struct SerializationSettings {
    std::filesystem::path filename;
};

namespace transport_catalogue {
namespace serialize = serialize_transport_catalogue;

class TransportCatalogueSerializer {
public:    
    using TransportCatalogue = ::transport_catalogue::TransportCatalogue;

    static void SerializeTransportCatalogue(const TransportCatalogue& catalogue, 
                                            std::ostream& out);

    static TransportCatalogue DeserializeTransportCatalogue(std::istream& in);

private:
    static std::vector<domain::BusPtr> GetBusPtrs(
                                           const TransportCatalogue& catalogue);

    static std::vector<domain::StopPtr> GetStopPtrs(
                                           const TransportCatalogue& catalogue);

    static serialize::Bus BuildSerializedBus(
                            const domain::Bus& bus,
                            const std::vector<domain::StopPtr> stop_ptrs);

    static serialize::Stop BuildSerializedStop(const domain::Stop& stop);

    static void DeserializeAndAddStop(
                            const serialize::Stop serialized_stop,
                            TransportCatalogue& catalogue);

    static void DeserializeAndAddBus(
                            const serialize::Bus serialized_bus, 
                            const std::vector<domain::StopPtr>& stop_ptrs,
                            TransportCatalogue& catalogue);

    static void DeserializeAndAddDistance(
                            const serialize::StopDistance serialized_distance,
                            const std::vector<domain::StopPtr>& stop_ptrs,
                            TransportCatalogue& catalogue);

    static serialize::StopDistance BuildSerializedDistance(
                    const std::pair<domain::StopPtr, domain::StopPtr>& stop_pair,
                    int distance, 
                    const std::vector<domain::StopPtr>& stop_ptrs);

    static size_t GetStopIndex(const std::vector<domain::StopPtr>& stop_ptrs, 
                               const std::string& stop_name);
};

namespace tests {

void TestTransportCatalogueSerialization();

} // namespace serialization::transport_catalogue::tests

} // namespace serialization::transport_catalogue

} // namespace serialize