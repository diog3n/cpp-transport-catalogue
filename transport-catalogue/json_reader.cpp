#include <algorithm>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include "domain.hpp"
#include "json.hpp"
#include "json_reader.hpp"
#include "request_handler.hpp"
#include "transport_catalogue.hpp"

namespace json_reader {

using namespace std::literals;

namespace util {

json::Node AssembleBusNode(domain::BusInfo& bus_info, int id) {
    if (bus_info.type == domain::InfoType::VALID || bus_info.type == domain::EMPTY) {
        return json::Dict{ { "request_id", id },
                           { "curvature", bus_info.curvature},
                           { "route_length", bus_info.route_length },
                           { "stop_count", static_cast<int>(bus_info.stops_on_route) },
                           { "unique_stop_count", static_cast<int>(bus_info.unique_stops) } };
    }
    
    return json::Dict{ { "request_id", id }, { "error_message", "not found" } };
}

json::Node AssembleStopNode(domain::StopInfo& stop_info, int id) {
    if (stop_info.type == domain::InfoType::VALID) {
        json::Array bus_array; 
        bus_array.reserve(stop_info.bus_names.size());

        for (const std::string_view view : stop_info.bus_names) {
            bus_array.push_back(std::string(view));
        }

        return json::Dict{ { "buses", bus_array },
                           { "request_id", id } };
    } else if (stop_info.type == domain::InfoType::EMPTY) {
        return json::Dict{ { "request_id", id },
                           { "error_message", "no buses" } };
    } else {
        return json::Dict{ { "request_id", id }, { "error_message", "not found" } };
    }
}

} // namespace json_reader::util

JSONReader::JSONReader(const transport_catalogue::TransportCatalogue& tc)
    : query_handler::QueryHandler(tc)
    , json_(json::Document{nullptr}) {}

JSONReader::JSONReader(std::unique_ptr<transport_catalogue::TransportCatalogue>&& tc_ptr)
    : query_handler::QueryHandler(std::move(tc_ptr))
    , json_(json::Document{nullptr}) {}

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
    ExecuteInputQueries();
}

void JSONReader::LoadJSON(std::istream& in) {
    json_ = json::Load(in);

    ParseQueries();
    ExecuteInputQueries();
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

void JSONReader::ExecuteOutputQueries(query_handler::OutputContext& context) const {
    auto bus_iter = bus_output_queries_.begin();
    auto stop_iter = stop_output_queries_.begin();
    json::Array output_array;

    while (true) {
        if ((bus_iter != bus_output_queries_.end() 
             && stop_iter == stop_output_queries_.end()) 
             || bus_iter->id < stop_iter->id) {
            domain::BusInfo bus_info = catalogue_ptr_->GetBusInfo(bus_iter->bus_name);

            output_array.push_back(util::AssembleBusNode(bus_info, bus_iter->id));
            bus_iter++;
            continue;
        } else if ((bus_iter == bus_output_queries_.end() 
                    && stop_iter != stop_output_queries_.end()) 
                    || bus_iter->id > stop_iter->id) {
            domain::StopInfo stop_info = catalogue_ptr_->GetStopInfo(stop_iter->stop_name);

            output_array.push_back(util::AssembleStopNode(stop_info, stop_iter->id));
            stop_iter++;
            continue;
        } else {
            throw std::logic_error("Same ID for different queries");
        }
        break;
    }

    json::Document output_doc{output_array};
    json::Print(output_doc, context.out);
}

void JSONReader::PrintTo(std::ostream& out) const {
    query_handler::OutputContext context(out);
    ExecuteOutputQueries(context);
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