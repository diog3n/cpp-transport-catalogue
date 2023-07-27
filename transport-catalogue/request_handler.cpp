#include "request_handler.hpp"
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
    const renderer::MapRenderer& renderer)
        : db_(db), renderer_(renderer) {} 

domain::BusInfo RequestHandler::GetBusInfo(const std::string_view& bus_name) const {
    return db_.GetBusInfo(bus_name);
}

const std::vector<std::string_view> RequestHandler::GetBusNamesByStop(const std::string_view& stop_name) const {
    return db_.GetStopInfo(stop_name).bus_names;
}

} // namespace request_handler