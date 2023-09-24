#include <exception>
#include <optional>
#include <ostream>
#include <sstream>
#include <cassert>
#include <variant>

#include "transport_catalogue.hpp"
#include "transport_router.hpp"
#include "json_reader.hpp"
#include "graph.hpp"

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

size_t TransportGraph::ComputeAmountOfVertecies() const {
    size_t stop_count = catalogue_->GetStopCount();

    size_t sum_bus_routes_count = 0;

    for (std::string_view bus_name : catalogue_->GetBusNames()) {
        sum_bus_routes_count += catalogue_->FindBus(bus_name)->route.size();
    }

    std::cerr << "(DEBUG INFO) amount of vertecies = " 
              << stop_count + sum_bus_routes_count << std::endl;

    return stop_count + sum_bus_routes_count;
}

Weight TransportGraph::ComputeTravelTime(std::string_view from,
                                          std::string_view to) const {
    constexpr double MIN_PER_HOUR  = 60;
    constexpr double METERS_PER_KM = 1000;


    double distance = catalogue_->GetDistance(from, to);

    /* Distance is measured in meters, velocity is km/h, waiting time is in minutes.
     * The best decision is to transform velocity into meters per minute. */
    return (distance * MIN_PER_HOUR) / (settings_.bus_velocity * METERS_PER_KM);
}

void TransportGraph::EnumerateVertecies(VertexInfo vertex_info) {

    bool stop_vertex_existed = stop_name_to_vertex_id_
                                    .count(vertex_info.stop_name) > 0;
    bool span_vertex_existed = vertex_info_to_span_vertex_id_
                                    .count(vertex_info) > 0;

    if (!stop_vertex_existed) {
        stop_name_to_vertex_id_[vertex_info.stop_name] = current_vertex_id++;
    } 

    if (!span_vertex_existed) {
        vertex_info_to_span_vertex_id_[vertex_info] = current_vertex_id++;
    }

    if (stop_vertex_existed && span_vertex_existed) return;

    /* Add an edge сonnecting wait_vertex to a stop_vertex. This edge
     * means that passenger waited for bus_wait_time and got on a bus */
    EdgeId wait_edge = route_graph_.AddEdge({ 
                          /* from:   */  stop_name_to_vertex_id_.at(
                                                            vertex_info.stop_name), 
                          /* to:     */  vertex_info_to_span_vertex_id_.at(
                                                                      vertex_info),
                          /* weight: */  settings_.bus_wait_time });
    
    std::cerr << "(DEBUG INFO) TransportGraph::EnumerateVertecies(...): "
              << "saved edge stop_name = " << vertex_info.stop_name 
              << " with seq_number = "     << vertex_info.sequence_number 
              << std::endl;

    EnumerateWaitEdge(wait_edge, vertex_info.stop_name);
}

void TransportGraph::EnumerateSpanEdge(EdgeId edge, std::string_view bus_name) {
    span_edges.insert(edge);
    span_edge_id_to_bus_name_[edge] = bus_name;
}

void TransportGraph::EnumerateWaitEdge(EdgeId edge, std::string_view stop_name) {
    wait_edges.insert(edge);
    wait_edge_id_to_stop_name_[edge] = stop_name;
}

TransportGraph::EdgeId TransportGraph::AddSpanToSpanEdge(
                                                    VertexInfo from,
                                                    VertexInfo to,
                                                    std::string_view bus_name) {
    EdgeId span_to_span_edge = route_graph_.AddEdge({ 
        /* from:   */  vertex_info_to_span_vertex_id_.at(from),
        /* to:     */  vertex_info_to_span_vertex_id_.at(to),
        /* weight: */  ComputeTravelTime(from.stop_name, to.stop_name)
    });
    
    EnumerateSpanEdge(span_to_span_edge, bus_name);

    return span_to_span_edge;
}

TransportGraph::EdgeId TransportGraph::AddSpanToWaitEdge(
                                                    VertexInfo from,
                                                    VertexInfo to,
                                                    std::string_view bus_name) {
    EdgeId span_to_wait_edge = route_graph_.AddEdge({ 
        /* from:   */  vertex_info_to_span_vertex_id_.at(from),
        /* to:     */  stop_name_to_vertex_id_.at(to.stop_name),
        /* weight: */  ComputeTravelTime(from.stop_name, to.stop_name)  
    });

    EnumerateSpanEdge(span_to_wait_edge, bus_name);

    return span_to_wait_edge;
} 

void TransportGraph::BuildGraph() {
    // All bus names
    std::vector<std::string_view> bus_names = catalogue_->GetBusNames();
    
    size_t current_seq_number = 1;

    /* For every bus we iterate over its route */
    for (const std::string_view bus_name : bus_names) {
        
        const domain::BusPtr bus_ptr = catalogue_->FindBus(bus_name);
        
        const std::vector<domain::StopPtr>& route = bus_ptr->route;
        
        // Enumerating the first stop in the route
        EnumerateVertecies({ route.front()->name, current_seq_number });

        auto last_stop_iter = route.end() - 1;

        for (auto l_iter  = route.begin(), r_iter  = route.begin() + 1; 
                  r_iter != route.end();   l_iter++, r_iter++) {
            
            const VertexInfo from{ (*l_iter)->name,   current_seq_number };
            const VertexInfo to  { (*r_iter)->name, ++current_seq_number };

            EnumerateVertecies(to);

            if (!(r_iter == last_stop_iter)) {
                AddSpanToSpanEdge(from, to, bus_name);
            }

            AddSpanToWaitEdge(from, to, bus_name);

            std::cerr << "(DEBUG INFO) TransportGraph::BuildGraph(...): saved bus_name = " << bus_name << std::endl;
        }
    }

    std::cerr << "(DEBUG INFO) TransportGraph::BuildGraph(...): "
              << "Total vertecies count:"
              << vertex_info_to_span_vertex_id_.size() 
               + stop_name_to_vertex_id_.size()
              << std::endl
              << "(DEBUG INFO) TransportGraph::BuildGraph(...): " 
              << "current_vertex_id = " << current_vertex_id << std::endl;
}

std::optional<TransportGraph::VertexId> TransportGraph::GetStopVertexId(
                                                  std::string_view stop_name) const {
    if (stop_name_to_vertex_id_.count(stop_name) < 1) return std::nullopt;

    return stop_name_to_vertex_id_.at(stop_name);
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

    std::optional<VertexId> to_opt   = transport_graph_.GetStopVertexId(to);
    if (!to_opt.has_value()) return std::nullopt;
    std::cerr << "(DEBUG INFO) to_opt has value" << std::endl;

    
    if (from == to) {
        return RoutingResult{ /* "total_time": */ 0.0, /* "items": */ {} };
    }

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

bool DoubleEq(const double lhs, const double rhs) {
    constexpr double MAX_DEVITAION = 1e-6;

    return std::abs(lhs - rhs) < MAX_DEVITAION;
}

void PrintDebugRoutingResultMessage(std::optional<RoutingResult> route_result,
                                    std::string label) {
    for (RouteItem route_item : route_result->items) {
        if (std::holds_alternative<RouteItemWait>(route_item)) {
            auto wait_item = std::get<RouteItemWait>(route_item);
            std::cerr << "(DEBUG INFO) " << label << " wait_item: stop_name = " 
                      << wait_item.stop_name << std::endl
                      << "(DEBUG INFO) " << label << " wait_item: time = " 
                      << wait_item.time << std::endl;

        } else if (std::holds_alternative<RouteItemBus>(route_item)) {
            auto bus_item = std::get<RouteItemBus>(route_item);
            std::cerr << "(DEBUG INFO) " << label << " bus_item: bus_name = "
                      << bus_item.bus_name << std::endl
                      << "(DEBUG INFO) " << label << " bus_item: span_count = "
                      << bus_item.span_count << std::endl
                      << "(DEBUG INFO) " << label << " bus_item: total_time = "
                      << bus_item.time << std::endl;
        }
    }
}

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
        /* bus_wait_time: */  6,
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
        PrintDebugRoutingResultMessage(route_result1, "route_result1");
    }

    bool test_total_time1 = DoubleEq(route_result1->total_time, 11.235);
    
    if (SHOW_DEBUG_MESSAGES) 
        std::cerr << "(DEBUG INFO) route_result1->total_time = " 
                  << route_result1->total_time << std::endl;
    
    assert(test_total_time1);

    if (SHOW_DEBUG_MESSAGES) {
        PrintDebugRoutingResultMessage(route_result2, "route_result2");
    }

    bool test_total_time2 = DoubleEq(route_result2->total_time, 24.21);
    
    if (SHOW_DEBUG_MESSAGES)
        std::cerr << "(DEBUG INFO) route_result1->total_time = " 
                  << route_result1->total_time << std::endl;

    assert(test_total_time2);
}



void TestComplexRouting() {
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
                  "Biryusinka",
                  "Apteka",
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
                  "Biryusinka",
                  "TETs 26",
                  "Pokrovskaya",
                  "Prazhskaya"
              ],
              "type": "Bus"
          },
          {
              "is_roundtrip": false,
              "name": "828",
              "stops": [
                  "Biryulyovo Zapadnoye",
                  "TETs 26",
                  "Biryusinka",
                  "Universam",
                  "Pokrovskaya",
                  "Rossoshanskaya ulitsa"
              ],
              "type": "Bus"
          },
          {
              "latitude": 55.574371,
              "longitude": 37.6517,
              "name": "Biryulyovo Zapadnoye",
              "road_distances": {
                  "Biryulyovo Tovarnaya": 2600,
                  "TETs 26": 1100
              },
              "type": "Stop"
          },
          {
              "latitude": 55.587655,
              "longitude": 37.645687,
              "name": "Universam",
              "road_distances": {
                  "Biryulyovo Tovarnaya": 1380,
                  "Biryusinka": 760,
                  "Pokrovskaya": 2460
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
              "latitude": 55.581065,
              "longitude": 37.64839,
              "name": "Biryusinka",
              "road_distances": {
                  "Apteka": 210,
                  "TETs 26": 400
              },
              "type": "Stop"
          },
          {
              "latitude": 55.580023,
              "longitude": 37.652296,
              "name": "Apteka",
              "road_distances": {
                  "Biryulyovo Zapadnoye": 1420
              },
              "type": "Stop"
          },
          {
              "latitude": 55.580685,
              "longitude": 37.642258,
              "name": "TETs 26",
              "road_distances": {
                  "Pokrovskaya": 2850
              },
              "type": "Stop"
          },
          {
              "latitude": 55.603601,
              "longitude": 37.635517,
              "name": "Pokrovskaya",
              "road_distances": {
                  "Rossoshanskaya ulitsa": 3140
              },
              "type": "Stop"
          },
          {
              "latitude": 55.595579,
              "longitude": 37.605757,
              "name": "Rossoshanskaya ulitsa",
              "road_distances": {
                  "Pokrovskaya": 3210
              },
              "type": "Stop"
          },
          {
              "latitude": 55.611717,
              "longitude": 37.603938,
              "name": "Prazhskaya",
              "road_distances": {
                  "Pokrovskaya": 2260
              },
              "type": "Stop"
          },
          {
              "is_roundtrip": false,
              "name": "750",
              "stops": [
                  "Tolstopaltsevo",
                  "Rasskazovka"
              ],
              "type": "Bus"
          },
          {
              "latitude": 55.611087,
              "longitude": 37.20829,
              "name": "Tolstopaltsevo",
              "road_distances": {
                  "Rasskazovka": 13800
              },
              "type": "Stop"
          },
          {
              "latitude": 55.632761,
              "longitude": 37.333324,
              "name": "Rasskazovka",
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
          "bus_velocity": 30,
          "bus_wait_time": 2
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
              "name": "828",
              "type": "Bus"
          },
          {
              "id": 4,
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
        /* bus_wait_time: */  2,
        /* bus_velocity:      */  30
    };

    TransportRouter trouter(tc, settings);

    std::optional<RoutingResult> route_result1 = 
                                        trouter.BuildRoute("Biryulyovo Zapadnoye"sv,
                                                            "Apteka"sv);
    assert(route_result1.has_value());
    bool test_total_time1 = DoubleEq(route_result1->total_time, 7.42);

    if (SHOW_DEBUG_MESSAGES) {
        PrintDebugRoutingResultMessage(route_result1, "route_result1");
    }

    std::cerr << "test_total_time1: route_result1->total_time = " 
              << route_result1->total_time << std::endl;
    assert(test_total_time1);

    std::optional<RoutingResult> route_result2 = 
                                        trouter.BuildRoute("Biryulyovo Zapadnoye"sv,
                                                            "Pokrovskaya"sv);
    assert(route_result2.has_value());
    bool test_total_time2 = DoubleEq(route_result2->total_time, 11.44);
    std::cerr << "test_total_time2: route_result2->total_time = " 
              << route_result2->total_time << std::endl;
    
    if (SHOW_DEBUG_MESSAGES) {
        PrintDebugRoutingResultMessage(route_result2, "route_result2");
    }

    assert(test_total_time2);

    std::optional<RoutingResult> route_result3 = 
                                        trouter.BuildRoute("Biryulyovo Tovarnaya"sv,
                                                           "Pokrovskaya"sv);
    assert(route_result3.has_value());
    bool test_total_time3 = DoubleEq(route_result3->total_time, 10.7);
    std::cerr << "test_total_time3: route_result3->total_time = " 
              << route_result3->total_time << std::endl;
    
    if (SHOW_DEBUG_MESSAGES) {
        PrintDebugRoutingResultMessage(route_result3, "route_result3");
    }

    assert(test_total_time3);

    std::optional<RoutingResult> route_result4 = 
                                        trouter.BuildRoute("Biryulyovo Tovarnaya"sv,
                                                           "Biryulyovo Zapadnoye"sv);
    assert(route_result4.has_value());
    bool test_total_time4 = DoubleEq(route_result4->total_time, 8.56);

    if (SHOW_DEBUG_MESSAGES) {
        PrintDebugRoutingResultMessage(route_result4, "route_result4");
    }

    std::cerr << "test_total_time4: route_result4->total_time = " 
              << route_result4->total_time << std::endl;
    assert(test_total_time4);

    std::optional<RoutingResult> route_result5 = 
                                        trouter.BuildRoute("Biryulyovo Tovarnaya"sv,
                                                            "Prazhskaya"sv);
    assert(route_result5.has_value());
    bool test_total_time5 = DoubleEq(route_result5->total_time, 16.32);

    if (SHOW_DEBUG_MESSAGES) {
        PrintDebugRoutingResultMessage(route_result5, "route_result5");
    }

    std::cerr << "test_total_time5: route_result5->total_time = " 
              << route_result5->total_time << std::endl;

    assert(test_total_time5);
    
    std::optional<RoutingResult> route_result6 = 
                                        trouter.BuildRoute("Apteka"sv,
                                                           "Biryulyovo Tovarnaya"sv);
    assert(route_result6.has_value());
    bool test_total_time6 = DoubleEq(route_result6->total_time, 12.04);

    if (SHOW_DEBUG_MESSAGES) {
        PrintDebugRoutingResultMessage(route_result6, "route_result6");
    }

    std::cerr << "test_total_time6: route_result6->total_time = " 
              << route_result6->total_time << std::endl;
    assert(test_total_time6);
    
    std::optional<RoutingResult> route_result7 = 
                                        trouter.BuildRoute("Biryulyovo Zapadnoye"sv,
                                                           "Tolstopaltsevo"sv);
    assert(!route_result7.has_value());

}

void TestTrickyRouting() {
    using namespace std::literals;
    transport_catalogue::TransportCatalogue tc;
    json_reader::JSONReader jreader(tc);

    std::istringstream input{
    R"( 
         {
      "base_requests": [
          {
              "is_roundtrip": true,
              "name": "289",
              "stops": [
                  "Zagorye",
                  "Lipetskaya ulitsa 46",
                  "Lipetskaya ulitsa 40",
                  "Lipetskaya ulitsa 40",
                  "Lipetskaya ulitsa 46",
                  "Moskvorechye",
                  "Zagorye"
              ],
              "type": "Bus"
          },
          {
              "latitude": 55.579909,
              "longitude": 37.68372,
              "name": "Zagorye",
              "road_distances": {
                  "Lipetskaya ulitsa 46": 230
              },
              "type": "Stop"
          },
          {
              "latitude": 55.581441,
              "longitude": 37.682205,
              "name": "Lipetskaya ulitsa 46",
              "road_distances": {
                  "Lipetskaya ulitsa 40": 390,
                  "Moskvorechye": 12400
              },
              "type": "Stop"
          },
          {
              "latitude": 55.584496,
              "longitude": 37.679133,
              "name": "Lipetskaya ulitsa 40",
              "road_distances": {
                  "Lipetskaya ulitsa 40": 1090,
                  "Lipetskaya ulitsa 46": 380
              },
              "type": "Stop"
          },
          {
              "latitude": 55.638433,
              "longitude": 37.638433,
              "name": "Moskvorechye",
              "road_distances": {
                  "Zagorye": 10000
              },
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
          "bus_velocity": 30,
          "bus_wait_time": 2
      },
      "stat_requests": [
          {
              "id": 1,
              "name": "289",
              "type": "Bus"
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
        /* bus_wait_time: */  2,
        /* bus_velocity:      */  30
    };

    TransportRouter trouter(tc, settings);

    std::optional<RoutingResult> route_result1 = trouter.BuildRoute("Zagorye", 
                                                                    "Moskvorechye");
    assert(route_result1.has_value());

    if (SHOW_DEBUG_MESSAGES) {
        PrintDebugRoutingResultMessage(route_result1, "route_result1");
    }

    std::cerr << "test_total_time1: route_result1->total_time = " 
              << route_result1->total_time << std::endl;

    bool test_total_time1 = DoubleEq(route_result1->total_time, 29.26);
    assert(test_total_time1);

    std::optional<RoutingResult> route_result2 = trouter.BuildRoute("Moskvorechye", 
                                                                    "Zagorye");
    assert(route_result2.has_value());

    if (SHOW_DEBUG_MESSAGES) {
        PrintDebugRoutingResultMessage(route_result2, "route_result2");

    }

    std::cerr << "test_total_time2: route_result2->total_time = " 
              << route_result2->total_time << std::endl;

    bool test_total_time2 = DoubleEq(route_result2->total_time, 22);
    assert(test_total_time2);

    std::optional<RoutingResult> route_result3 = trouter.BuildRoute(
                                                            "Lipetskaya ulitsa 40",
                                                            "Lipetskaya ulitsa 40");
    assert(route_result3.has_value());

    if (SHOW_DEBUG_MESSAGES) {
        PrintDebugRoutingResultMessage(route_result3, "route_result3");
    }

    std::cerr << "test_total_time3: route_result3->total_time = " 
              << route_result3->total_time << std::endl;

    bool test_total_time3 = DoubleEq(route_result3->total_time, 0);
    assert(test_total_time3);
}

} // namespace transport_router::tests

} // namespace transport_router
