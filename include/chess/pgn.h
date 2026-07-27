#pragma once

#include "chess/board.h"
#include "chess/move.h"
#include "chess/movegen.h"

#include <string>
#include <vector>

namespace chess {

    // Converts a move to standard algebric notation. The board must be in the 
    // position before the move is played
    std::string moveToSan(Board& board, const Move& m);

    // Builds a complete PGN string from a move list, replaying from the start position
    std::string buildPgn(const MoveList& moves,
                         GameResult result,
                         const std::string& whiteName,
                         const std::string& blackName);
    
    // Writes the PGN to disk. Returns false if the file could not be opened
    bool savePgn(const std::string& pgn, const std::string& path);

} // namespace chess