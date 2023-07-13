#pragma once
#include <memory>

#include "json.hpp"
#include "domain.hpp"
#include "transport_catalogue.hpp"
#include "request_handler.hpp"

// TODO: try using an array of pointers to OutputQuery base struct to use 
//       to store both Bus and Stop output queries and speed the process of
//       executing them up.

namespace json_reader {

namespace util {

json::Node AssembleBusNode(domain::BusInfo& bus_info, int id);

json::Node AssembleStopNode(domain::StopInfo& stop_info, int id);

} // namespace json_reader::utils

class JSONReader final : private handlers::QueryHandler {
public:

    JSONReader(const transport_catalogue::TransportCatalogue& tc);

    JSONReader(std::unique_ptr<transport_catalogue::TransportCatalogue>&& tc_ptr);

    void LoadJSON(const std::string& document);

    void LoadJSON(std::istream& in);

    void PrintTo(std::ostream& out) const;

private:

    void ExecuteOutputQueries(handlers::OutputContext& context) const override; 

    void ParseQueries();

    json::Document json_;

    std::deque<const domain::OutputQuery*> query_ptrs_;

    domain::StopOutputQuery AssembleStopOutputQuery(const json::Node& query_node) const;

    domain::BusOutputQuery AssembleBusOutputQuery(const json::Node& query_node) const;

    domain::BusInputQuery AssembleBusInputQuery(const json::Node& query_node) const;

    domain::StopInputQuery AssembleStopInputQuery(const json::Node& query_node) const;

};

namespace tests {

void TestAssembleQuery();

} // namespace json_reader::tests

} // namespace json_reader