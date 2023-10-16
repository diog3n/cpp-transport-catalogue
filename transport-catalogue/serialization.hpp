#pragma once
#include "transport_catalogue.hpp"
#include <filesystem>

namespace serialization {

struct SerializationSettings {
    std::filesystem::path filename;
};

namespace transport_catalogue {

using TransportCatalogue = ::transport_catalogue::TransportCatalogue;

void SerializeTransportCatalogue(const TransportCatalogue&, std::ostream& out);
TransportCatalogue DeserializeTransportCatalogue(std::istream& in);

} // namespace serialization::transport_catalogue

} // namespace serialize