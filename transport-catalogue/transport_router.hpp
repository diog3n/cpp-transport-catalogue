#include "router.hpp"
#include "graph.hpp"
#include "transport_catalogue.hpp"
#include <unordered_map>

namespace transport_router {

using Weight = double;

struct RoutingSettings {
    double wait_time_minutes;
    double bus_velocity;
};

struct RouteItem {
    std::string type;
    double time;
};

struct RouteItemStop: public RouteItem {
    std::string stop_name;
};

struct RouteItemBus: public RouteItem {
    std::string bus;
    int span_count;
};

struct RouteInfo {
    Weight total_time;
    std::vector<RouteItem> items;
};

class TransportRouter {
public:
    using Router             = graph::Router<Weight>;
    using Graph              = graph::DirectedWeightedGraph<Weight>;
    using VertexId           = graph::VertexId;
    using TransportCatalogue = transport_catalogue::TransportCatalogue;


    explicit TransportRouter(const TransportCatalogue& catalogue, 
                             RoutingSettings settings)
        : catalogue_(&catalogue)
        , road_graph_(BuildGraph())
        , router_(road_graph_)
        , settings_(std::move(settings)) {}

    // Builds a route for two stop_names
    std::optional<std::vector<RouteItem>> BuildRoute(std::string_view from, 
                                                     std::string_view to) const;

    /* Returns a stop id, mapped to it. */
    std::optional<VertexId> GetStopId(std::string_view stop_name) const;

    /* Returns a stop name, mapped to it */
    std::optional<std::string_view> GetStopNameById(VertexId vertex_id) const;

private:

    bool PartOfSameRoute(std::string_view from, std::string_view to) const;

    /* Builds a graph based on info from transport catalogue
     * and fills stop_name_to_vertex_id_ map (hence not being const) */
    Graph BuildGraph();

    /* Compute weight of the edge between two vertecies defined by the stop names. 
     * Weight is a sum of time it takes to get to the destination, in minutes, 
     * plus time it takes to wait for a bus before embarking, in minutes. */
    Weight ComputeWeight(std::string_view from, std::string_view to) const;

    /* Maps a given stop name to a VertexId (enumearates it, hence the name) 
     * if such name is not already mapped. If it is, nothing happens */
    void EnumerateIfNot(std::string_view stop_name);

    // Transport catalogue 
    const TransportCatalogue* catalogue_;

    // Graph with stops as vertecies 
    Graph road_graph_;

    // A router used to build routes
    Router router_;

    // Routing settings necessary to compute weights
    RoutingSettings settings_;

    // Maps stop name to vertex_id
    std::unordered_map<std::string_view, VertexId> stop_name_to_vertex_id_;

    // Maps vertex_id to stop name
    std::unordered_map<VertexId, std::string_view> vertex_id_to_stop_name_;

    VertexId current_vertex_id = 0;
};

namespace tests {

void TestBasicRouting();

} // namespace transport_router::tests

} // namespace transport_router