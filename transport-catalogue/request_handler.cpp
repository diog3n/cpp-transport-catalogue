#include "request_handler.hpp"

namespace request_handler {

RequestHandler::RequestHandler()
    : catalogue_ptr_(std::make_shared<transport_catalogue::TransportCatalogue>()) {}

RequestHandler::RequestHandler(const transport_catalogue::TransportCatalogue& tc)
    : catalogue_ptr_(std::make_shared<transport_catalogue::TransportCatalogue>(tc)) {}

RequestHandler::RequestHandler(std::unique_ptr<transport_catalogue::TransportCatalogue>&& tc_ptr)
    : catalogue_ptr_(std::move(tc_ptr)) {}

// Executes input queries in a specific order: stop queries are executed
// first, then the bus queries get executed after to avoid conflicts
void RequestHandler::ExecuteInputQueries() {
    std::for_each(stop_input_queries_.begin(), stop_input_queries_.end(), [this](const domain::StopInputQuery& stop_query) {
        catalogue_ptr_->AddStop(std::string(stop_query.stop_name), stop_query.coordinates);
    });
    std::for_each(stop_input_queries_.begin(), stop_input_queries_.end(), [this](const domain::StopInputQuery& stop_query) {
        for (const auto& [dest_name, distance] : stop_query.distances) {
            catalogue_ptr_->AddDistance(stop_query.stop_name, dest_name, distance);
        }
    });
    std::for_each(bus_input_queries_.begin(), bus_input_queries_.end(), [this](const domain::BusInputQuery bus_query) {
        catalogue_ptr_->AddBus(std::string(bus_query.bus_name), bus_query.stop_names);
    });
}

// Execites output queries. Unlike input queries, these are not supposed
// to be executed in a particular order. Instead, they are executed in the
// same order as they were stated
void RequestHandler::ExecuteOutputQueries(std::ostream& out, const std::string& raw_line) {
    
}

} // namespace request_handler