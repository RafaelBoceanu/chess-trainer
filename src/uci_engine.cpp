#include "chess/uci_engine.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace chess {

    // Platform specific process plumbing lives behind this struct so the header
    // stays clean and the rest of the project never sees a Handle or pid_t
    struct UciEngine::Impl {
    #ifdef _WIN32
        HANDLE childStdinWrite = nullptr;
        HANDLE childStdoutRead = nullptr;
        PROCESS_INFORMATION procInfo{};
        bool running = false;
        std::string readBuffer;
    #else
        int toEngine = -1;
        int fromEngine = -1;
        pid_t pid = -1;
        bool running = false;
        std::string readBuffer;
    #endif
    };

    UciEngine::UciEngine() : impl_(std::make_unique<Impl>()) {}

    UciEngine::~UciEngine() {
        stop();
    }

    bool UciEngine::isRunning() const {
        return impl_->running;
    }

    #ifdef _WIN32

    bool UciEngine::start(const std::string& enginePath) {
        stop();

        SECURITY_ATTRIBUTES sa{};
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = nullptr;

        HANDLE stdinRead = nullptr;
        HANDLE stdoutWrite = nullptr;

        if (!CreatePipe(&stdinRead, &impl_->childStdinWrite, &sa, 0)) {
            return false;
        }
        if (!SetHandleInformation(impl_->childStdinWrite, HANDLE_FLAG_INHERIT, 0)) {
            CloseHandle(stdinRead);
            CloseHandle(impl_->childStdinWrite);
            return false;
        }
        if (!CreatePipe(&impl_->childStdoutRead, &stdoutWrite, &sa, 0)) {
            CloseHandle(stdinRead);
            CloseHandle(impl_->childStdinWrite);
            return false;
        }
        if (!SetHandleInformation(impl_->childStdoutRead, HANDLE_FLAG_INHERIT, 0)) {
            CloseHandle(stdinRead);
            CloseHandle(stdoutWrite);
            return false;
        }

        STARTUPINFOA si{};
        si.cb = sizeof(si);
        si.hStdInput = stdinRead;
        si.hStdOutput = stdoutWrite;
        si.hStdError = stdoutWrite;
        si.dwFlags |= STARTF_USESTDHANDLES;

        std::string cmd = enginePath;
        std::vector<char> cmdBuf(cmd.begin(), cmd.end());
        cmdBuf.push_back('\0');

        BOOL ok = CreateProcessA(nullptr, cmdBuf.data(), nullptr, nullptr, TRUE,
                                 CREATE_NO_WINDOW, nullptr, nullptr, &si, &impl_->procInfo);
                                
        CloseHandle(stdinRead);
        CloseHandle(stdoutWrite);

        if (!ok) {
            CloseHandle(impl_->childStdinWrite);
            CloseHandle(impl_->childStdoutRead);
            return false;
        }

        impl_->running = true;

        send("uci");
        // Drain everything up to uciok so the id and option lines do not pollute
        // the buffer for the next command
        for (;;) {
            std::string line = readLine();
            if (line.empty() || line == "uciok") {
                break;
            }
        }

        return waitForReady();
    }

    void UciEngine::stop() {
        if (!impl_->running) {
            return;
        }

        send("quit");
        WaitForSingleObject(impl_->procInfo.hProcess, 1000);
        TerminateProcess(impl_->procInfo.hProcess, 0);

        CloseHandle(impl_->procInfo.hProcess);
        CloseHandle(impl_->procInfo.hThread);
        CloseHandle(impl_->childStdinWrite);
        CloseHandle(impl_->childStdoutRead);

        impl_->running = false;
    }

    void UciEngine::send(const std::string& command) {
        if (!impl_->running) {
            return;
        }
        std::string data = command + "\n";
        DWORD written = 0;
        WriteFile(impl_->childStdinWrite, data.c_str(),
                  static_cast<DWORD>(data.size()), &written, nullptr);
    }

    std::string UciEngine::readLine() {
        if (!impl_->running) {
            return "";
        }

        for (;;) {
            size_t newlinePos = impl_->readBuffer.find('\n');
            if (newlinePos != std::string::npos) {
                std::string line = impl_->readBuffer.substr(0, newlinePos);
                impl_->readBuffer.erase(0, newlinePos + 1);
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                return line;
            }

            std::array<char, 4096> buf{};
            DWORD read = 0;
            BOOL ok = ReadFile(impl_->childStdoutRead, buf.data(),
                               static_cast<DWORD>(buf.size()), &read, nullptr);
            if (!ok || read == 0) {
                return "";
            }
            impl_->readBuffer.append(buf.data(), read);
        }
    }

    #else

    bool UciEngine::start(const std::string& enginePath) {
        stop();

        int inPipe[2]; // Parent writes to inPipe[1], child reads inPipe[0]
        int outPipe[2]; // Child writes to outPipe[1], parent reads outPipe[0]

        if (pipe(inPipe) != 0) {
            return false;
        }
        if (pipe(outPipe) != 0) {
            close(inPipe[0]);
            close(inPipe[1]);
            return false;
        }

        pid_t pid = fork();
        if (pid < 0) {
            close(inPipe[0]);
            close(inPipe[1]);
            close(outPipe[0]);
            close(outPipe[1]);
            return false;
        }

        if (pid == 0) {
            // Child: wire the pipes to stdin/stdout and exec the engine
            dup2(inPipe[0], STDIN_FILENO);
            dup2(outPipe[1], STDOUT_FILENO);
            dup2(outPipe[1], STDERR_FILENO);

            close(inPipe[0]);
            close(inPipe[1]);
            close(outPipe[0]);
            close(outPipe[1]);

            execlp(enginePath.c_str(), enginePath.c_str(), nullptr);
            // Only reached if exec failed
            _exit(127);
        }

        close(inPipe[0]);
        close(outPipe[1]);

        impl_->toEngine = inPipe[1];
        impl_->fromEngine = outPipe[0];
        impl_->pid = pid;
        impl_->running = true;

        send("uci");
        for (;;) {
            std::string line = readLine();
            if (line.empty() || line == "uciok") {
                break;
            }
        }

        return waitForReady();
    }

    void UciEngine::stop() {
        if (!impl_->running) {
            return;
        }

        send("quit");

        close(impl_->toEngine);
        impl_->toEngine = -1;

        // Give the engine a moment to exit cleanly before forcing it
        int status = 0;
        for (int i = 0; i < 100; ++i) {
            pid_t result = waitpid(impl_->pid, &status, WNOHANG);
            if (result == impl_->pid) {
                impl_->pid = -1;
                break;
            }
            usleep(10000);
        }
        if (impl_->pid != -1) {
            kill(impl_->pid, SIGKILL);
            waitpid(impl_->pid, &status, 0);
            impl_->pid = -1;
        }

        close(impl_->fromEngine);
        impl_->fromEngine = -1;
        impl_->running = false;
    }

    void UciEngine::send(const std::string& command) {
        if (!impl_->running) {
            return;
        }
        std::string data = command + "\n";
        ssize_t written = write(impl_->toEngine, data.c_str(), data.size());
        (void)written;
    }

    std::string UciEngine::readLine() {
        if (!impl_->running) {
            return "";
        }

        for (;;) {
            size_t newlinePos = impl_->readBuffer.find('\n');
            if (newlinePos != std::string::npos) {
                std::string line = impl_->readBuffer.substr(0, newlinePos);
                impl_->readBuffer.erase(0, newlinePos + 1);
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                return line;
            }

            std::array<char, 4096> buf{};
            ssize_t read = ::read(impl_->fromEngine, buf.data(), buf.size());
            if (read <= 0) {
                return "";
            }
            impl_->readBuffer.append(buf.data(), static_cast<size_t>(read));
        }
    }

    #endif

    bool UciEngine::waitForReady() {
        send("isready");
        for (int i = 0; i < 200; ++i) {
            std::string line = readLine();
            if (line == "readyok") {
                return true;
            }
            if (line.empty()) {
                return false;
            }
        }
        return false;
    }

    void UciEngine::setOption(const std::string& name, const std::string& value) {
        send("setoption name " + name + " value " + value);
        waitForReady();
    }

    void UciEngine::newGame() {
        send("ucinewgame");
        waitForReady();
    }

    EngineLine UciEngine::analyse(const std::string& fen, int depth) {
        EngineLine result;
        if (!impl_->running) {
            return result;
        }

        send("position fen " + fen);
        send("go depth " + std::to_string(depth));

        // Keep the last complete info line seen. Stockfish emits one per depth,
        // so the final one before bestmove is the deepest
        for (;;) {
            std::string line = readLine();
            if (line.empty()) {
                break;
            }

            if (line.rfind("info", 0) == 0) {
                std::istringstream iss(line);
                std::string token;

                int scoreCp = result.scoreCp;
                int mateIn = 0;
                bool isMate = false;
                std::vector<Move> pv;
                bool sawScore = false;
                bool sawPv = false;

                while (iss >> token) {
                    if (token == "score") {
                        std::string kind;
                        iss >> kind;
                        if (kind == "cp") {
                            iss >> scoreCp;
                            isMate = false;
                            sawScore = true;
                        } else if (kind == "mate") {
                            iss >> mateIn;
                            isMate = true;
                            sawScore = true;
                        }
                    } else if (token == "pv") {
                        std::string moveStr;
                        while (iss >> moveStr) {
                            Move m = moveFromUci(moveStr);
                            if (m.isNull()) {
                                break;
                            }
                            pv.push_back(m);
                        }
                        sawPv = true;
                    }
                }

                // Ignore lower-bound / upper-bound partial lines that carry no pv
                if (sawScore && sawPv && !pv.empty()) {
                    result.scoreCp = scoreCp;
                    result.mateIn = mateIn;
                    result.isMate = isMate;
                    result.pv = pv;
                    result.bestMove = pv.front();
                }
            } else if (line.rfind("bestmove", 0) == 0) {
                std::istringstream iss(line);
                std::string token, moveStr;
                iss >> token >> moveStr;
                Move m = moveFromUci(moveStr);
                if (!m.isNull()) {
                    result.bestMove = m;
                }
                break;
            }
        }

        return result;
    }

    Move UciEngine::getBestMove(const std::string& fen, int movetimeMs) {
        if (!impl_->running) {
            return Move{};
        }

        send("position fen " + fen);
        send("go movetime " + std::to_string(movetimeMs));

        for (;;) {
            std::string line = readLine();
            if (line.empty()) {
                return Move{};
            }
            if (line.rfind("bestmove", 0) == 0) {
                std::istringstream iss(line);
                std::string token, moveStr;
                iss >> token >> moveStr;
                return moveFromUci(moveStr);
            } 
        }
    }
} // namespace chess