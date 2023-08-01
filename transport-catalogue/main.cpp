#include <filesystem>
#include <iostream>

#include "map_renderer.hpp"
#include "request_handler.hpp"
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

        TestAssembleQuery();
        cerr << "TestAssembleQuery OK!"s << endl;

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

    //renderer::MapRenderer renderer(reader.GetRenderSettings());

    //request_handler::RequestHandler rh(tc, renderer);

    //svg::Document document = rh.RenderMap();

    //document.Render(cout);

    reader.PrintTo(std::cout);

    return 0;
}