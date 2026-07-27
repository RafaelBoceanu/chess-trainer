#include "chess/game.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

    void printUsage() {
        std::cout << "Usage: chess-trainer [options]\n\n";
        std::cout << "Options:\n";
        std::cout << "  --engine <path>   Path to the Stockfish binary (default: stockfish)\n";
        std::cout << "  --black           Play as Black (default: White)\n";
        std::cout << "  --skill <0-20>    Engine skill level (default: 5)\n";
        std::cout << "  --movetime <ms>   Engine thinking time per move (default: 500)\n";
        std::cout << "  --depth <n>       Review search depth (default: 16)\n";
        std::cout << "  --pgn <path>      Where to save the game (default: game.pgn)\n";
        std::cout << "  --help            Show this message\n";
    }

    int clampInt(int value, int lo, int hi) {
        if (value < lo) return lo;
        if (value > hi) return hi;
        return value;
    }

} // namespace

int main(int argc, char** argv) {
    chess::GameConfig config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        } else if (arg == "--black") {
            config.humanColor = chess::Color::Black;
        } else if (arg == "--engine" && i + 1 < argc) {
            config.enginePath = argv[++i];
        } else if (arg == "--skill" && i + 1 < argc) {
            config.engineSkillLevel = clampInt(std::atoi(argv[++i]), 0, 20);
        } else if (arg == "--movetime" && i + 1 < argc) {
            config.engineMoveTimeMs = clampInt(std::atoi(argv[++i]), 50, 60000);
        } else if (arg == "--depth" && i + 1 < argc) {
            config.reviewDepth = clampInt(std::atoi(argv[++i]), 1, 30);
        } else if (arg == "--pgn" && i + 1 < argc) {
            config.pgnPath = argv[++i];
        } else {
            std::cerr << "Unknown option: " << arg << "\n\n";
            printUsage();
            return 1;
        }
    }

    std::cout << "==================================================\n";
    std::cout << "                 CHESS TRAINER\n";
    std::cout << "==================================================\n\n";

    chess::GameSession session(config);

    if (!session.initialise()) {
        return 1;
    }

    session.play();
    session.review();

    std::cout << "\nThanks for playing.\n";
    return 0;
}