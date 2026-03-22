#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <chrono>
#include <cstdio>
#include <deque>
#include <string>

struct LogEntry {
    std::string text;
    float timestamp;  // seconds since resetClock
};

// Stream buffer that prepends elapsed-time timestamps to every line
// AND captures complete lines into a ring buffer for in-game display.
// Install once at program start:
//     TimestampBuf tsBuf(std::cout.rdbuf());
//     std::cout.rdbuf(&tsBuf);
class TimestampBuf : public std::streambuf {
    std::streambuf* original;
    bool atLineStart = true;
    static inline std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    // In-game log ring buffer
    std::string currentLine;
    static constexpr int MAX_ENTRIES = 200;
    static inline std::deque<LogEntry> entries;

    float elapsedSeconds() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<float>(now - start).count();
    }

    int overflow(int c) override {
        if (atLineStart && c != EOF && c != '\n') {
            auto now = std::chrono::steady_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
            char buf[32];
            std::snprintf(buf, sizeof(buf), "[%3lld.%03lld] ", (long long)(ms / 1000), (long long)(ms % 1000));
            for (char* p = buf; *p; ++p) original->sputc(*p);
            atLineStart = false;
        }
        if (c == '\n') {
            atLineStart = true;
            // Push completed line to ring buffer
            if (!currentLine.empty()) {
                entries.push_back({currentLine, elapsedSeconds()});
                if ((int)entries.size() > MAX_ENTRIES) entries.pop_front();
                currentLine.clear();
            }
        } else if (c != EOF) {
            currentLine += static_cast<char>(c);
        }
        return original->sputc(c);
    }

    int sync() override {
        return original->pubsync();
    }

public:
    TimestampBuf(std::streambuf* orig) : original(orig) {}

    // Call when game starts — all players reset to 0.000 at the same logical moment
    static void resetClock() {
        start = std::chrono::steady_clock::now();
        entries.clear();
    }

    static const std::deque<LogEntry>& getEntries() { return entries; }
    static float currentTime() {
        return std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
    }
};

#endif
