#pragma once

#include <ostream>

#include "transport_catalogue.hpp"
#include "domain.hpp"

// TODO: make it use RequestHandler

namespace stat_reader {

using namespace transport_catalogue;

class StatReader {
public:

    explicit StatReader(const TransportCatalogue& tc): catalogue_(tc) {}

    void ReadInput(std::istream& in);

    void DisplayOutput(std::ostream& out);

    static void PrintBusInfo(std::ostream& out, const BusInfo& bus_info, bool newline = true);

    static void PrintStopInfo(std::ostream& out, const StopInfo& stop_info, bool newline = true);

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