#include <algorithm>

#include "request_handler.hpp"
#include "json_reader.hpp"
#include "map_renderer.hpp"
#include "transport_catalogue.hpp"

namespace request_handler {

RequestHandler::RequestHandler(
    const transport_catalogue::TransportCatalogue& db, 
    renderer::MapRenderer& renderer)
        : catalogue_(db), renderer_(renderer) {} 

// Here RequestHandler orchestrates the interchange between classes
svg::Document RequestHandler::RenderMap() const {
    return renderer_.RenderMap(catalogue_);
}

namespace tests {

void TestRender() {
}

} // namespace request_handler::tests

} // namespace request_handler