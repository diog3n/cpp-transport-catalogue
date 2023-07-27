#include <algorithm>
#include <cassert>
#include <exception>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include "domain.hpp"
#include "json.hpp"
#include "json_reader.hpp"

namespace json_reader {

using namespace std::literals;

namespace util {

json::Node AssembleErrorNode(const int id, const domain::InfoType& type) {
    json::Dict result;
    result["request_id"s] = id;

    result["error_message"s] = type == domain::InfoType::NOT_FOUND 
                               ? "not found"s 
                               : nullptr;
    return result;
}

json::Node AssembleBusNode(domain::BusInfo& bus_info, int id) {
    if (bus_info.type == domain::InfoType::VALID || bus_info.type == domain::EMPTY) {
        return json::Dict{ { "request_id"s, id },
                           { "curvature"s, bus_info.curvature},
                           { "route_length"s, bus_info.route_length },
                           { "stop_count"s, static_cast<int>(bus_info.stops_on_route) },
                           { "unique_stop_count"s, static_cast<int>(bus_info.unique_stops) } };
    }
    
    return AssembleErrorNode(id, bus_info.type);
}

json::Node AssembleStopNode(domain::StopInfo& stop_info, int id) {
    if (stop_info.type == domain::InfoType::VALID 
        || stop_info.type == domain::InfoType::EMPTY) {
        
        json::Array bus_array; 
        bus_array.reserve(stop_info.bus_names.size());

        for (const std::string_view view : stop_info.bus_names) {
            bus_array.push_back(std::string(view));
        }

        return json::Dict{ { "buses"s, bus_array },
                           { "request_id"s, id } };
    }
    
    return AssembleErrorNode(id, stop_info.type);
}

void PrintLnBusInfo(std::ostream& out, domain::BusInfo bus_info) {
    out << "name:           " << bus_info.name << std:: endl
        << "curvature:      " << bus_info.curvature << std:: endl
        << "route_length:   " << bus_info.route_length << std:: endl
        << "stops_on_route: " << bus_info.stops_on_route << std:: endl
        << "unique_stops:   " << bus_info.unique_stops << std:: endl;
}

void PrintLnStopInfo(std::ostream& out, domain::StopInfo stop_info) {
    out << "name: " << stop_info.name << std::endl;
    for (const auto& bus_name : stop_info.bus_names) {
        out << "bus:  " << bus_name << std::endl;
    }
}

} // namespace json_reader::util

JSONReader::JSONReader(const transport_catalogue::TransportCatalogue& tc)
    : handlers::QueryHandler(tc)
    , json_(json::Document{nullptr}) {}

JSONReader::JSONReader(std::unique_ptr<transport_catalogue::TransportCatalogue>&& tc_ptr)
    : handlers::QueryHandler(std::move(tc_ptr))
    , json_(json::Document{nullptr}) {}

void JSONReader::ParseQueries() {
    const json::Dict& root_map = json_.GetRoot().AsMap();

    const json::Array& base_requests = root_map.at("base_requests"s).AsArray();
    const json::Array& stat_requests = root_map.at("stat_requests"s).AsArray();

    std::for_each(base_requests.begin(), base_requests.end(), [this](const json::Node& node) {
        const json::Dict& query_map = node.AsMap();

        const std::string_view type = query_map.at("type"s).AsString();

        if (type == "Stop"sv) {
            stop_input_queries_.push_back(AssembleStopInputQuery(node));
        } else if (type == "Bus"sv) {
            bus_input_queries_.push_back(AssembleBusInputQuery(node));
        } else {
            throw std::invalid_argument("Unknown query type: "s + std::string(type));
        }
    });

    std::for_each(stat_requests.begin(), stat_requests.end(), [this](const json::Node& node) {
        const json::Dict& query_map = node.AsMap();

        const std::string_view type = query_map.at("type"s).AsString();

        if (type == "Stop"sv) {
            stop_output_queries_.push_back(AssembleStopOutputQuery(node));
            query_ptrs_.push_back(&stop_output_queries_.back());
        } else if (type == "Bus"sv) {
            bus_output_queries_.push_back(AssembleBusOutputQuery(node));
            query_ptrs_.push_back(&bus_output_queries_.back());
        } else {
            throw std::invalid_argument("Unknown query type: "s + std::string(type));
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
    const int id = request_map.at("id"s).AsInt(); 
    const std::string_view name = request_map.at("name"s).AsString();

    return { id, name };
}

domain::BusOutputQuery JSONReader::AssembleBusOutputQuery(const json::Node& query_node) const {
    const json::Dict& request_map = query_node.AsMap();
    const int id = request_map.at("id"s).AsInt(); 
    const std::string_view name = request_map.at("name"s).AsString();

    return { id, name };
}

domain::BusInputQuery JSONReader::AssembleBusInputQuery(const json::Node& query_node) const {
    const json::Dict& request_map = query_node.AsMap();
    const std::string_view bus_name = request_map.at("name"s).AsString();
    const json::Array& stops_array = request_map.at("stops"s).AsArray();
    const bool is_roundtrip = request_map.at("is_roundtrip"s).AsBool();

    std::vector<std::string_view> stops;

    for (auto iter = stops_array.begin(); 
         iter != stops_array.end(); iter++) {
        stops.push_back(iter->AsString());
    }

    if (!is_roundtrip) for (auto riter = stops_array.rbegin() + 1; 
                            riter != stops_array.rend(); riter++) {
        stops.push_back(riter->AsString());
    }

    return { bus_name, std::move(stops) };
}

domain::StopInputQuery JSONReader::AssembleStopInputQuery(const json::Node& query_node) const {
    const json::Dict& request_map = query_node.AsMap();
    const std::string_view stop_name = request_map.at("name"s).AsString();
    const geo::Coordinates coordinates{ request_map.at("latitude"s).AsDouble(),
                                        request_map.at("longitude"s).AsDouble() };
    
    std::unordered_map<std::string_view, int> distances;

    for (const auto& [name, node] : request_map.at("road_distances"s).AsMap()) {
        distances[name] = node.AsInt();
    }

    return { stop_name, std::move(coordinates), std::move(distances) };
}

void JSONReader::ExecuteOutputQueries(handlers::OutputContext& context) const {
    json::Array output_array;

    std::for_each(query_ptrs_.begin(), query_ptrs_.end(), 
    [this, &output_array](const domain::OutputQuery* query_ptr) {
        if (query_ptr->type == domain::QueryType::STOP) {
            domain::StopInfo stop_info = catalogue_ptr_->GetStopInfo(query_ptr->name);
            util::PrintLnStopInfo(std::cerr, stop_info);
            output_array.push_back(util::AssembleStopNode(stop_info, query_ptr->id));

        } else if (query_ptr->type == domain::QueryType::BUS) {
            domain::BusInfo bus_info = catalogue_ptr_->GetBusInfo(query_ptr->name);
            util::PrintLnBusInfo(std::cerr, bus_info);
            output_array.push_back(util::AssembleBusNode(bus_info, query_ptr->id));
        }
    });

    json::Document output_doc{output_array};
    json::Print(output_doc, context.out);
}

void JSONReader::PrintTo(std::ostream& out) const {
    handlers::OutputContext context(out);
    ExecuteOutputQueries(context);
}

/*std::string JSONReader::ReadJSON(std::istream& in) {
    char c;
    char brace;
    char rbrace;
    int brace_count = 1;
    std::string result;

    in >> c;
    result += c;

    if (c == '{') brace = '{', rbrace = '}';
    else if (c == '[') brace = '[', rbrace = ']';
    else throw json::ParsingError("Not a JSON");

    while (brace_count != 0) {
        in >> c;
        if (c == brace) brace_count++;
        else if (c == rbrace) brace_count--;

        result += c;

        if (brace_count < 0) throw json::ParsingError("Invalid JSON");
    }

    return result;
}*/

std::string JSONReader::ReadJSON(std::istream& in) {
    std::vector<std::string> lines;
    std::string line;

    std::string result;

    char brace;
    char rbrace;
    int brace_count = 0;

    while (getline(in, line)) lines.push_back(line);

    if (lines.front().front() == '{') brace = '{', rbrace = '}';
    else if (lines.front().front() == '[') brace = '[', rbrace = ']';
    else throw json::ParsingError("Not a JSON");

    for (const std::string& line : lines) {
        for (const char c : line) {
            if (c == brace) brace_count++;
            else if (c == rbrace) brace_count--;

            result += c;

            if (brace_count < 0) throw json::ParsingError("Invalid JSON");
        }
    }

    if (brace_count != 0) throw json::ParsingError("Invalid JSON");
    return result;
}

const json::Document& JSONReader::GetDoc() const {
    return json_;
}

namespace tests {

void TestAssembleQuery() {
    transport_catalogue::TransportCatalogue tc;

    JSONReader jreader(std::make_unique<transport_catalogue::TransportCatalogue>(tc));

    std::istringstream input1{
        "{"
        "   \"base_requests\": ["
        "       {"
        "          \"type\": \"Bus\","
        "          \"name\": \"114\","
        "          \"stops\": [\"Морской вокзал\", \"Ривьерский мост\"],"
        "          \"is_roundtrip\": false"
        "       },"
        "       {"
        "          \"type\": \"Stop\","
        "          \"name\": \"Ривьерский мост\","
        "          \"latitude\": 43.587795,"
        "          \"longitude\": 39.716901,"
        "          \"road_distances\": {\"Морской вокзал\": 850}"
        "        },"
        "        {"
        "          \"type\": \"Stop\","
        "          \"name\": \"Морской вокзал\","
        "          \"latitude\": 43.581969,"
        "          \"longitude\": 39.719848,"
        "          \"road_distances\": {\"Ривьерский мост\": 850, \"Наличная улица\": 1000}"
        "        },"
        "        {"
        "          \"type\": \"Stop\","
        "          \"name\": \"Наличная улица\","
        "          \"latitude\": 41.581969,"
        "          \"longitude\": 33.719548,"
        "          \"road_distances\": {\"Ривьерский мост\": 1850}"
        "        }"
        "        {"
        "          \"type\": \"Bus\","
        "          \"name\": \"41\","
        "          \"stops\": [\"Морской вокзал\", \"Наличная улица\", \"Морской вокзал\"],"
        "          \"is_roundtrip\": true"
        "         }"
        "    ],"
        "    \"stat_requests\": ["
        "        { \"id\": 1, \"type\": \"Stop\", \"name\": \"Ривьерский мост\" },"
        "        { \"id\": 2, \"type\": \"Bus\", \"name\": \"114\" },"
        "        { \"id\": 3, \"type\": \"Bus\", \"name\": \"45\" },"
        "        { \"id\": 4, \"type\": \"Stop\", \"name\": \"У-м Гаванский\" },"
        "        { \"id\": 5, \"type\": \"Stop\", \"name\": \"Наличная улица\" }"
        "        { \"id\": 6, \"type\": \"Bus\", \"name\": \"41\" }"
        "    ]"
        "}" 
    };

    jreader.LoadJSON(input1);

    std::ostringstream out;

    jreader.PrintTo(out);

    std::istringstream input2(out.str());

    std::istringstream input3("[ { \"buses\": [ \"114\" ], \"request_id\": 1 }, { \"curvature\": 1.23199, \"request_id\": 2, \"route_length\": 1700, \"stop_count\": 3, \"unique_stop_count\": 2 }, { \"error_message\": \"not found\", \"request_id\": 3 }, { \"error_message\": \"not found\", \"request_id\": 4 }, { \"buses\": [ \"41\" ], \"request_id\": 5 }, { \"curvature\": 0.00185499, \"request_id\": 6, \"route_length\": 2000, \"stop_count\": 3, \"unique_stop_count\": 2 } ]");

    bool test_load = json::Load(input2) == json::Load(input3);

    assert(test_load);
}

} // namespace json_reader::tests

} // namespace json_reader