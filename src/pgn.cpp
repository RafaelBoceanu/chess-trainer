#include "chess/pgn.h"

#include <ctime>
#include <fstream>
#include <sstream>

namespace chess {

    namespace {

        char pieceLetterForSan(PieceType pt) {
            switch (pt) {
                case PieceType::Knight: return 'N';
                case PieceType::Bishop: return 'B';
                case PieceType::Rook:   return 'R';
                case PieceType::Queen:  return 'Q';
                case PieceType::King:   return 'K';
                default:                return '\0';
            }
        }

        std::string todayIsoDate() {
            std::time_t t = std::time(nullptr);
            std::tm tm{};
        #ifdef _WIN32
            localtime_s(&tm, &t);
        #else
            localtime_r(&t, &tm);
        #endif
            char buf[16];
            std::strftime(buf, sizeof(buf), "%Y.%m.%d", &tm);
            return std::string(buf);
        }


    } // namespace

    std::string moveToSan(Board& board, const Move& m) {
        Piece moving = board.pieceAt(m.from);
        if (moving.isEmpty()) {
            return moveToUci(m);
        }

        // Castling has it own notation and skips all the disambiguation logic
        if (moving.type == PieceType::King && std::abs(fileOf(m.to) - fileOf(m.from)) == 2) {
            std::string san = fileOf(m.to) == 6 ? "0-0" : "0-0-0";
            board.makeMove(m);
            if (board.isInCheck(board.sideToMove())) {
                MoveList replies = generateLegalMoves(board);
                san += replies.empty() ? "#" : "+";
            }
            board.unmakeMove();
            return san;
        }

        bool isEnPassant = moving.type == PieceType::Pawn && m.to == board.enPassantSquare();
        bool isCapture = !board.pieceAt(m.to).isEmpty() || isEnPassant;

        std::string san;

        if (moving.type == PieceType::Pawn) {
            if (isCapture) {
                san += static_cast<char>('a' + fileOf(m.from));
            }
        } else {
            san += pieceLetterForSan(moving.type);

            // Disambiguation: find other pieces of the same type that could also
            // legally reach this square
            MoveList legal = generateLegalMoves(board);
            bool needFile = false;
            bool needRank = false;
            bool ambiguous = false;

            for (const Move& other : legal) {
                if (other.from == m.from || other.to != m.to) {
                    continue;
                }
                Piece op = board.pieceAt(other.from);
                if (op.type != moving.type || op.color != moving.color) {
                    continue;
                }
                ambiguous = true;
                if (fileOf(other.from) == fileOf(m.from)) {
                    needRank = true;
                }
                if (rankOf(other.from) == rankOf(m.from)) {
                    needFile = true;
                }
            }

            if (ambiguous) {
                // Prefer the file when it alone is enough, which is the standard rule
                if (!needFile && !needRank) {
                    needFile = true;
                }
                if (needFile) {
                    san += static_cast<char>('a' + fileOf(m.from));
                }
                if (needRank) {
                    san += static_cast<char>('1' + rankOf(m.from));
                }
            }
        }

        if (isCapture) {
            san += 'x';
        }

        san += squareToString(m.to);

        if (m.promotion != PieceType::None) {
            san += '=';
            san += static_cast<char>(std::toupper(promotionToChar(m.promotion)));
        }

        board.makeMove(m);
        if (board.isInCheck(board.sideToMove())) {
            MoveList replies = generateLegalMoves(board);
            san += replies.empty() ? "#" : "+";
        }
        board.unmakeMove();

        return san;
    }

    std::string buildPgn(const MoveList& moves,
                         GameResult result,
                         const std::string& whiteName,
                         const std::string& blackName) {
        std::ostringstream oss;

        std::string resultTag = resultToPgnTag(result);

        oss << "[Event \"Casual Game\"]\n";
        oss << "[Site \"Chess Trainer\"]\n";
        oss << "[Date \"" << todayIsoDate() << "\"]\n";
        oss << "[Round \"-\"]\n";
        oss << "[White \"" << whiteName << "\"]\n";
        oss << "[Black \"" << blackName << "\"]\n";
        oss << "[Result \"" << resultTag << "\"]\n";
        oss << "\n";

        Board board;
        std::ostringstream body;
        int moveNumber = 1;
        int lineLength = 0;

        for (size_t i = 0; i < moves.size(); ++i) {
            std::string token;
            if (i % 2 == 0) {
                token = std::to_string(moveNumber) + ". ";
            }
            token += moveToSan(board, moves[i]);
            board.makeMove(moves[i]);

            if (i % 2 == 1) {
                ++moveNumber;
            }

            // PGN convention is to wrap around 80 columns
            if (lineLength + static_cast<int>(token.size()) + 1 > 79) {
                body << "\n";
                lineLength = 0;
            } else if (lineLength > 0) {
                body << " ";
                lineLength += 1;
            }
            body << token;
            lineLength += static_cast<int>(token.size());
        }

        if (lineLength + static_cast<int>(resultTag.size()) + 1 < 79) {
            body << "\n" << resultTag;
        } else {
            body << " " << resultTag;
        }

        oss << body.str() << "\n";
        return oss.str();
    }

    bool savePgn(const std::string& pgn, const std::string& path) {
        std::ofstream out(path);
        if (!out) {
            return false;
        }
        out << pgn;
        return out.good();
    }


}   // namespace chess