#include <cstdlib>

#include "loguru.hpp"
#include "cxxopts.hpp"
#include "options.hpp"

#include "pinger.hpp"

int main(int argc, char** argv) {
    cxxopts::Options options("myping", "custom ping tool for learning purposes");
    cxxopts::ParseResult result;

    // clang-format off
    options.add_options()
        ("d,dev", "Egress interface", cxxopts::value<std::string>())
        ("c,count", "Number of packets to send", cxxopts::value<int>()->default_value("4"))
        ("i,interval", "Interval between packets", cxxopts::value<int>()->default_value("1"))
        ("s,size", "Size of each packet", cxxopts::value<int>()->default_value("56"))
        ("t,target", "Target IP address", cxxopts::value<std::string>())
        ("v,verbose", "Verbose output")
        ("h,help", "Print usage")
    ;
    // clang-format on

    if (init_options(argc, argv, options, result) != 0) {
        LOG_F(ERROR, "Failed to initialize options");
        return EXIT_FAILURE;
    }

    loguru::init(argc, argv);

    LOG_F(INFO, "Egress interface: %s", result["dev"].as<std::string>().c_str());
    LOG_F(INFO, "Number of packets to send: %d", result["count"].as<int>());
    LOG_F(INFO, "Interval between packets: %d", result["interval"].as<int>());
    LOG_F(INFO, "Size of each packet: %d", result["size"].as<int>());
    LOG_F(INFO, "Target IP address: %s", result["target"].as<std::string>().c_str());

    Pinger pinger(result["dev"].as<std::string>(), result["size"].as<int>());
    pinger.init();
    pinger.send_ping(result["target"].as<std::string>());
    pinger.receive_ping();
    pinger.close();

    return EXIT_SUCCESS;
}