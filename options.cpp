#include <cstdlib>
#include <iostream>

#include "cxxopts.hpp"
#include "loguru.hpp"

int init_options(const int argc, char** argv, cxxopts::Options& options, cxxopts::ParseResult& result) {
    result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        exit(EXIT_SUCCESS);
    }

    if (!result.count("dev")) {
        LOG_F(ERROR, "Egress interface is required");
        LOG_F(ERROR, "Use -h or --help for usage");
        exit(EXIT_FAILURE);
    }

    if (result.arguments().empty()) {
        std::cout << options.help() << std::endl;
        exit(EXIT_SUCCESS);
    }

    return 0;
}