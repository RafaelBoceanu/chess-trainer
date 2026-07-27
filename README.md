# Chess Coach

A console chess game against Stockfish, with an automatic post-game review that
walks back through every move and shows what the engine would have played instead.

## What it does

- Full chess rules implemented from scratch in C++ (move generation, castling,
  en passant, promotion, checkmate, stalemate, fifty-move rule, threefold
  repetition, insufficient material)
- Plays against Stockfish over the UCI protocol at a configurable skill level
- Exports the finished game to PGN
- Reviews the game move by move at full engine strength, classifying each move
  as best / good / inaccuracy / mistake / blunder based on centipawn loss, and
  showing the better move plus the line that refutes what was played

## Requirements

- CMake 3.15+
- A C++17 compiler
- Stockfish on your PATH (or pass `--engine /path/to/stockfish`)

Install Stockfish:

    # macOS
    brew install stockfish

    # Debian / Ubuntu
    sudo apt install stockfish

    # Windows
    Download from stockfishchess.org and pass the path with --engine

## Build

    cmake -B build
    cmake --build build

## Run

    ./build/chess-coach

Options:

    --engine <path>   Path to the Stockfish binary (default: stockfish)
    --black           Play as Black (default: White)
    --skill <0-20>    Engine skill level (default: 5)
    --movetime <ms>   Engine thinking time per move (default: 500)
    --depth <n>       Review search depth (default: 16)
    --pgn <path>      Where to save the game (default: game.pgn)

During the game, enter moves in UCI (`e2e4`, `e7e8q`) or SAN (`Nf3`, `O-O`,
`exd5`). Type `moves` for the legal move list, `board` to redraw, `fen` for the
current position, or `quit` to stop and go straight to the review.

## Verify the move generator

    ./build/perft

This runs perft against six standard test positions with known node counts.
Every position must match exactly. If one fails, the test prints a per-move
divide so you can bisect down to the position where the generator disagrees.

Deeper run (slower):

    ./build/perft 5

## Architecture

    types.h/cpp       Squares, pieces, colours, FEN character mapping
    move.h/cpp        Move struct, UCI serialisation
    board.h/cpp       Board state, make/unmake, FEN, attack detection, Zobrist hashing
    movegen.h/cpp     Legal move generation, game result detection, perft
    pgn.h/cpp         SAN conversion, PGN export
    uci_engine.h/cpp  Stockfish subprocess management and UCI protocol
    review.h/cpp      Post-game analysis and move classification
    game.h/cpp        Console play loop
    main.cpp          Argument parsing

The board uses a mailbox representation (array of 64 pieces) rather than
bitboards. Stockfish does the actual thinking, so the move generator only needs
to be correct and fast enough for legality checks and perft, and mailbox is far
easier to read and debug.

Legal move generation is pseudo-legal generation followed by a make/unmake
filter that discards moves leaving the king in check. This is slower than
computing pins and checks directly, but it is much harder to get wrong.

The review reuses the evaluation of position N+1 as the "before" evaluation of
move N+1, halving the number of engine calls over a full game.