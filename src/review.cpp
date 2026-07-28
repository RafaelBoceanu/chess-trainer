#include "chess/review.h"

#include "chess/movegen.h"
#include "chess/pgn.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace chess {

    namespace {

        // A mate score is clamped to a large centipawn value so the loss arithmetic
        // stays meaningful without overflowing into nonsense
        constexpr int MATE_SCORE = 10000;

        int lineToCp(const EngineLine& line) {
            if (!line.isMate) {
                return line.scoreCp;
            }
            if (line.mateIn > 0) {
                return MATE_SCORE - line.mateIn;
            }
            return -MATE_SCORE - line.mateIn;
        }

        MoveQuality classify(int loss, bool matchedBest) {
            if (matchedBest) {
                return MoveQuality::Best;
            }
            if (loss < 30)  return MoveQuality::Good;
            if (loss < 90)  return MoveQuality::Inaccuracy;
            if (loss < 200) return MoveQuality::Mistake;
            return MoveQuality::Blunder;
        }

        // Renders a principal variation in SAN by replaying it on a scratch board
        std::string pvToSan(const std::string& fen, const std::vector<Move>& pv, int maxPlies) {
            Board board;
            if (!board.setFromFen(fen)) {
                return "";
            }

            std::ostringstream oss;
            int plies = std::min(static_cast<int>(pv.size()), maxPlies);
            int moveNum = board.fullmoveNumber();
            bool whiteToMove = board.sideToMove() == Color::White;

            for (int i = 0; i < plies; ++i) {
                // The PV comes from the engine and should be legal, but truncated or
                // malformed line should not take the whole review down
                if (!isLegalMove(board, pv[i])) {
                    break;
                }

                if (i > 0) {
                    oss << " ";
                }
                if (whiteToMove) {
                    oss << moveNum << ". ";
                } else if (i == 0) {
                    oss << moveNum << "... ";
                }

                oss << moveToSan(board, pv[i]);
                board.makeMove(pv[i]);

                if (!whiteToMove) {
                    ++moveNum;
                }
                whiteToMove = !whiteToMove;
            }

            return oss.str();
        }

        std::string formatEval(int cp, bool isMate, int mateIn) {
            std::ostringstream oss;
            if (isMate) {
                oss << "M" << std::abs(mateIn);
                return (mateIn > 0 ? "+" : "-") + oss.str();
            }
            double pawns = cp / 100.0;
            oss << std::showpos << std::fixed << std::setprecision(2) << pawns;
            return oss.str();
        }

    } // namespace

    std::string qualityToString(MoveQuality q) {
        switch (q) {
            case MoveQuality::Book:       return "book";
            case MoveQuality::Best:       return "best";
            case MoveQuality::Good:       return "good";
            case MoveQuality::Inaccuracy: return "inaccuracy";
            case MoveQuality::Mistake:    return "mistake";
            case MoveQuality::Blunder:    return "BLUNDER";
        }
        return "";
    }

    GameReview reviewGame(UciEngine& engine,
                          const MoveList& moves,
                          const ReviewConfig& config,
                          void (*progressCallback)(int, int)) {
        GameReview review;

        Board board;
        int totalPlies = static_cast<int>(moves.size());

        // Evaluate the starting position once. From then on, the eval of the
        // position after move N is reused as the eval before move N+1, which
        // halves the number of engine calls
        EngineLine current = engine.analyse(board.toFen(), config.depth);

        for (int i = 0; i < totalPlies; ++i) {
            if (progressCallback) {
                progressCallback(i + 1, totalPlies);
            }

            const Move& played = moves[i];
            Color mover = board.sideToMove();
            std::string fenBefore = board.toFen();

            ReviewedMove rm;
            rm.moveNumber = board.fullmoveNumber();
            rm.side = mover;
            rm.played = played;
            rm.fen = fenBefore;
            rm.playedSan = moveToSan(board, played);
            rm.best = current.bestMove;
            rm.bestSan = current.bestMove.isNull() ? "" : moveToSan(board, current.bestMove);
            rm.evalBefore = lineToCp(current);
            rm.mateBefore = current.isMate;
            rm.mateInBefore = current.mateIn;
            rm.bestLineSan = pvToSan(fenBefore, current.pv, config.pvLength);

            board.makeMove(played);
            rm.fenAfter = board.toFen();

            GameResult resultAfterMove = getGameResult(board);
            EngineLine after;

            if (resultAfterMove == GameResult::WhiteWins || resultAfterMove == GameResult::BlackWins) {
                rm.evalAfter = MATE_SCORE;
            } else if (resultAfterMove != GameResult::Ongoing) {
                rm.evalAfter = 0;
            } else {
                after = engine.analyse(board.toFen(), config.depth);
                rm.evalAfter = -lineToCp(after);
            }

            int loss = rm.evalBefore - rm.evalAfter;
            rm.centipawnLoss = std::max(0, loss);

            bool matchedBest = !current.bestMove.isNull() && current.bestMove == played;

            if (rm.moveNumber <= config.bookMoves) {
                rm.quality = MoveQuality::Book;
                rm.centipawnLoss = 0;
            } else {
                rm.quality = classify(rm.centipawnLoss, matchedBest);
            }

            review.moves.push_back(rm);
            current = after;
        }
        
        // Aggregate. Book moves are excluded from the averages so a solid opening
        // does not flatter the numbers
        int whiteCount = 0, blackCount = 0;
        int whiteTotal = 0, blackTotal = 0;

        for (const ReviewedMove& rm : review.moves) {
            if (rm.quality == MoveQuality::Book) {
                continue;
            }

            if (rm.side == Color::White) {
                whiteTotal += rm.centipawnLoss;
                ++whiteCount;
                if (rm.quality == MoveQuality::Blunder)     ++review.whiteBlunders;
                if (rm.quality == MoveQuality::Mistake)     ++review.whiteMistakes;
                if (rm.quality == MoveQuality::Inaccuracy)  ++review.whiteInaccuracies;
            } else {
                blackTotal += rm.centipawnLoss;
                ++blackCount;
                if (rm.quality == MoveQuality::Blunder)     ++review.blackBlunders;
                if (rm.quality == MoveQuality::Mistake)     ++review.blackMistakes;
                if (rm.quality == MoveQuality::Inaccuracy)  ++review.blackInaccuracies;
            }
        }

        review.whiteAvgLoss = whiteCount > 0 ? whiteTotal / whiteCount : 0;
        review.blackAvgLoss = blackCount > 0 ? blackTotal / blackCount : 0;

        return review;
    }

    std::string formatReview(const GameReview& review, bool verbose) {
        std::ostringstream oss;

        oss << "\n";
        oss << "==================================================\n";
        oss << "                 GAME REVIEW\n";
        oss << "==================================================\n\n";

        for (const ReviewedMove& rm : review.moves) {
            // In non-verbose mode only the moves worth talking about are shown
            if (!verbose && rm.quality != MoveQuality::Inaccuracy &&
                rm.quality != MoveQuality::Mistake &&
                rm.quality != MoveQuality::Blunder) {
                continue;
            }

            oss << rm.moveNumber << (rm.side == Color::White ? ". " : "... ");
            oss << rm.playedSan;
            oss << "  [" << qualityToString(rm.quality) << "]";

            if (rm.quality != MoveQuality::Book) {
                oss << " eval" << formatEval(rm.evalBefore, rm.mateBefore, rm.mateInBefore)
                    << " -> " << formatEval(rm.evalAfter, false, 0);
                if (rm.centipawnLoss > 0) {
                    oss << " (loss " << rm.centipawnLoss << "cp)";
                }
            }
            oss << "\n";

            if (rm.quality == MoveQuality::Inaccuracy ||
                rm.quality == MoveQuality::Mistake ||
                rm.quality == MoveQuality::Blunder) {
                oss << "    Better: " << rm.bestSan << "\n";
                if (!rm.bestLineSan.empty()) {
                    oss << "    Line:   " << rm.bestLineSan << "\n";
                }
            }
            oss << "\n";
        }

        oss << "--------------------------------------------------\n";
        oss << "SUMMARY\n";
        oss << "--------------------------------------------------\n";
        oss << "                    White      Black\n";
        oss << "Avg loss (cp)      " << std::setw(6) << review.whiteAvgLoss
            << "     " << std::setw(6) << review.blackAvgLoss << "\n";
        oss << "Inaccuracies       " << std::setw(6) << review.whiteInaccuracies
            << "     " << std::setw(6) << review.blackInaccuracies << "\n";
        oss << "Mistakes           " << std::setw(6) << review.whiteMistakes
            << "     " << std::setw(6) << review.blackMistakes << "\n";
        oss << "Blunders           " << std::setw(6) << review.whiteBlunders
            << "     " << std::setw(6) << review.blackBlunders << "\n";
        oss << "--------------------------------------------------\n";

        return oss.str();
    }

} // namespace chess