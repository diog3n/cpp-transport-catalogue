#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string_view>

#include "stat_reader.hpp"

namespace stat_reader {

namespace util {

const double MAX_DEVIATION = 1e-6;

bool DoubleEq(double lhs, double rhs) {
    return std::abs(lhs - rhs) < MAX_DEVIATION;
}

} // namespace stat_reader::util

using namespace std::literals;

// Outputs bus info in a specified format
void StatReader::PrintBusInfo(std::ostream& out, const BusInfo& bus_info, bool newline) {
    std::string endline = newline ? "\n" : "";  

    if (bus_info.type == InfoType::NOT_FOUND) {
        out << "Bus "s << bus_info.name << ": not found"s << endline << std::flush;
        return;
    } else if (bus_info.type == InfoType::EMPTY) {
        out << "Bus "s << bus_info.name << ": no stops"s << endline << std::flush;
        return;
    }

    out << "Bus "s << bus_info.name << ": "s
        << bus_info.stops_on_route << " stops on route, "s
        << bus_info.unique_stops << " unique stops, "s
        << bus_info.route_length << " route length, "s 
        << bus_info.curvature << " curvature" << endline << std::flush;
}

// Output stop info in a specified format
void StatReader::PrintStopInfo(std::ostream& out, const StopInfo& stop_info, bool newline) {
    std::string endline = newline ? "\n" : "";  
    
    if (stop_info.type == InfoType::NOT_FOUND) {
        out << "Stop "s << stop_info.name 
            << ": not found"s << endline << std::flush;
        return;
    } else if (stop_info.type == InfoType::EMPTY) {
        out << "Stop "s << stop_info.name 
            << ": no buses"s << endline << std::flush;
        return;
    }

    out << "Stop "s << stop_info.name << ": buses"s;
    for (const std::string_view& bus_name : stop_info.bus_names) {
        out << " "s << bus_name; 
    }
    out << endline << std::flush;
}

// Executes bus output queries
void StatReader::ExecuteStopOutputQuery(std::ostream& out, const StopOutputQuery& stop_query) const {
    PrintStopInfo(out, catalogue_.GetStopInfo(stop_query.stop_name));
}

// Executes bus output queries
void StatReader::ExecuteBusOutputQuery(std::ostream& out, const BusOutputQuery& bus_query) const {
    PrintBusInfo(out, catalogue_.GetBusInfo(bus_query.bus_name));
}

// Parses bus output query. At this point query without a type is
// just a bus name. So it just trims it from leading and
// trailing spaces. EXAMPLE: 257
BusOutputQuery StatReader::ParseBusOutputQuery(std::string_view raw_line) {
    std::string_view bus_name = transport_catalogue::util::view::Trim(raw_line, ' ');
    return { current_request_id_++, bus_name };
}

// Parses output stop query. At this point, queries only contain
// query type and stop name, so this function only trimms the name 
// of trailing and leading spaces. INPUT EXAMPLE: Marushkino
StopOutputQuery StatReader::ParseStopOutputQuery(std::string_view raw_line) {
    return { current_request_id_++, transport_catalogue::util::view::Trim(raw_line, ' ') }; 
}

// Execites output queries. Unlike input queries, these are not supposed
// to be executed in a particular order. Instead, they are executed in the
// same order as they were stated
void StatReader::ExecuteQuery(std::ostream& out, const std::string& raw_line) {
    std::string_view line_view = raw_line;
    std::string_view type = transport_catalogue::util::view::Substr(line_view, 0, line_view.find_first_of(' '));
    std::string_view rest = transport_catalogue::util::view::Substr(line_view, line_view.find_first_of(' ') + 1, line_view.size());

    if (type == "Stop"sv) {
        ExecuteStopOutputQuery(out, ParseStopOutputQuery(rest));
    } else if (type == "Bus"sv) {
        ExecuteBusOutputQuery(out, ParseBusOutputQuery(rest));
    } else {
        throw std::invalid_argument("Invalid query type: "s + std::string(type));
    }
}

// Asks user for input
void StatReader::ReadInput(std::istream& in) {
    int output_queries_count;

    in >> output_queries_count; // Prevents input from
    in >> std::ws;              // falling into getline

    for (int i = 0; i < output_queries_count; i++) {
        std::string query;
        getline(in, query);
        raw_queries_.push_back(query);
    }
}

// Executes queries
void StatReader::DisplayOutput(std::ostream& out) {
    std::for_each(raw_queries_.begin(), raw_queries_.end(), [this, &out](const std::string& query) {
        ExecuteQuery(out, query);
    });
}


namespace tests {

void TestBusStatReader() {
    using std::cout, std::endl, std::cerr;

    std::ostringstream out;

    struct TestBusInfo { std::string_view name; std::vector<std::string_view> stop_names; };
    struct TestStopInfo { std::string_view name; geo::Coordinates coordinates; std::unordered_map<std::string_view, int> distances; };    
    
    TransportCatalogue tc;

    std::vector<TestStopInfo> stop_info;

    stop_info.push_back({
        "Tolstopaltsevo"sv,
        { 55.611087, 37.20829 },
        { 
            { "Marushkino"sv, 3900 }
        }
    });

    stop_info.push_back({
        "Marushkino"sv,
        { 55.595884, 37.209755 },
        {
            { "Rasskazovka"sv, 9900 },
            { "Marushkino"sv, 100 }
        }
    });

    stop_info.push_back({
        "Rasskazovka"sv,
        { 55.632761, 37.333324 },
        {
            { "Marushkino"sv, 9500 }
        }
    });

    stop_info.push_back({ // 1800m to Biryusinka, 2400m to Universam
        "Biryulyovo Zapadnoye"sv,
        { 55.574371, 37.6517 },
        {
            { "Biryusinka"sv, 1800 },
            { "Universam"sv, 2400 },
        }
    });

    stop_info.push_back({
        "Biryusinka"sv,
        { 55.581065, 37.64839 },
        {
            { "Universam"sv, 750 }
        }
    });

    stop_info.push_back({
        "Universam"sv,
        { 55.587655, 37.645687 },
        {
            { "Rossoshanskaya ulitsa"sv, 7500 },
            { "Biryulyovo Tovarnaya"sv, 900 }
        }
    });

    stop_info.push_back({
        "Biryulyovo Tovarnaya"sv,
        { 55.592028, 37.653656 },
        {
            { "Biryulyovo Passazhirskaya"sv, 1300 }
        }
    });

    stop_info.push_back({
        "Biryulyovo Passazhirskaya"sv,
        { 55.580999, 37.659164 },
        {
            { "Biryulyovo Zapadnoye"sv, 1200 }
        }
    });

    stop_info.push_back({
        "Rossoshanskaya ulitsa"sv,
        { 55.595579, 37.605757 },
        {}
    });

    stop_info.push_back({
        "Prazhskaya"sv,
        { 55.611678, 37.603831 },
        {}
    });

    for (const auto& stop : stop_info) tc.AddStop(stop.name, stop.coordinates);

    for (const auto& stop : stop_info) {
        for (const auto& [dest, distance] : stop.distances) {
            tc.AddDistance(stop.name, dest, distance);
        }
    }

    std::vector<TestBusInfo> bus_info; 
    bus_info.push_back({ "256"sv, { "Biryulyovo Zapadnoye"sv, "Biryusinka"sv, "Universam"sv, "Biryulyovo Tovarnaya"sv, "Biryulyovo Passazhirskaya"sv, "Biryulyovo Zapadnoye" } });
    bus_info.push_back({ "750"sv, { "Tolstopaltsevo"sv, "Marushkino"sv, "Marushkino"sv, "Rasskazovka"sv, "Marushkino"sv, "Marushkino"sv, "Tolstopaltsevo"sv } });

    for (const auto& bus : bus_info) tc.AddBus(bus.name, bus.stop_names);

    StatReader sr(tc);
    
    sr.PrintBusInfo(out, tc.GetBusInfo("256"sv));

    cerr << "TestBusStatReader (TEST OUTPUT): "; sr.PrintBusInfo(cerr, tc.GetBusInfo("256"sv));
    bool test1 = out.str() == "Bus 256: 6 stops on route, 5 unique stops, 5950 route length, 1.36124 curvature\n";

    // Bus 256: 6 stops on route, 5 unique stops, 5950 route length 1.36124 curvature

    assert(test1);

    out.str("");
    sr.PrintBusInfo(out, tc.GetBusInfo("750"sv));

    cerr << "TestBusStatReader (TEST OUTPUT): "; sr.PrintBusInfo(cerr, tc.GetBusInfo("750"sv));
    bool test2 = out.str() == "Bus 750: 7 stops on route, 3 unique stops, 27400 route length, 1.30853 curvature\n";
    assert(test2);
}

void TestStopStatReader() {
    using std::cerr;

    struct TestBusInfo { std::string_view name; std::vector<std::string_view> stop_names; };
    struct TestStopInfo { std::string_view name; geo::Coordinates coordinates; };
    
    TransportCatalogue tc;

    TestStopInfo stop1{ "Marushkino"sv, { 55.595884, 37.209755 } };
    TestStopInfo stop2{ "Tolstopaltsevo"sv, { 55.611087, 37.208290 } };
    TestStopInfo stop3{ "Biryusinka Miryusinka"sv, { 55.581065, 37.648390 } };
    TestStopInfo stop4{ "Rasskazovka"sv, {20.000000, 21.000000} };

    TestBusInfo bus1{ "256"sv, { "Marushkino"sv, "Biryusinka Miryusinka"sv, "Marushkino"sv } };
    TestBusInfo bus2{ "47"sv, { "Tolstopaltsevo"sv, "Biryusinka Miryusinka"sv, "Tolstopaltsevo"sv } };
    TestBusInfo bus3{ "11"sv, { "Tolstopaltsevo"sv, "Marushkino"sv, "Tolstopaltsevo"sv } };

    tc.AddStop(stop1.name, stop1.coordinates);
    tc.AddStop(stop2.name, stop2.coordinates);
    tc.AddStop(stop3.name, stop3.coordinates);
    tc.AddStop(stop4.name, stop3.coordinates);

    tc.AddBus(bus1.name, bus1.stop_names); 
    tc.AddBus(bus2.name, bus2.stop_names); 
    tc.AddBus(bus3.name, bus3.stop_names); 

    StopInfo stop_info1 = tc.GetStopInfo("Marushkino"s);
    StopInfo stop_info2 = tc.GetStopInfo("Tolstopaltsevo"s);
    StopInfo empty_info = tc.GetStopInfo("Rasskazovka"s);
    StopInfo invalid_info = tc.GetStopInfo("Samara"s);

    std::ostringstream out;

    StatReader sr(tc);

    cerr << "TestStopStatReader (TEST OUTPUT): "s; sr.PrintStopInfo(cerr, stop_info1);
    sr.PrintStopInfo(out, stop_info1);
    bool test1 = out.str() == "Stop Marushkino: buses 256 11\n"s
                 || out.str() == "Stop Marushkino: buses 11 256\n"s;
    assert(test1);

    cerr << "TestStopStatReader (TEST OUTPUT): "s; sr.PrintStopInfo(cerr, stop_info2);
    out.str("");
    sr.PrintStopInfo(out, stop_info2);
    bool test2 = out.str() == "Stop Tolstopaltsevo: buses 47 11\n"s
                 || out.str() == "Stop Tolstopaltsevo: buses 11 47\n"s;
    assert(test2);

    cerr << "TestStopStatReader (TEST OUTPUT): "s; sr.PrintStopInfo(cerr, empty_info);
    out.str("");
    sr.PrintStopInfo(out, empty_info);
    bool test3 = out.str() == "Stop Rasskazovka: no buses\n";
    assert(test3);
    
    cerr << "TestStopStatReader (TEST OUTPUT): "s; sr.PrintStopInfo(cerr, invalid_info);
    out.str("");
    sr.PrintStopInfo(out, invalid_info);
    bool test4 = out.str() == "Stop Samara: not found\n";
    assert(test4);
}

void TestParseQuery() {
    struct TestBusInfo { std::string_view name; std::vector<std::string_view> stop_names; };
    struct TestStopInfo { std::string_view name; geo::Coordinates coordinates; };
    
    TransportCatalogue tc;

    TestStopInfo stop1{ "Marushkino"sv, { 55.595884, 37.209755 } };
    TestStopInfo stop2{ "Tolstopaltsevo"sv, { 55.611087, 37.208290 } };
    TestStopInfo stop3{ "Biryusinka Miryusinka"sv, { 55.581065, 37.648390 } };
    TestStopInfo stop4{ "Rasskazovka"sv, {20.000000, 21.000000} };

    tc.AddStop(stop1.name, stop1.coordinates);
    tc.AddStop(stop2.name, stop2.coordinates);
    tc.AddStop(stop3.name, stop3.coordinates);
    tc.AddStop(stop4.name, stop3.coordinates);

    TestBusInfo bus1{ "256"sv, { "Marushkino"sv, "Biryusinka Miryusinka"sv, "Marushkino"sv } };
    TestBusInfo bus2{ "47"sv, { "Tolstopaltsevo"sv, "Biryusinka Miryusinka"sv, "Tolstopaltsevo"sv } };
    TestBusInfo bus3{ "11"sv, { "Tolstopaltsevo"sv, "Marushkino"sv, "Tolstopaltsevo"sv } };

    tc.AddBus(bus1.name, bus1.stop_names); 
    tc.AddBus(bus2.name, bus2.stop_names); 
    tc.AddBus(bus3.name, bus3.stop_names); 
    
    std::string query1 = "Stop Marushkino"s;
    std::string query2 = "Stop Tolstopaltsevo"s;
    std::string query3 = "Stop Rasskazovka"s;
    std::string query4 = "Stop Samara"s;

    StatReader sr(tc);

    std::istringstream in{
        "4"s
        "Stop Marushkino"s
        "Stop Tolstopaltsevo"s
        "Stop Rasskazovka"s
        "Stop Samara"s
        "Bus 256"s
        "Bus 47"s
        "Bus 13"s
    };

    sr.ReadInput(in);

    std::cout << "TestParseBusOutputQuery (TEST OUTPUT):" << std::endl;

    sr.DisplayOutput(std::cout);
}

} // namespace stat_reader::tests

} // namespace stat_reader