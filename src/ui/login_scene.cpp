#include "ui/login_scene.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace angry
{
namespace
{

constexpr float kCardW    = 480.f;
constexpr float kCardH    = 420.f;
constexpr float kFieldW   = 360.f;
constexpr float kFieldH   = 46.f;
constexpr int   kMaxLen   = 24;

void draw_gradient ( sf::RenderWindow& window, sf::Color top, sf::Color bottom )
{
    const sf::Vector2f ws ( window.getSize() );
    sf::Vertex bg[] = {
        {{0.f, 0.f},    top},
        {{ws.x, 0.f},   top},
        {{ws.x, ws.y},  bottom},
        {{0.f, ws.y},   bottom},
    };
    window.draw ( bg, 4, sf::PrimitiveType::TriangleFan );
}

void draw_glow ( sf::RenderWindow& window, sf::Vector2f pos, float r, sf::Color c )
{
    sf::CircleShape g ( r );
    g.setOrigin ( {r, r} );
    g.setPosition ( pos );
    g.setFillColor ( c );
    window.draw ( g );
}

sf::Color status_color ( int kind )
{
    // 0=none, 1=pending, 2=error, 3=success
    switch ( kind )
    {
    case 1: return sf::Color ( 200, 220, 255 );
    case 2: return sf::Color ( 255, 120, 120 );
    case 3: return sf::Color ( 100, 220, 140 );
    default: return sf::Color ( 180, 200, 230 );
    }
}

}  // namespace

LoginScene::LoginScene ( const sf::Font& font, AccountService& accounts )
    : accounts_ ( accounts )
    , font_ ( font )
{
}

void LoginScene::do_login()
{
    if ( username_buf_.empty() || password_buf_.empty() )
    {
        status_msg_  = "Enter username and password";
        status_kind_ = StatusKind::Error;
        status_clock_.restart();
        return;
    }
    status_msg_  = "Logging in...";
    status_kind_ = StatusKind::Pending;
    status_clock_.restart();

    const AuthResult r = accounts_.login ( username_buf_, password_buf_ );
    if ( r.success )
    {
        status_msg_  = "Login successful";
        status_kind_ = StatusKind::Success;
    }
    else
    {
        status_msg_  = r.errorMessage.empty() ? "Invalid username or password"
                                              : r.errorMessage;
        status_kind_ = StatusKind::Error;
    }
    status_clock_.restart();
}

void LoginScene::do_register()
{
    if ( username_buf_.empty() || password_buf_.empty() )
    {
        status_msg_  = "Enter username and password";
        status_kind_ = StatusKind::Error;
        status_clock_.restart();
        return;
    }
    status_msg_  = "Creating account...";
    status_kind_ = StatusKind::Pending;
    status_clock_.restart();

    const AuthResult r = accounts_.registerAndLogin ( username_buf_, password_buf_ );
    if ( r.success )
    {
        status_msg_  = "Registration successful";
        status_kind_ = StatusKind::Success;
    }
    else
    {
        status_msg_  = r.errorMessage.empty() ? "Registration failed" : r.errorMessage;
        status_kind_ = StatusKind::Error;
    }
    status_clock_.restart();
}

SceneId LoginScene::handle_input ( const sf::Event& event )
{
    if ( const auto* key = event.getIf<sf::Event::KeyPressed>() )
    {
        if ( key->code == sf::Keyboard::Key::Tab )
        {
            focus_ = ( focus_ == FocusField::Username )
                         ? FocusField::Password
                         : FocusField::Username;
            return SceneId::None;
        }

        if ( key->code == sf::Keyboard::Key::Backspace )
        {
            auto& buf = ( focus_ == FocusField::Username ) ? username_buf_ : password_buf_;
            if ( !buf.empty() )
                buf.pop_back();
            return SceneId::None;
        }

        if ( key->code == sf::Keyboard::Key::Enter )
        {
            if ( focus_ == FocusField::Username )
            {
                focus_ = FocusField::Password;
                return SceneId::None;
            }
            do_login();
            if ( status_kind_ == StatusKind::Success )
                return SceneId::Menu;
            return SceneId::None;
        }

        // F1 = Register, F2 = Guest, for keyboard-only fallback
        if ( key->code == sf::Keyboard::Key::F1 )
        {
            do_register();
            if ( status_kind_ == StatusKind::Success )
                return SceneId::Menu;
            return SceneId::None;
        }

        if ( key->code == sf::Keyboard::Key::F2 )
            return SceneId::Menu;
    }

    if ( const auto* text = event.getIf<sf::Event::TextEntered>() )
    {
        const uint32_t ch = text->unicode;
        // Only printable ASCII, skip backspace (handled above)
        if ( ch >= 32u && ch < 127u )
        {
            auto& buf = ( focus_ == FocusField::Username ) ? username_buf_ : password_buf_;
            if ( static_cast<int> ( buf.size() ) < kMaxLen )
                buf += static_cast<char> ( ch );
        }
    }

    return SceneId::None;
}

void LoginScene::update()
{
}

void LoginScene::render ( sf::RenderWindow& window )
{
    const sf::Vector2f ws ( window.getSize() );
    const float        t  = anim_clock_.getElapsedTime().asSeconds();
    const float        pulse = 0.5f + 0.5f * std::sin ( t * 2.1f );

    // Background
    draw_gradient ( window, sf::Color ( 14, 22, 48 ), sf::Color ( 30, 70, 90 ) );
    draw_glow ( window,
                {ws.x * 0.20f + std::sin ( t * 0.28f ) * 50.f, ws.y * 0.25f},
                280.f, sf::Color ( 80, 140, 255, 20 ) );
    draw_glow ( window,
                {ws.x * 0.82f + std::cos ( t * 0.22f ) * 40.f, ws.y * 0.70f},
                240.f, sf::Color ( 255, 200, 80, 16 ) );

    // ---- Left decorative zone ----
    {
        sf::Text logo ( font_, "AngryMipts", 52 );
        logo.setStyle ( sf::Text::Bold );
        logo.setFillColor ( sf::Color ( 242, 248, 255 ) );
        logo.setOutlineThickness ( 2.5f );
        logo.setOutlineColor ( sf::Color ( 20, 30, 55, 180 ) );
        const auto lb = logo.getLocalBounds();
        logo.setOrigin ( {lb.position.x, lb.position.y + lb.size.y / 2.f} );
        logo.setPosition ( {ws.x * 0.07f, ws.y * 0.28f + std::sin ( t * 1.1f ) * 2.5f} );
        window.draw ( logo );

        const char* bullets[] = {
            "Save progress locally",
            "Submit scores online",
            "Compete in leaderboards",
        };
        for ( int i = 0; i < 3; ++i )
        {
            sf::Text b ( font_, std::string ( "  " ) + bullets[i], 18 );
            b.setFillColor ( sf::Color ( 160, 200, 240,
                                         static_cast<uint8_t> ( 160 + 60 * pulse ) ) );
            const auto bb = b.getLocalBounds();
            b.setOrigin ( {bb.position.x, bb.position.y} );
            b.setPosition ( {ws.x * 0.07f,
                              ws.y * 0.42f + static_cast<float> ( i ) * 30.f} );
            window.draw ( b );
        }
    }

    // ---- Auth card ----
    const float card_cx = ws.x * 0.68f;
    const float card_cy = ws.y * 0.50f;

    {
        sf::RectangleShape shadow ( {kCardW + 8.f, kCardH + 8.f} );
        shadow.setOrigin ( {( kCardW + 8.f ) * 0.5f, ( kCardH + 8.f ) * 0.5f} );
        shadow.setPosition ( {card_cx + 6.f, card_cy + 8.f} );
        shadow.setFillColor ( sf::Color ( 4, 8, 18, 100 ) );
        window.draw ( shadow );

        sf::RectangleShape card ( {kCardW, kCardH} );
        card.setOrigin ( {kCardW * 0.5f, kCardH * 0.5f} );
        card.setPosition ( {card_cx, card_cy} );
        card.setFillColor ( sf::Color ( 10, 18, 36, 200 ) );
        card.setOutlineThickness ( 2.f );
        card.setOutlineColor ( sf::Color ( 80, 140, 220, 110 ) );
        window.draw ( card );

        // top accent bar
        sf::RectangleShape accent ( {kCardW - 8.f, 5.f} );
        accent.setOrigin ( {( kCardW - 8.f ) * 0.5f, 0.f} );
        accent.setPosition ( {card_cx, card_cy - kCardH * 0.5f + 5.f} );
        accent.setFillColor ( sf::Color ( 80, 160, 255, 160 ) );
        window.draw ( accent );
    }

    const float top_y = card_cy - kCardH * 0.5f;

    // Card title
    {
        sf::Text welcome ( font_, "Welcome", 30 );
        welcome.setStyle ( sf::Text::Bold );
        welcome.setFillColor ( sf::Color ( 236, 246, 255 ) );
        const auto wb = welcome.getLocalBounds();
        welcome.setOrigin ( {wb.position.x + wb.size.x / 2.f,
                             wb.position.y + wb.size.y / 2.f} );
        welcome.setPosition ( {card_cx, top_y + 38.f} );
        window.draw ( welcome );

        sf::Text sub ( font_, "Sign in to compete online", 15 );
        sub.setFillColor ( sf::Color ( 130, 175, 220 ) );
        const auto sb = sub.getLocalBounds();
        sub.setOrigin ( {sb.position.x + sb.size.x / 2.f,
                         sb.position.y + sb.size.y / 2.f} );
        sub.setPosition ( {card_cx, top_y + 66.f} );
        window.draw ( sub );
    }

    // Helper: draw a labelled text field
    auto draw_field = [&] ( const std::string& label, const std::string& buf,
                            bool is_password, float cy_field, bool focused )
    {
        const float fx = card_cx - kFieldW * 0.5f;
        const float fy = cy_field - kFieldH * 0.5f;

        // Label
        sf::Text lbl ( font_, label, 14 );
        lbl.setFillColor ( sf::Color ( 140, 185, 230 ) );
        lbl.setPosition ( {fx, fy - 20.f} );
        window.draw ( lbl );

        // Field background
        sf::RectangleShape bg ( {kFieldW, kFieldH} );
        bg.setPosition ( {fx, fy} );
        bg.setFillColor ( sf::Color ( 18, 28, 52, 200 ) );
        bg.setOutlineThickness ( focused ? 2.f : 1.f );
        bg.setOutlineColor ( focused ? sf::Color ( 80, 160, 255, 200 )
                                     : sf::Color ( 60, 90, 140, 120 ) );
        window.draw ( bg );

        // Text content
        std::string display = is_password ? std::string ( buf.size(), '*' ) : buf;

        // Blinking caret
        const bool caret_on = focused
            && ( static_cast<int> ( caret_clock_.getElapsedTime().asSeconds() * 2.f ) % 2 == 0 );
        if ( caret_on )
            display += '|';

        sf::Text content ( font_, display.empty() && !caret_on
                               ? ( is_password ? "Password" : "Username" )
                               : display,
                           18 );
        const bool is_placeholder = display.empty() && !caret_on;
        content.setFillColor ( is_placeholder ? sf::Color ( 80, 110, 160 )
                                              : sf::Color ( 225, 240, 255 ) );
        const auto cb = content.getLocalBounds();
        content.setOrigin ( {cb.position.x, cb.position.y + cb.size.y / 2.f} );
        content.setPosition ( {fx + 12.f, cy_field} );
        window.draw ( content );
    };

    const float field1_y = top_y + 130.f;
    const float field2_y = top_y + 215.f;

    draw_field ( "Username", username_buf_, false, field1_y, focus_ == FocusField::Username );
    draw_field ( "Password", password_buf_, true,  field2_y, focus_ == FocusField::Password );

    // Helper: draw a button, returns the rect for click detection
    auto draw_button = [&] ( const std::string& label, float cx, float cy,
                              float w, float h, sf::Color fill, sf::Color text_color )
    {
        sf::RectangleShape btn ( {w, h} );
        btn.setOrigin ( {w * 0.5f, h * 0.5f} );
        btn.setPosition ( {cx, cy} );
        btn.setFillColor ( fill );
        btn.setOutlineThickness ( 1.5f );
        btn.setOutlineColor ( sf::Color ( 255, 255, 255, 40 ) );
        window.draw ( btn );

        sf::Text txt ( font_, label, 17 );
        txt.setStyle ( sf::Text::Bold );
        txt.setFillColor ( text_color );
        const auto tb = txt.getLocalBounds();
        txt.setOrigin ( {tb.position.x + tb.size.x / 2.f,
                         tb.position.y + tb.size.y / 2.f} );
        txt.setPosition ( {cx, cy} );
        window.draw ( txt );
    };

    const float btn_y   = top_y + 295.f;
    const float btn_w   = 164.f;
    const float btn_h   = 42.f;
    const float gap     = 12.f;

    draw_button ( "Login",    card_cx - btn_w * 0.5f - gap * 0.5f, btn_y,
                  btn_w, btn_h,
                  sf::Color ( 210, 135, 30, 230 ), sf::Color::White );
    draw_button ( "Register", card_cx + btn_w * 0.5f + gap * 0.5f, btn_y,
                  btn_w, btn_h,
                  sf::Color ( 30, 100, 190, 220 ), sf::Color::White );

    // Guest link
    {
        sf::Text guest ( font_, "Continue as Guest  (F2)", 15 );
        guest.setFillColor ( sf::Color ( 130, 175, 230,
                                          static_cast<uint8_t> ( 150 + 80 * pulse ) ) );
        const auto gb = guest.getLocalBounds();
        guest.setOrigin ( {gb.position.x + gb.size.x / 2.f,
                           gb.position.y + gb.size.y / 2.f} );
        guest.setPosition ( {card_cx, top_y + 355.f} );
        window.draw ( guest );
    }

    // Status message
    if ( !status_msg_.empty() )
    {
        const int kind = static_cast<int> ( status_kind_ );
        sf::Text st ( font_, status_msg_, 16 );
        st.setFillColor ( status_color ( kind ) );
        const auto sb = st.getLocalBounds();
        st.setOrigin ( {sb.position.x + sb.size.x / 2.f,
                        sb.position.y + sb.size.y / 2.f} );
        st.setPosition ( {card_cx, top_y + 393.f} );
        window.draw ( st );
    }

    // Bottom hint
    {
        sf::Text hint ( font_, "[Tab] switch field   [Enter] login   [F1] register", 13 );
        hint.setFillColor ( sf::Color ( 100, 140, 190, 140 ) );
        const auto hb = hint.getLocalBounds();
        hint.setOrigin ( {hb.position.x + hb.size.x / 2.f,
                          hb.position.y + hb.size.y / 2.f} );
        hint.setPosition ( {ws.x * 0.5f, ws.y * 0.94f} );
        window.draw ( hint );
    }
}

}  // namespace angry
