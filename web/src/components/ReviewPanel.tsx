import type { GameReview, ReviewedMove, Color } from "../types";

interface ReviewPanelProps {
  review: GameReview;
  humanColor: Color;
  selectedMove: ReviewedMove | null;
  onSelectMove: (move: ReviewedMove) => void;
}

const QUALITY_COLORS: Record<ReviewedMove["quality"], string> = {
  book: "#9e9e9e",
  best: "#2e7d32",
  good: "#66bb6a",
  inaccuracy: "#f9a825",
  mistake: "#ef6c00",
  BLUNDER: "#c62828",
};

const QUALITY_LABELS: Record<ReviewedMove["quality"], string> = {
  book: "Book",
  best: "Best",
  good: "Good",
  inaccuracy: "Inaccuracy",
  mistake: "Mistake",
  BLUNDER: "Blunder",
};

function clampEval(cp: number): number {
  const CAP = 800;
  return Math.max(-CAP, Math.min(CAP, cp));
}

// The graph always plots the full game, both sides, so the player's moves
// read in context rather than as disconnected data points. Only the player's
// own moves are clickable and get a filled dot; the opponent's moves render
// as small, inert markers that just trace the line.
function EvalGraph({
  moves,
  humanColor,
  selectedMove,
  onSelectMove,
}: {
  moves: ReviewedMove[];
  humanColor: Color;
  selectedMove: ReviewedMove | null;
  onSelectMove: (move: ReviewedMove) => void;
}) {
  const width = 480;
  const height = 120;
  const padding = 8;

  if (moves.length === 0) {
    return null;
  }

  const whitePerspective = moves.map((m) =>
    m.side === "white" ? clampEval(m.evalAfter) : -clampEval(m.evalAfter)
  );

  const points = whitePerspective.map((val, i) => {
    const x = padding + (i / Math.max(1, moves.length - 1)) * (width - padding * 2);
    const y = height / 2 - (val / 800) * (height / 2 - padding);
    return { x, y };
  });

  const path = points.map((p, i) => `${i === 0 ? "M" : "L"}${p.x},${p.y}`).join(" ");

  return (
    <svg width={width} height={height} style={{ display: "block", marginBottom: 16 }}>
      <rect x={0} y={0} width={width} height={height / 2} fill="#f5f5f5" />
      <rect x={0} y={height / 2} width={width} height={height / 2} fill="#3a3a3a" />
      <line x1={0} y1={height / 2} x2={width} y2={height / 2} stroke="#999" strokeWidth={1} />
      <path d={path} fill="none" stroke="#1565c0" strokeWidth={2} />
      {points.map((p, i) => {
        const move = moves[i];
        const isPlayerMove = move.side === humanColor;
        const isSelected = selectedMove === move;
        return (
          <circle
            key={i}
            cx={p.x}
            cy={p.y}
            r={isSelected ? 5 : isPlayerMove ? 3.5 : 2}
            fill={isPlayerMove ? QUALITY_COLORS[move.quality] : "#bbb"}
            stroke={isSelected ? "#000" : "none"}
            strokeWidth={1.5}
            style={{ cursor: isPlayerMove ? "pointer" : "default" }}
            onClick={() => isPlayerMove && onSelectMove(move)}
          />
        );
      })}
    </svg>
  );
}

function formatEval(cp: number): string {
  const pawns = cp / 100;
  return (pawns >= 0 ? "+" : "") + pawns.toFixed(2);
}

export default function ReviewPanel({
  review,
  humanColor,
  selectedMove,
  onSelectMove,
}: ReviewPanelProps) {
  const playerMoves = review.moves.filter((m) => m.side === humanColor);

  const avgLoss = humanColor === "white" ? review.summary.whiteAvgLoss : review.summary.blackAvgLoss;
  const inaccuracies =
    humanColor === "white" ? review.summary.whiteInaccuracies : review.summary.blackInaccuracies;
  const mistakes = humanColor === "white" ? review.summary.whiteMistakes : review.summary.blackMistakes;
  const blunders = humanColor === "white" ? review.summary.whiteBlunders : review.summary.blackBlunders;

  return (
    <div style={{ maxWidth: 480, fontFamily: "sans-serif" }}>
      <h2>Your Game Review</h2>

      <EvalGraph
        moves={review.moves}
        humanColor={humanColor}
        selectedMove={selectedMove}
        onSelectMove={onSelectMove}
      />

      <table style={{ width: "100%", borderCollapse: "collapse", marginBottom: 16, fontSize: 14 }}>
        <tbody>
          <tr>
            <td>Avg loss (cp)</td>
            <td style={{ textAlign: "right" }}>{avgLoss}</td>
          </tr>
          <tr>
            <td>Inaccuracies</td>
            <td style={{ textAlign: "right" }}>{inaccuracies}</td>
          </tr>
          <tr>
            <td>Mistakes</td>
            <td style={{ textAlign: "right" }}>{mistakes}</td>
          </tr>
          <tr>
            <td>Blunders</td>
            <td style={{ textAlign: "right" }}>{blunders}</td>
          </tr>
        </tbody>
      </table>

      <h3>Your moves</h3>
      <div style={{ maxHeight: 360, overflowY: "auto", border: "1px solid #ddd", borderRadius: 4 }}>
        {playerMoves.map((m, i) => (
          <div
            key={i}
            onClick={() => onSelectMove(m)}
            style={{
              display: "flex",
              alignItems: "center",
              gap: 8,
              padding: "6px 10px",
              cursor: "pointer",
              backgroundColor: selectedMove === m ? "#e3f2fd" : "transparent",
              borderBottom: "1px solid #eee",
            }}
          >
            <span style={{ width: 42, color: "#888", fontSize: 13 }}>
              {m.moveNumber}
              {m.side === "white" ? "." : "..."}
            </span>
            <span style={{ width: 60, fontWeight: 600 }}>{m.playedSan}</span>
            <span
              style={{
                fontSize: 12,
                fontWeight: 600,
                color: "#fff",
                backgroundColor: QUALITY_COLORS[m.quality],
                borderRadius: 3,
                padding: "1px 6px",
              }}
            >
              {QUALITY_LABELS[m.quality]}
            </span>
            <span style={{ marginLeft: "auto", fontSize: 13, color: "#666" }}>
              {formatEval(m.evalAfter)}
            </span>
          </div>
        ))}
      </div>

      {selectedMove && (
        <div
          style={{
            marginTop: 12,
            padding: 12,
            backgroundColor: "#fafafa",
            borderRadius: 4,
            lineHeight: 1.6,
          }}
        >
          <div style={{ marginBottom: 6 }}>
            <strong>
              {selectedMove.moveNumber}
              {selectedMove.side === "white" ? "." : "..."} {selectedMove.playedSan}
            </strong>{" "}
            <span
              style={{
                fontSize: 12,
                fontWeight: 600,
                color: "#fff",
                backgroundColor: QUALITY_COLORS[selectedMove.quality],
                borderRadius: 3,
                padding: "1px 6px",
              }}
            >
              {QUALITY_LABELS[selectedMove.quality]}
            </span>
          </div>

          {(selectedMove.quality === "inaccuracy" ||
            selectedMove.quality === "mistake" ||
            selectedMove.quality === "BLUNDER") && (
              <>
                <div style={{ fontSize: 14, color: "#333" }}>
                  This move cost <strong>{(selectedMove.centipawnLoss / 100).toFixed(2)} pawns</strong>{" "}
                  of evaluation.
                </div>
                <div style={{ fontSize: 14, color: "#333", marginTop: 2 }}>
                  Better was <strong style={{ color: "#2e7d32" }}>{selectedMove.bestSan}</strong>.
                </div>
                {selectedMove.bestLineSan && (
                  <div
                    style={{
                      fontSize: 13,
                      color: "#444",
                      marginTop: 6,
                      padding: "6px 8px",
                      backgroundColor: "#fff",
                      border: "1px solid #e0e0e0",
                      borderRadius: 3,
                      fontFamily: "monospace",
                    }}
                  >
                    {selectedMove.bestLineSan}
                  </div>
                )}
              </>
            )}

          {(selectedMove.quality === "best" || selectedMove.quality === "good") && (
            <div style={{ fontSize: 14, color: "#333" }}>
              Solid move — within {selectedMove.centipawnLoss}cp of the engine's top choice.
            </div>
          )}
        </div>
      )}
    </div>
  );
}