#include "ui/game_scene.hpp"
#include "ui/level_select_scene.hpp"
#include "ui/menu_scene.hpp"
#include "ui/result_scene.hpp"
#include "ui/scene_manager.hpp"

#include <SFML/Graphics.hpp>

#include <filesystem>
#include <iostream>
#include <string>

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

int main()
{
    const std::string fontPath = resolveProjectPath( "assets/fonts/liberation_sans.ttf" );
    const std::string windowTitle = "AngryMipts";

    sf::Font font;
    if ( !font.openFromFile ( fontPath ) )
    {
        std::cerr << "Failed to load font from: " << fontPath << std::endl;
        return 1;
    }

    sf::RenderWindow window;
    bool isFullscreen = false;
    sf::Vector2u windowedSize {1280u, 720u};

    auto recreateWindow = [&] ( bool fullscreen )
    {
        if ( fullscreen )
        {
            window.create ( sf::VideoMode::getDesktopMode(), windowTitle, sf::State::Fullscreen );
        }
        else
        {
            window.create ( sf::VideoMode ( windowedSize ), windowTitle,
                            sf::Style::Default, sf::State::Windowed );
        }

        window.setFramerateLimit ( 60 );
        isFullscreen = fullscreen;
    };

    recreateWindow ( true );

    auto level_select = std::make_unique<angry::LevelSelectScene> ( font );
    level_select->load_data ( resolveProjectPath ( "levels" ),
                              resolveProjectPath ( "scores.json" ) );

    angry::SceneManager scenes;
    scenes.add_scene ( angry::SceneId::Menu, std::make_unique<angry::MenuScene> ( font ) );
    scenes.add_scene ( angry::SceneId::LevelSelect, std::move ( level_select ) );
    scenes.add_scene ( angry::SceneId::Game, std::make_unique<angry::GameScene> ( font ) );
    scenes.add_scene ( angry::SceneId::Result, std::make_unique<angry::ResultScene> ( font ) );
    scenes.switch_to ( angry::SceneId::Menu );

    while ( window.isOpen() )
    {
        while ( const auto event = window.pollEvent() )
        {
            if ( event->is<sf::Event::Closed>() )
                window.close();

            if ( const auto* resized = event->getIf<sf::Event::Resized>() )
            {
                if ( !isFullscreen )
                    windowedSize = resized->size;
            }

            if ( const auto* key = event->getIf<sf::Event::KeyPressed>() )
            {
                const bool toggleFullscreen =
                    ( key->code == sf::Keyboard::Key::F11 )
                    || ( key->code == sf::Keyboard::Key::Enter && key->alt );
                if ( toggleFullscreen )
                {
                    if ( !isFullscreen )
                    {
                        windowedSize = window.getSize();
                    }
                    recreateWindow ( !isFullscreen );
                    continue;
                }

                if ( key->code == sf::Keyboard::Key::Escape )
                {
                    if ( isFullscreen )
                    {
                        recreateWindow ( false );
                        continue;
                    }
                    window.close();
                }
            }

            scenes.handle_input ( *event );
        }

        scenes.update();

        window.clear ( sf::Color ( 135, 206, 235 ) );
        scenes.render ( window );
        window.display();
    }

    return 0;
}
