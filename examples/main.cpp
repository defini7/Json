#define JSON_IMPL
#include "../include/json.hpp"

int main(int argc, char* argv[])
{
    Json::Node test = Json::ParseFile("../test.json");

    test.Dump();

    auto& a = test["list"].Array();

    for (auto& val : a)
        std::cout << val.Number() << std::endl;
}