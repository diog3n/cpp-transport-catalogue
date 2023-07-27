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
#include "map_renderer.hpp"

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

void JSONReader::ParseDocument() {
    const json::Dict& root_map = json_.GetRoot().AsMap();

    const json::Array& base_requests = root_map.at("base_requests"s).AsArray();
    const json::Array& stat_requests = root_map.at("stat_requests"s).AsArray();
    const json::Node& render_settings = root_map.at("render_settings"s);

    render_settings_ = AssembleRenderSettings(render_settings);

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

    ParseDocument();
    ExecuteInputQueries();
}

void JSONReader::LoadJSON(std::istream& in) {
    json_ = json::Load(in);

    ParseDocument();
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

renderer::RenderSettings JSONReader::AssembleRenderSettings(const json::Node& render_settings) const {
    const json::Dict& settings_map = render_settings.AsMap();
    
    renderer::RenderSettings rs;

    rs.width = settings_map.at("width"s).AsDouble();
    rs.height = settings_map.at("height"s).AsDouble();
    rs.padding = settings_map.at("padding"s).AsDouble();
    rs.stop_radius = settings_map.at("stop_radius"s).AsDouble();
    rs.line_width = settings_map.at("line_width"s).AsDouble();
    rs.bus_label_font_size = settings_map.at("bus_label_font_size"s).AsDouble();
    const json::Array& bus_label_offset_array = settings_map.at("bus_label_offset"s).AsArray();
    rs.bus_label_offset = {
        bus_label_offset_array[0].AsDouble(),
        bus_label_offset_array[1].AsDouble()
    };

    rs.stop_label_font_size = settings_map.at("stop_label_font_size"s).AsDouble();
    const json::Array& stop_label_offset_array = settings_map.at("stop_label_offset"s).AsArray();
    rs.stop_label_offset = {
        stop_label_offset_array[0].AsDouble(),
        stop_label_offset_array[1].AsDouble()
    };

    const json::Node& underlayer_color_node = settings_map.at("underlayer_color"s);
    rs.underlayer_color = ExtractColor(underlayer_color_node);
    rs.underlayer_width = settings_map.at("underlayer_width"s).AsDouble();
    const json::Array& color_palette_array = settings_map.at("color_palette"s).AsArray();
    for (const json::Node& color_node : color_palette_array) {
        rs.color_palette.push_back(ExtractColor(color_node));
    }

    return rs;
}

svg::Color JSONReader::ExtractColor(const json::Node& node) const {
    if (node.IsString()) return node.AsString();
    else if (node.IsArray()) {
        const json::Array& array = node.AsArray();
        if (array.size() == 3) {
            return svg::Rgb {
                static_cast<uint8_t>(array[0].AsInt()), 
                static_cast<uint8_t>(array[1].AsInt()), 
                static_cast<uint8_t>(array[2].AsInt())
            };
        } else if (array.size() == 4) {
            return svg::Rgba {
                static_cast<uint8_t>(array[0].AsInt()),
                static_cast<uint8_t>(array[1].AsInt()),
                static_cast<uint8_t>(array[2].AsInt()),
                array[3].AsDouble()
            };
        } else {
            throw json::ParsingError("Invalid color notation");
        }
    }
    else throw json::ParsingError("Invalid color notation");
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

const renderer::RenderSettings JSONReader::GetRenderSettings() const {
    return render_settings_;
}

namespace tests {

void TestJSON() {
    transport_catalogue::TransportCatalogue tc;

    JSONReader jreader(std::make_unique<transport_catalogue::TransportCatalogue>(tc));

    std::istringstream input1{
    R"({
        "base_requests": [
        {
          "type": "Bus",
          "name": "14",
          "stops": [
            "Ulitsa Lizy Chaikinoi",
            "Elektroseti",
            "Ulitsa Dokuchaeva",
            "Ulitsa Lizy Chaikinoi"
          ],
          "is_roundtrip": true
        },
        {
          "type": "Bus",
          "name": "114",
          "stops": [
            "Morskoy vokzal",
            "Rivierskiy most"
          ],
          "is_roundtrip": false
        },
        {
          "type": "Stop",
          "name": "Rivierskiy most",
          "latitude": 43.587795,
          "longitude": 39.716901,
          "road_distances": {
            "Morskoy vokzal": 850
          }
        },
        {
          "type": "Stop",
          "name": "Morskoy vokzal",
          "latitude": 43.581969,
          "longitude": 39.719848,
          "road_distances": {
            "Rivierskiy most": 850
          }
        },
        {
          "type": "Stop",
          "name": "Elektroseti",
          "latitude": 43.598701,
          "longitude": 39.730623,
          "road_distances": {
            "Ulitsa Dokuchaeva": 3000,
            "Ulitsa Lizy Chaikinoi": 4300
          }
        },
        {
          "type": "Stop",
          "name": "Ulitsa Dokuchaeva",
          "latitude": 43.585586,
          "longitude": 39.733879,
          "road_distances": {
            "Ulitsa Lizy Chaikinoi": 2000,
            "Elektroseti": 3000
          }
        },
        {
          "type": "Stop",
          "name": "Ulitsa Lizy Chaikinoi",
          "latitude": 43.590317,
          "longitude": 39.746833,
          "road_distances": {
            "Elektroseti": 4300,
            "Ulitsa Dokuchaeva": 2000
          }
        }
      ],
      "render_settings": {
        "width": 600,
        "height": 400,
        "padding": 50,
        "stop_radius": 5,
        "line_width": 14,
        "bus_label_font_size": 20,
        "bus_label_offset": [
          7,
          15
        ],
        "stop_label_font_size": 20,
        "stop_label_offset": [
          7,
          -3
        ],
        "underlayer_color": [
          255,
          255,
          255,
          0.85
        ],
        "underlayer_width": 3,
        "color_palette": [
          "green",
          [255, 160, 0],
          "red"
        ]
      },
      "stat_requests": [
      ]
    })"
    };

    jreader.LoadJSON(input1);

    renderer::RenderSettings rs = jreader.GetRenderSettings();

    bool test_width = rs.width == 600;
    assert(test_width);

    bool test_height = rs.height == 400;
    assert(test_height);

    bool test_padding = rs.padding == 50;
    assert(test_padding);

    bool test_stop_radius = rs.stop_radius == 5;
    assert(test_stop_radius);

    bool test_line_width = rs.line_width == 14;
    assert(test_line_width);

    bool test_bus_label_font_size = rs.bus_label_font_size == 20;
    assert(test_bus_label_font_size);

    bool test_bus_label_offset = rs.bus_label_offset == svg::Point{ 7, 15 };
    assert(test_bus_label_offset);

    bool test_stop_label_font_size = rs.stop_label_font_size == 20;
    assert(test_stop_label_font_size);

    bool test_stop_label_offset = rs.stop_label_offset == svg::Point{ 7, -3 };
    assert(test_stop_label_offset);

    bool test_underlayer_color = rs.underlayer_color == svg::Rgba{ 255, 255, 255, 0.85 };
    assert(test_underlayer_color);

    bool test_underlayer_width = rs.underlayer_width == 3;
    assert(test_underlayer_width);

    bool test_color_palette = rs.color_palette == std::vector<svg::Color>{ "green", svg::Rgb{ 255, 160, 0 }, "red" };
    assert(test_color_palette);

    std::ostringstream out;

    jreader.PrintTo(out);
}

} // namespace json_reader::tests

} // namespace json_reader