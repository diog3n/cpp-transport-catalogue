#pragma once

#include <ostream>

#include "transport_catalogue.hpp"
#include "domain.hpp"

namespace stat_reader {

using namespace transport_catalogue;

class StatReader {
public:

    explicit StatReader(const TransportCatalogue& tc): catalogue_(tc) {}

    void ReadInput(std::istream& in);

    void DisplayOutput(std::ostream& out);

    void PrintBusInfo(std::ostream& out, const std::string_view bus_name, bool newline = true) const;

    void PrintStopInfo(std::ostream& out, const std::string_view stop_name, bool newline = true) const;

private:

    std::deque<std::string> raw_queries_;

    std::vector<domain::BusOutputQuery> bus_output_queries_; 

    std::vector<domain::StopOutputQuery> stop_output_queries_;

    const TransportCatalogue& catalogue_;
    
    void ExecuteQuery(std::ostream& out, const std::string& raw_line);

    void ExecuteStopOutputQuery(std::ostream& out, const domain::StopOutputQuery& stop_query) const;

    void ExecuteBusOutputQuery(std::ostream& out, const domain::BusOutputQuery& bus_query) const;

    domain::BusOutputQuery ParseBusOutputQuery(std::string_view raw_line);

    domain::StopOutputQuery ParseStopOutputQuery(std::string_view raw_line);

    int current_request_id_;
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