#pragma once

#include <ostream>

#include "transport_catalogue.hpp"
#include "domain.hpp"
#include "request_handler.hpp"

namespace stat_reader {

using namespace transport_catalogue;

class StatReader: handlers::OutputHandler {
public:

    explicit StatReader(const TransportCatalogue& tc): catalogue_(tc) {}

    // Asks user for input
    void ReadInput(std::istream& in);

    // Executes queries
    void ExecuteOutputQueries(std::ostream& out) const override;

    // Outputs bus info in a specified format
    void PrintBusInfo(std::ostream& out, const std::string_view bus_name, bool newline = true) const;

    // Outputs stop info in a specified format
    void PrintStopInfo(std::ostream& out, const std::string_view stop_name, bool newline = true) const;

private:

    std::deque<std::string> raw_queries_;

    std::vector<domain::BusOutputQuery> bus_output_queries_; 

    std::vector<domain::StopOutputQuery> stop_output_queries_;

    const TransportCatalogue& catalogue_;
    

    // Execites output queries. Unlike input queries, these are not supposed
    // to be executed in a particular order. Instead, they are executed in the
    // same order as they were stated
    void ExecuteQuery(std::ostream& out, const std::string& raw_line) const;

    // Executes stop output queries
    void ExecuteStopOutputQuery(std::ostream& out, const domain::StopOutputQuery& stop_query) const;

    // Executes bus output queries
    void ExecuteBusOutputQuery(std::ostream& out, const domain::BusOutputQuery& bus_query) const;

    // Parses bus output query. At this point query without a type is
    // just a bus name. So it just trims it from leading and
    // trailing spaces. EXAMPLE: 257
    domain::BusOutputQuery ParseBusOutputQuery(std::string_view raw_line) const;

    // Parses output stop query. At this point, queries only contain
    // query type and stop name, so this function only trimms the name 
    // of trailing and leading spaces. INPUT EXAMPLE: Marushkino
    domain::StopOutputQuery ParseStopOutputQuery(std::string_view raw_line) const;
};

namespace tests {

void TestBusStatReader();
void TestStopStatReader();
void TestParseQuery();

} // namespace stat_reader::tests

namespace util {

bool DoubleEq(double lhs, double rhs);

} // namespace stat_reader::util

} // namespace stat_reader