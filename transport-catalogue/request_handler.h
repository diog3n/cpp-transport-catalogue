#pragma once
#include <algorithm>
#include <deque>
#include <memory>
#include <optional>

#include "transport_catalogue.h"
#include "transport_router.h"
#include "map_renderer.h"

namespace handlers {

// Is a base abstract class for any class that handles user input
class InputHandler {
public:

    virtual void ExecuteInputQueries() = 0;

    std::deque<domain::BusInputQuery> bus_input_queries_;

    std::deque<domain::StopInputQuery> stop_input_queries_;

    virtual ~InputHandler() = default;
};

// Is a base abstract class for any class that handles user output
class OutputHandler {
public:

    virtual void ExecuteOutputQueries(std::ostream& out) const = 0;

    std::deque<domain::StopOutputQuery> stop_output_queries_;

    std::deque<domain::BusOutputQuery> bus_output_queries_;

    virtual ~OutputHandler() = default;

};

} // namespace handlers

namespace request_handler {

class RequestHandler {
public:

    using MapRenderer = renderer::MapRenderer;
    using RoutingResult = transport_router::RoutingResult;
    using TransportRouter = transport_router::TransportRouter;
    using TransportCatalogue = transport_catalogue::TransportCatalogue;

    // Uses map_renderer
    RequestHandler(const transport_catalogue::TransportCatalogue* db, 
                   const transport_router::TransportRouter* router,
                   renderer::MapRenderer* renderer);

    // Returns a document with a map
    svg::Document RenderMap() const;

    // Returns a routing result
    std::optional<RoutingResult> BuildRoute(std::string_view from,
                                            std::string_view to) const;
    
private:
    
    std::shared_ptr<const TransportCatalogue> catalogue_;
    
    std::shared_ptr<const TransportRouter> router_;

    std::shared_ptr<MapRenderer> renderer_;
};

namespace tests {

void TestRender();

} // namespace request_handler::tests

} // namespace request_handler
