#include "request_handler.hpp"
#include "json_reader.hpp"
#include "map_renderer.hpp"
#include "transport_catalogue.hpp"

namespace handlers {

QueryHandler::QueryHandler()
    : catalogue_ptr_(std::make_shared<transport_catalogue::TransportCatalogue>()) {}

QueryHandler::QueryHandler(const transport_catalogue::TransportCatalogue& tc)
    : catalogue_ptr_(std::make_shared<transport_catalogue::TransportCatalogue>(tc)) {}

QueryHandler::QueryHandler(std::unique_ptr<transport_catalogue::TransportCatalogue>&& tc_ptr)
    : catalogue_ptr_(std::move(tc_ptr)) {}

// This function was moved here from InputReader class with minor changes.
// Usually this is the way input queries can be executed, so it is left here 
void QueryHandler::ExecuteInputQueries() {
    std::for_each(stop_input_queries_.begin(), stop_input_queries_.end(), [this](const domain::StopInputQuery& stop_query) {
        catalogue_ptr_->AddStop(std::string(stop_query.name), stop_query.coordinates);
    });
    std::for_each(stop_input_queries_.begin(), stop_input_queries_.end(), [this](const domain::StopInputQuery& stop_query) {
        for (const auto& [dest_name, distance] : stop_query.distances) {
            catalogue_ptr_->AddDistance(stop_query.name, dest_name, distance);
        }
    });
    std::for_each(bus_input_queries_.begin(), bus_input_queries_.end(), [this](const domain::BusInputQuery& bus_query) {
        catalogue_ptr_->AddBus(std::string(bus_query.name), bus_query.stop_names);
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

    std::for_each(bus_names.begin(), bus_names.end(), 
        [this, &projector](std::string_view bus_name) {
            std::vector<svg::Point> points = GetStopPoints(projector, bus_name);
            renderer_.DrawRoute(points);
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
    std::vector<geo::Coordinates> coordinates(stop_names.size());
    std::transform(stop_names.begin(), stop_names.end(), coordinates.begin(), 
        [this](std::string_view stop_name) {
            return catalogue_.FindStop(stop_name).coordinates;
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