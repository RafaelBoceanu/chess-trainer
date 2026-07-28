export type Color = "white" | "black";

export type GameResultTag =
  | "ongoing"
  | "white_wins"
  | "black_wins"
  | "draw_stalemate"
  | "draw_fifty_move"
  | "draw_repetition"
  | "draw_insufficient_material";

export interface GameState {
  sessionId?: string;
  fen: string;
  sideToMove: Color;
  humanColor: Color;
  result: GameResultTag;
  inCheck: boolean;
  legalMoves: Record<string, string[]>;
  moveHistorySan: string[];
}

export interface ReviewedMove {
  moveNumber: number;
  side: Color;
  playedSan: string;
  bestSan: string;
  evalBefore: number;
  evalAfter: number;
  centipawnLoss: number;
  quality: "book" | "best" | "good" | "inaccuracy" | "mistake" | "BLUNDER";
  bestLineSan: string;
}

export interface ReviewSummary {
  whiteAvgLoss: number;
  blackAvgLoss: number;
  whiteBlunders: number;
  blackBlunders: number;
  whiteMistakes: number;
  blackMistakes: number;
  whiteInaccuracies: number;
  blackInaccuracies: number;
}

export interface GameReview {
  moves: ReviewedMove[];
  summary: ReviewSummary;
}