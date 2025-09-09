#define JSON_IMPL
#include "../include/json.hpp"

int main(int argc, char* argv[])
{
    if (auto cfgOpt = Json::ParseFile("test.json"))
    {
        Json::Node& cfg = *cfgOpt;

        cfg.Dump();

        auto& a = cfg["list"].Array();

        for (auto& val : a)
            std::cout << val.Number() << std::endl;
    }    
}
