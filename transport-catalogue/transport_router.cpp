#include "transport_catalogue.hpp"
#include "transport_router.hpp"
#include "json_reader.hpp"
#include "graph.hpp"

#include <exception>
#include <optional>
#include <ostream>
#include <sstream>
#include <cassert>
#include <variant>

namespace transport_router {

void RouteItemBus::Clear() {
    this->type       = "Bus";
    this->time       = {};
    this->bus_name   = {};
    this->span_count = 0;
}

void RouteItemWait::Clear() {
    this->type       = "Wait";
    this->time       = {};
    this->stop_name  = {};
}

// ======================== TransportGraph ========================

Weight TransportGraph::ComputeTravelTime(std::string_view from,
                                          std::string_view to) const {

    double distance = catalogue_->GetDistance(from, to);
    return distance / settings_.bus_velocity;
}

void TransportGraph::EnumerateVertecies(std::string_view stop_name) {

    bool stop_vertex_existed = stop_name_to_vertex_id_.count(stop_name) > 0;
    bool span_vertex_existed = stop_name_to_span_vertex_id_.count(stop_name) > 0;

    if (!stop_vertex_existed) {
        vertex_id_to_stop_name_[current_vertex_id++] = stop_name;
        stop_name_to_vertex_id_[stop_name]           = current_vertex_id++;
    } 

    if (!span_vertex_existed) {
        span_vertex_id_to_stop_name_[current_vertex_id++] = stop_name;
        stop_name_to_span_vertex_id_[stop_name]           = current_vertex_id++;
    }

    if (stop_vertex_existed && span_vertex_existed) return;

    /* Add an edge сonnecting wait_vertex to a stop_vertex. This edge
     * means that passenger waited for wait_time_minutes and got on a bus */
    EdgeId wait_edge = route_graph_.AddEdge({ 
                          /* from:   */  stop_name_to_vertex_id_.at(stop_name), 
                          /* to:     */  stop_name_to_span_vertex_id_.at(stop_name),
                          /* weight: */  settings_.wait_time_minutes });
    
    std::cerr << "(DEBUG INFO) TransportGraph::EnumerateVertecies(...): saved edge stop_name = " << stop_name << std::endl;

    wait_edges.insert(wait_edge);

    wait_edge_id_to_stop_name_[wait_edge] = stop_name;
}

void TransportGraph::BuildGraph() {
    // All bus names
    std::vector<std::string_view> bus_names = catalogue_->GetBusNames();

    /* For every bus we iterate over its route */
    for (const std::string_view bus_name : bus_names) {
        
        const domain::BusPtr bus_ptr = catalogue_->FindBus(bus_name);
        
        const std::vector<domain::StopPtr>& route = bus_ptr->route;
        
        // Enumerating the first stop in the route
        EnumerateVertecies(route.front()->name);

        for (auto l_iter  = route.begin(), r_iter  = route.begin() + 1; 
                  r_iter != route.end();   l_iter++, r_iter++) {
            
            const domain::StopPtr from = *l_iter;
            const domain::StopPtr to   = *r_iter;

            EnumerateVertecies(to->name);

            /* Add an edge connecting two bus vertecies. This edge
             * means passenger goes from stop to stop and does not 
             * disembark on any of them. */
            EdgeId span_to_span_edge = route_graph_.AddEdge({ 
                /* from:   */  stop_name_to_span_vertex_id_.at(from->name),
                /* to:     */  stop_name_to_span_vertex_id_.at(to->name),
                /* weight: */  ComputeTravelTime(from->name, to->name)
            });

            /* Add an edge connecting span vertex to wait vertex. This 
             * edge means passenger goes from stop to stop and disembarks 
             * to get on another bus. */
            EdgeId span_to_wait_edge = route_graph_.AddEdge({ 
                /* from:   */  stop_name_to_span_vertex_id_.at(from->name),
                /* to:     */  stop_name_to_vertex_id_.at(to->name),
                /* weight: */  ComputeTravelTime(from->name, to->name)
            });

            span_edges.insert(span_to_span_edge);
            span_edges.insert(span_to_wait_edge);

            std::cerr << "(DEBUG INFO) TransportGraph::BuildGraph(...): saved bus_name = " << bus_name << std::endl;

            span_edge_id_to_bus_name_[span_to_span_edge] = bus_name;
            span_edge_id_to_bus_name_[span_to_wait_edge] = bus_name;
        }
    }
}

std::optional<TransportGraph::VertexId> TransportGraph::GetStopVertexId(
                                                  std::string_view stop_name) const {
    if (stop_name_to_vertex_id_.count(stop_name) < 1) return std::nullopt;

    return stop_name_to_vertex_id_.at(stop_name);
}

std::optional<TransportGraph::VertexId> TransportGraph::GetSpanVertexId(
                                                  std::string_view stop_name) const {
    if (stop_name_to_span_vertex_id_.count(stop_name) < 1) return std::nullopt;

    return stop_name_to_span_vertex_id_.at(stop_name);
}

const TransportGraph::Graph& TransportGraph::GetGraph() const {
    return route_graph_;
}

bool TransportGraph::IsSpanEdge(EdgeId edge) const {
    return span_edges.count(edge) > 0;
}

bool TransportGraph::IsWaitEdge(EdgeId edge) const {
    return wait_edges.count(edge) > 0;
}

Weight TransportGraph::GetEdgeWeight(EdgeId edge) const {
    return route_graph_.GetEdge(edge).weight;
}

std::string_view TransportGraph::GetSpanEdgeBusName(EdgeId edge) const {
    return span_edge_id_to_bus_name_.at(edge);
}

std::string_view TransportGraph::GetWaitEdgeStopName(EdgeId edge) const {
    return wait_edge_id_to_stop_name_.at(edge);
}

// ======================== TransportRouter ========================

std::optional<RoutingResult> TransportRouter::BuildRoute(std::string_view from, 
                                                         std::string_view to) const {
    // If one of the stops doesn't exist, abort and return nothing
    std::optional<VertexId> from_opt = transport_graph_.GetStopVertexId(from);
    if (!from_opt.has_value()) return std::nullopt;
    std::cerr << "(DEBUG INFO) from_opt has value" << std::endl;

    std::optional<VertexId> to_opt   = transport_graph_.GetSpanVertexId(to);
    if (!to_opt.has_value()) return std::nullopt;
    std::cerr << "(DEBUG INFO) to_opt has value" << std::endl;

    // Try building a route
    std::optional<Router::RouteInfo> route_info = router_.BuildRoute(*from_opt, 
                                                                     *to_opt);
    // If building a route failed, return nothing
    if (!route_info.has_value()) return std::nullopt;

    std::cerr << "(DEBUG INFO) route_info has value" << std::endl;
    
    std::vector<RouteItem> route_items;

    RouteItemBus current_bus;

    Weight total_time{};

    // For every edge create two items: wait and bus.
    for (EdgeId edge_id : route_info->edges) {
        std::cerr << "(DEBUG INFO) BuildRoute(): Entering a for loop, edge_id = " 
                  << edge_id << std::endl;
        /* Until we hit a "wait" edge, we're on the same bus,
         * so we just count spans and time.
         * As soon as we hit a "wait" edge, we push our bus item
         * into the result vector, and then we push a "wait" item. */
        if (transport_graph_.IsSpanEdge(edge_id)) {
            std::cerr << "(DEBUG INFO) BuildRoute(): edge(" 
                      << edge_id << ") is a span edge" << std::endl;

            current_bus.bus_name  =  transport_graph_.GetSpanEdgeBusName(edge_id);
            current_bus.time      += transport_graph_.GetEdgeWeight(edge_id);
            current_bus.span_count++;
            std::cerr << "(DEBUG INFO) BuildRoute()(IsSpanEdge): span_edge bus_name = "
                      << current_bus.bus_name << std::endl
                      << "(DEBUG INFO) BuildRoute()(IsSpanEdge): span_edge time = "
                      << current_bus.time << std::endl
                      << "(DEBUG INFO) BuildRoute()(IsSpanEdge): span_edge span_count = "
                      << current_bus.span_count << std::endl;
        } else if (transport_graph_.IsWaitEdge(edge_id)) {
            std::cerr << "(DEBUG INFO) BuildRoute(): edge(" 
                      << edge_id << ") is a wait edge" << std::endl;
            
            total_time += transport_graph_.GetEdgeWeight(edge_id);

            if (current_bus.span_count > 0) {
                std::cerr << "(DEBUG INFO) BuildRoute()(IsWaitEdge): span_edge bus_name = "
                          << current_bus.bus_name << std::endl
                          << "(DEBUG INFO) BuildRoute()(IsWaitEdge): span_edge time = "
                          << current_bus.time << std::endl
                          << "(DEBUG INFO) BuildRoute()(IsWaitEdge): span_edge span_count = "
                          << current_bus.span_count << std::endl;
                total_time += current_bus.time;
                route_items.emplace_back(current_bus);
                current_bus.Clear();                
            }

            std::string stop_name = 
                        std::string(transport_graph_.GetWaitEdgeStopName(edge_id));

            std::cerr << "(DEBUG INFO) BuildRoute(): wait_edge stop_name = "
                      << stop_name << std::endl
                      << "(DEBUG INFO) BuildRoute(): wait_edge time = "
                      << transport_graph_.GetEdgeWeight(edge_id) << std::endl;

            route_items.emplace_back(std::move(RouteItemWait(
                /* "stop_name": */  stop_name,
                /* "time":      */  transport_graph_.GetEdgeWeight(edge_id)
            )));
        }
    }

    route_items.emplace_back(current_bus);
    total_time += current_bus.time;

    std::cerr << "(DEBUG INFO) BuildRoute(): Returning route_items of size = "
              << route_items.size() << std::endl;
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
    assert(route_result1.has_value());

    assert(route_result2.has_value());

    if (SHOW_DEBUG_MESSAGES) {
        for (RouteItem route_item : *route_result1->items) {
            if (std::holds_alternative<RouteItemWait>(route_item)) {
                auto wait_item = std::get<RouteItemWait>(route_item);
                std::cerr << "(DEBUG INFO) route_result1 wait_item: stop_name = " 
                          << wait_item.stop_name << std::endl
                          << "(DEBUG INFO) route_result1 wait_item: time = " 
                          << wait_item.time << std::endl;

            } else if (std::holds_alternative<RouteItemBus>(route_item)) {
                auto bus_item = std::get<RouteItemBus>(route_item);
                std::cerr << "(DEBUG INFO) route_result1 bus_item: bus_name = "
                          << bus_item.bus_name << std::endl
                          << "(DEBUG INFO) route_result1 bus_item: span_count = "
                          << bus_item.span_count << std::endl
                          << "(DEBUG INFO) route_result1 bus_item: total_time = "
                          << bus_item.time << std::endl;
            }
        }
    }

    bool test_total_time1 = route_result1->total_time == 11.235;
    
    if (SHOW_DEBUG_MESSAGES) 
        std::cerr << "(DEBUG INFO) route_result1->total_time = " 
                  << route_result1->total_time << std::endl;
    
    assert(test_total_time1);

    if (SHOW_DEBUG_MESSAGES) {
        for (RouteItem route_item : *route_result2->items) {
            if (std::holds_alternative<RouteItemWait>(route_item)) {
                auto wait_item = std::get<RouteItemWait>(route_item);
                std::cerr << "(DEBUG INFO) route_result2 wait_item: stop_name = " 
                          << wait_item.stop_name << std::endl
                          << "(DEBUG INFO) route_result2 wait_item: time = " 
                          << wait_item.time << std::endl;

            } else if (std::holds_alternative<RouteItemBus>(route_item)) {
                auto bus_item = std::get<RouteItemBus>(route_item);
                std::cerr << "(DEBUG INFO) route_result2 bus_item: bus_name = "
                          << bus_item.bus_name << std::endl
                          << "(DEBUG INFO) route_result2 bus_item: span_count = "
                          << bus_item.span_count << std::endl
                          << "(DEBUG INFO) route_result2 bus_item: total_time"
                          << bus_item.time << std::endl;
            }
        }
    }

    bool test_total_time2 = route_result2->total_time == 24.21;
    
    if (SHOW_DEBUG_MESSAGES)
        std::cerr << "(DEBUG INFO) route_result1->total_time = " 
                  << route_result1->total_time << std::endl;

    assert(test_total_time2);
}

} // namespace transport_router::tests

} // namespace transport_router
