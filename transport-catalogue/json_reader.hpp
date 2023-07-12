#pragma once
#include <memory>

#include "json.hpp"
#include "domain.hpp"
#include "transport_catalogue.hpp"
#include "request_handler.hpp"

namespace json_reader {

class JSONReader {
public:
    using RequestHandlerPtr = std::shared_ptr<request_handler::RequestHandler>;

    JSONReader(std::unique_ptr<transport_catalogue::TransportCatalogue>&& tc_ptr);

    JSONReader(std::unique_ptr<request_handler::RequestHandler>&& request_handler_ptr);

    JSONReader(const transport_catalogue::TransportCatalogue& tc);

    JSONReader(const request_handler::RequestHandler& request_handler);


    void LoadJSON(const std::string& document);

    void LoadJSON(std::istream& in);

private:

    void ParseQueries();

    RequestHandlerPtr request_handler_ptr_;

    json::Document json_;

    domain::StopOutputQuery AssembleStopOutputQuery(const json::Node& query_node) const;

    domain::BusOutputQuery AssembleBusOutputQuery(const json::Node& query_node) const;

    domain::BusInputQuery AssembleBusInputQuery(const json::Node& query_node) const;

    domain::StopInputQuery AssembleStopInputQuery(const json::Node& query_node) const;

};

namespace tests {

void TestAssembleQuery();

} // namespace json_reader::tests

} // namespace json_reader