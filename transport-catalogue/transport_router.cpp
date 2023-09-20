#include "transport_router.hpp"
#include "graph.hpp"
#include "transport_catalogue.hpp"
#include "json_reader.hpp"

#include <optional>
#include <sstream>

namespace transport_router {

Weight TransportRouter::ComputeWeight(std::string_view from,
                                      std::string_view to) const {
    double distance = catalogue_->GetDistance(from, to);
    return (distance / settings_.bus_velocity) * 60 + settings_.wait_time_minutes;
}

void TransportRouter::EnumerateIfNot(std::string_view stop_name) {
    if (stop_name_to_vertex_id_.count(stop_name) > 0) return;

    stop_name_to_vertex_id_[stop_name] = current_vertex_id;
    vertex_id_to_stop_name_[current_vertex_id] = stop_name;

    current_vertex_id++;
}

TransportRouter::Graph TransportRouter::BuildGraph() {
    // Graph that is being constructed
    Graph graph(catalogue_->GetStopCount());

    // All bus names
    std::vector<std::string_view> bus_names = catalogue_->GetBusNames();

    /* For every bus we iterate over its route */
    for (const std::string_view bus_name : bus_names) {
        
        const domain::BusPtr bus_ptr = catalogue_->FindBus(bus_name);
        
        const std::vector<domain::StopPtr>& route = bus_ptr->route;
        
        // Enumerating the first stop in the route
        EnumerateIfNot(route.front()->name);

        for (auto l_iter  = route.begin(), r_iter  = route.begin() + 1; 
                  r_iter != route.end();   l_iter++, r_iter++) {
            const domain::StopPtr from = *l_iter;
            const domain::StopPtr to   = *r_iter;

            EnumerateIfNot(from->name);

            graph.AddEdge(graph::Edge<Weight>{
                .from   = stop_name_to_vertex_id_.at(from->name),
                .to     = stop_name_to_vertex_id_.at(to->name),
                .weight = ComputeWeight(from->name, to->name)
            });
        }
    }

    return graph;
}

std::optional<TransportRouter::VertexId> TransportRouter::GetStopId(
                                        std::string_view stop_name) const {
    if (stop_name_to_vertex_id_.count(stop_name) < 1) return std::nullopt;

    return stop_name_to_vertex_id_.at(stop_name);
}

std::optional<std::string_view> TransportRouter::GetStopNameById(
                                         VertexId vertex_id) const {
    if (vertex_id_to_stop_name_.count(vertex_id) < 1) return std::nullopt;

    return vertex_id_to_stop_name_.at(vertex_id);
}

std::optional<std::vector<RouteItem>> TransportRouter::BuildRoute(
                                        std::string_view from, 
                                        std::string_view to) const {
    // If one of the stops doesn't exist, abort and return nothing
    std::optional<VertexId> from_opt = GetStopId(from);
    if (!from_opt.has_value()) return std::nullopt;

    std::optional<VertexId> to_opt   = GetStopId(to);
    if (!to_opt.has_value()) return std::nullopt;

    // Try building a route
    std::optional<Router::RouteInfo> route_info = router_.BuildRoute(*from_opt, 
                                                                     *to_opt);
    // If building a route failed, return nothing
    if (!route_info.has_value()) return std::nullopt;

    std::optional<std::vector<RouteItem>> route_items;

    // For every edge create two items: wait and bus.
    for (graph::EdgeId edge_id : route_info->edges) {
        const graph::Edge<Weight>& edge = road_graph_.GetEdge(edge_id);
        
        // They will exist anyway, no need to check
        std::string_view edge_from = *GetStopNameById(edge.from);
        std::string_view edge_to   = *GetStopNameById(edge.to);

        std::
    }

}

namespace tests {

void TestBasicRouting() {
    using namespace std::literals;
    transport_catalogue::TransportCatalogue tc;
    json_reader::JSONReader jreader(tc);

    std::istringstream input{
    R"( 
    {
      "base_requests": [
          {
              "is_roundtrip": true,
              "name": "297",
              "stops": [
                  "Biryulyovo Zapadnoye",
                  "Biryulyovo Tovarnaya",
                  "Universam",
                  "Biryulyovo Zapadnoye"
              ],
              "type": "Bus"
          },
          {
              "is_roundtrip": false,
              "name": "635",
              "stops": [
                  "Biryulyovo Tovarnaya",
                  "Universam",
                  "Prazhskaya"
              ],
              "type": "Bus"
          },
          {
              "latitude": 55.574371,
              "longitude": 37.6517,
              "name": "Biryulyovo Zapadnoye",
              "road_distances": {
                  "Biryulyovo Tovarnaya": 2600
              },
              "type": "Stop"
          },
          {
              "latitude": 55.587655,
              "longitude": 37.645687,
              "name": "Universam",
              "road_distances": {
                  "Biryulyovo Tovarnaya": 1380,
                  "Biryulyovo Zapadnoye": 2500,
                  "Prazhskaya": 4650
              },
              "type": "Stop"
          },
          {
              "latitude": 55.592028,
              "longitude": 37.653656,
              "name": "Biryulyovo Tovarnaya",
              "road_distances": {
                  "Universam": 890
              },
              "type": "Stop"
          },
          {
              "latitude": 55.611717,
              "longitude": 37.603938,
              "name": "Prazhskaya",
              "road_distances": {},
              "type": "Stop"
          }
      ],
      "render_settings": {
          "bus_label_font_size": 20,
          "bus_label_offset": [
              7,
              15
          ],
          "color_palette": [
              "green",
              [
                  255,
                  160,
                  0
              ],
              "red"
          ],
          "height": 200,
          "line_width": 14,
          "padding": 30,
          "stop_label_font_size": 20,
          "stop_label_offset": [
              7,
              -3
          ],
          "stop_radius": 5,
          "underlayer_color": [
              255,
              255,
              255,
              0.85
          ],
          "underlayer_width": 3,
          "width": 200
      },
      },
      "stat_requests": [
          {
              "id": 1,
              "name": "297",
              "type": "Bus"
          },
          {
              "id": 2,
              "name": "635",
              "type": "Bus"
          },
          {
              "id": 3,
              "name": "Universam",
              "type": "Stop"
          },
      ]
    }
    )"}; // end of raw string

    jreader.LoadJSON(input);

    RoutingSettings settings {
        .wait_time_minutes = 6,
        .bus_velocity = 40
    };

    TransportRouter trouter(tc, settings);

    //trouter.BuildRoute(std::string_view from, std::string_view to)
}

} // namespace transport_router::tests

} // namespace transport_router
