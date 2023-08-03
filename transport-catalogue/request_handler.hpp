#pragma once
#include <algorithm>
#include <deque>
#include <memory>
#include <optional>

#include "transport_catalogue.hpp"
#include "map_renderer.hpp"

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
    // Uses map_renderer
    RequestHandler(const transport_catalogue::TransportCatalogue& db, 
                   renderer::MapRenderer& renderer);

    // Returns a document with a map
    svg::Document RenderMap() const;
    
private:
    const transport_catalogue::TransportCatalogue& catalogue_;
    renderer::MapRenderer& renderer_;
};

namespace tests {

void TestRender();

} // namespace request_handler::tests

} // namespace request_handler
