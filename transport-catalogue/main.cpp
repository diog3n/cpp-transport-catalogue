#include <filesystem>
#include <iostream>

#include "transport_catalogue.hpp"
#include "json_reader.hpp"

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
        using namespace json_reader::tests;

        TestJSON();
        cerr << "TestJSON OK!"s << endl;
    }

    cerr << "All tests OK!"s << std::endl;
}

int main() {
    RunTests();

    transport_catalogue::TransportCatalogue tc;

    json_reader::JSONReader reader(tc);

    reader.LoadJSON(json_reader::JSONReader::ReadJSON(std::cin));

    reader.PrintTo(std::cout);

    return 0;
}