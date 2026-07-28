import type { GameState, GameReview, Color } from "./types";

const BASE_URL = "http://localhost:8080/api";

async function handleResponse<T>(res: Response): Promise<T> {
  if (!res.ok) {
    let message = `Request failed with status ${res.status}`;
    try {
      const body = await res.json();
      if (body.error) {
        message = body.error;
      }
    } catch {
      // Response body wasn't JSON; fall back to the generic message.
    }
    throw new Error(message);
  }
  return res.json() as Promise<T>;
}

export async function createGame(color: Color, skill: number): Promise<GameState> {
  const res = await fetch(`${BASE_URL}/game/new`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ color, skill }),
  });
  return handleResponse<GameState>(res);
}

export async function fetchGameState(sessionId: string): Promise<GameState> {
  const res = await fetch(`${BASE_URL}/game/${sessionId}/state`);
  return handleResponse<GameState>(res);
}

export async function makeMove(
  sessionId: string,
  from: string,
  to: string,
  promotion?: string
): Promise<GameState> {
  const res = await fetch(`${BASE_URL}/game/${sessionId}/move`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ from, to, promotion }),
  });
  return handleResponse<GameState>(res);
}

export async function fetchReview(sessionId: string): Promise<GameReview> {
  const res = await fetch(`${BASE_URL}/game/${sessionId}/review`, {
    method: "POST",
  });
  return handleResponse<GameReview>(res);
}