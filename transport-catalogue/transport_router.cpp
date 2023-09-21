#include "transport_catalogue.hpp"
#include "transport_router.hpp"
#include "json_reader.hpp"
#include "graph.hpp"

#include <exception>
#include <optional>
#include <sstream>
#include <cassert>

namespace transport_router {

void RouteItemBus::Clear() {
    this->type       = "Bus";
    this->time       = {};
    this->bus_name   = {};
    this->span_count = 0;
}

void RouteItemWait::Clear() {
    this->type      = "Wait";
    this->time      = {};
    this->stop_name = {};
}

Weight TransportRouter::ComputeTravelTime(std::string_view from,
                                          std::string_view to) const {

    double distance = catalogue_->GetDistance(from, to);
    return distance / settings_.bus_velocity;
}

void TransportRouter::EnumerateIfNot(std::string_view stop_name) {
     std::cerr << "(DEBUG INFO) TransportRouter::EnumerateIfNot(...): stop_name = " 
                  << stop_name << std::endl;

    if (stop_name_to_vertex_.count(stop_name) > 0) return;

    StopVertex vertex = { /* wait_vertex: */  current_vertex_id++,
                          /* span_vertex: */  current_vertex_id++ };

    vertex_to_stop_name_[vertex]    = stop_name;
    stop_name_to_vertex_[stop_name] = vertex;

    /* Add an edge сonnecting wait_vertex to a stop_vertex. This edge
     * means that passenger waited for wait_time_minutes and got on a bus */
    EdgeId wait_edge = road_graph_.AddEdge({ 
                          /* from:   */  vertex.wait_vertex, 
                          /* to:     */  vertex.span_vertex,
                          /* weight: */  settings_.wait_time_minutes });

    wait_edges.insert(wait_edge);

    wait_edge_id_to_stop_name_[wait_edge] = stop_name;
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

            EnumerateIfNot(to->name);

            /* Add an edge connecting two span vertecies. This edge
             * means passenger goes from stop to stop and does not 
             * disembark on any of them. */
            EdgeId span_to_span_edge = graph.AddEdge({ 
                /* from:   */  stop_name_to_vertex_.at(from->name).span_vertex,
                /* to:     */  stop_name_to_vertex_.at(to->name).span_vertex,
                /* weight: */  ComputeTravelTime(from->name, to->name)
            });

            /* Add an edge connecting span vertex to wait vertex. This edge
             * means passenger goes from stop to stop and disembarks to
             * get on another bus. */
            EdgeId span_to_wait_edge = graph.AddEdge({ 
                /* from:   */  stop_name_to_vertex_.at(from->name).span_vertex,
                /* to:     */  stop_name_to_vertex_.at(to->name).wait_vertex,
                /* weight: */  ComputeTravelTime(from->name, to->name)
            });

            span_edges.insert(span_to_span_edge);
            span_edges.insert(span_to_wait_edge);

            span_edge_id_to_bus_name_[span_to_span_edge] = bus_name;
            span_edge_id_to_bus_name_[span_to_wait_edge] = bus_name;
        }
    }

    return graph;
}

std::optional<TransportRouter::VertexId> TransportRouter::GetStopWaitId(
                                                  std::string_view stop_name) const {
    if (stop_name_to_vertex_.count(stop_name)) return std::nullopt;

    return stop_name_to_vertex_.at(stop_name).wait_vertex;
}

std::optional<TransportRouter::VertexId> TransportRouter::GetStopSpanId(
                                                  std::string_view stop_name) const {
    if (stop_name_to_vertex_.count(stop_name)) return std::nullopt;

    return stop_name_to_vertex_.at(stop_name).span_vertex;
}

std::optional<RoutingResult> TransportRouter::BuildRoute(std::string_view from, 
                                                         std::string_view to) const {
    // If one of the stops doesn't exist, abort and return nothing
    std::optional<VertexId> from_opt = GetStopWaitId(from);
    if (!from_opt.has_value()) return std::nullopt;

    std::optional<VertexId> to_opt   = GetStopSpanId(to);
    if (!to_opt.has_value()) return std::nullopt;

    // Try building a route
    std::optional<Router::RouteInfo> route_info = router_.BuildRoute(*from_opt, 
                                                                     *to_opt);
    // If building a route failed, return nothing
    if (!route_info.has_value()) return std::nullopt;

    std::vector<RouteItem> route_items(route_info->edges.size());

    RouteItemBus current_bus;

    Weight total_time{};

    // For every edge create two items: wait and bus.
    for (EdgeId edge_id : route_info->edges) {

        /* Until we hit a "wait" edge, we're on the same bus,
         * so we just count spans and time.
         * As soon as we hit a "wait" edge, we push our bus item
         * into the result vector, and then we push a "wait" item. */
        if (span_edges.count(edge_id) > 0) {
            current_bus.bus_name  =  span_edge_id_to_bus_name_.at(edge_id);
            current_bus.time      += road_graph_.GetEdge(edge_id).weight;
            current_bus.span_count++;
        } else if (wait_edges.count(edge_id) > 0) {
            total_time += current_bus.time;
            total_time += settings_.wait_time_minutes;

            route_items.emplace_back(current_bus);
            current_bus.Clear();

            std::string stop_name = 
                            std::string(wait_edge_id_to_stop_name_.at(edge_id));

            route_items.emplace_back(std::move(RouteItemWait(
                /* "stop_name": */  stop_name,
                /* "time":      */  settings_.wait_time_minutes
            )));
        }
    }

    return RoutingResult{ total_time, route_items };
}

namespace tests {

bool SHOW_DEBUG_MESSAGES = true;

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
      "routing_settings": {
          "bus_velocity": 40,
          "bus_wait_time": 6
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
          }
      ]
    }
  
    )"}; // end of raw string

    jreader.LoadJSON(input);

    if (SHOW_DEBUG_MESSAGES) {
        for (std::string_view stop_name : tc.GetStopNames()) {
            std::cerr << "(DEBUG INFO) stop: " << stop_name << std::endl;
        }

        for (std::string_view bus_name : tc.GetBusNames()) {
            std::cerr << "(DEBUG INFO) bus: " << bus_name << std::endl;

            const domain::BusPtr bus = tc.FindBus(bus_name);

            for (domain::StopPtr stop_ptr : bus->route) {
                std::cerr << "(DEBUG INFO) route stop: " << stop_ptr->name << std::endl;
            }
        }
    }

    RoutingSettings settings {
        /* wait_time_minutes: */  6,
        /* bus_velocity:      */  40
    };

    TransportRouter trouter(tc, settings);

    std::optional<RoutingResult> route_result1 = 
                                        trouter.BuildRoute("Biryulyovo Zapadnoye"sv,
                                                            "Universam"sv);
    std::optional<RoutingResult> route_result2 = 
                                        trouter.BuildRoute("Biryulyovo Zapadnoye"sv,
                                                            "Prazhskaya"sv);

    bool test_total_time1 = route_result1->total_time == 11.235;
    
    if (SHOW_DEBUG_MESSAGES) 
        std::cerr << "(DEBUG INFO) route_result1->total_time = " 
                  << route_result1->total_time << std::endl;
    
    assert(test_total_time1);

    bool test_total_time2 = route_result2->total_time == 24.21;
    
    if (SHOW_DEBUG_MESSAGES)
        std::cerr << "(DEBUG INFO) route_result1->total_time = " 
                  << route_result1->total_time << std::endl;

    assert(test_total_time2);
}

} // namespace transport_router::tests

} // namespace transport_router
