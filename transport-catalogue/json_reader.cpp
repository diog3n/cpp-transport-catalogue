#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include "json_reader.hpp"
#include "transport_catalogue.hpp"

namespace json_reader {

using namespace std::literals;

JSONReader(std::unique_ptr<transport_catalogue::TransportCatalogue>&& tc_ptr)

JSONReader(std::unique_ptr<request_handler::RequestHandler>&& request_handler_ptr);

JSONReader(const transport_catalogue::TransportCatalogue& tc);

JSONReader(const request_handler::RequestHandler& request_handler);

void JSONReader::ParseQueries() {
    const json::Dict& root_map = json_.GetRoot().AsMap();

    const json::Array& base_requests = root_map.at("base_requests").AsArray();
    const json::Array& stat_requests = root_map.at("stat_requests").AsArray();

    std::for_each(base_requests.begin(), base_requests.end(), [this](const json::Node& node) {
        const json::Dict& query_map = node.AsMap();

        const std::string_view type = query_map.at("type").AsString();

        if (type == "Stop"sv) {
            stop_input_queries_.push_back(AssembleStopInputQuery(node));
        } else if (type == "Bus"sv) {
            bus_input_queries_.push_back(AssembleBusInputQuery(node));
        } else {
            throw std::invalid_argument("Unknown query type: " + std::string(type));
        }
    });

    std::for_each(stat_requests.begin(), stat_requests.end(), [this](const json::Node& node) {
        const json::Dict& query_map = node.AsMap();

        const std::string_view type = query_map.at("type").AsString();

        if (type == "Stop"sv) {
            stop_output_queries_.push_back(AssembleStopOutputQuery(node));
        } else if (type == "Bus"sv) {
            bus_output_queries_.push_back(AssembleBusOutputQuery(node));
        } else {
            throw std::invalid_argument("Unknown query type: " + std::string(type));
        }
    });
}

void JSONReader::LoadJSON(const std::string& document) {    
    std::istringstream in(document);
    json_ = json::Load(in);

    ParseQueries();
}

void JSONReader::LoadJSON(std::istream& in) {
    json_ = json::Load(in);

    ParseQueries();
}

domain::StopOutputQuery JSONReader::AssembleStopOutputQuery(const json::Node& query_node) const {
    const json::Dict& request_map = query_node.AsMap();
    const int id = request_map.at("id").AsInt(); 
    const std::string_view name = request_map.at("name").AsString();

    return { id, name };
}

domain::BusOutputQuery JSONReader::AssembleBusOutputQuery(const json::Node& query_node) const {
    const json::Dict& request_map = query_node.AsMap();
    const int id = request_map.at("id").AsInt(); 
    const std::string_view name = request_map.at("name").AsString();

    return { id, name };
}

domain::BusInputQuery JSONReader::AssembleBusInputQuery(const json::Node& query_node) const {
    const json::Dict& request_map = query_node.AsMap();
    const std::string_view bus_name = request_map.at("name").AsString();
    const json::Array& stops_array = request_map.at("stops").AsArray();

    std::vector<std::string_view> stops;

    for (const json::Node& node : stops_array) {
        stops.push_back(node.AsString());
    }

    return { bus_name, std::move(stops) };
}

domain::StopInputQuery JSONReader::AssembleStopInputQuery(const json::Node& query_node) const {
    const json::Dict& request_map = query_node.AsMap();
    const std::string_view stop_name = request_map.at("name").AsString();
    const geo::Coordinates coordinates{ request_map.at("longitude").AsDouble(),
                                        request_map.at("latitude").AsDouble() };
    
    std::unordered_map<std::string_view, int> distances;

    for (const auto& [name, node] : request_map.at("road_distances").AsMap()) {
        distances[name] = node.AsInt();
    }

    return { stop_name, std::move(coordinates), std::move(distances) };
}

namespace tests {

void TestAssembleQuery() {
    transport_catalogue::TransportCatalogue tc;

    JSONReader jreader(std::make_unique<transport_catalogue::TransportCatalogue>(tc));

    std::istringstream input{
        "{"
            "\"base_requests\": ["
                "{"
                "  \"type\": \"Bus\","
                "  \"name\": \"114\","
                "  \"stops\": [\"Морской вокзал\", \"Ривьерский мост\"],"
                "  \"is_roundtrip\": false"
                "},"
                "{"
                "  \"type\": \"Stop\","
                "  \"name\": \"Ривьерский мост\","
                "  \"latitude\": 43.587795,"
                "  \"longitude\": 39.716901,"
                "  \"road_distances\": {\"Морской вокзал\": 850}"
                "},"
                "{"
                "  \"type\": \"Stop\","
                "  \"name\": \"Морской вокзал\","
                "  \"latitude\": 43.581969,"
                "  \"longitude\": 39.719848,"
                "  \"road_distances\": {\"Ривьерский мост\": 850}"
                "}"
            "],"
          "\"stat_requests\": ["
          "  { \"id\": 1, \"type\": \"Stop\", \"name\": \"Ривьерский мост\" },"
          "  { \"id\": 2, \"type\": \"Bus\", \"name\": \"114\" }"
          "]"
        "}" 
    };

    jreader.LoadJSON(input);


}

} // namespace json_reader::tests

} // namespace json_reader