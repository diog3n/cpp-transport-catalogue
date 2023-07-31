#include "request_handler.hpp"
#include "json_reader.hpp"
#include "map_renderer.hpp"
#include "transport_catalogue.hpp"
#include <algorithm>
#include <unordered_map>

namespace handlers {

QueryHandler::QueryHandler(transport_catalogue::TransportCatalogue& tc)
    : catalogue_(tc) {}

// This function was moved here from InputReader class with minor changes.
// Usually this is the way input queries can be executed, so it is left here 
void QueryHandler::ExecuteInputQueries() {
    std::for_each(stop_input_queries_.begin(), stop_input_queries_.end(), [this](const domain::StopInputQuery& stop_query) {
        catalogue_.AddStop(std::string(stop_query.name), stop_query.coordinates);
    });
    std::for_each(stop_input_queries_.begin(), stop_input_queries_.end(), [this](const domain::StopInputQuery& stop_query) {
        for (const auto& [dest_name, distance] : stop_query.distances) {
            catalogue_.AddDistance(stop_query.name, dest_name, distance);
        }
    });
    std::for_each(bus_input_queries_.begin(), bus_input_queries_.end(), [this](const domain::BusInputQuery& bus_query) {
        catalogue_.AddBus(std::string(bus_query.name), bus_query.stop_names, bus_query.is_roundtrip);
    });
}

}

namespace request_handler {

RequestHandler::RequestHandler(
    const transport_catalogue::TransportCatalogue& db, 
    renderer::MapRenderer& renderer)
        : catalogue_(db), renderer_(renderer) {} 

domain::BusInfo RequestHandler::GetBusInfo(const std::string_view bus_name) const {
    return catalogue_.GetBusInfo(bus_name);
}

const std::vector<std::string_view> RequestHandler::GetBusNamesByStop(const std::string_view& stop_name) const {
    return catalogue_.GetStopInfo(stop_name).bus_names;
}

std::vector<std::string_view> RequestHandler::GetBusNames() const {
    return catalogue_.GetBusNames();
}

std::vector<std::string_view> RequestHandler::GetStopNames() const {
    return catalogue_.GetStopNames();
}

svg::Document RequestHandler::RenderMap() const {
    renderer::util::SphereProjector projector = GetSphereProjector();

    std::vector<std::string_view> bus_names = GetBusNames();
    std::vector<std::string_view> stop_names = GetStopNames();

    std::unordered_map<std::string_view, std::vector<svg::Point>> bus_names_to_points;

    // Rendering bus route lines
    std::for_each(bus_names.begin(), bus_names.end(), 
        [this, &projector, &bus_names_to_points](std::string_view bus_name) {
            std::vector<svg::Point> points = GetStopPoints(projector, bus_name);
            renderer_.DrawRoute(bus_name, points);
            bus_names_to_points[bus_name] = std::move(points);
        });

    // Rendering bus route names
    std::for_each(bus_names_to_points.begin(), bus_names_to_points.end(), 
        [this](const std::pair<std::string_view, std::vector<svg::Point>>& bus_name_to_points) {
            const auto& [bus_name, points] = bus_name_to_points;
                
            const domain::Bus& bus = catalogue_.FindBus(bus_name);

            if (bus.route.empty()) return;

            if (bus.is_roundtrip) {
                renderer_.DrawRoundRouteName(bus_name, points.front());
            } else {
                svg::Point midpoint = points.at(points.size() / 2);

                renderer_.DrawRouteName(bus_name, points.front(), midpoint);
            }
        });

    // Rendering stop circles
    std::for_each(stop_names.begin(), stop_names.end(), 
        [this, &projector](const std::string_view& stop_name) {
            const domain::Stop& stop = catalogue_.FindStop(stop_name);

            if (stop.buses.empty()) return;

            svg::Point pos = GetStopPoint(projector, stop_name);

            renderer_.DrawStop(pos);
        });

    // Rendering stop names
    std::for_each(stop_names.begin(), stop_names.end(),
        [this, &projector](const std::string_view& stop_name) {
            const domain::Stop& stop = catalogue_.FindStop(stop_name);

            if (stop.buses.empty()) return;

            svg::Point pos = GetStopPoint(projector, stop_name);

            renderer_.DrawStopName(stop_name, pos);
        });

    return renderer_.GetDoc();
}

// Collects stop points for a given bus
std::vector<svg::Point> RequestHandler::GetStopPoints(const renderer::util::SphereProjector& projector, const std::string_view bus_name) const {
    const domain::Bus& bus = catalogue_.FindBus(bus_name);

    std::vector<svg::Point> points(bus.route.size());

    std::transform(bus.route.begin(), bus.route.end(), points.begin(),
        [this, &projector](const domain::StopPtr stop_ptr) {
            return GetStopPoint(projector, stop_ptr->name);
        });

    return points;
}

// Projects stop geo coordinates onto a plane
svg::Point RequestHandler::GetStopPoint(const renderer::util::SphereProjector& projector, const std::string_view stop_name) const {
    const domain::Stop& stop = catalogue_.FindStop(stop_name);
    return projector(stop.coordinates);
}


// Collects coordinates of all stops and returns an initialized sphere projector
renderer::util::SphereProjector RequestHandler::GetSphereProjector() const {
    std::vector<std::string_view> stop_names = GetStopNames();
    std::vector<geo::Coordinates> coordinates;
    std::for_each(stop_names.begin(), stop_names.end(), 
        [this, &coordinates](std::string_view stop_name) {
            const domain::Stop& stop = catalogue_.FindStop(stop_name);
            if (!stop.buses.empty()) {
                coordinates.push_back(stop.coordinates);
            }
        });

    return renderer::util::SphereProjector(
                           coordinates.begin(), 
                           coordinates.end(), 
                           renderer_.render_settings.width, 
                           renderer_.render_settings.height, 
                           renderer_.render_settings.padding);
}

namespace tests {

void TestRender() {
}

} // namespace request_handler::tests

} // namespace request_handler