#include "chess/move.h"

namespace chess {

    std::string moveToUci(const Move& m) {
        if (m.isNull()) {
            return "0000";
        }
        std::string result = squareToString(m.from) + squareToString(m.to);
        if (m.promotion != PieceType::None) {
            char c = promotionToChar(m.promotion);
            if (c != '\0') {
                result += c;
            }
        }
        return result;
    }

    Move moveFromUci(const std::string& str) {
        Move m;
        if (str.size() < 4 || str.size() > 5) {
            return m;
        }

        Square from = squareFromString(str.substr(0, 2));
        Square to = squareFromString(str.substr(2, 2));
        if (from == NO_SQUARE || to == NO_SQUARE) {
            return m;
        }

        m.from = from;
        m.to = to;

        if (str.size() == 5) {
            PieceType promo = promotionFromChar(str[4]);
            if (promo == PieceType::None) {
                return Move{};
            }
            m.promotion = promo;
            m.flag = MoveFlag::Promotion;
        }

        return m;
    }

} // namespace chess