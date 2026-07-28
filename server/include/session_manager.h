#pragma once

#include "chess/board.h"
#include "chess/movegen.h"
#include "chess/review.h"
#include "chess/uci_engine.h"

#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace chess::server {

class GameSessionServer {
public:
    GameSessionServer(std::string enginePath, Color humanColor, int skillLevel);
    ~GameSessionServer();

    bool start();

    Color humanColor() const { return humanColor_; }
    const Board& board() const { return board_; }
    const MoveList& moveHistory() const { return moveHistory_; }
    GameResult result() const { return result_; }

    bool applyHumanMove(const Move& humanMove);

    GameReview runReview(int depth);

private:
    void applyEngineMoveIfNeeded();

    std::string enginePath_;
    Color humanColor_;
    int skillLevel_;

    Board board_;
    UciEngine engine_;
    MoveList moveHistory_;
    GameResult result_ = GameResult::Ongoing;
};

class SessionManager {
public:
    std::string createSession(const std::string& enginePath, Color humanColor, int skillLevel);

    GameSessionServer* get(const std::string& id);

    void remove(const std::string& id);

private:
    std::mutex mutex_;
    std::map<std::string, std::unique_ptr<GameSessionServer>> sessions_;
};

} // namespace chess::server