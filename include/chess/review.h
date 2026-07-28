#pragma once

#include "chess/board.h"
#include "chess/move.h"
#include "chess/uci_engine.h"

#include <string>
#include <vector>

namespace chess {

    enum class MoveQuality {
        Book,       // Opening theory, not judged
        Best,       // Matched the engine's choice
        Good,       // Loss under 30cp
        Inaccuracy, // 30 to 90cp
        Mistake,    // 90 to 200cp
        Blunder     // Over 200cp
    };

    std::string qualityToString(MoveQuality q);

    struct ReviewedMove {
        int moveNumber = 0;
        Color side = Color::White;
        Move played;
        std::string fen;
        std::string fenAfter;
        std::string playedSan;
        Move best;
        std::string bestSan;

        int evalBefore = 0;     // Centipawns, from the mover's perspective
        int evalAfter = 0;      // After the move played, still from the mover's perspective
        int centipawnLoss = 0;
        bool mateBefore = false;
        int mateInBefore = 0;

        MoveQuality quality = MoveQuality::Good;
        std::string bestLineSan; // The engine's principal variation in SAN
    };

    struct GameReview {
        std::vector<ReviewedMove> moves;

        int whiteAvgLoss = 0;
        int blackAvgLoss = 0;
        int whiteBlunders = 0;
        int blackBlunders = 0;
        int whiteMistakes = 0;
        int blackMistakes = 0;
        int whiteInaccuracies = 0;
        int blackInaccuracies = 0;
    };

    // Config for the review pass
    struct ReviewConfig {
        int depth = 16;
        int bookMoves = 8;      // Full moves treated as opening theory and not judged
        int pvLength = 5;       // Plies of the best line to render
    };

    // Runs every position of the game through the engine and classifies each move
    // The callback receives (currentPly, totalPlies) so callers can show progress
    GameReview reviewGame(UciEngine& engine,
                          const MoveList& moves,
                          const ReviewConfig& config,
                          void (*progressCallback)(int, int) = nullptr);

    // Renders the review as a human-readable report
    std::string formatReview(const GameReview& review, bool verbose);

} // namespace chess