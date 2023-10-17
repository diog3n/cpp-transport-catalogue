#include "transport_catalogue.hpp"
#include "transport_router.hpp"
#include "request_handler.hpp"
#include "serialization.hpp"
#include "map_renderer.hpp"
#include "json_reader.hpp"

#include <filesystem>
#include <iostream>

using namespace std;

void RunTests() {

    {
        using namespace transport_catalogue::tests;

        TestAddFindMethods();
        cerr << "TestAddFindMethods OK!"s << endl;

        TestGetBusInfo();
        cerr << "TestGetBusInfo OK!"s << endl;

        TestGetStopInfo();
        cerr << "TestGetStopInfo OK!"s << endl;

        TestDistances();
        cerr << "TestDistances OK!"s << endl;        
    }

    {
        /*using namespace json_reader::tests;

        TestAssembleQuery();
        cerr << "TestAssembleQuery OK!"s << endl;

        TestJSON();
        cerr << "TestJSON OK!"s << endl;*/
    }

    {
        /*using namespace transport_router::tests;

        TestBasicRouting();
        cerr << "TestBasicRouting OK!"s << endl;

        TestComplexRouting();
        cerr << "TestComplexRouting OK!"s << endl;

        TestTrickyRouting();
        cerr << "TestTrickyRouting OK!"s << endl;*/
    }
    
    {
        using namespace serialization::transport_catalogue::tests;

        TestTransportCatalogueSerialization();
        cerr << "TestTransportCatalogueSerialization OK!"s << endl;
    }

    cerr << "All tests OK!"s << std::endl;
}

using namespace std::literals;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
    RunTests();

    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);

    json_reader::JSONReader reader;
    if (mode == "make_base"sv) {
        reader.LoadMakeBaseJSON(json_reader::JSONReader::ReadJSON(std::cin));
    } else if (mode == "process_requests"sv) {
        reader.LoadRequestsJSON(json_reader::JSONReader::ReadJSON(std::cin));
        reader.ExecuteOutputQueries(std::cout);
    } else {
        PrintUsage();
        return 1;
    }
}