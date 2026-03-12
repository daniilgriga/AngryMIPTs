#include "ui/menu_scene.hpp"

#include <cstdint>
#include <cmath>

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

void draw_soft_glow ( sf::RenderWindow& window, sf::Vector2f pos, float radius,
                      sf::Color color )
{
    sf::CircleShape glow ( radius );
    glow.setOrigin ( {radius, radius} );
    glow.setPosition ( pos );
    glow.setFillColor ( color );
    window.draw ( glow );
}

}  // namespace

MenuScene::MenuScene ( const sf::Font& font, AccountService& accounts )
    : accounts_ ( accounts )
    , font_( font )
    , title_( font_, "AngryMipts", 64 )
    , prompt_( font_, "Press Enter to start", 24 )
{
    title_.setFillColor ( sf::Color::White );
    prompt_.setFillColor ( sf::Color ( 230, 245, 255 ) );
}

SceneId MenuScene::handle_input ( const sf::Event& event )
{
    if ( const auto* key = event.getIf<sf::Event::KeyPressed>() )
    {
        if ( key->code == sf::Keyboard::Key::Enter )
            return SceneId::LevelSelect;
    }
    return SceneId::None;
}

void MenuScene::update()
{
}

void MenuScene::render( sf::RenderWindow& window )
{
    static sf::Clock anim_clock;
    const float t = anim_clock.getElapsedTime().asSeconds();
    const float pulse = 0.5f + 0.5f * std::sin ( t * 2.1f );
    const auto window_size = sf::Vector2f ( window.getSize() );

    draw_vertical_gradient ( window, sf::Color ( 18, 30, 60 ), sf::Color ( 44, 112, 120 ) );

    draw_soft_glow ( window,
                     {window_size.x * 0.82f + std::sin ( t * 0.22f ) * 40.f,
                      window_size.y * 0.20f},
                     210.f, sf::Color ( 255, 240, 190, 45 ) );
    draw_soft_glow ( window,
                     {window_size.x * 0.23f + std::sin ( t * 0.35f ) * 55.f,
                      window_size.y * 0.30f + std::cos ( t * 0.43f ) * 22.f},
                     250.f, sf::Color ( 160, 210, 255, 24 ) );
    draw_soft_glow ( window,
                     {window_size.x * 0.70f + std::cos ( t * 0.31f ) * 35.f,
                      window_size.y * 0.68f},
                     320.f, sf::Color ( 110, 170, 225, 20 ) );

    sf::RectangleShape panel_shadow ( {window_size.x * 0.60f, window_size.y * 0.44f} );
    panel_shadow.setOrigin ( {panel_shadow.getSize().x * 0.5f, panel_shadow.getSize().y * 0.5f} );
    panel_shadow.setPosition ( {window_size.x * 0.5f + 8.f, window_size.y * 0.43f + 10.f} );
    panel_shadow.setFillColor ( sf::Color ( 5, 10, 18, 110 ) );
    window.draw ( panel_shadow );

    sf::RectangleShape panel ( {window_size.x * 0.60f, window_size.y * 0.44f} );
    panel.setOrigin ( {panel.getSize().x * 0.5f, panel.getSize().y * 0.5f} );
    panel.setPosition ( {window_size.x * 0.5f, window_size.y * 0.43f} );
    panel.setFillColor ( sf::Color ( 8, 15, 28, 138 ) );
    panel.setOutlineThickness ( 2.5f );
    panel.setOutlineColor ( sf::Color ( 200, 230, 255, 120 ) );
    window.draw ( panel );

    sf::RectangleShape panel_accent ( {panel.getSize().x - 8.f, 6.f} );
    panel_accent.setOrigin ( {panel_accent.getSize().x * 0.5f, 0.f} );
    panel_accent.setPosition ( {panel.getPosition().x, panel.getPosition().y - panel.getSize().y * 0.5f + 6.f} );
    panel_accent.setFillColor ( sf::Color ( 130, 195, 255, 145 ) );
    window.draw ( panel_accent );

    title_.setCharacterSize ( 74 );
    title_.setStyle ( sf::Text::Bold );
    title_.setFillColor ( sf::Color ( 242, 248, 255 ) );
    title_.setOutlineThickness ( 3.f );
    title_.setOutlineColor ( sf::Color ( 22, 34, 58, 170 ) );
    auto title_bounds = title_.getLocalBounds();
    title_.setOrigin ( {title_bounds.position.x + title_bounds.size.x / 2.f,
                        title_bounds.position.y + title_bounds.size.y / 2.f} );
    title_.setPosition (
        {window_size.x / 2.f, window_size.y * 0.33f + std::sin ( t * 1.1f ) * 3.5f} );

    const uint8_t prompt_alpha =
        static_cast<uint8_t> ( 185.f + 70.f * pulse );
    prompt_.setFillColor ( sf::Color ( 232, 244, 255, prompt_alpha ) );
    prompt_.setStyle ( sf::Text::Regular );
    auto prompt_bounds = prompt_.getLocalBounds();
    prompt_.setOrigin ( {prompt_bounds.position.x + prompt_bounds.size.x / 2.f,
                         prompt_bounds.position.y + prompt_bounds.size.y / 2.f} );
    prompt_.setPosition ( {window_size.x / 2.f, window_size.y * 0.54f} );

    window.draw ( title_ );
    window.draw ( prompt_ );
}

}  // namespace angry
