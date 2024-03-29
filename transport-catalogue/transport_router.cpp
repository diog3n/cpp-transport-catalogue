#include <exception>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <cassert>
#include <stdexcept>
#include <variant>

#include "domain.h"
#include "geo.h"
#include "graph.pb.h"
#include "router.h"
#include "transport_catalogue.h"
#include "transport_router.h"
#include "json_reader.h"
#include "graph.h"

namespace transport_router {

void TransportRouterInfo::AddVertexInfo(VertexInfo info) {
    vertexes_.push_back(std::move(info));
}

void TransportRouterInfo::AddEdgeInfo(EdgeInfo info) {
    edges_.push_back(info);
}

void TransportRouterInfo::SetRoutingSettings(RoutingSettings routing_settings) {
    routing_settings_ = std::move(routing_settings);
}

const std::vector<TransportRouterInfo::EdgeInfo>& 
                  TransportRouterInfo::GetEdgesInfo() const {
    return edges_;
}

const std::vector<TransportRouterInfo::VertexInfo>&
                  TransportRouterInfo::GetVertexesInfo() const {
    return vertexes_;
}

RoutingSettings TransportRouterInfo::GetRoutingSettings() const {
    return routing_settings_;
}


void TransportRouter::EnumerateVertecies(std::string_view stop_name) {

    if (stop_name_to_wait_vertex_id_.count(stop_name) > 0) return;

    stop_name_to_wait_vertex_id_[stop_name] = current_vertex_id;

    stop_name_to_bus_vertex_id_[stop_name] = current_vertex_id + 1;

    /* Add an edge сonnecting wait_vertex to a stop_vertex. This edge
     * means that passenger waited for bus_wait_time and got on a bus */
    EdgeId wait_edge = route_graph_->AddEdge({ 
                        /* from:   */  current_vertex_id,
                        /* to:     */  current_vertex_id + 1,
                        /* weight: */  settings_.bus_wait_time });
    
    current_vertex_id += 2;

    wait_edge_id_to_edge_info_[wait_edge] = { stop_name, settings_.bus_wait_time };
}

void TransportRouter::BuildGraph() {
    std::vector<std::string_view> bus_names = catalogue_->GetBusNames();
    
    for (std::string_view bus_name : bus_names) {

        const domain::BusPtr bus_ptr = catalogue_->FindBus(bus_name);
        
        const std::vector<domain::StopPtr>& route = bus_ptr->route;

        /* This is by far the trickiest part of the code. This vector will
         * contain time and span counts of the first route.size() - 1 edges. 
         * Such measurements will allow us to quickly compute distances. */
        std::vector<BusEdgeInfo> edge_measurements(route.size() - 1);
        
        EnumerateVertecies(route.front()->name);

        for (auto from_iter =  route.begin(); 
                  from_iter != (route.end() - 1); from_iter++) {

            const domain::StopPtr from_ptr = *from_iter;

            for (auto to_iter = from_iter + 1; 
                      to_iter != route.end(); to_iter++) {

                const domain::StopPtr to_ptr = *to_iter;

                EnumerateVertecies(to_ptr->name);

                BusEdgeInfo edge_info;

                /* For the first route.size() - 1 edges all the measurements 
                 * will be computed head-on, but for any others it'll be done at 
                 * constant time. */
                if (from_iter == route.begin()) {
                    edge_info = AssembleBusEdgeInfo(from_iter, to_iter, 
                                                               bus_ptr);
                    edge_measurements[to_iter - from_iter - 1] = edge_info; 
                } else {
                    const auto& l_info {
                        edge_measurements.at(from_iter - route.begin() - 1)        
                    };

                    const auto& r_info {
                        edge_measurements.at(to_iter - route.begin() - 1)
                    };

                    edge_info = {
                        bus_ptr->name,
                        r_info.span_count - l_info.span_count,
                        static_cast<Weight>(
                            r_info.total_time - l_info.total_time
                        )
                    };
                }

                EdgeId route_edge = route_graph_->AddEdge({
                /* "from":   */ stop_name_to_bus_vertex_id_.at(from_ptr->name),
                /* "to":     */ stop_name_to_wait_vertex_id_.at(to_ptr->name),
                /* "weight": */ edge_info.total_time
                });

                bus_edge_id_to_edge_info_[route_edge] = std::move(edge_info);
            }
        }
    }
}

void TransportRouter::BuildGraphFromInfo(const TransportRouterInfo& info) {
    for (const TransportRouterInfo::VertexInfo& v_info : info.GetVertexesInfo()) {
        auto stop_ptr = catalogue_->FindStop(v_info.stop_name);

        if (v_info.is_bus_vertex) {
            stop_name_to_bus_vertex_id_[stop_ptr->name] = v_info.id;
            continue;
        }

        stop_name_to_wait_vertex_id_[stop_ptr->name] = v_info.id;
    }

    current_vertex_id = route_graph_->GetVertexCount();

    for (const TransportRouterInfo::EdgeInfo& e_info : info.GetEdgesInfo()) {

        EdgeId edge = route_graph_->AddEdge({ e_info.from, 
                                              e_info.to, 
                                              e_info.weight });

        if (e_info.is_bus_edge) {
            domain::BusPtr bus_ptr = catalogue_->FindBus(e_info.name);

            BusEdgeInfo edge_info {
                bus_ptr->name,
                e_info.span_count,
                e_info.weight
            };

            bus_edge_id_to_edge_info_[edge] = std::move(edge_info);
            continue;
        } 

        domain::StopPtr stop_ptr = catalogue_->FindStop(e_info.name);

        WaitEdgeInfo edge_info {
            stop_ptr->name,
            e_info.weight
        };

        wait_edge_id_to_edge_info_[edge] = std::move(edge_info);
    }
}

std::optional<TransportRouter::VertexId> TransportRouter::GetStopVertexId(
                                             std::string_view stop_name) const {
    if (stop_name_to_wait_vertex_id_.count(stop_name) < 1) return std::nullopt;

    return stop_name_to_wait_vertex_id_.at(stop_name);
}

bool TransportRouter::IsBusEdge(EdgeId edge) const {
    return bus_edge_id_to_edge_info_.count(edge) > 0;
}

bool TransportRouter::IsWaitEdge(EdgeId edge) const {
    return wait_edge_id_to_edge_info_.count(edge) > 0;
}

TransportRouter::BusEdgeInfo TransportRouter::GetBusEdgeInfo(
                                                            EdgeId edge) const {
    return bus_edge_id_to_edge_info_.at(edge);
}

TransportRouter::WaitEdgeInfo TransportRouter::GetWaitEdgeInfo(
                                                            EdgeId edge) const {
    return wait_edge_id_to_edge_info_.at(edge);
}

std::optional<RoutingResult> TransportRouter::BuildRoute(
                                                    std::string_view from, 
                                                    std::string_view to) const {
    
    // If one of the stops doesn't exist, abort and return nothing
    std::optional<VertexId> from_opt = GetStopVertexId(from);
    if (!from_opt.has_value()) return std::nullopt;

    std::optional<VertexId> to_opt   = GetStopVertexId(to);
    if (!to_opt.has_value()) return std::nullopt;

    
    if (from == to) {
        return RoutingResult{ /* "total_time": */ 0.0, /* "items": */ {} };
    }

    // Try building a route
    std::optional<Router::RouteInfo> route_info = router_->BuildRoute(*from_opt, 
                                                                      *to_opt);
    // If building a route failed, return nothing
    if (!route_info.has_value()) return std::nullopt;

    std::vector<RouteItem> route_items;

    RouteItemBus current_bus;

    Weight total_time{};

    // For every edge create two items: wait and bus.
    for (EdgeId edge_id : route_info->edges) {
        if (IsBusEdge(edge_id)) {
            auto edge_info = GetBusEdgeInfo(edge_id); 

            route_items.emplace_back(RouteItemBus{ 
                                        std::string(edge_info.bus_name),
                                        edge_info.span_count,
                                        edge_info.total_time });
            total_time += edge_info.total_time;
        } else if (IsWaitEdge(edge_id)) {
            auto edge_info = GetWaitEdgeInfo(edge_id);

            route_items.emplace_back(RouteItemWait{ 
                                        std::string(edge_info.stop_name),
                                        edge_info.total_time });
            total_time += edge_info.total_time;
        }
    }

    return RoutingResult{ total_time, route_items };
}

const TransportRouter::Graph& TransportRouter::GetRouteGraph() const {
    return *route_graph_;
}

const TransportRouterInfo TransportRouter::ExportRouterInfo() const {
    TransportRouterInfo router_info;

    for (const auto& stop_name : catalogue_->GetStopNames()) {
        TransportRouterInfo::VertexInfo info;

        if (stop_name_to_bus_vertex_id_.count(stop_name) == 0) continue;

        info.is_bus_vertex = true;
        info.stop_name = stop_name;
        info.id = stop_name_to_bus_vertex_id_.at(stop_name);

        router_info.AddVertexInfo(info);

        info.is_bus_vertex = false;
        info.id = stop_name_to_wait_vertex_id_.at(stop_name);

        router_info.AddVertexInfo(info);
    }

    for (EdgeId edge_id = 0; 
                edge_id < route_graph_->GetEdgeCount(); edge_id++) {

        if (IsBusEdge(edge_id)) {
            TransportRouterInfo::EdgeInfo info;
            auto edge = route_graph_->GetEdge(edge_id);
            auto internal_bus_edge_info = GetBusEdgeInfo(edge_id);

            info.from = edge.from;
            info.to = edge.to;
            info.is_bus_edge = true;
            info.span_count = internal_bus_edge_info.span_count;
            info.name = internal_bus_edge_info.bus_name;
            info.weight = internal_bus_edge_info.total_time;

            router_info.AddEdgeInfo(info);
        } else if (IsWaitEdge(edge_id)) {
            TransportRouterInfo::EdgeInfo info;
            auto edge = route_graph_->GetEdge(edge_id);
            auto internal_wait_edge_info = GetWaitEdgeInfo(edge_id);

            info.from = edge.from;
            info.to = edge.to;
            info.is_bus_edge = false;
            info.span_count = 0;
            info.name = internal_wait_edge_info.stop_name;
            info.weight = internal_wait_edge_info.total_time;

            router_info.AddEdgeInfo(info);
        } else {
            std::cerr << "(EXCEPTION) edge id = " << edge_id 
                      << " is invalid!" << std::endl;
            throw std::invalid_argument("invalid edge_id");
        }
    }

    router_info.SetRoutingSettings(std::move(settings_));

    return router_info;
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
        /* bus_velocity:  */  40
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
                  << route_result2->total_time << std::endl;

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
        /* bus_velocity:  */  30
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
        /* bus_velocity:  */  30
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
