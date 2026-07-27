#include "chess/game.h"

#include "chess/pgn.h"

#include <algorithm>
#include <iostream>
#include <sstream>

namespace chess {

    namespace {

        void reviewProgress(int current, int total) {
            std::cout << "\rAnalysing position " << current << " of " << total << "..." << std::flush;
            if (current == total) {
                std::cout << "\n";
            }
        }

        std::string trim(const std::string& s) {
            size_t start = s.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) {
                return "";
            }
            size_t end = s.find_last_not_of(" \t\r\n");
            return s.substr(start, end - start + 1);
        }

    } // namespace

    GameSession::GameSession(const GameConfig& config) : config_(config) {}

    GameSession::~GameSession() {
        engine_.stop();
    }

    bool GameSession::initialise() {
        std::cout << "Starting engine: " << config_.enginePath << "\n";

        if (!engine_.start(config_.enginePath)) {
            std::cerr << "Failed to start engine at '" << config_.enginePath << "'.\n";
            std::cerr << "Make sure Stockfish is installed and on your PATH, or pass "
                         "--engine <path>.\n";
            return false;
        }

        // Skill Level caps the engine's strength for play. The review pass runs at
        // full strength regardless, since it is a separate set of commands
        engine_.setOption("Skill Level", std::to_string(config_.engineSkillLevel));
        engine_.newGame();

        board_.setStartPosition();
        moveHistory_.clear();
        result_ = GameResult::Ongoing;
        quitRequested_ = false;

        std::cout << "Engine ready. Skill level " << config_.engineSkillLevel << ".\n";
        std::cout << "You are playing "
                  << (config_.humanColor == Color::White ? "White" : "Black") << ".\n\n";
        printHelp();

        return true;
    }

    void GameSession::printBoard() const {
        std::cout << "\n" << board_.toAsciiBoard();
        std::cout << (board_.sideToMove() == Color::White ? "White" : "Black") << " to move";
        if (board_.isInCheck(board_.sideToMove())) {
            std::cout << "  (check)";
        }
        std::cout << "\n\n";
    }

    void GameSession::printHelp() const {
        std::cout << "Commands:\n";
        std::cout << "  <move>   Play a move. UCI (e2e4, e7e8q) or SAN (Nf3, O-O, exd5).\n";
        std::cout << "  moves    List all legal moves.\n";
        std::cout << "  board    Redraw the board.\n";
        std::cout << "  fen      Print the current FEN.\n";
        std::cout << "  help     Show this message.\n";
        std::cout << "  quit     Resign and go straight to the review.\n\n";
    }

    void GameSession::printLegalMoves() {
        MoveList legal = generateLegalMoves(board_);
        std::cout << "Legal moves (" << legal.size() << "): ";
        for (size_t i = 0; i < legal.size(); ++i) {
            if (i > 0) {
                std::cout << " ";
            }
            std::cout << moveToSan(board_, legal[i]);
        }
        std::cout << "\n\n";
    }

    Move GameSession::parseSanInput(const std::string& input) {
        MoveList legal = generateLegalMoves(board_);

        // Normalise away decorations the user might type but SAN generation omits
        // or includes inconsistently
        std::string cleaned;
        for (char c : input) {
            if (c != '+' && c != '#' && c != 'x' && c != '-' && c != '=') {
                cleaned += c;
            }
        }

        for (const Move& m : legal) {
            std::string san = moveToSan(board_, m);
            if (san == input) {
                return m;
            }

            std::string cleanedSan;
            for (char c : san) {
                if (c != '+' && c != '#' && c != 'x' && c != '-' && c != '=') {
                    cleanedSan += c;
                }
            }
            if (cleanedSan == cleaned) {
                return m;
            }
        }

        return Move{};
    }

    bool GameSession::readHumanMove(Move& out) {
        for (;;) {
            std::cout << "Your move: ";
            std::string input;
            if (!std::getline(std::cin, input)) {
                quitRequested_ = true;
                return false;
            }

            input = trim(input);
            if (input.empty()) {
                continue;
            }

            std::string lower = input;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            if (lower == "quit" || lower == "exit" || lower == "resign") {
                quitRequested_ = true;
                return false;
            }
            if (lower == "help") {
                printHelp();
                continue;
            }
            if (lower == "board") {
                printBoard();
                continue;
            }
            if (lower == "moves") {
                printLegalMoves();
                continue;
            }
            if (lower == "fen") {
                std::cout << board_.toFen() << "\n\n";
                continue;
            }

            // Try UCI first since it is unambiguous, then fall back to SAN
            Move m = moveFromUci(lower);
            if (!m.isNull() && isLegalMove(board_, m)) {
                out = m;
                return true;
            }

            m = parseSanInput(input);
            if (!m.isNull()) {
                out = m;
                return true;
            }

            std::cout << "Not a legal move. Type 'moves' to see the options.\n";
        }
    }

    void GameSession::doEngineMove() {
        std::cout << "Engine thinking...\n";

        Move m = engine_.getBestMove(board_.toFen(), config_.engineMoveTimeMs);

        if (m.isNull() || !isLegalMove(board_, m)) {
            std::cerr << "Engine returned an unusable move. Aborting game.\n";
            quitRequested_ = true;
            return;
        }

        std::string san = moveToSan(board_, m);
        board_.makeMove(m);
        moveHistory_.push_back(m);

        std::cout << "Engine plays: " << san << "\n";
    }

    void GameSession::play() {
        printBoard();

        for (;;) {
            result_ = getGameResult(board_);
            if (result_ != GameResult::Ongoing) {
                break;
            }

            if (board_.sideToMove() == config_.humanColor) {
                Move m;
                if (!readHumanMove(m)) {
                    break;
                }

                std::string san = moveToSan(board_, m);
                board_.makeMove(m);
                moveHistory_.push_back(m);
                std::cout << "You play: " << san << "\n";
            } else {
                doEngineMove();
                if (quitRequested_) {
                    break;
                }
            }

            printBoard();
        }

        std::cout << "\n";
        if (quitRequested_ && result_ == GameResult::Ongoing) {
            std::cout << "Game abandoned.\n";
        } else {
            std::cout << resultToString(result_) << "\n";
        }

        if (!moveHistory_.empty()) {
            std::string whiteName = config_.humanColor == Color::White ? "Human" : "Stockfish";
            std::string blackName = config_.humanColor == Color::White ? "Stockfish" : "Human";
            std::string pgn = buildPgn(moveHistory_, result_, whiteName, blackName);

            std::cout << "\n" << pgn << "\n";

            if (savePgn(pgn, config_.pgnPath)) {
                std::cout << "PGN saved to " << config_.pgnPath << "\n";
            } else {
                std::cerr << "Could not write to PGN to " << config_.pgnPath << "\n";
            }
        }
    }

    void GameSession::review() {
        if (moveHistory_.empty()) {
            std::cout << "No moves to review.\n";
            return;
        }

        std::cout << "\nRunning review at depth " << config_.reviewDepth << ".\n";

        // The review must run at full strength, so the skill cap used for play is
        // lifted before analysing
        engine_.setOption("Skill Level", "20");
        engine_.newGame();

        ReviewConfig rc;
        rc.depth = config_.reviewDepth;

        GameReview gr = reviewGame(engine_, moveHistory_, rc, reviewProgress);

        std::cout << formatReview(gr, false);

        std::cout << "\nShow every move, including the good ones? (y/N): ";
        std::string answer;
        std::getline(std::cin, answer);
        if (!answer.empty() && (answer[0] == 'y' || answer[0] == 'Y')) {
            std::cout << formatReview(gr, true);
        }
    }

} // namespace chess