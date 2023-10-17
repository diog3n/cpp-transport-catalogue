#pragma once
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <cstddef>
#include <variant>
#include <vector>

#include "domain.h"
#include "transport_catalogue.h"
#include "router.h"
#include "graph.h"

namespace transport_router {

class TransportRouter;
using Weight = double;

struct RoutingSettings {
    Weight bus_wait_time; // in minutes
    double bus_velocity;
};

struct BaseRouteItem {
    BaseRouteItem() = default;

    BaseRouteItem(std::string type, Weight time)
        : type(std::move(type))
        , time(std::move(time)) {}

    std::string type;
    Weight time;
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
};

using RouteItem = std::variant<RouteItemWait, 
                               RouteItemBus,
                               std::nullptr_t>;

struct RoutingResult {
    Weight total_time;
    std::vector<RouteItem> items;
};

/* Вы правы, использование двух классов не было необходимым, поскольку класс 
 * TransportGraph никогда не использовался чем-либо кроме TransportRouter, 
 * так что я решил объединить их. */ 
class TransportRouter {
public:
    using Graph              = graph::DirectedWeightedGraph<Weight>;
    using Router             = graph::Router<Weight>;
    using EdgeId             = graph::EdgeId;
    using VertexId           = graph::VertexId;
    using TransportCatalogue = transport_catalogue::TransportCatalogue;

    explicit TransportRouter(const TransportCatalogue& catalogue, 
                             RoutingSettings settings)
        : catalogue_  (&catalogue)
        , route_graph_(catalogue_->GetStopCount() * 2)
        , settings_   (std::move(settings)) {
            BuildGraph();
            router_ = std::make_unique<Router>(route_graph_);
        }
    
    // Builds a route for two stop_names
    std::optional<RoutingResult> BuildRoute(std::string_view from, 
                                            std::string_view to) const;
private:    

    struct BusEdgeInfo;
    
    struct WaitEdgeInfo;

    bool IsBusEdge(EdgeId edge) const;

    bool IsWaitEdge(EdgeId edge) const;

    BusEdgeInfo GetBusEdgeInfo(EdgeId edge) const;

    WaitEdgeInfo GetWaitEdgeInfo(EdgeId edge) const;

    std::optional<VertexId> GetStopVertexId(std::string_view stop_name) const;

    struct WaitEdgeInfo {
        std::string_view stop_name;
        Weight total_time;

        bool operator==(const WaitEdgeInfo& other) const {
            return stop_name  == other.stop_name
                && total_time == other.total_time;
        }
    };

    struct BusEdgeInfo {
        std::string_view bus_name;
        int span_count;
        Weight total_time;

        bool operator==(const BusEdgeInfo& other) const {
            return bus_name   == other.bus_name
                && span_count == other.span_count
                && total_time == other.total_time;
        }
    };

    /* Builds a graph based on info from transport catalogue
     * and fills stop_name_to_vertex_id_ map (hence not being const) */
    void BuildGraph();

    /* Maps a given stop name to a vertex id (enumerates it, hence the name) 
     * maps a VertexInfo to a vertex id, adds an edge between them with weight 
     * equal to wait_time in the settings */
    void EnumerateVertecies(std::string_view stop_name);

    template <typename InputIt>
    BusEdgeInfo AssembleBusEdgeInfo(InputIt from_iter, 
                                    InputIt to_iter, 
                                    domain::BusPtr bus_ptr) const;

    // Transport catalogue 
    const TransportCatalogue* catalogue_;

    Graph route_graph_;

    // A router used to build routes
    std::unique_ptr<Router> router_;

    // Routing settings necessary to compute weights
    RoutingSettings settings_;

    // Maps stop name to vertex_id
    std::unordered_map<std::string_view, 
                       VertexId> stop_name_to_wait_vertex_id_;

    std::unordered_map<std::string_view,
                       VertexId> stop_name_to_bus_vertex_id_;

    // Maps span edge ids to the bus name
    std::unordered_map<EdgeId, 
                       BusEdgeInfo> bus_edge_id_to_edge_info_;

    std::unordered_map<EdgeId,
                       WaitEdgeInfo> wait_edge_id_to_edge_info_;

    VertexId current_vertex_id = 0;

};


template <typename InputIt>
TransportRouter::BusEdgeInfo TransportRouter::AssembleBusEdgeInfo(
                                                InputIt from_iter,
                                                InputIt to_iter,
                                                domain::BusPtr bus_ptr)  const {
    constexpr double MIN_PER_HOUR  = 60;
    
    constexpr double METERS_PER_KM = 1000;

    assert(from_iter < to_iter);

    const std::vector<domain::StopPtr>& route = bus_ptr->route;

    assert(from_iter < (route.end() - 1));

    double total_distance = 0.0;
    int span_count = 0;
    auto l_iter = from_iter, r_iter = from_iter + 1;

    for (; r_iter != to_iter; r_iter++, l_iter++) {
        total_distance += catalogue_->GetDistance((*l_iter)->name, 
                                                  (*r_iter)->name);
        span_count++;
    }

    if (r_iter == to_iter) {
        total_distance += catalogue_->GetDistance((*l_iter)->name,
                                                  (*r_iter)->name);
        span_count++;
    }

    Weight total_time = (total_distance / settings_.bus_velocity) 
                      * (MIN_PER_HOUR / METERS_PER_KM);


    /* Distance is measured in meters, velocity is km/h, waiting time is in minutes.
     * The best decision is to transform velocity into meters per minute. */
    return { bus_ptr->name, span_count, total_time };
}

namespace tests {

bool DoubleEq(const double lhs, const double rhs);

void PrintDebugRoutingResultMessage(std::optional<RoutingResult> route_result,
                                    std::string label);

void TestBasicRouting();

void TestComplexRouting();

void TestTrickyRouting();

} // namespace transport_router::tests

} // namespace transport_router