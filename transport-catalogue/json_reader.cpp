#include <algorithm>
#include <memory>
#include <stdexcept>
#include <exception>
#include <cassert>
#include <ostream>
#include <sstream>
#include <variant>

#include "json_builder.hpp"
#include "map_renderer.hpp"
#include "json_reader.hpp"
#include "domain.hpp"
#include "json.hpp"
#include "router.hpp"
#include "transport_catalogue.hpp"
#include "transport_router.hpp"

namespace json_reader {

using namespace std::literals;

namespace util {

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

JSONReader::JSONReader(transport_catalogue::TransportCatalogue& tc)
    : catalogue_(tc)
    , json_(json::Document{nullptr}) {}

void JSONReader::ParseDocument() {
    std::cerr << "(DEBUG INFO) Parsing a document" << std::endl;

    const json::Dict& root_map = json_.GetRoot().AsMap();

    const json::Array& base_requests   = root_map.at("base_requests"s).AsArray();
    const json::Array& stat_requests   = root_map.at("stat_requests"s).AsArray();
    const json::Node& render_settings  = root_map.at("render_settings"s);
    const json::Node& routing_settings = root_map.at("routing_settings"s);

    render_settings_  = AssembleRenderSettings(render_settings);
    routing_settings_ = AssembleRoutingSettings(routing_settings);

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
        } else if (type == "Map"sv) {
            map_output_queries_.push_back(AssembleMapOutputQuery(node));
            query_ptrs_.push_back(&map_output_queries_.back());
        } else if (type == "Route"sv) {
            route_output_queries_.push_back(AssembleRouteOutputQuery(node));
            query_ptrs_.push_back(&route_output_queries_.back());
        } else {
            throw std::invalid_argument("Unknown query type: "s + std::string(type));
        }
    });
}

void JSONReader::InitializeRouter() {
    using namespace transport_router;

    router_ = std::make_unique<TransportRouter>(catalogue_, routing_settings_);
}

void JSONReader::LoadJSON(const std::string& document) {    
    std::istringstream in(document);
    json_ = json::Load(in);
    
    ParseDocument();
    ExecuteInputQueries();
    InitializeRouter();
}

void JSONReader::LoadJSON(std::istream& in) {
    json_ = json::Load(in);

    ParseDocument();
    ExecuteInputQueries();
    InitializeRouter();
}

json::Node JSONReader::AssembleErrorNode(const int id) const {
    json::Node result = json::Builder{}.StartDict()
                                .Key("request_id"s).Value(id)
                                .Key("error_message"s).Value("not found"s)
                             .EndDict().Build();
    return result;
}

json::Node JSONReader::AssembleBusNode(domain::BusInfoOpt& bus_info_opt, int id) const {
    if (bus_info_opt) {
        return json::Builder{}.StartDict()
                        .Key("request_id"s).Value(id)
                        .Key("curvature"s).Value(bus_info_opt->curvature)
                        .Key("route_length"s).Value(bus_info_opt->route_length)
                        .Key("stop_count"s).Value(static_cast<int>(bus_info_opt->stops_on_route))
                        .Key("unique_stop_count"s).Value(static_cast<int>(bus_info_opt->unique_stops))
                    .EndDict().Build();
    }
    
    return AssembleErrorNode(id);
}

json::Node JSONReader::AssembleStopNode(domain::StopInfoOpt& stop_info_opt, int id) const {
    if (stop_info_opt) {
        json::Array bus_array; 
        bus_array.reserve(stop_info_opt->bus_names.size());

        for (const std::string_view view : stop_info_opt->bus_names) {
            bus_array.push_back(std::string(view));
        }

        return json::Builder{}
                    .StartDict()
                        .Key("buses"s).Value(bus_array)
                        .Key("request_id"s).Value(id)
                    .EndDict().Build();
    }
    
    return AssembleErrorNode(id);
}

json::Node JSONReader::AssembleMapNode(int id) const {
    renderer::MapRenderer renderer(render_settings_);

    request_handler::RequestHandler rh(catalogue_, renderer);

    svg::Document document = rh.RenderMap();

    std::ostringstream out;

    document.Render(out);

    return json::Builder{}
                .StartDict()
                    .Key("map"s).Value(out.str())
                    .Key("request_id"s).Value(id)
                .EndDict().Build();
}

json::Node JSONReader::AssembleRouteNode(
                    std::optional<transport_router::RoutingResult> routing_result,
                                                               int id) const {
    using namespace transport_router;

    if (!routing_result.has_value()) {
        return AssembleErrorNode(id);
    }
    
    json::Array items_array;

    for (const RouteItem& item : routing_result->items) {
        if (std::holds_alternative<RouteItemBus>(item)) {
            RouteItemBus bus_item = std::get<RouteItemBus>(item);

            json::Node bus_item_node = 
                            json::Builder{}.StartDict()
                                .Key("bus").Value(bus_item.bus_name)
                                .Key("span_count").Value(bus_item.span_count)
                                .Key("time").Value(bus_item.time)
                                .Key("type").Value(bus_item.type)
                            .EndDict().Build();

            items_array.push_back(bus_item_node);
        } else if (std::holds_alternative<RouteItemWait>(item)) {
            RouteItemWait wait_item = std::get<RouteItemWait>(item); 
            
            json::Node wait_item_node = 
                            json::Builder{}.StartDict()
                                .Key("stop_name").Value(wait_item.stop_name)
                                .Key("time").Value(wait_item.time)
                                .Key("type").Value(wait_item.type)
                            .EndDict().Build();

            items_array.push_back(wait_item_node);
        }
    }

    return json::Builder{}.StartDict()
                .Key("request_id").Value(id)
                .Key("total_time").Value(routing_result->total_time)
                .Key("items").Value(items_array)
            .EndDict().Build();
}

domain::MapOutputQuery JSONReader::AssembleMapOutputQuery(const json::Node& query_node) const {
    const json::Dict& request_map = query_node.AsMap();
    const int id = request_map.at("id").AsInt();

    return { id };
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

    return { bus_name, std::move(stops), is_roundtrip };
}

domain::StopInputQuery JSONReader::AssembleStopInputQuery(const json::Node& query_node) const {
    const json::Dict& request_map    = query_node.AsMap();
    const std::string_view stop_name = request_map.at("name"s).AsString();
    const geo::Coordinates coordinates{ request_map.at("latitude"s).AsDouble(),
                                        request_map.at("longitude"s).AsDouble() };
    
    std::unordered_map<std::string_view, int> distances;

    for (const auto& [name, node] : request_map.at("road_distances"s).AsMap()) {
        distances[name] = node.AsInt();
    }

    return { stop_name, std::move(coordinates), std::move(distances) };
}

domain::RouteOutputQuery JSONReader::AssembleRouteOutputQuery(
                                                const json::Node& query_node) const {
    const json::Dict& request_map = query_node.AsMap();
    const std::string_view from   = request_map.at("from").AsString();
    const std::string_view to     = request_map.at("to").AsString();
    const int id = request_map.at("id").AsInt();

    return { id, from, to };
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

transport_router::RoutingSettings JSONReader::AssembleRoutingSettings(
                                        const json::Node& routing_settings) const {
    const json::Dict& settings_map = routing_settings.AsMap();

    double bus_velocity  = settings_map.at("bus_velocity").AsDouble();
    double bus_wait_time = settings_map.at("bus_wait_time").AsDouble();

    return { bus_wait_time, bus_velocity };
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

void JSONReader::ExecuteInputQueries() {
    std::cerr << "(DEBUG INFO) Executing Input Queries" << std::endl;

    std::for_each(stop_input_queries_.begin(), stop_input_queries_.end(), [this](const domain::StopInputQuery& stop_query) {
        catalogue_.AddStop(std::string(stop_query.name), stop_query.coordinates);
    });
    std::for_each(stop_input_queries_.begin(), stop_input_queries_.end(), [this](const domain::StopInputQuery& stop_query) {
        for (const auto& [dest_name, distance] : stop_query.distances) {
            catalogue_.AddDistance(stop_query.name, dest_name, distance);
        }
    });
    std::for_each(bus_input_queries_.begin(), bus_input_queries_.end(), [this](const domain::BusInputQuery& bus_query) {
        catalogue_.AddBus(std::string(bus_query.name), bus_query.stop_names, bus_query.is_roundtrip);
    });
}

void JSONReader::ExecuteOutputQueries(std::ostream& out) const {
    std::cerr << "(DEBUG INFO) Executing Output Queries" << std::endl;
    
    json::Array output_array;

    std::for_each(query_ptrs_.begin(), query_ptrs_.end(), 
    [this, &output_array](const domain::OutputQuery* query_ptr) {
        if (query_ptr->type == domain::QueryType::STOP) {
            
            const domain::StopOutputQuery* stop_query_ptr = 
                              static_cast<const domain::StopOutputQuery*>(query_ptr);
            
            domain::StopInfoOpt stop_info_opt = 
                                   catalogue_.GetStopInfo(stop_query_ptr->stop_name);
            
            output_array.push_back(AssembleStopNode(stop_info_opt, query_ptr->id));
        
        } else if (query_ptr->type == domain::QueryType::BUS) {
        
            const domain::BusOutputQuery* bus_query_ptr = 
                               static_cast<const domain::BusOutputQuery*>(query_ptr);            
            domain::BusInfoOpt bus_info_opt = 
                                      catalogue_.GetBusInfo(bus_query_ptr->bus_name);
            
            output_array.push_back(AssembleBusNode(bus_info_opt, query_ptr->id));
        
        } else if (query_ptr->type == domain::QueryType::MAP) {
        
            const domain::MapOutputQuery* map_query_ptr = 
                               static_cast<const domain::MapOutputQuery*>(query_ptr);
        
            output_array.push_back(AssembleMapNode(map_query_ptr->id));

        } else if (query_ptr->type == domain::QueryType::ROUTE) {
            
            const domain::RouteOutputQuery* route_query_ptr = 
                             static_cast<const domain::RouteOutputQuery*>(query_ptr);
            
            std::optional<transport_router::RoutingResult> routing_result = 
                    router_->BuildRoute(route_query_ptr->from, route_query_ptr->to);
            
            output_array.push_back(AssembleRouteNode(routing_result, 
                                                     route_query_ptr->id));

        }
    });

    json::Document output_doc{output_array};
    json::Print(output_doc, out);
}

std::string JSONReader::ReadJSON(std::istream& in) {
    std::vector<std::string> lines;
    std::string line;

    std::string result;

    char brace;
    char rbrace;
    int brace_count = 0;

    while (getline(in, line)) lines.push_back(line);

    std::istringstream first_line_input{lines.front()}; 

    char first_valid_char;

    first_line_input >> first_valid_char;

    if (first_valid_char == '{') brace = '{', rbrace = '}';
    else if (first_valid_char == '[') brace = '[', rbrace = ']';
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

    JSONReader jreader(tc);

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
      "routing_settings": {
          "bus_velocity": 30,
          "bus_wait_time": 2
      },
      "stat_requests": [
         {"id": 1, "type": "Stop", "name": "Ulitsa Lizy Chaikinoi"}
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

    bool test_bus_count = tc.GetBusNames().size() == 2;
    assert(test_bus_count);

    bool test_stop_count = tc.GetStopNames().size() == 5;
    assert(test_stop_count);
}

void TestAssembleQuery() {
    transport_catalogue::TransportCatalogue tc;

    JSONReader jreader(tc);

    std::istringstream input1 {
        R"({
        "base_requests": [
            {
                "type": "Bus",
                "name": "114",
                "stops": ["Морской вокзал", "Ривьерский мост"],
                "is_roundtrip": false
            },
            {
                "type": "Stop",
                "name": "Ривьерский мост",
                "latitude": 43.587795,
                "longitude": 39.716901,
                "road_distances": {"Морской вокзал": 850}
            },
            {
                "type": "Stop",
                "name": "Морской вокзал",
                "latitude": 43.581969,
                "longitude": 39.719848,
                "road_distances": {"Ривьерский мост": 850, "Наличная улица": 1000}
            },
            {
                "type": "Stop",
                "name": "Наличная улица",
                "latitude": 41.581969,
                "longitude": 33.719548,
                "road_distances": {"Ривьерский мост": 1850}
            }
            {
                "type": "Bus",
                "name": "41",
                "stops": ["Морской вокзал", "Наличная улица", "Морской вокзал"],
                "is_roundtrip": true
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
        "routing_settings": {
          "bus_velocity": 30,
          "bus_wait_time": 2
        },
        "stat_requests": [
            { "id": 1, "type": "Stop", "name": "Ривьерский мост" },
            { "id": 2, "type": "Bus", "name": "114" },
            { "id": 3, "type": "Bus", "name": "45" },
            { "id": 4, "type": "Stop", "name": "У-м Гаванский" },
            { "id": 5, "type": "Stop", "name": "Наличная улица" }
            { "id": 6, "type": "Bus", "name": "41" }    
        ]
        } )"
    };

    jreader.LoadJSON(input1);

    std::ostringstream out;

    jreader.ExecuteOutputQueries(out);

    std::istringstream input2(out.str());

    std::istringstream input3("[ { \"buses\": [ \"114\" ], \"request_id\": 1 }, { \"curvature\": 1.23199, \"request_id\": 2, \"route_length\": 1700, \"stop_count\": 3, \"unique_stop_count\": 2 }, { \"error_message\": \"not found\", \"request_id\": 3 }, { \"error_message\": \"not found\", \"request_id\": 4 }, { \"buses\": [ \"41\" ], \"request_id\": 5 }, { \"curvature\": 0.00185499, \"request_id\": 6, \"route_length\": 2000, \"stop_count\": 3, \"unique_stop_count\": 2 } ]");

    bool test_load = json::Load(input2) == json::Load(input3);

    assert(test_load);
}

} // namespace json_reader::tests

} // namespace json_reader