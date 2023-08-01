#pragma once
#include <memory>

#include "json.hpp"
#include "domain.hpp"
#include "transport_catalogue.hpp"
#include "request_handler.hpp"
#include "map_renderer.hpp"

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

    // Loads JSON document from a string
    void LoadJSON(const std::string& document);

    // Loads JSON document from an input stream
    void LoadJSON(std::istream& in);

    // Handles the user input
    static std::string ReadJSON(std::istream& in);

    // Outputs JSON document to the given stream
    void PrintTo(std::ostream& out) const;

    const json::Document& GetDoc() const;

    const renderer::RenderSettings GetRenderSettings() const; 

private:

    // Executes output queries in the order they were given in
    void ExecuteOutputQueries(std::ostream& out) const override; 

    // Executes input queries. Stops first, then buses
    void ExecuteInputQueries() override;

    // Parses the document, collecting queries into respective containers
    void ParseDocument();

    // Map rendering settings taken from the give JSON
    renderer::RenderSettings render_settings_;

    // A reference to the transport database
    transport_catalogue::TransportCatalogue& catalogue_;

    // Loaded JSON document
    json::Document json_;

    // A container for the map output queries
    std::deque<domain::MapOutputQuery> map_output_queries_;

    /* A container that houses pointers to output queries, allowing 
     * for the easy iteration and execution in a given order */
    std::deque<const domain::OutputQuery*> query_ptrs_;

    /* Set of assembler-methods is used to assemble input/output queries
     * from JSON nodes */

    domain::MapOutputQuery AssembleMapOutputQuery(const json::Node& query_node) const;

    domain::StopOutputQuery AssembleStopOutputQuery(const json::Node& query_node) const;

    domain::BusOutputQuery AssembleBusOutputQuery(const json::Node& query_node) const;

    domain::BusInputQuery AssembleBusInputQuery(const json::Node& query_node) const;

    domain::StopInputQuery AssembleStopInputQuery(const json::Node& query_node) const;

    renderer::RenderSettings AssembleRenderSettings(const json::Node& render_settings) const;

    // Extracts color from a JSON Node
    svg::Color ExtractColor(const json::Node& node) const;
    
    /* This set of methods assembles JSON nodes, so that they
     * can be then easily printed out */

    json::Node AssembleErrorNode(const int id) const;

    json::Node AssembleBusNode(domain::BusInfoOpt& bus_info_opt, int id) const;

    json::Node AssembleStopNode(domain::StopInfoOpt& stop_info_opt, int id) const;

    json::Node AssembleMapNode(int id) const;

};

namespace tests {

void TestJSON();

void TestAssembleQuery();

} // namespace json_reader::tests

} // namespace json_reader