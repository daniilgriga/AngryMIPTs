#pragma once
#include "data/account_service.hpp"
#include "data/level_loader.hpp"
#include "data/OnlineScoreClient.hpp"
#include "data/score_saver.hpp"
#include "scene.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

namespace angry
{

class LevelSelectScene : public Scene
{
private:
    AccountService* accounts_ = nullptr;
    sf::Font font_;
    sf::Text title_;
    sf::Text prompt_;
    sf::Text badge_text_;
    sf::Text badge_btn_;

    std::vector<LevelMeta>  levels_;
    std::vector<LevelScore> scores_;
    std::vector<sf::Text>   level_texts_;
    std::string             scores_path_;

    int   selected_          = 0;
    int   selected_level_id_ = -1;
    float scroll_offset_     = 0.f;

    // Hit-test rects updated each render() call
    sf::FloatRect              rect_badge_;
    sf::FloatRect              rect_right_panel_;
    sf::FloatRect              rect_left_panel_;
    std::vector<sf::FloatRect> rects_level_items_screen_; // actual screen Y per item

    // Preview panel leaderboard — async fetch per selection
    struct PreviewState
    {
        std::mutex mutex;
        int        fetched_level_id = -1;
        LeaderboardFetchStatus fetch_status = LeaderboardFetchStatus::Unavailable;
        std::vector<LeaderboardEntry> entries;
    };
    std::shared_ptr<PreviewState> preview_ = std::make_shared<PreviewState>();
    int  preview_requested_id_ = -1;   // level id currently being fetched
    float preview_scroll_      = 0.f;

    void rebuild_texts();
    const LevelScore* find_score ( int level_id ) const;
    void fetch_preview ( int level_id );

public:
    explicit LevelSelectScene ( const sf::Font& font, AccountService* accounts = nullptr );

    void load_data ( const std::string& levels_dir, const std::string& scores_path );
    void reload_scores();

    int get_selected_level_id() const { return selected_level_id_; }
    const std::string& get_scores_path() const { return scores_path_; }

    SceneId handle_input ( const sf::Event& event ) override;
    void update() override;
    void render ( sf::RenderWindow& window ) override;
};

}  // namespace angry
