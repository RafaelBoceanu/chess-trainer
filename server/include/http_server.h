#pragma once

#include "session_manager.h"

#include <string>

namespace chess::server {

    struct ServerConfig {
        std::string enginePath = "stockfish";
        int port = 8080;
        int reviewDepth = 16;
    };

    // Owns the HTTP server and routes requests into the SessionManager. Blocks
    // the calling thread until stopped
    class HttpServer {
        public:
            explicit HttpServer(ServerConfig config);

            void run();

        private:
            ServerConfig config_;
            SessionManager sessions_;
    };

} // namespace chess::server