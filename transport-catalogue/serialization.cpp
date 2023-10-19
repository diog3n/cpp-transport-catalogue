#include "transport_catalogue.pb.h"
#include "transport_catalogue.h"
#include "transport_router.h"
#include "map_renderer.pb.h"
#include "serialization.h"
#include "map_renderer.h"
#include "graph.pb.h"
#include "domain.h"
#include "svg.pb.h"
#include "geo.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <set>
#include <sstream>
#include <variant>

namespace serialization {

namespace database {

void DatabaseSerializer::Serialize(const TransportCatalogue& catalogue,
                                   const RenderSettings& render_settings,
                                   const TransportRouter& router,
                                   std::ostream& out) {
    BuildSerialized(catalogue, 
                    render_settings,
                    router
    ).SerializeToOstream(&out);
}

DatabaseSerializer::SerializedDatabase
DatabaseSerializer::BuildSerialized(const TransportCatalogue& catalogue, 
                                    const RenderSettings& render_settings,
                                    const TransportRouter& router) {
    using TCSerializer  = transport_catalogue::TransportCatalogueSerializer;
    using SVGSerializer = svg::SVGSerializer;
    using RouterSerializer = router::RouterSerializer;

    serialize::Database serialized_db;

    *serialized_db.mutable_catalogue() = TCSerializer
                                       ::BuildSerialized(catalogue);
    *serialized_db.mutable_render_settings() = SVGSerializer
                                            ::BuildSerialized(render_settings); 
    *serialized_db.mutable_graph_info() = RouterSerializer
                                        ::BuildSerialized(router);
    return serialized_db;
}

Database DatabaseSerializer::Deserialize(std::istream &in) {
    using TCSerializer  = transport_catalogue::TransportCatalogueSerializer;
    using SVGSerializer = svg::SVGSerializer;
    using RouterSerializer = router::RouterSerializer;

    serialize::Database serialized_db;

    serialized_db.ParseFromIstream(&in);

    TransportCatalogue catalogue = TCSerializer
                                 ::BuildDeserialized(serialized_db.catalogue());

    RenderSettings render_settings = 
              SVGSerializer::BuildDeserialized(serialized_db.render_settings());

    transport_router::TransportRouterInfo router_info =
                RouterSerializer::BuildDeserialized(serialized_db.graph_info());

    return {
        std::move(catalogue),
        std::move(render_settings),
        std::move(router_info)
    };
}

namespace tests {

void TestDatabaseSerialization() {
    renderer::RenderSettings rs;

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
        {
            { "Biryusinka Miryusinka"sv, 500 } 
        }
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

    rs.width = 600;
    rs.height = 400;
    rs.padding = 50;
    rs.stop_radius = 5;
    rs.line_width = 14;
    rs.bus_label_font_size = 20;
    rs.bus_label_offset = ::svg::Point{ 7, 15 };
    rs.stop_label_font_size = 20;
    rs.stop_label_offset = ::svg::Point{ 7, -3 };
    rs.underlayer_color = ::svg::Rgba{ 255, 255, 255, 0.85 };
    rs.underlayer_width = 3;
    rs.color_palette = std::vector<::svg::Color>{ "green", ::svg::Rgb{ 255, 160, 0 }, "red" };

    transport_router::RoutingSettings routing_settings {
        /* bus_wait_time: */  6,
        /* bus_velocity:  */  40
    };

    transport_router::TransportRouter router(tc, routing_settings);
    std::ostringstream output(std::ios::binary);
 
    DatabaseSerializer::Serialize(tc, rs, router, output);
    std::istringstream input(output.str(), std::ios::binary);

    Database db = DatabaseSerializer::Deserialize(input);
    transport_router::TransportRouterInfo& router_info = db.router_info;
    transport_router::TransportRouter deserialized_router(tc, router_info);

    std::optional<transport_router::RoutingResult> route = 
                            router.BuildRoute("Biryusinka Miryusinka"sv, 
                                              "Marushkino"sv);
    

    std::optional<transport_router::RoutingResult> deserialized_route = 
                deserialized_router.BuildRoute("Biryusinka Miryusinka"sv, 
                                               "Marushkino"sv);
    assert(route.has_value());
    assert(deserialized_route.has_value());

    assert(route.value().total_time == deserialized_route.value().total_time);

    TransportCatalogue& deserialized_tc = db.catalogue;
    renderer::RenderSettings& deserialized_rs = db.render_settings;

    assert(deserialized_tc.GetStopNames() == tc.GetStopNames());

    assert(deserialized_tc.GetBusNames() == tc.GetBusNames());

    assert(deserialized_tc.GetDistance("Biryusinka Miryusinka"sv, 
                                       "Marushkino"sv) == 1500);
    assert(deserialized_tc.GetDistance("Marushkino"sv,
                                       "Tolstopaltsevo"sv) == 200);
    
    bool test_width = deserialized_rs.width == 600;
    assert(test_width);

    bool test_height = deserialized_rs.height == 400;
    assert(test_height);

    bool test_padding = deserialized_rs.padding == 50;
    assert(test_padding);

    bool test_stop_radius = deserialized_rs.stop_radius == 5;
    assert(test_stop_radius);

    bool test_line_width = deserialized_rs.line_width == 14;
    assert(test_line_width);

    bool test_bus_label_font_size = deserialized_rs.bus_label_font_size == 20;
    assert(test_bus_label_font_size);

    bool test_bus_label_offset = deserialized_rs.bus_label_offset == ::svg::Point{ 7, 15 };
    assert(test_bus_label_offset);

    bool test_stop_label_font_size = deserialized_rs.stop_label_font_size == 20;
    assert(test_stop_label_font_size);

    bool test_stop_label_offset = deserialized_rs.stop_label_offset == ::svg::Point{ 7, -3 };
    assert(test_stop_label_offset);

    bool test_underlayer_color = deserialized_rs.underlayer_color == ::svg::Rgba{ 255, 255, 255, 0.85 };
    assert(test_underlayer_color);

    bool test_underlayer_width = deserialized_rs.underlayer_width == 3;
    assert(test_underlayer_width);

    bool test_color_palette = deserialized_rs.color_palette == std::vector<::svg::Color>{ "green", ::svg::Rgb{ 255, 160, 0 }, "red" };
    assert(test_color_palette);

}

}

}

namespace router {

void RouterSerializer::Serialize(const TransportRouter& router, 
                                 std::ostream& out) {
    BuildSerialized(router).SerializeToOstream(&out);
}

RouterSerializer::TransportRouterInfo 
RouterSerializer::Deserialize(std::istream& in) {
    serialize::GraphInfo deserialized_router_info;

    deserialized_router_info.ParseFromIstream(&in);

    return BuildDeserialized(deserialized_router_info);
}

serialize::GraphInfo RouterSerializer::BuildSerialized(
                                         const TransportRouter& router) {
    serialize::GraphInfo graph_info;
    TransportRouterInfo router_info = router.ExportRouterInfo();

    for (const auto& vertex_info : router_info.GetVertexesInfo()) {
        *graph_info.add_vertexes() = BuildSerializedVertexInfo(vertex_info);
    }

    for (const auto& edge_info : router_info.GetEdgesInfo()) {
        *graph_info.add_edges() = BuildSerializedEdgeInfo(edge_info);
    }

    *graph_info.mutable_routing_settings() = 
               BuildSerializedRoutingSettings(router_info.GetRoutingSettings());

    return graph_info;
}

RouterSerializer::TransportRouterInfo RouterSerializer::BuildDeserialized(
                            const serialize::GraphInfo& serialized_graph) {
    TransportRouterInfo router_info;

    for (const auto& serialized_edge : serialized_graph.edges()) {
        router_info.AddEdgeInfo(BuildDeserializedEdgeInfo(serialized_edge));
    } 

    for (const auto& serialized_vtx : serialized_graph.vertexes()) {
        router_info.AddVertexInfo(BuildDeserializedVertexInfo(serialized_vtx));
    }

    router_info.SetRoutingSettings(
        BuildDeserializedRoutingSettings(serialized_graph.routing_settings()));

    return router_info;
}

serialize::EdgeInfo RouterSerializer::BuildSerializedEdgeInfo(
                            const TransportRouterInfo::EdgeInfo& edge_info) {
    serialize::EdgeInfo serialized_edge;

    serialized_edge.set_is_bus_edge(edge_info.is_bus_edge);
    serialized_edge.set_from_id(edge_info.from);
    serialized_edge.set_to_id(edge_info.to);
    serialized_edge.set_span_count(edge_info.span_count);
    serialized_edge.set_name(edge_info.name);
    serialized_edge.set_weight(edge_info.weight);

    return serialized_edge;
}

serialize::VertexInfo RouterSerializer::BuildSerializedVertexInfo(
                            const TransportRouterInfo::VertexInfo& vtx_info) {
    serialize::VertexInfo serialized_vtx;

    serialized_vtx.set_stop_name(vtx_info.stop_name);
    serialized_vtx.set_is_bus_stop(vtx_info.is_bus_vertex);
    serialized_vtx.set_id(vtx_info.id);

    return serialized_vtx;
}

RouterSerializer::TransportRouterInfo::EdgeInfo 
RouterSerializer::BuildDeserializedEdgeInfo(
                                const serialize::EdgeInfo& serialized_edge) {
    return TransportRouterInfo::EdgeInfo {
        serialized_edge.from_id(),
        serialized_edge.to_id(),
        serialized_edge.span_count(),
        serialized_edge.is_bus_edge(),
        serialized_edge.name(),
        serialized_edge.weight()
    };
}

RouterSerializer::TransportRouterInfo::VertexInfo 
RouterSerializer::BuildDeserializedVertexInfo(
                                const serialize::VertexInfo& serialized_vtx) {
    return TransportRouterInfo::VertexInfo {
        serialized_vtx.stop_name(),
        serialized_vtx.id(),
        serialized_vtx.is_bus_stop()
    };
}

serialize::RoutingSettings RouterSerializer::BuildSerializedRoutingSettings(
                        const RoutingSettings& settings) {
    serialize::RoutingSettings serialized_settings;

    serialized_settings.set_bus_velocity(settings.bus_velocity);
    serialized_settings.set_bus_wait_time(settings.bus_wait_time);

    return serialized_settings;
}

RouterSerializer::RoutingSettings 
RouterSerializer::BuildDeserializedRoutingSettings(
                        const serialize::RoutingSettings& serialized_settings) {
    return RoutingSettings {
        serialized_settings.bus_wait_time(),
        serialized_settings.bus_velocity()
    };
}

}

namespace svg {

serialize::RenderSettings SVGSerializer::BuildSerialized(
                                         const RenderSettings render_settings) {
    serialize::RenderSettings serialized_settings;

    serialized_settings.set_width(render_settings.width);
    serialized_settings.set_height(render_settings.height);
    serialized_settings.set_padding(render_settings.padding);
    serialized_settings.set_stop_radius(render_settings.stop_radius);
    serialized_settings.set_line_width(render_settings.line_width);
    serialized_settings.set_bus_label_font_size(
                        render_settings.bus_label_font_size);
    serialized_settings.set_stop_label_font_size(
                        render_settings.stop_label_font_size);
    serialized_settings.set_underlayer_width(
                        render_settings.underlayer_width);
    *serialized_settings.mutable_bus_label_offset() = 
                         BuildSerializedPoint(render_settings.bus_label_offset);
    *serialized_settings.mutable_stop_label_offset() =
                         BuildSerializedPoint(render_settings.stop_label_offset);
    *serialized_settings.mutable_underlayer_color() = 
                         BuildSerializedColor(render_settings.underlayer_color);

    for (const auto& color : render_settings.color_palette) {
        *serialized_settings.add_color_palette() = 
                             BuildSerializedColor(color);
    }

    return serialized_settings;
}

serialize::Point SVGSerializer::BuildSerializedPoint(const Point& point) {
    serialize::Point serialized_point;

    serialized_point.set_x(point.x);
    serialized_point.set_y(point.y);

    return serialized_point;
}

serialize::Color SVGSerializer::BuildSerializedColor(const Color& color) {
    using namespace ::svg;
    serialize::Color serialized_color;

    if (std::holds_alternative<Rgb>(color)) {
        serialize::Rgb serialized_rgb;

        Rgb rgb = std::get<Rgb>(color);
        serialized_rgb.set_r(rgb.red);
        serialized_rgb.set_g(rgb.green);
        serialized_rgb.set_b(rgb.blue);

        *serialized_color.mutable_rgb() = serialized_rgb;

        return serialized_color;
    } else if (std::holds_alternative<Rgba>(color)) {
        serialize::Rgba serialized_rgba;

        Rgba rgba = std::get<Rgba>(color);
        serialized_rgba.set_r(rgba.red);
        serialized_rgba.set_g(rgba.green);
        serialized_rgba.set_b(rgba.blue);
        serialized_rgba.set_a(rgba.opacity);

        *serialized_color.mutable_rgba() = serialized_rgba;

        return serialized_color;
    } else if (std::holds_alternative<std::string>(color)) {
        std::string color_string = std::get<std::string>(color);

        serialized_color.set_color_str(color_string);

        return serialized_color; 
    } 

    serialized_color.set_color_str(std::get<std::string>(NoneColor));
    return serialized_color;
}

void SVGSerializer::Serialize(const RenderSettings& settings, 
                                 std::ostream& out) { 
    BuildSerialized(settings).SerializeToOstream(&out);
}

SVGSerializer::RenderSettings SVGSerializer::BuildDeserialized(
                        const serialize::RenderSettings& serialized_settings) {
    RenderSettings settings;

    settings.width = serialized_settings.width();
    settings.height = serialized_settings.height();
    settings.padding = serialized_settings.padding();
    settings.stop_radius = serialized_settings.stop_radius();
    settings.line_width = serialized_settings.line_width();
    settings.bus_label_font_size = serialized_settings.bus_label_font_size();
    settings.stop_label_font_size = serialized_settings.stop_label_font_size();
    settings.underlayer_width = serialized_settings.underlayer_width();

    settings.bus_label_offset = 
                       DeserializePoint(serialized_settings.bus_label_offset());
    settings.stop_label_offset =
                       DeserializePoint(serialized_settings.stop_label_offset());

    settings.underlayer_color =
                       DeserializeColor(serialized_settings.underlayer_color());
    
    for (const auto& serialized_color : serialized_settings.color_palette()) {
        settings.color_palette.emplace_back(
                                std::move(DeserializeColor(serialized_color)));
    }   

    return settings;
}

SVGSerializer::Point SVGSerializer::DeserializePoint(
                                     const serialize::Point& serialized_point) {
    return Point {
        serialized_point.x(),
        serialized_point.y()
    };
}

SVGSerializer::Color SVGSerializer::DeserializeColor(
                                     const serialize::Color& serialized_color) {
    using namespace ::svg;

    if (serialized_color.has_rgb()) {
        return Rgb {
            static_cast<uint8_t>(serialized_color.rgb().r()),
            static_cast<uint8_t>(serialized_color.rgb().g()),
            static_cast<uint8_t>(serialized_color.rgb().b())
        };
    } else if (serialized_color.has_rgba()) {
        return Rgba {
            static_cast<uint8_t>(serialized_color.rgba().r()),
            static_cast<uint8_t>(serialized_color.rgba().g()),
            static_cast<uint8_t>(serialized_color.rgba().b()),
            static_cast<double>(serialized_color.rgba().a())
        };
    }
    return serialized_color.color_str();
}

SVGSerializer::RenderSettings SVGSerializer::Deserialize(std::istream &in) {
    serialize::RenderSettings serialized_settings;

    serialized_settings.ParseFromIstream(&in);

    return BuildDeserialized(serialized_settings);
}

}

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

void TransportCatalogueSerializer::Serialize(
                                const TransportCatalogue& catalogue, 
                                std::ostream &out) {
    BuildSerialized(catalogue).SerializeToOstream(&out);
}

serialize::TransportCatalogue 
TransportCatalogueSerializer::BuildSerialized(
                                          const TransportCatalogue& catalogue) {
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
    return tc;
}

TransportCatalogueSerializer::TransportCatalogue 
TransportCatalogueSerializer::BuildDeserialized(
                     const serialize::TransportCatalogue& serialized_catalogue) {
    TransportCatalogue catalogue;

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
TransportCatalogueSerializer::Deserialize(std::istream &in) {
    TransportCatalogue catalogue;

    serialize::TransportCatalogue serialized_catalogue;

    serialized_catalogue.ParseFromIstream(&in);

    return BuildDeserialized(serialized_catalogue);
}

} // namespace serialization::transport_catalogue

} // namespace serialization