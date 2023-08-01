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

};

// Is a base abstract class for any class that handles user output
class OutputHandler {
public:

    virtual void ExecuteOutputQueries(std::ostream& out) const = 0;

    std::deque<domain::StopOutputQuery> stop_output_queries_;

    std::deque<domain::BusOutputQuery> bus_output_queries_;

};

// Is a base class for any class that handles both input and output 
class QueryHandler : public InputHandler, public OutputHandler {
public:

    QueryHandler() = delete;

    QueryHandler(transport_catalogue::TransportCatalogue& tc);

    virtual void ExecuteInputQueries();

    virtual void ExecuteOutputQueries(std::ostream& out) const = 0;

    virtual ~QueryHandler() = default;

protected:

    transport_catalogue::TransportCatalogue& catalogue_;

};

} // namespace handlers

namespace request_handler {

class RequestHandler {
public:
    // Uses map_renderer
    RequestHandler(const transport_catalogue::TransportCatalogue& db, 
                   renderer::MapRenderer& renderer);

    // Returns a document with a drawn map in it
    svg::Document RenderMap() const;

private:
    const transport_catalogue::TransportCatalogue& catalogue_;
    renderer::MapRenderer& renderer_;

    // Wrapper method. Returns names of all buses in catalogue
    std::vector<std::string_view> GetBusNames() const;

    // Wrapper method. Returns names of all stops in catalogue
    std::vector<std::string_view> GetStopNames() const;

    // Factory that sets up a sphere projector
    renderer::util::SphereProjector GetSphereProjector() const;

    // Returns a vector of coordinates for stops in a bus route
    std::vector<svg::Point> GetStopPoints(const renderer::util::SphereProjector& projector, const std::string_view bus_name) const;

    // Returns a svg::Point-projected coordinate of a given stop
    svg::Point GetStopPoint(const renderer::util::SphereProjector& projector, const std::string_view stop_name) const; 

};

namespace tests {

void TestRender();

} // namespace request_handler::tests

} // namespace request_handler
