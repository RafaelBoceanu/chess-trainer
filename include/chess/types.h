#pragma once

#include <cstdint>
#include <string>

namespace chess {
    
    enum class Color : uint8_t {
        White = 0,
        Black = 1
    };

    inline Color opposite(Color c) {
        return c == Color::White ? Color::Black : Color::White;
    }

    enum class PieceType : uint8_t {
        None   = 0,
        Pawn   = 1,
        Knight = 2,
        Bishop = 3,
        Rook   = 4,
        Queen  = 5,
        King   = 6
    };

    struct Piece {
        PieceType type = PieceType::None;
        Color color = Color::White;

        bool isEmpty() const { return type == PieceType::None; }

        bool operator==(const Piece& other) const {
            if (type == PieceType::None && other.type == PieceType::None) {
                return true;
            }
            return type == other.type && color == other.color;
        }

        bool operator!=(const Piece& other) const { return !(*this == other); }
    };

    using Square = int;

    constexpr Square NO_SQUARE = -1;

    inline Square makeSquare(int file, int rank) {
        return rank * 8 + file;
    }

    inline int fileOf(Square sq) {
        return sq % 8;
    }

    inline int rankOf(Square sq) {
        return sq / 8;
    }

    inline bool isValidSquare(Square sq) {
        return sq >= 0 && sq < 64;
    }

    // Convert a square index to an algebric notation
    std::string squareToString(Square sq);

    // Parse algebric notations intoa a square index
    Square squareFromString(const std::string& str);

    // Castling rights as bitmasks
    enum CastlingRight : uint8_t {
        NoCastling     = 0,
        WhiteKingSide  = 1 << 0,
        WhiteQueenSide = 1 << 1,
        BlackKingSide  = 1 << 2,
        BlackQueenSide = 1 << 3,
        AllCastling    = WhiteKingSide | WhiteQueenSide | BlackKingSide | BlackQueenSide
    };

    // Character used for a piece. Uppercase for white, lowercase for black.
    char pieceToChar(Piece p);

    // Parse a piece as character
    Piece pieceFromChar(char c);

    // Lowercase promotion character used in UCI move notation
    char promotionToChar(PieceType pt);

    // Parse a UCI promotion character
    PieceType promotionFromChar(char c);
    
} // namespace chess