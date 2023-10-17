#include <algorithm>
#include <memory>

#include "request_handler.h"
#include "json_reader.h"
#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"

namespace request_handler {

RequestHandler::RequestHandler(
                    const transport_catalogue::TransportCatalogue* db, 
                    const transport_router::TransportRouter* router,
                    renderer::MapRenderer* renderer)
        : catalogue_(db)
        , router_   (router) 
        , renderer_ (renderer) {}

// Here RequestHandler orchestrates the interchange between classes
svg::Document RequestHandler::RenderMap() const {
    if (renderer_ == nullptr || catalogue_ == nullptr) return {};

    return renderer_->RenderMap(*catalogue_);
}

std::optional<transport_router::RoutingResult> RequestHandler::BuildRoute(
                                                        std::string_view from,
                                                        std::string_view to) const {
    if (router_ == nullptr || catalogue_ == nullptr) return std::nullopt;

    return router_->BuildRoute(from, to);
}

namespace tests {

void TestRender() {
}

} // namespace request_handler::tests

} // namespace request_handler