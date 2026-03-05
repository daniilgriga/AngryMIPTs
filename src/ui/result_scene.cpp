#include "ui/result_scene.hpp"

#include <cstdint>
#include <cmath>
#include <string>

namespace angry
{
namespace
{

void draw_vertical_gradient ( sf::RenderWindow& window,
                              sf::Color top, sf::Color bottom )
{
    const sf::Vector2f ws ( window.getSize() );
    sf::Vertex background[] = {
        {{0.f, 0.f}, top},
        {{ws.x, 0.f}, top},
        {{ws.x, ws.y}, bottom},
        {{0.f, ws.y}, bottom},
    };
    window.draw ( background, 4, sf::PrimitiveType::TriangleFan );
}

}  // namespace

ResultScene::ResultScene ( const sf::Font& font )
    : font_ ( font )
    , title_ ( font_, "", 48 )
    , score_text_ ( font_, "", 28 )
    , stars_text_ ( font_, "", 36 )
    , prompt_ ( font_, "[Enter] Retry   [Backspace] Menu", 20 )
{
    title_.setFillColor ( sf::Color::White );
    score_text_.setFillColor ( sf::Color::White );
    stars_text_.setFillColor ( sf::Color ( 255, 215, 0 ) );
    prompt_.setFillColor ( sf::Color ( 230, 245, 255 ) );
}

void ResultScene::set_result ( const LevelResult& result )
{
    result_ = result;

    title_.setString ( result_.win ? "Level Complete!" : "Level Failed" );
    title_.setFillColor ( result_.win ? sf::Color ( 50, 200, 50 ) : sf::Color ( 200, 50, 50 ) );

    score_text_.setString ( "Score: " + std::to_string ( result_.score ) );

    std::string star_str;
    for ( int i = 0; i < 3; ++i )
        star_str += ( i < result_.stars ) ? "* " : "- ";
    stars_text_.setString ( star_str );
}

SceneId ResultScene::handle_input ( const sf::Event& event )
{
    if ( const auto* key = event.getIf<sf::Event::KeyPressed>() )
    {
        if ( key->code == sf::Keyboard::Key::Enter )
            return SceneId::Game;
        if ( key->code == sf::Keyboard::Key::Backspace )
            return SceneId::Menu;
    }
    return SceneId::None;
}

void ResultScene::update()
{
}

void ResultScene::render ( sf::RenderWindow& window )
{
    static sf::Clock anim_clock;
    const float t = anim_clock.getElapsedTime().asSeconds();
    const float pulse = 0.5f + 0.5f * std::sin ( t * 2.8f );
    auto ws = sf::Vector2f ( window.getSize() );

    if ( result_.win )
    {
        draw_vertical_gradient ( window, sf::Color ( 18, 42, 64 ), sf::Color ( 42, 96, 118 ) );
    }
    else
    {
        draw_vertical_gradient ( window, sf::Color ( 34, 22, 34 ), sf::Color ( 88, 38, 58 ) );
    }

    sf::CircleShape glow ( 240.f );
    glow.setOrigin ( {240.f, 240.f} );
    glow.setPosition ( {ws.x * 0.78f + std::sin ( t * 0.35f ) * 32.f, ws.y * 0.20f} );
    glow.setFillColor ( result_.win ? sf::Color ( 255, 236, 156, 42 )
                                    : sf::Color ( 255, 162, 176, 36 ) );
    window.draw ( glow );

    sf::RectangleShape panel ( {ws.x * 0.52f, ws.y * 0.52f} );
    panel.setOrigin ( {panel.getSize().x * 0.5f, panel.getSize().y * 0.5f} );
    panel.setPosition ( {ws.x * 0.5f, ws.y * 0.47f} );
    panel.setFillColor ( sf::Color ( 10, 15, 30, 145 ) );
    panel.setOutlineThickness ( 2.5f );
    panel.setOutlineColor ( result_.win ? sf::Color ( 210, 236, 255, 138 )
                                        : sf::Color ( 255, 195, 210, 132 ) );
    window.draw ( panel );

    sf::RectangleShape panel_accent ( {panel.getSize().x - 8.f, 6.f} );
    panel_accent.setOrigin ( {panel_accent.getSize().x * 0.5f, 0.f} );
    panel_accent.setPosition ( {panel.getPosition().x, panel.getPosition().y - panel.getSize().y * 0.5f + 7.f} );
    panel_accent.setFillColor ( result_.win ? sf::Color ( 132, 232, 186, 160 )
                                            : sf::Color ( 255, 148, 168, 152 ) );
    window.draw ( panel_accent );

    auto center_text = [&] ( sf::Text& text, float y )
    {
        auto bounds = text.getLocalBounds();
        text.setOrigin ( {bounds.position.x + bounds.size.x / 2.f,
                          bounds.position.y + bounds.size.y / 2.f} );
        text.setPosition ( {ws.x / 2.f, y} );
    };

    title_.setStyle ( sf::Text::Bold );
    title_.setOutlineThickness ( 2.f );
    title_.setOutlineColor ( sf::Color ( 10, 16, 28, 160 ) );
    center_text ( title_, ws.y * 0.25f + std::sin ( t * 1.4f ) * 2.f );

    stars_text_.setFillColor ( result_.win ? sf::Color ( 255, 226, 104 )
                                           : sf::Color ( 245, 194, 204 ) );
    score_text_.setFillColor ( sf::Color ( 236, 246, 255 ) );
    prompt_.setFillColor ( sf::Color ( 230, 245, 255,
                                       static_cast<uint8_t> ( 182.f + 70.f * pulse ) ) );

    center_text ( stars_text_, ws.y * 0.40f );
    center_text ( score_text_, ws.y * 0.52f );
    center_text ( prompt_, ws.y * 0.70f );

    window.draw ( title_ );
    window.draw ( stars_text_ );
    window.draw ( score_text_ );
    window.draw ( prompt_ );
}

}  // namespace angry
