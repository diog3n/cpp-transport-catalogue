#pragma once

#include "transport_catalogue.hpp"

namespace stat_reader {

using namespace transport_catalogue;

class StatReader {
public:

    static void PrintBusInfo(std::ostream& out, const BusInfo& bus_info, bool newline = true);

    static void PrintStopInfo(std::ostream& out, const StopInfo& stop_info, bool newline = true);

};

namespace tests {

void TestBusStatReader();
void TestStopStatReader();

} // namespace stat_reader::tests

namespace util {

bool DoubleEq(double lhs, double rhs);

} // namespace stat_reader::util

} // namespace stat_reader