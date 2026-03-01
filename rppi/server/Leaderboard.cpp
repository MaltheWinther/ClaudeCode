#include "Leaderboard.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <cstdlib>   // getenv

std::string Leaderboard::path() {
    const char* home = std::getenv("HOME");
    return std::string(home ? home : ".") + "/.rppi_leaderboard.json";
}

std::vector<LeaderboardEntry> Leaderboard::load() {
    std::vector<LeaderboardEntry> entries;
    std::ifstream f(path());
    if (!f.is_open()) return entries;

    try {
        json j;
        f >> j;
        for (const auto& e : j) {
            entries.push_back({
                e.value("players",        ""),
                e.value("elapsed_secs",   0),
                e.value("levels_reached", 1),
                e.value("won",            false),
                e.value("date",           "")
            });
        }
    } catch (const std::exception& ex) {
        std::cerr << "[Leaderboard] Parse error: " << ex.what() << "\n";
    }
    return entries;
}

void Leaderboard::addEntry(const LeaderboardEntry& e) {
    auto entries = load();
    entries.push_back(e);

    json arr = json::array();
    for (const auto& entry : entries) {
        arr.push_back({
            {"players",        entry.players},
            {"elapsed_secs",   entry.elapsedSecs},
            {"levels_reached", entry.levelsReached},
            {"won",            entry.won},
            {"date",           entry.date}
        });
    }

    std::ofstream f(path());
    if (f.is_open())
        f << arr.dump(2);
    else
        std::cerr << "[Leaderboard] Could not write to " << path() << "\n";
}

std::vector<LeaderboardEntry> Leaderboard::topN(int n) {
    auto entries = load();

    std::sort(entries.begin(), entries.end(),
              [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
        if (a.won != b.won)              return a.won > b.won;           // won first
        if (a.levelsReached != b.levelsReached)
                                         return a.levelsReached > b.levelsReached;
        return a.elapsedSecs < b.elapsedSecs;  // faster is better
    });

    if ((int)entries.size() > n)
        entries.resize(n);
    return entries;
}
