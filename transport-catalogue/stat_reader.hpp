#pragma once

#include <ostream>

#include "transport_catalogue.hpp"

namespace stat_reader {

using namespace transport_catalogue;

struct BusQuery {
    std::string_view bus_name;
};

struct StopQuery {
    std::string_view stop_name;
};

class StatReader {
public:

    explicit StatReader(const TransportCatalogue& tc): catalogue_(tc) {}

    void ReadInput(std::istream& in);

    void DisplayOutput(std::ostream& out);

    static void PrintBusInfo(std::ostream& out, const BusInfo& bus_info, bool newline = true);

    static void PrintStopInfo(std::ostream& out, const StopInfo& stop_info, bool newline = true);

private:

    std::deque<std::string> raw_queries_;

    std::vector<BusQuery> bus_output_queries_; 

    std::vector<StopQuery> stop_output_queries_;

    const TransportCatalogue& catalogue_;
    
    void ExecuteQuery(std::ostream& out, const std::string& raw_line);

    void ExecuteStopQuery(std::ostream& out, const StopQuery& stop_query) const;

    void ExecuteBusQuery(std::ostream& out, const BusQuery& bus_query) const;

    static BusQuery ParseBusQuery(std::string_view raw_line);

    static StopQuery ParseStopQuery(std::string_view raw_line);

    
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