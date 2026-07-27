#include "chess/board.h"

#include <cctype>
#include <random>
#include <sstream>

namespace chess {

    namespace {
        
        // Zobrist keys
        struct ZobristKeys {
            uint64_t pieces[2][7][64];
            uint64_t castling[16];
            uint64_t enPassantFile[8];
            uint64_t sideToMove;

            ZobristKeys() {
                std::mt19937_64 rng(0x9E3779B97F4A7C15ULL);
                for (int c = 0; c < 2; ++c) {
                    for (int p = 0; p < 7; ++p) {
                        for (int s = 0; s < 64; ++s) {
                            pieces[c][p][s] = rng();
                        }
                    }
                }
                for (int i = 0; i < 16; ++i) {
                    castling[i] = rng();
                }
                for (int i = 0; i < 8; ++i) {
                    enPassantFile[i] = rng();
                }
                sideToMove = rng();
            }
        };

        const ZobristKeys& zobrist() {
            static const ZobristKeys keys;
            return keys;
        }

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
    
    } // namespace

    Board::Board() {
        setStartPosition();
    }

    void Board::clear() {
        squares_.fill(Piece{});
        sideToMove_ = Color::White;
        enPassantSquare_ = NO_SQUARE;
        castlingRights_ = NoCastling;
        halfmoveClock_ = 0;
        fullmoveNumber_ = 1;
        hash_ = 0;
        history_.clear();
        positionHashes_.clear();
    }

    void Board::setStartPosition() {
        setFromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    }

    bool Board::setFromFen(const std::string& fen) {
        clear();

        std::istringstream iss(fen);
        std::string placement, active, castling, enPassant;
        int halfmove = 0;
        int fullmove = 1;

        if (!(iss >> placement >> active >> castling >> enPassant)) {
            setStartPosition();
            return false;
        }
        // Halfmove and fullmove counters are optional in some FEN dialects
        if (!(iss >> halfmove)) {
            halfmove = 0;
        }
        if (!(iss >> fullmove)) {
            fullmove = 1;
        }

        int rank = 7;
        int file = 0;
        for (char c : placement) {
            if (c == '/') {
                if (file != 8) {
                    setStartPosition();
                    return false;
                }
                --rank;
                file = 0;
                if (rank < 0) {
                    setStartPosition();
                    return false;
                }
            } else if (std::isdigit(static_cast<unsigned char>(c))) {
                file += c - '0';
                if (file > 8) {
                    setStartPosition();
                    return false;
                }
            } else {
                Piece p = pieceFromChar(c);
                if (p.isEmpty() || file > 7 || rank < 0) {
                    setStartPosition();
                    return false;
                }
                squares_[makeSquare(file, rank)] = p;
                ++file;
            }
        }
        if (rank != 0 || file != 8) {
            setStartPosition();
            return false;
        }

        if (active == "w") {
            sideToMove_ = Color::White;
        } else if (active == "b") {
            sideToMove_ = Color::Black;
        } else {
            setStartPosition();
            return false;
        }

        castlingRights_ = NoCastling;
        if (castling != "-") {
            for (char c : castling) {
                switch (c) {
                    case 'K': castlingRights_ |= WhiteKingSide;  break;
                    case 'Q': castlingRights_ |= WhiteQueenSide; break;
                    case 'k': castlingRights_ |= BlackKingSide;  break;
                    case 'q': castlingRights_ |= BlackQueenSide; break;
                    default:
                        setStartPosition();
                        return false;
                }
            }
        }

        if (enPassant == "-") {
            enPassantSquare_ = NO_SQUARE;
        } else {
            enPassantSquare_ = squareFromString(enPassant);
            if (enPassantSquare_ == NO_SQUARE) {
                setStartPosition();
                return false;
            }
        }

        halfmoveClock_ = halfmove;
        fullmoveNumber_ = fullmove;
        hash_ = computeHash();
        positionHashes_.push_back(hash_);
        return true;
    }

    std::string Board::toFen() const {
        std::string fen;

        for (int rank =7; rank >= 0; --rank) {
            int empty = 0;
            for (int file = 0; file < 8; ++ file) {
                Piece p = squares_[makeSquare(file, rank)];
                if (p.isEmpty()) {
                    ++empty;
                } else {
                    if (empty > 0) {
                        fen += std::to_string(empty);
                        empty = 0;
                    }
                    fen += pieceToChar(p);
                }
            }
            if (empty > 0) {
                fen += std::to_string(empty);
            }
            if (rank > 0) {
                fen += '/';
            }
        }

        fen += sideToMove_ == Color::White ? " w " : " b ";

        if (castlingRights_ == NoCastling) {
            fen += '-';
        } else {
            if (castlingRights_ & WhiteKingSide)  fen += 'K';
            if (castlingRights_ & WhiteQueenSide) fen += 'Q';
            if (castlingRights_ & BlackKingSide)  fen += 'k';
            if (castlingRights_ & BlackQueenSide) fen += 'q';
        }

        fen += ' ';
        fen += enPassantSquare_ == NO_SQUARE ? "-" : squareToString(enPassantSquare_);
        fen += ' ' + std::to_string(halfmoveClock_);
        fen += ' ' + std::to_string(fullmoveNumber_);
    
        return fen;
    }

    Square Board::kingSquare(Color c) const {
        for (Square sq = 0; sq < 64; ++sq) {
            Piece p = squares_[sq];
            if (p.type == PieceType::King && p.color == c) {
                return sq;
            }
        }

        return NO_SQUARE;
    }

    uint64_t Board::computeHash() const {
        const ZobristKeys& z = zobrist();
        uint64_t h = 0;

        for (Square sq = 0; sq < 64; ++sq) {
            Piece p = squares_[sq];
            if (!p.isEmpty()) {
                h ^= z.pieces[static_cast<int>(p.color)][static_cast<int>(p.type)][sq];
            }
        }
        h ^= z.castling[castlingRights_];
        if (enPassantSquare_ != NO_SQUARE) {
            h ^= z.enPassantFile[fileOf(enPassantSquare_)];
        }
        if (sideToMove_ == Color::Black) {
            h ^= z.sideToMove;
        }

        return h;
    }

    void Board::placePiece(Square sq, Piece p) {
        squares_[sq] = p;
    }

    void Board::removePiece(Square sq) {
        squares_[sq] = Piece{};
    }

    void Board::movePiece(Square from, Square to) {
        squares_[to] = squares_[from];
        squares_[from] = Piece{};
    }

    void Board::updateCastlingRights(Square from, Square to) {
        // Any king or rook leaving its home square, or any capture landing on
        // a rook home square, kills the relevant right
        if (from == makeSquare(4, 0) || to == makeSquare(4, 0)) {
            castlingRights_ &= ~(WhiteKingSide | WhiteQueenSide); 
        }
        if (from == makeSquare(4, 7) || to == makeSquare(4, 7)) {
            castlingRights_ &= ~(BlackKingSide | BlackQueenSide); 
        }
        if (from == makeSquare(7, 0) || to == makeSquare(7, 0)) {
            castlingRights_ &= ~WhiteKingSide; 
        }
        if (from == makeSquare(0, 0) || to == makeSquare(0, 0)) {
            castlingRights_ &= ~WhiteQueenSide; 
        }
        if (from == makeSquare(7, 7) || to == makeSquare(7, 7)) {
            castlingRights_ &= ~BlackKingSide; 
        }
        if (from == makeSquare(0, 7) || to == makeSquare(0, 7)) {
            castlingRights_ &= ~BlackQueenSide; 
        }
    }

    void Board::makeMove(const Move& m) {
        UndoInfo undo;
        undo.enPassantSquare = enPassantSquare_;
        undo.castlingRights = castlingRights_;
        undo.halfmoveClock = halfmoveClock_;
        undo.hash = hash_;

        Piece moving = squares_[m.from];
        Move resolved = m;

        // Resolve the flag from board context so callers can pass bare from/to moves
        if (resolved.promotion != PieceType::None) {
            resolved.flag = MoveFlag::Promotion;
        } else if (moving.type == PieceType::King &&
                   std::abs(fileOf(m.to) - fileOf(m.from)) == 2) {
            resolved.flag = MoveFlag::Castle;
        } else if (moving.type == PieceType::Pawn && m.to == enPassantSquare_) {
            resolved.flag = MoveFlag::EnPassant;
        } else if (moving.type == PieceType::Pawn &&
                   std::abs(rankOf(m.to) - rankOf(m.from)) == 2) {
            resolved.flag = MoveFlag::DoublePush;
        } else {
            resolved.flag = MoveFlag::Normal;
        }

        undo.move = resolved;

        if (resolved.flag == MoveFlag::EnPassant) {
            Square capturedSq = makeSquare(fileOf(m.to), rankOf(m.from));
            undo.captured = squares_[capturedSq];
            removePiece(capturedSq);
        } else {
            undo.captured = squares_[m.to];
        }

        history_.push_back(undo);

        bool isCapture = !undo.captured.isEmpty();
        bool isPawnMove = moving.type == PieceType::Pawn;
        halfmoveClock_ = (isCapture || isPawnMove) ? 0 : halfmoveClock_ + 1;

        movePiece(m.from, m.to);

        if (resolved.flag == MoveFlag::Promotion) {
            placePiece(m.to, Piece{resolved.promotion, moving.color});
        }

        if (resolved.flag == MoveFlag::Castle) {
            int rank = rankOf(m.from);
            if (fileOf(m.to) == 6) {
                movePiece(makeSquare(7, rank), makeSquare(5, rank));
            } else {
                movePiece(makeSquare(0, rank), makeSquare(3, rank));
            }
        }

        enPassantSquare_ = NO_SQUARE;
        if (resolved.flag == MoveFlag::DoublePush) {
            int midRank = (rankOf(m.from) + rankOf(m.to)) / 2;
            enPassantSquare_ = makeSquare(fileOf(m.from), midRank);
        }

        updateCastlingRights(m.from, m.to);

        if (sideToMove_ == Color::Black) {
            ++fullmoveNumber_;
        }
        sideToMove_ = opposite(sideToMove_);

        hash_ = computeHash();
        positionHashes_.push_back(hash_);
    }

    void Board::unmakeMove() {
        if (history_.empty()) {
            return;
        }

        UndoInfo undo = history_.back();
        history_.pop_back();
        if (!positionHashes_.empty()) {
            positionHashes_.pop_back();
        }

        sideToMove_ = opposite(sideToMove_);
        if (sideToMove_ == Color::Black) {
            --fullmoveNumber_;
        }

        const Move& m = undo.move;

        movePiece(m.to, m.from);

        if (m.flag == MoveFlag::Promotion) {
            placePiece(m.from, Piece{PieceType::Pawn, sideToMove_});
        }

        if (m.flag == MoveFlag::EnPassant) {
            placePiece(makeSquare(fileOf(m.to), rankOf(m.from)), undo.captured);
        } else if (!undo.captured.isEmpty()) {
            placePiece(m.to, undo.captured);
        }

        if (m.flag == MoveFlag::Castle) {
            int rank = rankOf(m.from);
            if (fileOf(m.to) == 6) {
                movePiece(makeSquare(5, rank), makeSquare(7, rank));
            } else {
                movePiece(makeSquare(3, rank), makeSquare(0, rank));
            }
        }

        enPassantSquare_ = undo.enPassantSquare;
        castlingRights_ = undo.castlingRights;
        halfmoveClock_ = undo.halfmoveClock;
        hash_ = undo.hash;
    }

    bool Board::isSquareAttacked(Square sq, Color by) const {
        int targetFile = fileOf(sq);
        int targetRank = rankOf(sq);

        // Pawns. A white pawn on rank r attacks rank r+1, so to be attacked by
        // white the attacker sits one rank below the target
        int pawnRank = by == Color::White ? targetRank - 1 : targetRank + 1;
        if (pawnRank >= 0 && pawnRank < 8) {
            for (int df : {-1, 1}) {
                int f = targetFile + df;
                if (f < 0 || f > 7) {
                    continue;
                }
                Piece p = squares_[makeSquare(f, pawnRank)];
                if (p.type == PieceType::Pawn && p.color == by) {
                    return true;
                }
            }
        }

        for (const auto& off : KNIGHT_OFFSETS) {
            int f = targetFile + off[0];
            int r = targetRank + off[1];
            if (f < 0 || f > 7 || r < 0 || r > 7) {
                continue;
            }
            Piece p = squares_[makeSquare(f, r)];
            if (p.type == PieceType::Knight && p.color == by) {
                return true;
            }
        }

        for (const auto& off : KING_OFFSETS) {
            int f = targetFile + off[0];
            int r = targetRank + off[1];
            if (f < 0 || f > 7 || r < 0 || r > 7) {
                continue;
            }
            Piece p = squares_[makeSquare(f, r)];
            if (p.type == PieceType::King && p.color == by) {
                return true;
            }
        }

        for (const auto& dir : BISHOP_DIRS) {
            int f = targetFile + dir[0];
            int r = targetRank + dir[1];
            while (f >= 0 && f <= 7 && r >= 0 && r <= 7) {
                Piece p = squares_[makeSquare(f, r)];
                if (!p.isEmpty()) {
                    if (p.color == by &&
                        (p.type == PieceType::Bishop || p.type == PieceType::Queen)) {
                            return true;
                        }
                        break;
                }
                f += dir[0];
                r += dir[1];
            }
        }

        for (const auto& dir : ROOK_DIRS) {
            int f = targetFile + dir[0];
            int r = targetRank + dir[1];
            while (f >= 0 && f <= 7 && r >= 0 && r <= 7) {
                Piece p = squares_[makeSquare(f, r)];
                if (!p.isEmpty()) {
                    if (p.color == by &&
                        (p.type == PieceType::Rook || p.type == PieceType::Queen)) {
                        return true;
                    }
                    break;
                }
                f += dir[0];
                r += dir[1];
            }
        }

        return false;
    }

    bool Board::isInCheck(Color c) const {
        Square king = kingSquare(c);
        if (king == NO_SQUARE) {
            return false;
        }
        return isSquareAttacked(king, opposite(c));
    }

    bool Board::isInsufficientMaterial() const {
        int knights = 0;
        int bishops = 0;
        bool bishopOnLight = false;
        bool bishopOnDark = false;

        for (Square sq = 0; sq < 64; ++sq) {
            Piece p = squares_[sq];
            switch (p.type) {
                case PieceType::None:
                case PieceType::King:
                    break;
                case PieceType::Knight:
                    ++knights;
                    break;
                case PieceType::Bishop:
                    ++bishops;
                    if ((fileOf(sq) + rankOf(sq)) % 2 == 0) {
                        bishopOnDark = true;
                    } else {
                        bishopOnLight = true;
                    }
                    break;
                default:
                    // Any pawn, rook or queen is sufficient material
                    return false;
            }
        }

        if (knights == 0 && bishops == 0) {
            return true; // K vs K
        }
        if (knights == 1 && bishops == 0) {
            return true; // K+N vs K
        }
        if (knights == 0 && bishops >= 1) {
            // Bishops all on one colour complex can never mate
            return !(bishopOnLight && bishopOnDark);
        }
        return false;
    }

    int Board::repetitionCount() const {
        int count = 0;
        for (uint64_t h : positionHashes_) {
            if (h == hash_) {
                ++count;
            }
        }
        return count;
    }

    std::string Board::toAsciiBoard() const {
        std::string out;
        out += "  +------------------------+\n";
        for (int rank = 7; rank >= 0; --rank) {
            out += static_cast<char>('1' + rank);
            out += " |";
            for (int file = 0; file < 8; ++file) {
                out += ' ';
                out += pieceToChar(squares_[makeSquare(file, rank)]);
                out += ' ';
            }
            out += "|\n";
        }
        out += "  +------------------------+\n";
        out += "    a  b  c  d  e  f  g  h\n";
        return out; 
    }

} // namespace chess