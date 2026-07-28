import { Chessboard } from "react-chessboard";
import type { GameState } from "../types";

interface ChessBoardProps {
  gameState: GameState;
  onMove: (from: string, to: string, promotion?: string) => void;
  disabled: boolean;
}

// Thin wrapper around react-chessboard. All legality is decided server-side;
// this component only decides whether a drag is even worth attempting
// (right color piece, on the right turn) so the board doesn't feel broken
// before the request round-trips.
export default function ChessBoard({ gameState, onMove, disabled }: ChessBoardProps) {
  function isDraggablePiece({ piece }: { piece: string }): boolean {
    if (disabled || gameState.result !== "ongoing") {
      return false;
    }
    if (gameState.sideToMove !== gameState.humanColor) {
      return false;
    }
    const pieceColor = piece[0] === "w" ? "white" : "black";
    return pieceColor === gameState.humanColor;
  }

  function onDrop(sourceSquare: string, targetSquare: string, piece: string): boolean {
    const destinations = gameState.legalMoves[sourceSquare] ?? [];
    if (!destinations.includes(targetSquare)) {
      return false;
    }

    // Auto-queen promotions for now; a picker can be added if under-promotion
    // ever matters for the coaching angle.
    const isPawn = piece[1] === "P";
    const promotionRank = targetSquare[1] === "8" || targetSquare[1] === "1";
    const promotion = isPawn && promotionRank ? "q" : undefined;

    onMove(sourceSquare, targetSquare, promotion);
    return true;
  }

  return (
    <Chessboard
      position={gameState.fen}
      onPieceDrop={onDrop}
      isDraggablePiece={isDraggablePiece}
      boardOrientation={gameState.humanColor}
      boardWidth={480}
    />
  );
}