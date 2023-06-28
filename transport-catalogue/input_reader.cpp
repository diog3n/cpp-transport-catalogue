#include <cstring>
#include <iterator>
#include <sstream>
#include <cassert>
#include <stdexcept>
#include <string_view>
#include <algorithm>
#include <unordered_map>
#include <vector>

#include "geo.hpp"
#include "input_reader.hpp"
#include "transport_catalogue.hpp"

namespace input_reader {

const int COORDS_PRECISION = 6;

using namespace std::literals;

namespace util {

namespace view {

// Returns a substring [start_pos, end_pos) of a given string_view 
std::string_view Substr(std::string_view view, size_t start_pos, size_t end_pos) {
    assert(end_pos > start_pos);
    size_t length = end_pos - start_pos;
    return view.substr(start_pos, length);
}

// Moves the start of given view to the first non-to_remove character
// and moves the end of given view to the last non-to_remove character
std::string_view Trim(std::string_view view, char to_remove = ' ') {
    while (!view.empty() && view.front() == to_remove) view.remove_prefix(1);
    while (!view.empty() && view.back() == to_remove) view.remove_suffix(1);
    return view;
}

// Splits a string_view into a vector of string_views, delimited by a delim char
std::vector<std::string_view> SplitBy(std::string_view view, const char delim = ' ') {
    std::vector<std::string_view> result;

    size_t next_delim = view.find_first_of(delim);

    while (next_delim != std::string_view::npos) {
        std::string_view token = Substr(view, 0, next_delim);
        token = Trim(token, ' ');
        result.emplace_back(token);
        view = Substr(view, next_delim + 1, view.size());
        next_delim = view.find_first_of(delim);
    }

    std::string_view token = Substr(view, 0, view.size());
    result.emplace_back(Trim(token, ' '));

    return result;
}

} // namespace input_reader::util::view


} // namespace input_reader::util

// Splits input stream into a vector of input lines
std::vector<std::string> InputReader::GetSeparateLines(std::istream& input) {
    std::string line;
    std::vector<std::string> lines;
    while (getline(input, line)) lines.push_back(line);
    return lines;
}

// Converts a non-loop route into a vector of stop_names
std::vector<std::string_view> InputReader::TransformToLoopSequence(std::string_view line) {
    std::vector<std::string_view> stop_names = util::view::SplitBy(line, '-');
    for (int i = stop_names.size() - 2; i >= 0; i--) {
        stop_names.push_back(stop_names[i]);
    }
    return stop_names;
}

// Extracts a vector of stop_names from a given string.
// String MUST contain stop names deided by delimiters only.
// EXAMPLE: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Zapadnoye
// OR: Biryulyovo Zapadnoye - Biryusinka - Universam
std::vector<std::string_view> InputReader::GetStopSequence(std::string_view line) {
    bool is_loop = line.find_first_of('>') != std::string_view::npos;
    if (!is_loop) return TransformToLoopSequence(line);
    return util::view::SplitBy(line, '>');
}

// Adds queries to the internal containers of an input reader
// Though queries could be given in any order, stop queries MUST
// be executed first
void InputReader::AddQuery(const std::string& raw_line) {
    raw_queries_.push_back(raw_line);
    std::string_view line_view = raw_queries_.back();
    const std::string_view type = util::view::Substr(line_view, 0, line_view.find_first_of(' '));
    std::string_view rest = util::view::Substr(line_view, line_view.find_first_of(' ') + 1, line_view.size());

    if (type == "Stop"sv) {
        stop_input_queries_.push_back(ParseStopQuery(rest));
    } else if (type == "Bus"sv) {
        bus_input_queries_.push_back(ParseBusQuery(rest));
    } else {
        throw std::invalid_argument("Invalid query type: "s + std::string(type));
    }
}

// Asks user for input
void InputReader::ReadInput(std::istream& in) {
    int input_queries_count;

    in >> input_queries_count; // Prevents input from
    in >> std::ws;             // falling into getline
    for (int i = 0; i < input_queries_count; i++) {
        std::string query;
        getline(in, query);
        AddQuery(query);
    } 
    ExecuteQueries();   
}

std::vector<BusQuery>& InputReader::GetBusQueries() {
    return bus_input_queries_;
}

std::vector<StopQuery>& InputReader::GetStopQueries() {
    return stop_input_queries_;
}

// Returns a constant reference to the internal transport catalogue. The main use case of
// this function would be an initialization of stat_readers in order to use the same database
const transport_catalogue::TransportCatalogue& InputReader::GetCatalogue() const {
    return catalogue_;
}

// Executes input queries in a specific order: stop queries are executed
// first, then the bus queries get executed after to avoid conflicts
void InputReader::ExecuteQueries() {
    std::for_each(stop_input_queries_.begin(), stop_input_queries_.end(), [this](const StopQuery& stop_query) {
        catalogue_.AddStop(std::string(stop_query.stop_name), stop_query.coordinates);
    });
    std::for_each(stop_input_queries_.begin(), stop_input_queries_.end(), [this](const StopQuery& stop_query) {
        for (const auto& [dest_name, distance] : stop_query.distances) {
            catalogue_.AddDistance(stop_query.stop_name, dest_name, distance);
        }
    });
    std::for_each(bus_input_queries_.begin(), bus_input_queries_.end(), [this](const BusQuery bus_query) {
        catalogue_.AddBus(std::string(bus_query.bus_name), bus_query.stop_names);
    });
}



// Parses a bus query. Input MUST contain route name and route,
// delimited by a ':' character. Surrounding spaces are ignored.
// EXAMPLE: 750: Tolstopaltsevo - Marushkino - Rasskazovka
BusQuery InputReader::ParseBusQuery(std::string_view raw_line) {
    using namespace util;

    std::string_view bus_name(view::Substr(raw_line, raw_line.find_first_not_of(' '), raw_line.find_first_of(':')));
    std::string_view stop_names_view(view::Substr(raw_line, raw_line.find_first_of(':') + 1, raw_line.size()));
    std::vector<std::string_view> stop_names = GetStopSequence(stop_names_view);

    return { bus_name, stop_names };
}

// Parses stop query. Input MUST contain stop name and coordinates
// delimited by ':' character. Coordinates MUST be floating-point numbers
// and MUST be delimited by ',' character, distances MUST follow the format "Xm to Y",
// where X is a positive integer, Y is a name of a STOP that already exists in a database. 
// Leading and trailing spaces are ignored.
// EXAMPLE: Rasskazovka: 55.632761, 37.333324, 3200m to Tchepultsevo, 104m to Universam
StopQuery InputReader::ParseStopQuery(std::string_view raw_line) {
    using namespace util;

    raw_line = view::Trim(raw_line, ' ');

    std::string_view stop_name(view::Substr(raw_line, 0, raw_line.find_first_of(':')));
    raw_line = view::Substr(raw_line, raw_line.find_first_of(':') + 1, raw_line.size());

    std::vector<std::string_view> tokens = view::SplitBy(raw_line, ',');

    double lat = std::stod(std::string(tokens[0]));

    double lng = std::stod(std::string(tokens[1]));

    if (tokens.size() == 2) return { stop_name, { lat, lng }, {} };

    std::unordered_map<std::string_view, int> distances;

    for (size_t i = 2; i < tokens.size(); i++) {
        int distance = std::stoi(std::string(view::Substr(tokens[i], 0, tokens[i].find_first_of(' ') - 1)));
        tokens[i] = view::Substr(tokens[i], tokens[i].find_first_of(' ') + 1, tokens[i].size());
        
        // Do this twice to remove "to" from a token
        tokens[i] = view::Substr(tokens[i], tokens[i].find_first_of(' ') + 1, tokens[i].size());
        
        // Extract a stop name, which is the rest of it 
        std::string_view stop_name = view::Trim(tokens[i]);
        distances[stop_name] = distance;
    }

    return { stop_name, { lat, lng }, std::move(distances) };
}

namespace tests {

void TestAddQuery() {
    std::istringstream input(
        "Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka\n"s
        "Stop Tolstopaltsevo: 55.611087, 37.208290\n"s
        "Stop Biryusinka Miryusinka: 55.581065, 37.648390\n"s
        "Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye\n"s
        "Bus 47: Universam Gavansky - Metro Primorskaya - Nalichnaya\n"s
    );

     transport_catalogue::TransportCatalogue tc;

    InputReader ir(tc);

    auto input_lines = ir.GetSeparateLines(input);

    for (const std::string& line : input_lines) {
        ir.AddQuery(line);
    }

    auto& bus_queries = ir.GetBusQueries();
    auto& stop_queries = ir.GetStopQueries(); 

    bool test1 = bus_queries.size() == 3;
    assert(test1);
    bool test2 = stop_queries.size() == 2;
    assert(test2);

    bool test3 = bus_queries.at(0).bus_name == "750"sv 
                 && bus_queries.at(0).stop_names == std::vector{ "Tolstopaltsevo"sv, "Marushkino"sv, "Rasskazovka"sv, "Marushkino"sv, "Tolstopaltsevo"sv };
    assert(test3);

    bool test4 = bus_queries.at(1).bus_name == "256"sv 
                 && bus_queries.at(1).stop_names == std::vector{ "Biryulyovo Zapadnoye"sv, "Biryusinka"sv, "Universam"sv, "Biryulyovo Tovarnaya"sv, "Biryulyovo Passazhirskaya"sv, "Biryulyovo Zapadnoye"sv };
    assert(test4);

    bool test5 = bus_queries.at(2).bus_name == "47"sv 
                 && bus_queries.at(2).stop_names == std::vector{ "Universam Gavansky"sv, "Metro Primorskaya"sv, "Nalichnaya"sv, "Metro Primorskaya"sv, "Universam Gavansky"sv };
    assert(test5);

    bool test6 = stop_queries.at(0).stop_name == "Tolstopaltsevo"sv 
                 && stop_queries.at(0).coordinates == Coordinates{ 55.611087, 37.208290 };
    assert(test6);

    bool test7 = stop_queries.at(1).stop_name == "Biryusinka Miryusinka"sv
                 && stop_queries.at(1).coordinates == Coordinates{ 55.581065, 37.648390 };
    assert(test7);
}

void TestGetSeparateLines() {
    std::istringstream input(
        "  line 1    \n"s
        " line 2\n"s
        "line 3   \n"s
        "line 4\n"s
    );

    transport_catalogue::TransportCatalogue tc;

    InputReader ir(tc);

    auto v1 = ir.GetSeparateLines(input);
    assert(v1[0] == "  line 1    "s);
    assert(v1[1] == " line 2"s);
    assert(v1[2] == "line 3   "s);
    assert(v1[3] == "line 4"s);
}

void TestParseStopQuery() {
    std::string input1 = "Stop Marushkino: 55.595884, 37.209755"s;
    std::string input2 = "Stop Tolstopaltsevo: 55.611087, 37.208290, 3200m to Marushkino, 120m to Universam Gavansky"s;
    std::string input3 = "Stop Biryusinka Miryusinka: 55.581065, 37.648390, 13m to Universam Gavansky"s;

    transport_catalogue::TransportCatalogue tc;

    InputReader ir(tc);

    ir.AddQuery(input1);
    ir.AddQuery(input2);
    ir.AddQuery(input3);

    auto stop_queries = ir.GetStopQueries();

    bool test_size = stop_queries.size() == 3;

    assert(test_size);

    bool test1 = stop_queries[0].stop_name == "Marushkino"s 
                 && stop_queries[0].coordinates == Coordinates{55.595884, 37.209755}
                 && stop_queries[0].distances == std::unordered_map<std::string_view, int>();
    assert(test1);

    bool test2 = stop_queries[1].stop_name == "Tolstopaltsevo"s
                 && stop_queries[1].coordinates == Coordinates{55.611087, 37.208290}
                 && stop_queries[1].distances == std::unordered_map<std::string_view, int>{ { "Marushkino"sv, 3200 }, { "Universam Gavansky"sv, 120 } };
    assert(test2);

    bool test3 = stop_queries[2].stop_name == "Biryusinka Miryusinka"s
                 && stop_queries[2].coordinates == Coordinates{55.581065, 37.648390}
                 && stop_queries[2].distances == std::unordered_map<std::string_view, int>{ { "Universam Gavansky"sv, 13 } };
    assert(test3);

}

void TestParseBusQuery() {
    std::string input1 = "Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye"s;
    std::string input2 = "Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka"s;

    transport_catalogue::TransportCatalogue tc;

    InputReader ir(tc);

    ir.AddQuery(input1);
    ir.AddQuery(input2);

    auto bus_queries = ir.GetBusQueries();

    bool test_size = bus_queries.size() == 2;
    assert(test_size);

    bool test1 = bus_queries[0].bus_name == "256"s 
                 && bus_queries[0].stop_names == std::vector{
                     "Biryulyovo Zapadnoye"sv, 
                     "Biryusinka"sv,
                     "Universam"sv,
                     "Biryulyovo Tovarnaya"sv,
                     "Biryulyovo Passazhirskaya"sv,
                     "Biryulyovo Zapadnoye"sv
                 };
    assert(test1);

    bool test2 = bus_queries[1].bus_name == "750"s 
                 && bus_queries[1].stop_names == std::vector{
                    "Tolstopaltsevo"sv,
                    "Marushkino"sv,
                    "Rasskazovka"sv,
                    "Marushkino"sv,
                    "Tolstopaltsevo"sv    
                 };
    assert(test2);
}


} // namespace input_reader::tests

} // namespace input_reader

