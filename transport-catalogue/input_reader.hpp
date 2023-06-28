#pragma once

#include <iostream>
#include <unordered_map>
#include <vector>

#include "geo.hpp"
#include "stat_reader.hpp"
#include "transport_catalogue.hpp"

namespace input_reader {

struct BusQuery {
    std::string_view bus_name;
    std::vector<std::string_view> stop_names;
};

struct StopQuery {
    std::string_view stop_name;
    Coordinates coordinates;
    std::unordered_map<std::string_view, int> distances;
};

class InputReader {

public:
    InputReader() = default;

    explicit InputReader(transport_catalogue::TransportCatalogue& tc): catalogue_(tc) {};

    std::vector<std::string> GetSeparateLines(std::istream& input); 

    void ReadInput(std::istream& in);

    void DisplayOutput(std::ostream& out);

    void AddQuery(const std::string& raw_line);

    void ExecuteQueries();

    std::vector<BusQuery>& GetBusQueries();

    std::vector<StopQuery>& GetStopQueries();

    const transport_catalogue::TransportCatalogue& GetCatalogue() const;

private:

    transport_catalogue::TransportCatalogue catalogue_;
    
    std::deque<std::string> raw_queries_;

    std::vector<BusQuery> bus_input_queries_;

    std::vector<StopQuery> stop_input_queries_;
    
    static BusQuery ParseBusQuery(std::string_view raw_line);

    static StopQuery ParseStopQuery(std::string_view raw_line);
    
    static std::vector<std::string_view> GetStopSequence(std::string_view line);
    
    static std::vector<std::string_view> TransformToLoopSequence(std::string_view line);
    
    static std::vector<std::string_view> SplitIntoStopNames(std::string_view view, const char delim);
    
    void TestExample();
};

namespace tests {

void TestParseStopQuery();
void TestParseBusQuery();
void TestAddQuery();
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
