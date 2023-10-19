#pragma once
#include "transport_catalogue.pb.h"
#include "transport_catalogue.h"
#include "transport_router.h"
#include "map_renderer.pb.h"
#include "map_renderer.h"
#include "graph.pb.h"

#include <filesystem>
#include <istream>
#include <vector>

namespace serialization {

struct SerializationSettings {
    std::filesystem::path filename;
};

namespace database {
namespace serialize = serialize_transport_catalogue;

struct Database {
    transport_catalogue::TransportCatalogue catalogue;
    renderer::RenderSettings render_settings;
    transport_router::TransportRouterInfo router_info;
};

class DatabaseSerializer {
public:
    using RenderSettings     = renderer::RenderSettings;
    using TransportCatalogue = transport_catalogue::TransportCatalogue;
    using SerializedDatabase = serialize::Database;
    using TransportRouter    = transport_router::TransportRouter;
    
    static void Serialize(const TransportCatalogue& catalogue,
                          const RenderSettings& render_settings,
                          const TransportRouter& router,
                          std::ostream& out);

    static Database Deserialize(std::istream& in);

    static SerializedDatabase BuildSerialized(
                          const TransportCatalogue& catalogue,
                          const RenderSettings& render_settings,
                          const TransportRouter& router);
};

namespace tests {

void TestDatabaseSerialization();

}

}

namespace router {
namespace serialize = serialize_transport_catalogue;

class RouterSerializer {
public:
    using Graph = transport_router::TransportRouter::Graph;
    using EdgeId = transport_router::TransportRouter::EdgeId;
    using TransportRouter = transport_router::TransportRouter;
    using RoutingSettings = transport_router::RoutingSettings;
    using TransportRouterInfo = transport_router::TransportRouterInfo;

    static void Serialize(const TransportRouter& router, std::ostream& out);

    static TransportRouterInfo Deserialize(std::istream& in);

    static serialize::GraphInfo BuildSerialized(const TransportRouter& router);

    static TransportRouterInfo BuildDeserialized(const serialize::GraphInfo& serialized_graph);
private:
    static serialize::EdgeInfo BuildSerializedEdgeInfo(
                                const TransportRouterInfo::EdgeInfo& edge_info);
    static serialize::VertexInfo BuildSerializedVertexInfo(
                                const TransportRouterInfo::VertexInfo& vtx_info);
    static TransportRouterInfo::EdgeInfo BuildDeserializedEdgeInfo(
                                    const serialize::EdgeInfo& serialized_edge);
    static TransportRouterInfo::VertexInfo BuildDeserializedVertexInfo(
                                    const serialize::VertexInfo& serialized_vtx);
    static serialize::RoutingSettings BuildSerializedRoutingSettings(
                          const RoutingSettings& settings);
    static RoutingSettings BuildDeserializedRoutingSettings(
                          const serialize::RoutingSettings& serialized_settings);

};


}

namespace svg {
namespace serialize = serialize_transport_catalogue;

class SVGSerializer {
public:
    using Point          = ::svg::Point;
    using Color          = ::svg::Color;
    using RenderSettings = renderer::RenderSettings;

    static void Serialize(const RenderSettings& settings, std::ostream& out);
    
    static RenderSettings Deserialize(std::istream& in);

    static serialize::RenderSettings BuildSerialized(
                                          const RenderSettings render_settings);
    static RenderSettings BuildDeserialized(
                            const serialize::RenderSettings& serialized_settings);

private:
    static serialize::Point BuildSerializedPoint(const Point& point);

    static serialize::Color BuildSerializedColor(const Color& color);

    static Point DeserializePoint(const serialize::Point& serialized_point);

    static Color DeserializeColor(const serialize::Color& serialized_color);

};

}

namespace transport_catalogue {
namespace serialize = serialize_transport_catalogue;

class TransportCatalogueSerializer {
public:    
    using TransportCatalogue = ::transport_catalogue::TransportCatalogue;

    static void Serialize(const TransportCatalogue& catalogue, 
                                            std::ostream& out);

    static TransportCatalogue Deserialize(std::istream& in);

    static serialize::TransportCatalogue BuildSerialized(
                                            const TransportCatalogue& catalogue);
    static TransportCatalogue BuildDeserialized(
                     const serialize::TransportCatalogue& serialized_catalogue);
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

} // namespace serialization::transport_catalogue

} // namespace serialize