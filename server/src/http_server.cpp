#include "http_server.h"

#include "chess/movegen.h"
#include "chess/pgn.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <iostream>

namespace chess::server {

using json = nlohmann::json;

namespace {

Color colorFromString(const std::string& s) {
    return s == "black" ? Color::Black : Color::White;
}

std::string colorToString(Color c) {
    return c == Color::White ? "white" : "black";
}

std::string resultTag(GameResult r) {
    switch (r) {
        case GameResult::Ongoing:                  return "ongoing";
        case GameResult::WhiteWins:                return "white_wins";
        case GameResult::BlackWins:                return "black_wins";
        case GameResult::DrawStalemate:             return "draw_stalemate";
        case GameResult::DrawFiftyMove:              return "draw_fifty_move";
        case GameResult::DrawRepetition:             return "draw_repetition";
        case GameResult::DrawInsufficientMaterial:   return "draw_insufficient_material";
    }
    return "unknown";
}

json legalMovesJson(Board& board) {
    json out = json::object();
    for (const Move& m : generateLegalMoves(board)) {
        out[squareToString(m.from)].push_back(squareToString(m.to));
    }
    return out;
}

json gameStateJson(GameSessionServer& session) {
    Board boardCopy = session.board();
    json j;
    j["fen"] = session.board().toFen();
    j["sideToMove"] = colorToString(session.board().sideToMove());
    j["humanColor"] = colorToString(session.humanColor());
    j["result"] = resultTag(session.result());
    j["inCheck"] = session.board().isInCheck(session.board().sideToMove());
    j["legalMoves"] = legalMovesJson(boardCopy);

    json moves = json::array();
    Board replay;
    for (const Move& m : session.moveHistory()) {
        moves.push_back(moveToSan(replay, m));
        replay.makeMove(m);
    }
    j["moveHistorySan"] = moves;

    return j;
}

json reviewedMoveJson(const ReviewedMove& rm) {
    json j;
    j["moveNumber"] = rm.moveNumber;
    j["side"] = colorToString(rm.side);
    j["fen"] = rm.fen;
    j["fenAfter"] = rm.fenAfter;
    j["playedSan"] = rm.playedSan;
    j["bestSan"] = rm.bestSan;
    j["evalBefore"] = rm.evalBefore;
    j["evalAfter"] = rm.evalAfter;
    j["centipawnLoss"] = rm.centipawnLoss;
    j["quality"] = qualityToString(rm.quality);
    j["bestLineSan"] = rm.bestLineSan;
    return j;
}

json gameReviewJson(const GameReview& review) {
    json moves = json::array();
    for (const ReviewedMove& rm : review.moves) {
        moves.push_back(reviewedMoveJson(rm));
    }

    json summary;
    summary["whiteAvgLoss"] = review.whiteAvgLoss;
    summary["blackAvgLoss"] = review.blackAvgLoss;
    summary["whiteBlunders"] = review.whiteBlunders;
    summary["blackBlunders"] = review.blackBlunders;
    summary["whiteMistakes"] = review.whiteMistakes;
    summary["blackMistakes"] = review.blackMistakes;
    summary["whiteInaccuracies"] = review.whiteInaccuracies;
    summary["blackInaccuracies"] = review.blackInaccuracies;

    json j;
    j["moves"] = moves;
    j["summary"] = summary;
    return j;
}

void setCommonHeaders(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

} // namespace

HttpServer::HttpServer(ServerConfig config) : config_(std::move(config)) {}

void HttpServer::run() {
    httplib::Server svr;

    svr.set_pre_routing_handler([](const httplib::Request&, httplib::Response& res) {
        setCommonHeaders(res);
        return httplib::Server::HandlerResponse::Unhandled;
    });

    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
    });

    svr.Post("/api/game/new", [this](const httplib::Request& req, httplib::Response& res) {
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"error":"invalid json"})", "application/json");
            return;
        }

        std::string colorStr = body.value("color", "white");
        int skill = body.value("skill", 5);
        skill = std::max(0, std::min(20, skill));

        std::string id = sessions_.createSession(config_.enginePath, colorFromString(colorStr),
                                                  skill);
        if (id.empty()) {
            res.status = 500;
            res.set_content(
                R"({"error":"failed to start engine, check the --engine path"})",
                "application/json");
            return;
        }

        GameSessionServer* session = sessions_.get(id);
        json j = gameStateJson(*session);
        j["sessionId"] = id;

        res.set_content(j.dump(), "application/json");
    });

    svr.Get(R"(/api/game/(\w+)/state)", [this](const httplib::Request& req,
                                               httplib::Response& res) {
        std::string id = req.matches[1];
        GameSessionServer* session = sessions_.get(id);
        if (!session) {
            res.status = 404;
            res.set_content(R"({"error":"session not found"})", "application/json");
            return;
        }
        res.set_content(gameStateJson(*session).dump(), "application/json");
    });

    svr.Post(R"(/api/game/(\w+)/move)", [this](const httplib::Request& req,
                                               httplib::Response& res) {
        std::string id = req.matches[1];
        GameSessionServer* session = sessions_.get(id);
        if (!session) {
            res.status = 404;
            res.set_content(R"({"error":"session not found"})", "application/json");
            return;
        }

        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"error":"invalid json"})", "application/json");
            return;
        }

        std::string from = body.value("from", "");
        std::string to = body.value("to", "");
        std::string promo = body.value("promotion", "");

        Move m;
        m.from = squareFromString(from);
        m.to = squareFromString(to);
        if (!promo.empty()) {
            m.promotion = promotionFromChar(promo[0]);
        }

        if (m.from == NO_SQUARE || m.to == NO_SQUARE) {
            res.status = 400;
            res.set_content(R"({"error":"invalid square"})", "application/json");
            return;
        }

        bool ok = session->applyHumanMove(m);
        if (!ok) {
            res.status = 422;
            res.set_content(R"({"error":"illegal move"})", "application/json");
            return;
        }

        res.set_content(gameStateJson(*session).dump(), "application/json");
    });

    svr.Post(R"(/api/game/(\w+)/review)", [this](const httplib::Request& req,
                                                 httplib::Response& res) {
        std::string id = req.matches[1];
        GameSessionServer* session = sessions_.get(id);
        if (!session) {
            res.status = 404;
            res.set_content(R"({"error":"session not found"})", "application/json");
            return;
        }

        GameReview review = session->runReview(config_.reviewDepth);
        res.set_content(gameReviewJson(review).dump(), "application/json");
    });

    std::cout << "Chess server listening on http://localhost:" << config_.port << "\n";
    std::cout << "Engine: " << config_.enginePath << "\n";
    svr.listen("0.0.0.0", config_.port);
}

} // namespace chess::server