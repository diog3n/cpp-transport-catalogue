#pragma once

#include <iostream>
#include <unordered_map>
#include <vector>

#include "geo.hpp"
#include "transport_catalogue.hpp"
#include "domain.hpp"
#include "request_handler.hpp"

// TODO: make it use RequestHandler

namespace input_reader {

class InputReader final : private handlers::InputHandler {

public:
    InputReader() = default;

    explicit InputReader(transport_catalogue::TransportCatalogue& tc): catalogue_(tc) {};

    std::vector<std::string> GetSeparateLines(std::istream& input); 

    void ReadInput(std::istream& in);

    void DisplayOutput(std::ostream& out);

    void AddQuery(const std::string& raw_line);

    void ExecuteInputQueries() override;

    std::deque<domain::BusInputQuery>& GetBusQueries();

    std::deque<domain::StopInputQuery>& GetStopQueries();

    const transport_catalogue::TransportCatalogue& GetCatalogue() const;

private:

    transport_catalogue::TransportCatalogue catalogue_;
    
    std::deque<std::string> raw_queries_;

    static domain::BusInputQuery ParseBusInputQuery(std::string_view raw_line);

    static domain::StopInputQuery ParseStopInputQuery(std::string_view raw_line);
    
    static std::vector<std::string_view> GetStopSequence(std::string_view line);
    
    static std::vector<std::string_view> TransformToLoopSequence(std::string_view line);
    
    static std::vector<std::string_view> SplitIntoStopNames(std::string_view view, const char delim);
    
};

namespace tests {

void TestParseStopInputQuery();
void TestParseBusInputQuery();
void TestAddQuery();
void TestGetSeparateLines();

} // namespace input_reader::tests

namespace util {

namespace view {

std::string_view Substr(std::string_view view, size_t start_pos, size_t end_pos);

std::string_view Trim(std::string_view view, char to_remove);

std::vector<std::string_view> SplitBy(std::string_view view, char delim);

} // namespace input_reader::util::view

} // namespace input_reader::util

} // namespace input_reader
