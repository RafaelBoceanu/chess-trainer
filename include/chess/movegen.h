#pragma once

#include "chess/board.h"
#include "chess/move.h"

#include <cstdint>

namespace chess {

    enum class GameResult {
        Ongoing,
        WhiteWins,
        BlackWins,
        DrawStalemate,
        DrawFiftyMove,
        DrawRepetition,
        DrawInsufficientMaterial
    };

    std::string resultToString(GameResult r);

    // PGN result tag: "1-0", "0-1", "1/2-1/2" or "*"
    std::string resultToPgnTag(GameResult r);

    // All pseudo-legal moves for the side to move. These may leave the king in check
    MoveList generatePseudoLegalMoves(const Board& board);

    // All fully legal moves for the side to move
    MoveList generateLegalMoves(Board& board);

    // True if the move is legal in the current position
    bool isLegalMove(Board& board, const Move& m);

    // Determines whether the game has ended and how
    GameResult getGameResult(Board& board);

    // Node count for a perft search to the given depth. Correctness test for movegen
    uint64_t perft(Board& board, int depth);

    // Per-move node counts at the root for debugging a failing perft
    void perftDivide(Board& board, int depth);

} // namespace chess
