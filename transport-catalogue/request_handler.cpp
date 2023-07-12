#include "request_handler.hpp"
#include "transport_catalogue.hpp"

namespace query_handler {

QueryHandler::QueryHandler()
    : catalogue_ptr_(std::make_shared<transport_catalogue::TransportCatalogue>()) {}

QueryHandler::QueryHandler(const transport_catalogue::TransportCatalogue& tc)
    : catalogue_ptr_(std::make_shared<transport_catalogue::TransportCatalogue>(tc)) {}

QueryHandler::QueryHandler(std::unique_ptr<transport_catalogue::TransportCatalogue>&& tc_ptr)
    : catalogue_ptr_(std::move(tc_ptr)) {}

void QueryHandler::ExecuteInputQueries() {
    std::for_each(stop_input_queries_.begin(), stop_input_queries_.end(), [this](const domain::StopInputQuery& stop_query) {
        catalogue_ptr_->AddStop(std::string(stop_query.stop_name), stop_query.coordinates);
    });
    std::for_each(stop_input_queries_.begin(), stop_input_queries_.end(), [this](const domain::StopInputQuery& stop_query) {
        for (const auto& [dest_name, distance] : stop_query.distances) {
            catalogue_ptr_->AddDistance(stop_query.stop_name, dest_name, distance);
        }
    });
    std::for_each(bus_input_queries_.begin(), bus_input_queries_.end(), [this](const domain::BusInputQuery& bus_query) {
        catalogue_ptr_->AddBus(std::string(bus_query.bus_name), bus_query.stop_names);
    });
}
}

namespace request_handler {

} // namespace request_handler