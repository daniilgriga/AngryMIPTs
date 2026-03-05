#include "ui/game_scene.hpp"

#include "data/logger.hpp"

#include <algorithm>
#include <filesystem>
#include <string>

namespace angry
{
namespace
{

std::string resolveProjectPath( const std::filesystem::path& relativePath )
{
    if ( std::filesystem::exists( relativePath ) )
    {
        return relativePath.string();
    }

#ifdef ANGRY_MIPTS_SOURCE_DIR
    const std::filesystem::path fromSourceDir =
        std::filesystem::path( ANGRY_MIPTS_SOURCE_DIR ) / relativePath;
    if ( std::filesystem::exists( fromSourceDir ) )
    {
        return fromSourceDir.string();
    }
#endif

    return relativePath.string();
}

}  // namespace

WorldSnapshot GameScene::make_mock_snapshot()
{
    WorldSnapshot snap{};
    snap.score = 0;
    snap.shotsRemaining = 3;
    snap.totalShots = 3;
    snap.status = LevelStatus::Running;
    snap.stars = 0;
    snap.physicsStepMs = 0.f;

    // slingshot
    snap.slingshot.basePx = {200.f, 550.f};
    snap.slingshot.pullOffsetPx = {0.f, 0.f};
    snap.slingshot.maxPullPx = 120.f;
    snap.slingshot.canShoot = true;
    snap.slingshot.nextProjectile = ProjectileType::Standard;

    EntityId id = 1;

    // ground
    snap.objects.push_back ( {id++, ObjectSnapshot::Kind::Block,
                              {640.f, 700.f}, 0.f, {1280.f, 40.f}, 0.f,
                              Material::Stone, ProjectileType::Standard, 1.f, true} );

    // left pillar
    snap.objects.push_back ( {id++, ObjectSnapshot::Kind::Block,
                              {800.f, 580.f}, 0.f, {20.f, 100.f}, 0.f,
                              Material::Wood, ProjectileType::Standard, 1.f, true} );

    // right pillar
    snap.objects.push_back ( {id++, ObjectSnapshot::Kind::Block,
                              {900.f, 580.f}, 0.f, {20.f, 100.f}, 0.f,
                              Material::Wood, ProjectileType::Standard, 1.f, true} );

    // top beam
    snap.objects.push_back ( {id++, ObjectSnapshot::Kind::Block,
                              {850.f, 520.f}, 0.f, {140.f, 20.f}, 0.f,
                              Material::Wood, ProjectileType::Standard, 0.8f, true} );

    // glass block on top
    snap.objects.push_back ( {id++, ObjectSnapshot::Kind::Block,
                              {850.f, 500.f}, 0.f, {60.f, 20.f}, 0.f,
                              Material::Glass, ProjectileType::Standard, 1.f, true} );

    // target (circle)
    snap.objects.push_back ( {id++, ObjectSnapshot::Kind::Target,
                              {850.f, 560.f}, 0.f, {0.f, 0.f}, 15.f,
                              Material::Wood, ProjectileType::Standard, 1.f, true} );

    return snap;
}

GameScene::GameScene ( const sf::Font& font )
    : snapshot_ ( make_mock_snapshot() )
    , font_ ( font )
    , hud_text_ ( font_, "", 20 )
{
    hud_text_.setFillColor ( sf::Color::White );
    hud_text_.setPosition ( {20.f, 20.f} );
}

void GameScene::load_level ( int level_id, const std::string& scores_path )
{
    level_id_ = level_id;
    scores_path_ = scores_path;
    pending_scene_ = SceneId::None;

    try
    {
        const std::string path = resolveProjectPath (
            "levels/level_0" + std::to_string ( level_id ) + ".json" );
        const LevelData level = level_loader_.load ( path );
        current_meta_ = level.meta;
        physics_.registerLevel ( level );
        physics_.loadLevel ( level );
        snapshot_ = physics_.getSnapshot();
        frame_clock_.restart();
        Logger::info ( "GameScene: loaded level {}", level_id );
    }
    catch ( const std::exception& e )
    {
        Logger::error ( "GameScene: failed to load level {}: {}", level_id, e.what() );
    }
}

void GameScene::retry()
{
    load_level ( level_id_, scores_path_ );
}

void GameScene::finish_level()
{
    const bool won = ( snapshot_.status == LevelStatus::Win );
    const int score = snapshot_.score;

    int stars = 0;
    if ( won && current_meta_.id > 0 )
    {
        if ( score >= current_meta_.star3Threshold )
            stars = 3;
        else if ( score >= current_meta_.star2Threshold )
            stars = 2;
        else if ( score >= current_meta_.star1Threshold )
            stars = 1;
    }

    last_result_ = { won, score, stars };

    if ( !scores_path_.empty() && level_id_ > 0 )
    {
        try
        {
            score_saver_.saveScore ( scores_path_, level_id_, score, stars );
        }
        catch ( const std::exception& e )
        {
            Logger::error ( "GameScene: failed to save score: {}", e.what() );
        }
    }

    pending_scene_ = SceneId::Result;
}

SceneId GameScene::handle_input ( const sf::Event& event )
{
    if ( pending_scene_ != SceneId::None )
    {
        SceneId next = pending_scene_;
        pending_scene_ = SceneId::None;
        return next;
    }

    if ( const auto* key = event.getIf<sf::Event::KeyPressed>() )
    {
        if ( key->code == sf::Keyboard::Key::Backspace )
            return SceneId::Menu;
    }

    if ( snapshot_.status == LevelStatus::Running )
    {
        auto cmd = slingshot_.handle_input ( event, snapshot_.slingshot );
        if ( cmd.has_value() )
            command_queue_.push ( *cmd );
    }

    return SceneId::None;
}

void GameScene::update()
{
    if ( snapshot_.status != LevelStatus::Running )
    {
        if ( pending_scene_ == SceneId::None )
            finish_level();
        return;
    }

    const float dt = std::clamp ( frame_clock_.restart().asSeconds(), 0.0f, 1.0f / 30.0f );
    physics_.processCommands ( command_queue_ );
    physics_.step ( dt );
    snapshot_ = physics_.getSnapshot();

    hud_text_.setString ( "Score: " + std::to_string ( snapshot_.score )
                          + "   [Backspace] Menu" );
}

void GameScene::render ( sf::RenderWindow& window )
{
    renderer_.draw_snapshot ( window, snapshot_ );
    slingshot_.render ( window, snapshot_.slingshot );
    renderer_.draw_hud ( window, snapshot_, hud_text_ );
}

}  // namespace angry
