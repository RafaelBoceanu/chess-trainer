#pragma once

#include "chess/board.h"
#include "chess/movegen.h"
#include "chess/review.h"
#include "chess/uci_engine.h"

#include <string>

namespace chess {

    struct GameConfig {
        std::string enginePath = "stockfish";
        Color humanColor = Color::White;
        int engineSkillLevel = 5;     // Stockfish "Skill Level", 0 to 20
        int engineMoveTimeMs = 500;
        int reviewDepth = 16;
        std::string pgnPath = "game.pgn";
    };

    // Drives a console game between a human and the engine, then runs the review
    class GameSession {
        public:
            explicit GameSession(const GameConfig& config);
            ~GameSession();

            bool initialise();

            // Runs the play loop until the game ends or the user quits
            void play();

            // Runs the post-game review over the moves played
            void review();

            const MoveList& moveHistory() const { return moveHistory_; }
            GameResult result() const { return result_; }
        
        private:
            void printBoard() const;
            void printHelp() const;
            void printLegalMoves();

            // Reads a move from stdin. Accepts UCI ("e2e4") and simple SAN ("Nf3")
            // Returns false if the user asked to quit
            bool readHumanMove(Move& out);

            // Resolves a SAN token to a legal move by matching against generated moves
            Move parseSanInput(const std::string& input);

            void doEngineMove();

            GameConfig config_;
            Board board_;
            UciEngine engine_;
            MoveList moveHistory_;
            GameResult result_ = GameResult::Ongoing;
            bool quitRequested_ = false;
    };

} // namespace chess