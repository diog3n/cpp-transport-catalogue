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
    renderer::util::CoordinatesTransformer transformer(catalogue_, renderer_);
    std::vector<std::string_view> bus_names = catalogue_.GetBusNames();
    std::vector<std::string_view> stop_names = catalogue_.GetStopNames();

    std::map<std::string_view, std::vector<svg::Point>> bus_names_to_points;

    // Rendering bus route lines
    std::for_each(bus_names.begin(), bus_names.end(), 
        [this, &transformer, &bus_names_to_points](std::string_view bus_name) {
            std::vector<svg::Point> points = transformer.TransformRouteCoords(catalogue_, bus_name);
            renderer_.DrawRoute(bus_name, points);
            bus_names_to_points[bus_name] = std::move(points);
        });

    // Rendering bus route names
    std::for_each(bus_names_to_points.begin(), bus_names_to_points.end(), 
        [this](const std::pair<std::string_view, std::vector<svg::Point>>& bus_name_to_points) {
            const auto& [bus_name, points] = bus_name_to_points;
                
            const domain::Bus& bus = *catalogue_.FindBus(bus_name);

            if (bus.route.empty()) return;

            if (bus.is_roundtrip) {
                renderer_.DrawRoundRouteName(bus_name, points.front());
            } else {
                svg::Point midpoint = points.at(points.size() / 2);

                if (midpoint == points.front()) {
                    renderer_.DrawRoundRouteName(bus_name, points.front());
                } else {
                    renderer_.DrawRouteName(bus_name, points.front(), midpoint);
                }
            }
        });

    // Rendering stop circles
    std::for_each(stop_names.begin(), stop_names.end(), 
        [this, &transformer](const std::string_view& stop_name) {
            const domain::Stop& stop = *catalogue_.FindStop(stop_name);

            if (stop.buses.empty()) return;

            svg::Point pos = transformer.TransformStopCoords(catalogue_, stop_name);

            renderer_.DrawStop(pos);
        });

    // Rendering stop names
    std::for_each(stop_names.begin(), stop_names.end(),
        [this, &transformer](const std::string_view& stop_name) {
            const domain::Stop& stop = *catalogue_.FindStop(stop_name);

            if (stop.buses.empty()) return;

            svg::Point pos = transformer.TransformStopCoords(catalogue_, stop_name);

            renderer_.DrawStopName(stop_name, pos);
        });

    return renderer_.GetDoc();
}

namespace tests {

void TestRender() {
}

} // namespace request_handler::tests

} // namespace request_handler