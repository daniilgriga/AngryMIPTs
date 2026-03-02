#include "render/renderer.hpp"

#include <algorithm>
#include <cmath>

namespace angry
{

void Renderer::draw_hud ( sf::RenderWindow& window, const WorldSnapshot& snapshot,
                          sf::Text& score_text )
{
    score_text.setPosition ( {20.f, 16.f} );
    window.draw ( score_text );

    const float radius = 10.f;
    const float spacing = 26.f;
    const float base_x = 20.f + radius;
    const float base_y = static_cast<float> ( window.getSize().y ) - 30.f;

    const int total = snapshot.totalShots;
    const int remaining = snapshot.shotsRemaining;

    for ( int i = 0; i < total; ++i )
    {
        sf::CircleShape icon ( radius );
        icon.setOrigin ( {radius, radius} );
        icon.setPosition ( {base_x + i * spacing, base_y} );

        if ( i < remaining )
        {
            icon.setFillColor ( sf::Color ( 50, 50, 50 ) );
            icon.setOutlineThickness ( 0.f );
        }
        else
        {
            icon.setFillColor ( sf::Color::Transparent );
            icon.setOutlineColor ( sf::Color ( 100, 100, 100 ) );
            icon.setOutlineThickness ( 2.f );
        }

        window.draw ( icon );
    }
}

void Renderer::draw_snapshot ( sf::RenderWindow& window, const WorldSnapshot& snapshot )
{
    draw_background ( window );
    draw_slingshot ( window, snapshot.slingshot );

    for ( const auto& obj : snapshot.objects )
    {
        if ( obj.isActive )
            draw_object ( window, obj );
    }
}

void Renderer::draw_background ( sf::RenderWindow& window )
{
    const auto ws = sf::Vector2f ( window.getSize() );

    // grass strip at the bottom
    sf::RectangleShape grass ( {ws.x, 40.f} );
    grass.setPosition ( {0.f, ws.y - 40.f} );
    grass.setFillColor ( sf::Color ( 80, 160, 60 ) );
    window.draw ( grass );

    // darker ground below grass
    sf::RectangleShape ground ( {ws.x, 20.f} );
    ground.setPosition ( {0.f, ws.y - 20.f} );
    ground.setFillColor ( sf::Color ( 60, 110, 40 ) );
    window.draw ( ground );
}

void Renderer::draw_object ( sf::RenderWindow& window, const ObjectSnapshot& obj )
{
    const bool uses_hp = ( obj.kind == ObjectSnapshot::Kind::Block
                           || obj.kind == ObjectSnapshot::Kind::Target );

    if ( obj.radiusPx > 0.f )
    {
        sf::CircleShape shape ( obj.radiusPx );
        shape.setOrigin ( {obj.radiusPx, obj.radiusPx} );
        shape.setPosition ( {obj.positionPx.x, obj.positionPx.y} );
        shape.setRotation ( sf::degrees ( obj.angleDeg ) );

        sf::Color color = ( obj.kind == ObjectSnapshot::Kind::Block )
                              ? material_color ( obj.material )
                              : kind_color ( obj.kind );
        if ( uses_hp )
            color = tint_by_hp ( color, obj.hpNormalized );
        shape.setFillColor ( color );

        window.draw ( shape );
    }
    else
    {
        sf::RectangleShape shape ( {obj.sizePx.x, obj.sizePx.y} );
        shape.setOrigin ( {obj.sizePx.x / 2.f, obj.sizePx.y / 2.f} );
        shape.setPosition ( {obj.positionPx.x, obj.positionPx.y} );
        shape.setRotation ( sf::degrees ( obj.angleDeg ) );

        sf::Color color;
        if ( obj.kind == ObjectSnapshot::Kind::Target
             || obj.kind == ObjectSnapshot::Kind::Projectile )
            color = kind_color ( obj.kind );
        else
            color = material_color ( obj.material );
        if ( uses_hp )
            color = tint_by_hp ( color, obj.hpNormalized );
        shape.setFillColor ( color );

        window.draw ( shape );
    }
}

void Renderer::draw_slingshot ( sf::RenderWindow& window, const SlingshotState& sling )
{
    sf::RectangleShape post ( {10.f, 60.f} );
    post.setOrigin ( {5.f, 60.f} );
    post.setPosition ( {sling.basePx.x, sling.basePx.y} );
    post.setFillColor ( sf::Color ( 101, 67, 33 ) );
    window.draw ( post );

    sf::RectangleShape left_prong ( {6.f, 25.f} );
    left_prong.setOrigin ( {3.f, 25.f} );
    left_prong.setPosition ( {sling.basePx.x - 8.f, sling.basePx.y - 55.f} );
    left_prong.setFillColor ( sf::Color ( 101, 67, 33 ) );
    window.draw ( left_prong );

    sf::RectangleShape right_prong ( {6.f, 25.f} );
    right_prong.setOrigin ( {3.f, 25.f} );
    right_prong.setPosition ( {sling.basePx.x + 8.f, sling.basePx.y - 55.f} );
    right_prong.setFillColor ( sf::Color ( 101, 67, 33 ) );
    window.draw ( right_prong );
}

sf::Color Renderer::material_color ( Material mat )
{
    switch ( mat )
    {
    case Material::Wood:
        return sf::Color ( 180, 120, 60 );
    case Material::Stone:
        return sf::Color ( 150, 150, 150 );
    case Material::Glass:
        return sf::Color ( 170, 220, 240, 180 );
    case Material::Ice:
        return sf::Color ( 200, 230, 255, 200 );
    default:
        return sf::Color::White;
    }
}

sf::Color Renderer::kind_color ( ObjectSnapshot::Kind kind )
{
    switch ( kind )
    {
    case ObjectSnapshot::Kind::Target:
        return sf::Color ( 220, 50, 50 );
    case ObjectSnapshot::Kind::Projectile:
        return sf::Color ( 50, 50, 50 );
    case ObjectSnapshot::Kind::Debris:
        return sf::Color ( 120, 120, 120 );
    default:
        return sf::Color::White;
    }
}

sf::Color Renderer::tint_by_hp ( sf::Color base, float hp )
{
    const float t = std::clamp ( hp, 0.f, 1.f );
    return sf::Color (
        static_cast<uint8_t> ( base.r * t + 80.f * ( 1.f - t ) ),
        static_cast<uint8_t> ( base.g * t ),
        static_cast<uint8_t> ( base.b * t ),
        base.a
    );
}

}  // namespace angry
