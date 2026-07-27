#pragma once

#include "chess/move.h"

#include <memory>
#include <string>
#include <vector>

namespace chess {

    // A single line of analysis returned by the engine
    struct EngineLine {
        int scoreCp = 0;        // Centipawns, from the side to move's perspective
        int mateIn = 0;         // Non-zero if forced mate. Positive = side to move mates
        bool isMate = false;
        Move bestMove;
        std::vector<Move> pv;   // Principal variation
    };

    // Wraps a UCI engine subprocess. Handles the pipe plumbing and
    // the request/response protocol
    class UciEngine {
    public:
        UciEngine();
        ~UciEngine();

        UciEngine(const UciEngine&) = delete;
        UciEngine& operator=(const UciEngine&) = delete;

        // Launch the engine binary and complete the uci/isready handshake
        bool start(const std::string&  enginePath);

        void stop();

        bool isRunning() const;

        // Set a UCI option
        void setOption(const std::string& name, const std::string& value);

        // Reset the engine's internal state between games
        void newGame();

        // Analyse a FEN position to the given depth
        EngineLine analyse(const std::string& fen, int depth);

        // Return the engine's chosen move for a position, searching for the given
        // time in milliseconds. Used for play mode
        Move getBestMove(const std::string& fen, int movetimeMs);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;

        void send(const std::string& command);
        std::string readLine();
        bool waitForReady();
    };

} // namespace chess