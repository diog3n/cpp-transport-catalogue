#include <filesystem>
#include <iostream>

#include "stat_reader.hpp"
#include "input_reader.hpp" 
#include "transport_catalogue.hpp"

using namespace std;

void RunTests() {
    {
        using namespace input_reader::tests;

        TestParseBusQuery();
        cerr << "TestParseBusQuery OK!"s << endl;

        TestParseStopQuery();
        cerr << "TestParseStopQuery OK!"s << endl;
        
        TestAddQuery();
        cerr << "TestAddQuery OK!"s << endl;
    }

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
        using namespace stat_reader::tests;

        TestBusStatReader();
        cerr << "TestStatReader OK!"s << std::endl;
        
        TestStopStatReader();
        cerr << "TestStopStatReader OK!"s << std::endl;
    }

    cerr << "All tests OK!"s << std::endl;
}

int main() {
    RunTests();

    input_reader::InputReader ir;
    stat_reader::StatReader sr(ir.GetCatalogue());

    ir.ReadInput(cin);
    sr.ReadInput(cin);
    sr.DisplayOutput(cout);

    return 0;
}