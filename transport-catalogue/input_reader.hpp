#pragma once

#include <iostream>
#include <unordered_map>
#include <vector>

#include "geo.hpp"
#include "stat_reader.hpp"
#include "transport_catalogue.hpp"

namespace input_reader {

struct BusInputQuery {
    std::string_view bus_name;
    std::vector<std::string_view> stop_names;
};

struct StopInputQuery {
    std::string_view stop_name;
    Coordinates coordinates;
    std::unordered_map<std::string_view, int> distances;
};

struct BusOutputQuery {
    std::string_view bus_name;
};

struct StopOutputQuery {
    std::string_view stop_name;
};

class InputReader {

public:
    InputReader() = default;

    explicit InputReader(transport_catalogue::TransportCatalogue& tc, stat_reader::StatReader sr): catalogue_(tc), stat_reader_(sr) {};

    std::vector<std::string> GetSeparateLines(std::istream& input); 

    void AddInputQuery(const std::string& raw_line);

    void ExecuteOutputQuery(const std::string& raw_line);

    void ExecuteInputQueries();

    void ReadInput();

    std::vector<BusInputQuery>& GetBusQueries();

    std::vector<StopInputQuery>& GetStopQueries();

private:
    transport_catalogue::TransportCatalogue catalogue_;
    
    stat_reader::StatReader stat_reader_;

    std::deque<std::string> raw_queries_;

    std::vector<BusInputQuery> bus_input_queries_;

    std::vector<StopInputQuery> stop_input_queries_;
    
    std::vector<BusOutputQuery> bus_output_queries_; 

    std::vector<StopOutputQuery> stop_output_queries_;

    void ExecuteStopOutputQuery(const StopOutputQuery& stop_query) const;

    void ExecuteBusOutputQuery(const BusOutputQuery& bus_query) const;

    static BusOutputQuery ParseBusOutputQuery(std::string_view raw_line);

    static BusInputQuery ParseBusInputQuery(std::string_view raw_line);
    
    static StopOutputQuery ParseStopOutputQuery(std::string_view raw_line);

    static StopInputQuery ParseStopInputQuery(std::string_view raw_line);
    
    static std::vector<std::string_view> GetStopSequence(std::string_view line);
    
    static std::vector<std::string_view> TransformToLoopSequence(std::string_view line);
    
    static std::vector<std::string_view> SplitIntoStopNames(std::string_view view, const char delim);
    
    void TestExample();
};

namespace tests {

void TestParseStopInputQuery();
void TestParseBusInputQuery();
void TestAddInputQuery();
void TestGetSeparateLines();

} // namespace input_reader::tests

namespace util {

namespace view {

std::string_view Substr(const std::string_view& view, size_t start_pos, size_t end_pos);

std::string_view Trim(std::string_view view, char to_remove);

std::vector<std::string_view> SplitBy(std::string_view view, char delim);

} // namespace input_reader::util::view

} // namespace input_reader::util

} // namespace input_reader
