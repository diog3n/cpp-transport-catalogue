#include "transport_catalogue.hpp"
#include "router.hpp"
#include "graph.hpp"

#include <unordered_map>
#include <cstddef>
#include <unordered_set>
#include <variant>
#include <vector>

namespace transport_router {

class TransportRouter;
using Weight = double;

struct RoutingSettings {
    double wait_time_minutes;
    double bus_velocity;
};

struct BaseRouteItem {
    BaseRouteItem() = default;

    BaseRouteItem(std::string type, Weight time)
        : type(std::move(type))
        , time(std::move(time)) {}

    std::string type;
    Weight time;

    virtual void Clear() = 0; 

    virtual ~BaseRouteItem() = default;
};

struct RouteItemWait: public BaseRouteItem {
    RouteItemWait()
        : BaseRouteItem("Wait", {})
        , stop_name{} {}

    RouteItemWait(std::string stop_name, Weight time)
        : BaseRouteItem("Wait", time)
        , stop_name(std::move(stop_name)) {}

    std::string stop_name;

    virtual void Clear() override;
};

struct RouteItemBus: public BaseRouteItem {
    RouteItemBus()
        : BaseRouteItem("Bus", {})
        , bus_name{}
        , span_count{} {}

    RouteItemBus(std::string bus_name, int span_count, Weight time)
        : BaseRouteItem("Bus", time)
        , bus_name(std::move(bus_name))
        , span_count(span_count) {}

    std::string bus_name;
    int span_count;

    virtual void Clear() override;
};

using RouteItem = std::variant<RouteItemWait, 
                               RouteItemBus,
                               std::nullptr_t>;

struct RoutingResult {
    Weight total_time;
    std::optional<std::vector<RouteItem>> items;
};

class TransportGraph {
public:
    using Graph              = graph::DirectedWeightedGraph<Weight>;
    using EdgeId             = graph::EdgeId;
    using VertexId           = graph::VertexId;
    using TransportCatalogue = transport_catalogue::TransportCatalogue;

    TransportGraph(const TransportCatalogue& catalogue,
                   RoutingSettings settings )
        : catalogue_(&catalogue)

        /* CHANGE THIS */
        , route_graph_(1024/*catalogue_->GetStopCount() * catalogue_->GetBusCount()*/)
        , settings_(std::move(settings)) {
            BuildGraph();
        }

    const Graph& GetGraph() const;

    bool IsSpanEdge(EdgeId edge) const;

    bool IsWaitEdge(EdgeId edge) const;

    Weight GetEdgeWeight(EdgeId edge) const;

    std::string_view GetSpanEdgeBusName(EdgeId edge) const;

    std::string_view GetWaitEdgeStopName(EdgeId edge) const;

    std::optional<VertexId> GetStopVertexId(std::string_view stop_name) const;

    std::optional<VertexId> GetSpanVertexId(std::string_view stop_name) const;

private:
    /* Builds a graph based on info from transport catalogue
     * and fills stop_name_to_vertex_id_ map (hence not being const) */
    void BuildGraph();

    /* Maps a given stop name to a StopVertex (enumerates it, hence the name) 
     * and vice versa. 
     * stop vertex as a start and bus vertex as an end. */
    void EnumerateVertecies(std::string_view stop_name);

    /* Compute weight of the edge between two vertecies defined by the stop names. 
     * Weight is a sum of time it takes to get to the destination, in minutes, 
     * plus time it takes to wait for a bus before embarking, in minutes. */
    Weight ComputeTravelTime(std::string_view from, std::string_view to) const;

    const TransportCatalogue* catalogue_;

    struct StopVertex {
        VertexId wait_vertex;
        VertexId span_vertex;

        bool operator==(const StopVertex& other) const {
            return wait_vertex == other.wait_vertex
                && span_vertex == other.span_vertex;
        }
    };

    struct StopVertexHasher {
        size_t operator()(const StopVertex& vertex) const {
            std::hash<VertexId> vertex_hasher;

            return vertex_hasher(vertex.wait_vertex) 
                   + 37 * vertex_hasher(vertex.span_vertex);
        }
    };

    Graph route_graph_;

    // Routing settings necessary to compute weights
    RoutingSettings settings_;

    // Set of "wait" item edges 
    std::set<EdgeId> wait_edges;
    
    // Set of "bus" item edges
    std::set<EdgeId> span_edges;

    // Maps stop name to vertex_id
    std::unordered_map<std::string_view, 
                       VertexId> stop_name_to_vertex_id_;

    std::unordered_map<VertexId, 
                       std::string_view> vertex_id_to_stop_name_; 

    std::unordered_map<std::string_view,
                       VertexId> stop_name_to_span_vertex_id_;

    std::unordered_map<VertexId,
                       std::string_view> span_vertex_id_to_stop_name_;

    // Maps span edge ids to the bus name
    std::unordered_map<EdgeId, 
                       std::string_view> span_edge_id_to_bus_name_;

    std::unordered_map<EdgeId,
                       std::string_view> wait_edge_id_to_stop_name_;

    VertexId current_vertex_id = 0;

};

class TransportRouter {
public:
    using Router             = graph::Router<Weight>;
    using EdgeId             = graph::EdgeId;
    using VertexId           = graph::VertexId;
    using TransportCatalogue = transport_catalogue::TransportCatalogue;

    // Builds a route for two stop_names
    std::optional<RoutingResult> BuildRoute(std::string_view from, 
                                            std::string_view to) const;
    

    explicit TransportRouter(const TransportCatalogue& catalogue, 
                             RoutingSettings settings)
        : catalogue_  (&catalogue)
        , transport_graph_(catalogue, std::move(settings))
        , router_(transport_graph_.GetGraph()) {}

private:
    
    // Transport catalogue 
    const TransportCatalogue* catalogue_;

    // Graph with stops as vertecies 
    TransportGraph transport_graph_;

    // A router used to build routes
    Router router_;
    
};

namespace tests {

void TestBasicRouting();

} // namespace transport_router::tests

} // namespace transport_router