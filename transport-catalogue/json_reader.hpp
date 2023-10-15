#pragma once
#include <memory>

#include "transport_catalogue.hpp"
#include "transport_router.hpp"
#include "request_handler.hpp"
#include "serialization.hpp"
#include "map_renderer.hpp"
#include "domain.hpp"
#include "json.hpp"

namespace json_reader {

namespace util {

// Prints bus info. Is used for debug purposes only
void PrintLnBusInfo(std::ostream& out, domain::BusInfo bus_info);

// Prints bus info. Is used for debug purposes only
void PrintLnStopInfo(std::ostream& out, domain::StopInfo stop_info);

} // namespace json_reader::utils

class JSONReader final : private handlers::InputHandler, 
                         private handlers::OutputHandler {
public:

    JSONReader(transport_catalogue::TransportCatalogue& tc);

    // Loads JSON document from an input stream
    void LoadMakeBaseJSON(std::istream& in);

    void LoadRequestsJSON(std::istream& in);

    void LoadJSON(std::istream& in);

    void LoadMakeBaseJSON(const std::string& in);

    void LoadRequestsJSON(const std::string& in);

    void LoadJSON(const std::string& in);

    // Handles the user input
    static std::string ReadJSON(std::istream& in);

    // Executes output queries in the order they were given in
    void ExecuteOutputQueries(std::ostream& out) const override;

    const json::Document& GetDoc() const;

    const renderer::RenderSettings GetRenderSettings() const; 

private:

    // Executes input queries. Stops first, then buses
    void ExecuteInputQueries() override;

    // Parses the document, collecting queries into respective containers
    void ParseMakeBaseJSON();

    void ParseRequestsJSON();

    // Binary serialization settings
    serializer::SerializationSettings serialization_settings_;

    // Map rendering settings taken from the given JSON
    renderer::RenderSettings render_settings_;

    // Routing settings taken from the given JSON
    transport_router::RoutingSettings routing_settings_;

    // A reference to the transport database
    transport_catalogue::TransportCatalogue& catalogue_;

    // Loaded JSON document
    json::Document json_;

    std::shared_ptr<transport_router::TransportRouter> router_;

    // A container for the map output queries
    std::deque<domain::MapOutputQuery> map_output_queries_;

    // A container for the route output queries
    std::deque<domain::RouteOutputQuery> route_output_queries_;

    /* A container that houses pointers to output queries, allowing 
     * for the easy iteration and execution in a given order */
    std::deque<const domain::OutputQuery*> query_ptrs_;

    /* Set of assembler-methods is used to assemble input/output queries
     * from JSON nodes */
    domain::BusInputQuery AssembleBusInputQuery(
                                            const json::Node& query_node) const;
    domain::MapOutputQuery AssembleMapOutputQuery(
                                            const json::Node& query_node) const;
    domain::BusOutputQuery AssembleBusOutputQuery(
                                            const json::Node& query_node) const;
    domain::StopInputQuery AssembleStopInputQuery(
                                            const json::Node& query_node) const;
    domain::StopOutputQuery AssembleStopOutputQuery(
                                            const json::Node& query_node) const;
    renderer::RenderSettings AssembleRenderSettings(
                                       const json::Node& render_settings) const;
    domain::RouteOutputQuery AssembleRouteOutputQuery(
                                            const json::Node& query_node) const;
    transport_router::RoutingSettings AssembleRoutingSettings(
                                      const json::Node& routing_settings) const;
    serializer::SerializationSettings AssembleSerializationSettings(
                                const json::Node& serialization_settings) const;

    void InitializeRouter();

    /* This set of methods assembles JSON nodes, so that they
     * can be then easily printed out */
    json::Node AssembleMapNode(int id) const;

    json::Node AssembleErrorNode(const int id) const;
    // Extracts color from a JSON Node
    svg::Color ExtractColor(const json::Node& node) const;

    json::Node AssembleBusNode(domain::BusInfoOpt& bus_info_opt, int id) const;

    json::Node AssembleStopNode(
                              domain::StopInfoOpt& stop_info_opt, int id) const;
    json::Node AssembleRouteNode(
                  std::optional<transport_router::RoutingResult> routing_result,
                                                                  int id) const;
};

namespace tests {

void TestJSON();

void TestAssembleQuery();

} // namespace json_reader::tests

} // namespace json_reader