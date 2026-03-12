#include "OnlineScoreClient.hpp"

#include "logger.hpp"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

namespace angry
{

using json = nlohmann::json;

namespace
{

constexpr int kBackendTimeoutMs = 3000;

}  // namespace

OnlineScoreClient::OnlineScoreClient(std::string baseUrl)
    : baseUrl_(std::move(baseUrl))
{
}

bool OnlineScoreClient::submitScore(
    const std::string& playerName,
    int levelId,
    int score,
    int stars)
{
    Logger::info("Submitting score to backend...");

    const json body = {
        {"playerName", playerName},
        {"levelId", levelId},
        {"score", score},
        {"stars", stars},
    };

    const cpr::Response response = cpr::Post(
        cpr::Url{baseUrl_ + "/scores"},
        cpr::Header{{"Content-Type", "application/json"}},
        cpr::Body{body.dump()},
        cpr::Timeout{kBackendTimeoutMs});

    if (response.error.code != cpr::ErrorCode::OK)
    {
        Logger::error("Backend unavailable.");
        return false;
    }

    const bool ok = response.status_code >= 200 && response.status_code < 300;
    if (!ok)
    {
        Logger::error("Backend unavailable.");
    }

    return ok;
}

std::vector<LeaderboardEntry> OnlineScoreClient::fetchLeaderboard(int levelId)
{
    std::vector<LeaderboardEntry> entries;

    const cpr::Response response = cpr::Get(
        cpr::Url{baseUrl_ + "/leaderboard"},
        cpr::Parameters{{"levelId", std::to_string(levelId)}},
        cpr::Timeout{kBackendTimeoutMs});

    if (response.error.code != cpr::ErrorCode::OK)
    {
        Logger::error("Backend unavailable.");
        return entries;
    }

    if (response.status_code < 200 || response.status_code >= 300)
    {
        Logger::error("Backend unavailable.");
        return entries;
    }

    const json data = json::parse(response.text, nullptr, false);
    if (!data.is_array())
    {
        Logger::error("Backend unavailable.");
        return entries;
    }

    entries.reserve(data.size());
    for (const json& item : data)
    {
        LeaderboardEntry entry;
        entry.playerName = item.value("playerName", "");
        entry.score = item.value("score", 0);
        entry.stars = item.value("stars", 0);
        entries.push_back(std::move(entry));
    }

    Logger::info("Leaderboard loaded.");
    return entries;
}

}  // namespace angry
