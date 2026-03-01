#pragma once

#include <vector>
#include "../common/Messages.hpp"

// Persistent leaderboard stored at ~/.rppi_leaderboard.json
// All methods are static — no instance needed.
class Leaderboard {
public:
    static std::vector<LeaderboardEntry> load();
    static void                          addEntry(const LeaderboardEntry& e);
    // Returns top N entries sorted: won desc → levelsReached desc → elapsedSecs asc
    static std::vector<LeaderboardEntry> topN(int n = 5);

private:
    static std::string path();
};
