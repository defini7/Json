#define JSON_IMPL
#include "../include/json.hpp"

int main(int argc, char* argv[])
{
    auto wrappedTest = Json::ParseFile("../test.json");
    
    if (!wrappedTest)
        return 1;

    Json::Node& test = wrappedTest.value();

    test.Dump();

    auto& a = test["list"].Array();

    for (auto& val : a)
        std::cout << val.Number() << std::endl;
}