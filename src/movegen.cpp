#include "chess/movegen.h"

#include <cstdlib>
#include <iostream>

namespace chess {

    namespace {

        const int KNIGHT_OFFSETS[8][2] = {
            {1, 2}, {2, 1}, {2, -1}, {1, -2},
            {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}
        };

        const int KING_OFFSETS[8][2] = {
            {0, 1}, {1, 1}, {1, 0}, {1, -1}, 
            {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}
        };

        const int BISHOP_DIRS[4][2] = {{1, 1}, {1, -1}, {-1, -1}, {-1, 1}};
        const int ROOK_DIRS[4][2] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
        const int QUEEN_DIRS[8][2] = {
            {1, 1}, {1, -1}, {-1, -1}, {-1, 1}, 
            {0, 1}, {1, 0}, {0, -1}, {-1, 0}
        };

        void addPawnMoves(const Board& board, Square from, MoveList& moves) {
            Color us = board.sideToMove();
            int forward = us == Color::White ? 1 : -1;
            int startRank = us == Color::White ? 1 : 6;
            int promoRank = us == Color::White ? 7 : 0;

            int file = fileOf(from);
            int rank = rankOf(from);

            // Single push
            int r1 = rank + forward;
            if (r1 >= 0 && r1 <= 7) {
                Square to = makeSquare(file, r1);
                if (board.pieceAt(to).isEmpty()) {
                    if (r1 == promoRank) {
                        for (PieceType promo : {PieceType::Queen, PieceType::Rook,
                                                PieceType::Bishop, PieceType::Knight}) {
                            moves.push_back(Move{from, to, promo, MoveFlag::Promotion});
                        }
                    } else {
                        moves.push_back(Move{from, to, PieceType::None, MoveFlag::Normal});

                        // Double push, only from the start rank and only if both squares are clear
                        if (rank == startRank) {
                            int r2 = rank + 2 * forward;
                            Square to2 = makeSquare(file, r2);
                            if (board.pieceAt(to2).isEmpty()) {
                                moves.push_back(Move{from, to2, PieceType::None, MoveFlag::DoublePush});
                            }
                        }
                    }
                }
            }

            // Captures, including en passant
            for (int df: {-1, 1}) {
                int f = file + df;
                int r = rank + forward;
                if (f < 0 || f > 7 || r < 0 || r > 7) {
                    continue;
                }
                Square to = makeSquare(f, r);
                Piece target = board.pieceAt(to);

                bool isEnPassant = (to == board.enPassantSquare());
                bool isCapture = !target.isEmpty() && target.color != us;

                if (!isCapture && !isEnPassant) {
                    continue;
                }

                if (r == promoRank) {
                    for (PieceType promo : {PieceType::Queen, PieceType::Rook,
                                            PieceType::Bishop, PieceType::Knight}) {
                        moves.push_back(Move{from, to, promo, MoveFlag::Promotion});
                    }
                } else {
                    MoveFlag flag = isEnPassant ? MoveFlag::EnPassant : MoveFlag::Normal;
                    moves.push_back(Move{from, to, PieceType::None, flag});
                }
            }
        }

        void addStepMoves(const Board& board, Square from, const int offsets[8][2], MoveList& moves) {
            Color us = board.sideToMove();
            int file = fileOf(from);
            int rank = rankOf(from);

            for (int i = 0; i < 8; ++i) {
                int f = file + offsets[i][0];
                int r = rank + offsets[i][1];
                if (f < 0 || f > 7 || r < 0 || r > 7) {
                    continue;
                }
                Square to = makeSquare(f, r);
                Piece target = board.pieceAt(to);
                if (target.isEmpty() || target.color != us) {
                    moves.push_back(Move{from, to, PieceType::None, MoveFlag::Normal});
                }
            }
        }

        void addSlidingMoves(const Board& board, Square from, const int dirs[][2], int dirCount,
                             MoveList& moves) {
            Color us = board.sideToMove();
            int file = fileOf(from);
            int rank = rankOf(from);

            for (int i = 0; i < dirCount; ++i) {
                int f = file + dirs[i][0];
                int r = rank + dirs[i][1];
                while (f >= 0 && f <= 7 && r >= 0 && r <= 7) {
                    Square to = makeSquare(f, r);
                    Piece target = board.pieceAt(to);
                    if (target.isEmpty()) {
                        moves.push_back(Move{from, to, PieceType::None, MoveFlag::Normal});
                    } else {
                        if (target.color != us) {
                            moves.push_back(Move{from, to, PieceType::None, MoveFlag::Normal});
                        }
                        break;
                    }
                    f += dirs[i][0];
                    r += dirs[i][1];
                }
            }
        }

        void addCastlingMoves(const Board& board, MoveList& moves) {
            Color us = board.sideToMove();
            Color them = opposite(us);

            // Cannot castle out of check
            if (board.isInCheck(us)) {
                return;
            }

            int rank = us == Color::White ? 0 : 7;
            Square kingSq = makeSquare(4, rank);

            uint8_t kingSideRight = us == Color::White ? WhiteKingSide : BlackKingSide;
            uint8_t queenSideRight = us == Color::White ? WhiteQueenSide : BlackQueenSide;

            if (board.castlingRights() & kingSideRight) {
                Square f1 = makeSquare(5, rank);
                Square g1 = makeSquare(6, rank);
                if (board.pieceAt(f1).isEmpty() && board.pieceAt(g1).isEmpty() &&
                    !board.isSquareAttacked(f1, them) && !board.isSquareAttacked(g1, them)) {
                    moves.push_back(Move{kingSq, g1, PieceType::None, MoveFlag::Castle});
                }
            }

            if (board.castlingRights() & queenSideRight) {
                Square d1 = makeSquare(3, rank);
                Square c1 = makeSquare(2, rank);
                Square b1 = makeSquare(1, rank);
                // b1 must be empty but is allowed to be attacked, since the king never crosses it
                if (board.pieceAt(d1).isEmpty() && board.pieceAt(c1).isEmpty() &&
                    board.pieceAt(b1).isEmpty() &&
                    !board.isSquareAttacked(d1, them) && !board.isSquareAttacked(c1, them)) {
                    moves.push_back(Move{kingSq, c1, PieceType::None, MoveFlag::Castle});
                }
            }
        }

    } // namespace

    std::string resultToString(GameResult r) {
        switch (r) {
            case GameResult::Ongoing:                  return "Game in progress";
            case GameResult::WhiteWins:                return "White wins by checkmate";
            case GameResult::BlackWins:                return "Black wins by checkmate";
            case GameResult::DrawStalemate:            return "Draw by stalemate";
            case GameResult::DrawFiftyMove:            return "Draw by fifty-move rule";
            case GameResult::DrawRepetition:           return "Draw by threefold repetition";
            case GameResult::DrawInsufficientMaterial: return "Draw by insufficient material";
        }
        return "Unknown";
    }

    std::string resultToPgnTag(GameResult r) {
        switch (r) {
            case GameResult::Ongoing:   return "*";
            case GameResult::WhiteWins: return "1-0";
            case GameResult::BlackWins: return "0-1";
            default:                    return "1/2-1/2";
        }
    }

    MoveList generatePseudoLegalMoves(const Board& board) {
        MoveList moves;
        moves.reserve(64);

        Color us = board.sideToMove();

        for (Square sq = 0; sq < 64; ++sq) {
            Piece p = board.pieceAt(sq);
            if (p.isEmpty() || p.color != us) {
                continue;
            }

            switch (p.type) {
                case PieceType::Pawn:
                    addPawnMoves(board, sq, moves);
                    break;
                case PieceType::Knight:
                    addStepMoves(board, sq, KNIGHT_OFFSETS, moves);
                    break;
                case PieceType::Bishop:
                    addSlidingMoves(board, sq, BISHOP_DIRS, 4, moves);
                    break;
                case PieceType::Rook:
                    addSlidingMoves(board, sq, ROOK_DIRS, 4, moves);
                    break;
                case PieceType::Queen:
                    addSlidingMoves(board, sq, QUEEN_DIRS, 8, moves);
                    break;
                case PieceType::King:
                    addStepMoves(board, sq, KING_OFFSETS, moves);
                    break;
                case PieceType::None:
                    break;
            }
        }

        addCastlingMoves(board, moves);
        return moves;
    }

    MoveList generateLegalMoves(Board& board) {
        MoveList legal;
        legal.reserve(48);

        Color us = board.sideToMove();
        MoveList pseudo = generatePseudoLegalMoves(board);

        for (const Move& m : pseudo) {
            board.makeMove(m);
            if (!board.isInCheck(us)) {
                legal.push_back(m);
            }
            board.unmakeMove();
        }

        return legal;
    }

    bool isLegalMove(Board& board, const Move& m) {
        MoveList legal = generateLegalMoves(board);
        for (const Move& lm : legal) {
            if (lm == m) {
                return true;
            }
        }
        return false;
    }

    GameResult getGameResult(Board& board) {
        MoveList legal = generateLegalMoves(board);

        if (legal.empty()) {
            if (board.isInCheck(board.sideToMove())) {
                return board.sideToMove() == Color::White ? GameResult::BlackWins
                                                          : GameResult::WhiteWins;
            }
            return GameResult::DrawStalemate;
        }

        if (board.halfmoveClock() >= 100) {
            return GameResult::DrawFiftyMove;
        }
        if (board.repetitionCount() >= 3) {
            return GameResult::DrawRepetition;
        }
        if (board.isInsufficientMaterial()) {
            return GameResult::DrawInsufficientMaterial;
        }

        return GameResult::Ongoing;
    }

    uint64_t perft(Board& board, int depth) {
        if (depth == 0) {
            return 1;
        }

        MoveList moves = generateLegalMoves(board);

        if (depth == 1) {
            return moves.size();
        }

        uint64_t nodes = 0;
        for (const Move& m : moves) {
            board.makeMove(m);
            nodes += perft(board, depth - 1);
            board.unmakeMove();
        }
        return nodes;
    }

    void perftDivide(Board& board, int depth) {
        if (depth < 1) {
            return;
        }

        MoveList moves = generateLegalMoves(board);
        uint64_t total = 0;

        for (const Move& m : moves) {
            board.makeMove(m);
            uint64_t nodes = perft(board, depth - 1);
            board.unmakeMove();
            std::cout << moveToUci(m) << ": " << nodes << "\n";
            total += nodes;
        }

        std::cout << "\nTotal: " << total << "\n";
    }

} // namespace chess