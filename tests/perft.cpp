#include "chess/board.h"
#include "chess/movegen.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct PerftCase {
    std::string name;
    std::string fen;
    std::vector<uint64_t> expected;  // Index 0 is depth 1.
};

// These are the standard test positions from the Chess Programming Wiki.
// They are chosen to exercise the edge cases that break naive move generators:
// en passant pins, castling through attacked squares, promotion with check.
const std::vector<PerftCase> CASES = {
    {
        "Start position",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        {20, 400, 8902, 197281, 4865609}
    },
    {
        "Kiwipete",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        {48, 2039, 97862, 4085603}
    },
    {
        "Position 3 (endgame)",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        {14, 191, 2812, 43238, 674624}
    },
    {
        "Position 4 (promotions)",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        {6, 264, 9467, 422333}
    },
    {
        "Position 5",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        {44, 1486, 62379, 2103487}
    },
    {
        "Position 6",
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
        {46, 2079, 89890, 3894594}
    }
};

} // namespace

int main(int argc, char** argv) {
    // A single depth cap keeps the default run fast. Pass a number to go deeper.
    int maxDepth = argc > 1 ? std::atoi(argv[1]) : 4;

    int passed = 0;
    int failed = 0;

    for (const PerftCase& tc : CASES) {
        std::cout << "\n" << tc.name << "\n";
        std::cout << tc.fen << "\n";

        chess::Board board;
        if (!board.setFromFen(tc.fen)) {
            std::cout << "  FEN PARSE FAILED\n";
            ++failed;
            continue;
        }

        for (size_t d = 0; d < tc.expected.size(); ++d) {
            int depth = static_cast<int>(d) + 1;
            if (depth > maxDepth) {
                break;
            }

            auto start = std::chrono::steady_clock::now();
            uint64_t nodes = chess::perft(board, depth);
            auto end = std::chrono::steady_clock::now();

            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            uint64_t expected = tc.expected[d];
            bool ok = nodes == expected;

            std::cout << "  depth " << depth
                      << "  nodes " << std::setw(10) << nodes
                      << "  expected " << std::setw(10) << expected
                      << "  " << std::setw(6) << ms << "ms  "
                      << (ok ? "PASS" : "FAIL") << "\n";

            if (ok) {
                ++passed;
            } else {
                ++failed;
                // Once a depth fails, deeper ones will too, and the divide
                // output below is what actually tells you which move is wrong.
                std::cout << "\n  Divide at depth " << depth << ":\n";
                chess::perftDivide(board, depth);
                break;
            }
        }
    }

    std::cout << "\n==================================================\n";
    std::cout << "Passed: " << passed << "   Failed: " << failed << "\n";
    std::cout << "==================================================\n";

    return failed == 0 ? 0 : 1;
}