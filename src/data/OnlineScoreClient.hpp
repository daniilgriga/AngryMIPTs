#pragma once

#include <string>
#include <vector>

namespace angry
{

struct LeaderboardEntry
{
    std::string playerName;
    int score = 0;
    int stars = 0;
};

class OnlineScoreClient
{
public:
    explicit OnlineScoreClient(std::string baseUrl = "http://127.0.0.1:8080");

    bool submitScore(const std::string& playerName, int levelId, int score, int stars);
    std::vector<LeaderboardEntry> fetchLeaderboard(int levelId);

private:
    std::string baseUrl_;
};

}  // namespace angry
