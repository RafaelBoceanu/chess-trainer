import { useState } from "react";
import ChessBoard from "./components/ChessBoard";
import ReviewPanel from "./components/ReviewPanel";
import { createGame, makeMove, fetchReview } from "./api";
import type { GameState, GameReview, Color, ReviewedMove } from "./types";

type Screen = "setup" | "playing" | "reviewing";

const RESULT_MESSAGES: Record<string, string> = {
  white_wins: "White wins by checkmate",
  black_wins: "Black wins by checkmate",
  draw_stalemate: "Draw by stalemate",
  draw_fifty_move: "Draw by fifty-move rule",
  draw_repetition: "Draw by threefold repetition",
  draw_insufficient_material: "Draw by insufficient material",
};

export default function App() {
  const [screen, setScreen] = useState<Screen>("setup");
  const [color, setColor] = useState<Color>("white");
  const [skill, setSkill] = useState(5);
  const [gameState, setGameState] = useState<GameState | null>(null);
  const [review, setReview] = useState<GameReview | null>(null);
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [selectedMove, setSelectedMove] = useState<ReviewedMove | null>(null);

  async function handleStart() {
    setBusy(true);
    setError(null);
    try {
      const state = await createGame(color, skill);
      setGameState(state);
      setScreen("playing");
    } catch (e) {
      setError(e instanceof Error ? e.message : "Failed to start game");
    } finally {
      setBusy(false);
    }
  }

  async function handleMove(from: string, to: string, promotion?: string) {
    if (!gameState?.sessionId || busy) {
      return;
    }
    setBusy(true);
    setError(null);
    try {
      const updated = await makeMove(gameState.sessionId, from, to, promotion);
      setGameState({ ...updated, sessionId: gameState.sessionId });
    } catch (e) {
      setError(e instanceof Error ? e.message : "Move failed");
    } finally {
      setBusy(false);
    }
  }

  async function handleReview() {
    if (!gameState?.sessionId) {
      return;
    }
    setBusy(true);
    setError(null);
    try {
      const r = await fetchReview(gameState.sessionId);
      setReview(r);
      const playerMoves = r.moves.filter((m) => m.side === gameState.humanColor);
      setSelectedMove(playerMoves.length > 0 ? playerMoves[playerMoves.length - 1] : null);
      setScreen("reviewing");
    } catch (e) {
      setError(e instanceof Error ? e.message : "Review failed");
    } finally {
      setBusy(false);
    }
  }

  if (screen === "setup") {
    return (
      <div style={{ maxWidth: 400, margin: "80px auto", fontFamily: "sans-serif" }}>
        <h1>Chess Trainer</h1>

        <label style={{ display: "block", marginBottom: 12 }}>
          Play as:
          <select value={color} onChange={(e) => setColor(e.target.value as Color)}>
            <option value="white">White</option>
            <option value="black">Black</option>
          </select>
        </label>

        <label style={{ display: "block", marginBottom: 12 }}>
          Engine skill (0-20):
          <input
            type="number"
            min={0}
            max={20}
            value={skill}
            onChange={(e) => setSkill(Number(e.target.value))}
          />
        </label>

        <button onClick={handleStart} disabled={busy}>
          {busy ? "Starting..." : "Start Game"}
        </button>

        {error && <p style={{ color: "#c62828" }}>{error}</p>}
      </div>
    );
  }

  if (!gameState) {
    return null;
  }

  return (
    <div style={{ maxWidth: 1000, margin: "40px auto", fontFamily: "sans-serif" }}>
      <h1>Chess Trainer</h1>

      <div style={{ display: "flex", gap: 32 }}>
        <div>
          <ChessBoard
            gameState={
              screen === "reviewing" && selectedMove
              ? { ...gameState, fen: selectedMove.fenAfter, legalMoves: {} }
              : gameState
            }
            onMove={handleMove}
            disabled={busy || screen === "reviewing"}
          />

          <div style={{ marginTop: 12 }}>
            {gameState.inCheck && gameState.result === "ongoing" && (
              <p style={{ color: "#ef6c00" }}>Check!</p>
            )}
            {gameState.result !== "ongoing" && (
              <p style={{ fontWeight: "bold" }}>
                {RESULT_MESSAGES[gameState.result] ?? gameState.result}
              </p>
            )}
            {gameState.result !== "ongoing" && screen !== "reviewing" && (
              <button onClick={handleReview} disabled={busy}>
                {busy ? "Analysing..." : "Review Game"}
              </button>
            )}
            {error && <p style={{ color: "#c62828" }}>{error}</p>}
          </div>

          <div style={{ marginTop: 12, fontSize: 14, color: "#666" }}>
            {gameState.moveHistorySan.join(" ")}
          </div>
        </div>

        {screen === "reviewing" && review && (
          <ReviewPanel 
            review={review}
            humanColor={gameState.humanColor} 
            selectedMove={selectedMove} 
            onSelectMove={setSelectedMove}/>
        )}
      </div>
    </div>
  );
}