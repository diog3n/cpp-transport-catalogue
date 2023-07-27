#pragma once
#include <algorithm>
#include <deque>
#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

#include "transport_catalogue.hpp"
#include "map_renderer.hpp"

namespace handlers {

struct OutputContext {
    
    explicit OutputContext(std::ostream& out): out(out) {}
    
    virtual ~OutputContext() = default;
    
    std::ostream& out;
};

class InputHandler {
public:

    virtual void ExecuteInputQueries() = 0;

    std::deque<domain::BusInputQuery> bus_input_queries_;

    std::deque<domain::StopInputQuery> stop_input_queries_;

};

class OutputHandler {
public:

    virtual void ExecuteOutputQueries(OutputContext& context) const = 0;

    std::deque<domain::StopOutputQuery> stop_output_queries_;

    std::deque<domain::BusOutputQuery> bus_output_queries_;

};

class QueryHandler : public InputHandler, public OutputHandler {
public:

    QueryHandler();

    QueryHandler(const transport_catalogue::TransportCatalogue& tc);

    QueryHandler(std::unique_ptr<transport_catalogue::TransportCatalogue>&& tc_ptr);

    virtual void ExecuteInputQueries();

    virtual void ExecuteOutputQueries(OutputContext& context) const = 0;

    virtual ~QueryHandler() = default;

protected:

    std::shared_ptr<transport_catalogue::TransportCatalogue> catalogue_ptr_;

};

} // namespace handlers

namespace request_handler {

class RequestHandler {
public:
    // Uses map_renderer
    RequestHandler(const transport_catalogue::TransportCatalogue& db, const renderer::MapRenderer& renderer);

    // Returns bus info (from Bus query)
    domain::BusInfo GetBusInfo(const std::string_view& bus_name) const;

    // Returns buses whose routes use given stop
    const std::vector<std::string_view> GetBusNamesByStop(const std::string_view& stop_name) const;

    // Returns a document with a drawn map in it
    svg::Document RenderMap() const;

private:
    const transport_catalogue::TransportCatalogue& db_;
    const renderer::MapRenderer& renderer_;
};

} // namespace request_handler
