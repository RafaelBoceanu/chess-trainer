#include "http_server.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

    void printUsage() {
        std::cout << "Usage: chess-server [options]\n\n";
        std::cout << "Options:\n";
        std::cout << "  --engine <path>   Path to the Stockfish binary (default: stockfish)\n";
        std::cout << "  --port <n>        Port to listen on (default: 8080)\n";
        std::cout << "  --depth <n>       Review search depth (default: 16)\n";
        std::cout << "  --help            Show this message\n";
    }

} // namespace

int main(int argc, char** argv) {
    chess::server::ServerConfig config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        } else if (arg == "--engine" && i + 1 < argc) {
            config.enginePath = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            config.port = std::atoi(argv[++i]);
        } else if (arg == "--depth" && i + 1 < argc) {
            config.reviewDepth = std::atoi(argv[++i]);
        } else {
            std::cerr << "Unknown option: " << arg << "\n\n";
            printUsage();
            return 1;
        }
    }

    chess::server::HttpServer server(config);
    server.run();
    return 0;
}