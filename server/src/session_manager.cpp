#include "session_manager.h"

#include <chrono>
#include <random>
#include <sstream>

namespace chess::server {

namespace {

std::string generateSessionId() {
    static std::mt19937_64 rng(
        std::chrono::steady_clock::now().time_since_epoch().count());
    std::ostringstream oss;
    oss << std::hex << rng();
    return oss.str();
}

} // namespace

GameSessionServer::GameSessionServer(std::string enginePath, Color humanColor, int skillLevel)
    : enginePath_(std::move(enginePath)), humanColor_(humanColor), skillLevel_(skillLevel) {}

GameSessionServer::~GameSessionServer() {
    engine_.stop();
}

bool GameSessionServer::start() {
    if (!engine_.start(enginePath_)) {
        return false;
    }
    engine_.setOption("Skill Level", std::to_string(skillLevel_));
    engine_.newGame();

    board_.setStartPosition();
    moveHistory_.clear();
    result_ = GameResult::Ongoing;

    applyEngineMoveIfNeeded();

    return true;
}

void GameSessionServer::applyEngineMoveIfNeeded() {
    result_ = getGameResult(board_);
    if (result_ != GameResult::Ongoing) {
        return;
    }
    if (board_.sideToMove() == humanColor_) {
        return;
    }

    Move m = engine_.getBestMove(board_.toFen(), 500);
    if (m.isNull() || !isLegalMove(board_, m)) {
        return;
    }

    board_.makeMove(m);
    moveHistory_.push_back(m);
    result_ = getGameResult(board_);
}

bool GameSessionServer::applyHumanMove(const Move& humanMove) {
    if (result_ != GameResult::Ongoing) {
        return false;
    }
    if (board_.sideToMove() != humanColor_) {
        return false;
    }
    if (!isLegalMove(board_, humanMove)) {
        return false;
    }

    board_.makeMove(humanMove);
    moveHistory_.push_back(humanMove);
    result_ = getGameResult(board_);

    applyEngineMoveIfNeeded();
    return true;
}

GameReview GameSessionServer::runReview(int depth) {
    engine_.setOption("Skill Level", "20");
    engine_.newGame();

    ReviewConfig rc;
    rc.depth = depth;
    return reviewGame(engine_, moveHistory_, rc);
}

std::string SessionManager::createSession(const std::string& enginePath, Color humanColor,
                                          int skillLevel) {
    auto session = std::make_unique<GameSessionServer>(enginePath, humanColor, skillLevel);
    if (!session->start()) {
        return "";
    }

    std::string id = generateSessionId();

    std::lock_guard<std::mutex> lock(mutex_);
    sessions_[id] = std::move(session);
    return id;
}

GameSessionServer* SessionManager::get(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(id);
    return it != sessions_.end() ? it->second.get() : nullptr;
}

void SessionManager::remove(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(id);
}

} // namespace chess::server