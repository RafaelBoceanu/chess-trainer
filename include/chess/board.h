#pragma once

#include "chess/move.h"
#include "chess/types.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace chess {

    struct UndoInfo {
        Move move;
        Piece captured;
        Square enPassantSquare = NO_SQUARE;
        uint8_t castlingRights = NoCastling;
        int halfmoveClock = 0;
        uint64_t hash = 0;
    };

    class Board {
    public:
        Board();

        void setStartPosition();

        // Load a position from FEN
        bool setFromFen(const std::string& fen);

        std::string toFen() const;

        Piece pieceAt(Square sq) const { return squares_[sq]; }

        Color sideToMove() const { return sideToMove_; }
        Square enPassantSquare() const { return enPassantSquare_; }
        uint8_t castlingRights() const { return castlingRights_; }
        int halfmoveClock() const { return halfmoveClock_; }
        int fullmoveNumber() const { return fullmoveNumber_; }
        uint64_t hash() const { return hash_; }

        Square kingSquare(Color c) const;

        // Apply a move
        void makeMove(const Move& m);

        void unmakeMove();

        // Check if the given square is attacked by any piece of a given colour
        bool isSquareAttacked(Square sq, Color by) const;

        bool isInCheck(Color c) const;

        // Check if there is insufficient material for either side to force a check mate
        bool isInsufficientMaterial() const;

        // Count how many times the current position has occurred in this game
        int repetitionCount() const;

        std::string toAsciiBoard() const;

    private:
        void clear();
        void placePiece(Square sq, Piece p);
        void removePiece(Square sq);
        void movePiece(Square from, Square to);
        void updateCastlingRights(Square from, Square to);
        uint64_t computeHash() const;

        std::array<Piece, 64> squares_{};
        Color sideToMove_ = Color::White;
        Square enPassantSquare_ = NO_SQUARE;
        uint8_t castlingRights_ = AllCastling;
        int halfmoveClock_ = 0;
        int fullmoveNumber_ = 1;
        uint64_t hash_ = 0;

        std::vector<UndoInfo> history_;
        std::vector<uint64_t> positionHashes_;
    };

} // namespace chess