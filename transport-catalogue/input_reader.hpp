#pragma once
#include <iostream>

#include "geo.hpp"
#include "transport_catalogue.hpp"
#include "domain.hpp"
#include "request_handler.hpp"

namespace input_reader {

class InputReader final : private handlers::InputHandler {

public:
    InputReader() = default;

    explicit InputReader(transport_catalogue::TransportCatalogue& tc): catalogue_(tc) {};

    // Splits input stream into a vector of input lines
    std::vector<std::string> GetSeparateLines(std::istream& input); 

    // Handles the user input
    void ReadInput(std::istream& in);

    void DisplayOutput(std::ostream& out);

    // Adds queries to the internal containers of an input reader
    // Though queries could be given in any order, stop queries MUST
    // be executed first
    void AddQuery(const std::string& raw_line);
    
    // Executes input queries in a specific order: stop queries are executed
    // first, then the bus queries get executed after to avoid conflicts
    void ExecuteInputQueries() override;

    std::deque<domain::BusInputQuery>& GetBusQueries();

    std::deque<domain::StopInputQuery>& GetStopQueries();

private:

    transport_catalogue::TransportCatalogue catalogue_;
    
    std::deque<std::string> raw_queries_;

    // Parses a bus query. Input MUST contain route name and route,
    // delimited by a ':' character. Surrounding spaces are ignored.
    // EXAMPLE: 750: Tolstopaltsevo - Marushkino - Rasskazovka
    static domain::BusInputQuery ParseBusInputQuery(std::string_view raw_line);

    // Parses stop query. Input MUST contain stop name and coordinates
    // delimited by ':' character. Coordinates MUST be floating-point numbers
    // and MUST be delimited by ',' character, distances MUST follow the format "Xm to Y",
    // where X is a positive integer, Y is a name of a STOP that already exists in a database. 
    // Leading and trailing spaces are ignored.
    // EXAMPLE: Rasskazovka: 55.632761, 37.333324, 3200m to Tchepultsevo, 104m to Universam
    static domain::StopInputQuery ParseStopInputQuery(std::string_view raw_line);
    
    // Extracts a vector of stop_names from a given string.
    // String MUST contain stop names deided by delimiters only.
    // EXAMPLE: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Zapadnoye
    // OR: Biryulyovo Zapadnoye - Biryusinka - Universam
    static std::vector<std::string_view> GetStopSequence(std::string_view line);
    
    // Converts a non-loop route into a vector of stop_names
    static std::vector<std::string_view> TransformToLoopSequence(std::string_view line);
    
    static std::vector<std::string_view> SplitIntoStopNames(std::string_view view, const char delim);
    
};

namespace tests {

void TestParseStopInputQuery();
void TestParseBusInputQuery();
void TestAddQuery();
void TestGetSeparateLines();

} // namespace input_reader::tests

} // namespace input_reader
