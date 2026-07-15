#include "chess/types.h"

#include <cctype>

namespace chess {

    std::string squareToString(Square sq) {
        if (!isValidSquare(sq)) {
            return "-";
        }
        std::string result;
        result += static_cast<char>('a' + fileOf(sq));
        result += static_cast<char>('1' + rankOf(sq));
        return result;
    }

    Square squareFromString(const std::string& str) {
        if (str.size() != 2) {
            return NO_SQUARE;
        }
        char fileChar = static_cast<char>(std::tolower(static_cast<unsigned char>(str[0])));
        char rankChar = str[1];

        if (fileChar < 'a' || fileChar > 'h') {
            return NO_SQUARE;
        }
        if (rankChar < '1' || rankChar > '8') {
            return NO_SQUARE;
        }

        int file = fileChar - 'a';
        int rank = rankChar - '1';
        return makeSquare(file, rank);
    }

    char pieceToChar(Piece p) {
        char c = '.';
        switch(p.type) {
            case PieceType::Pawn:   c = 'p'; break;
            case PieceType::Knight: c = 'n'; break;
            case PieceType::Bishop: c = 'b'; break;
            case PieceType::Rook:   c = 'r'; break;
            case PieceType::Queen:  c = 'q'; break;
            case PieceType::King:   c = 'k'; break;
            case PieceType::None:   return '.';
        }
        if (p.color == Color::White) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        return c;
    }

    Piece pieceFromChar(char c) {
        Color color = std::isupper(static_cast<unsigned char>(c)) ? Color::White : Color::Black;
        char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        PieceType type = PieceType::None;
        switch(lower) {
            case 'p': type = PieceType::Pawn;   break;
            case 'n': type = PieceType::Knight; break;
            case 'b': type = PieceType::Bishop; break;
            case 'r': type = PieceType::Rook;   break;
            case 'q': type = PieceType::Queen;  break;
            case 'k': type = PieceType::King;   break;
            default:  return Piece{};
        }
        return Piece{type, color};
    }

    char promotionToChar(PieceType pt) {
        switch (pt) {
            case PieceType::Knight: return 'n';
            case PieceType::Bishop: return 'b';
            case PieceType::Rook:   return 'r';
            case PieceType::Queen:  return 'q';
            default:                return '\0';
        }
    }

    PieceType promotionFromChar(char c) {
        switch (std::tolower(static_cast<unsigned char>(c))) {
            case 'n': return PieceType::Knight;
            case 'b': return PieceType::Bishop;
            case 'r': return PieceType::Rook;
            case 'q': return PieceType::Queen;
            default:  return PieceType::None;
        }
    }

} // namespace chess