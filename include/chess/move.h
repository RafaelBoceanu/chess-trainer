#pragma once

#include "chess/types.h"

#include <string>
#include <vector>

namespace chess {

    enum class MoveFlag : uint8_t {
        Normal     = 0,
        DoublePush = 1,
        EnPassant  = 2,
        Castle     = 3,
        Promotion  = 4
    };

    struct Move {
        Square from = NO_SQUARE;
        Square to = NO_SQUARE;
        PieceType promotion = PieceType::None;
        MoveFlag flag = MoveFlag::Normal;

        bool isNull() const { return from == NO_SQUARE || to == NO_SQUARE; }

        bool operator==(const Move& other) const {
            return from == other.from && to == other.to && promotion == other.promotion;
        }

        bool operator!=(const Move& other) const { return !(*this == other); }
    };

    using MoveList = std::vector<Move>;

    // Serialise a move to UCI long algebric notation
    std::string moveToUci(const Move& m);

    // Parse UCI notation into a move
    Move moveFromUci(const std::string& str);

} // namespace chess