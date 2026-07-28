import type { GameReview, ReviewedMove } from "../types";

interface ReviewPanelProps {
  review: GameReview;
}

const QUALITY_COLORS: Record<ReviewedMove["quality"], string> = {
  book: "#888",
  best: "#2e7d32",
  good: "#558b2f",
  inaccuracy: "#f9a825",
  mistake: "#ef6c00",
  BLUNDER: "#c62828",
};

function flaggedOnly(moves: ReviewedMove[]): ReviewedMove[] {
  return moves.filter(
    (m) => m.quality === "inaccuracy" || m.quality === "mistake" || m.quality === "BLUNDER"
  );
}

export default function ReviewPanel({ review }: ReviewPanelProps) {
  const flagged = flaggedOnly(review.moves);

  return (
    <div style={{ maxWidth: 480, fontFamily: "sans-serif" }}>
      <h2>Game Review</h2>

      <table style={{ width: "100%", borderCollapse: "collapse", marginBottom: 16 }}>
        <thead>
          <tr>
            <th></th>
            <th>White</th>
            <th>Black</th>
          </tr>
        </thead>
        <tbody>
          <tr>
            <td>Avg loss (cp)</td>
            <td>{review.summary.whiteAvgLoss}</td>
            <td>{review.summary.blackAvgLoss}</td>
          </tr>
          <tr>
            <td>Inaccuracies</td>
            <td>{review.summary.whiteInaccuracies}</td>
            <td>{review.summary.blackInaccuracies}</td>
          </tr>
          <tr>
            <td>Mistakes</td>
            <td>{review.summary.whiteMistakes}</td>
            <td>{review.summary.blackMistakes}</td>
          </tr>
          <tr>
            <td>Blunders</td>
            <td>{review.summary.whiteBlunders}</td>
            <td>{review.summary.blackBlunders}</td>
          </tr>
        </tbody>
      </table>

      <h3>Moves worth a second look</h3>
      {flagged.length === 0 && <p>No inaccuracies, mistakes, or blunders. Clean game.</p>}

      {flagged.map((m, i) => (
        <div
          key={i}
          style={{
            borderLeft: `4px solid ${QUALITY_COLORS[m.quality]}`,
            paddingLeft: 8,
            marginBottom: 10,
          }}
        >
          <strong>
            {m.moveNumber}
            {m.side === "white" ? "." : "..."} {m.playedSan}
          </strong>{" "}
          <span style={{ color: QUALITY_COLORS[m.quality] }}>[{m.quality}]</span>
          <div style={{ fontSize: 14, color: "#444" }}>
            Loss: {m.centipawnLoss}cp — better was <strong>{m.bestSan}</strong>
          </div>
          {m.bestLineSan && (
            <div style={{ fontSize: 13, color: "#666" }}>Line: {m.bestLineSan}</div>
          )}
        </div>
      ))}
    </div>
  );
}