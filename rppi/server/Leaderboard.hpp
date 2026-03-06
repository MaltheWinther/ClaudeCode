#pragma once

#include <vector>
#include "../common/Messages.hpp"

// Persistent leaderboard stored at ~/.rppi_leaderboard.json
// All methods are static — no instance needed.
class Leaderboard {
public:
    static std::vector<LeaderboardEntry> load();
    static void                          addEntry(const LeaderboardEntry& e);
    // Returns top N entries for a game type, sorted appropriately
    static std::vector<LeaderboardEntry> topN(int n = 5,
                                              const std::string& gameType = "gyropi");

private:
    static std::string path();
};
