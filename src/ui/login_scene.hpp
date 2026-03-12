#pragma once
#include "data/account_service.hpp"
#include "scene.hpp"

#include <string>

namespace angry
{

class LoginScene : public Scene
{
public:
    explicit LoginScene ( const sf::Font& font, AccountService& accounts );

    SceneId handle_input ( const sf::Event& event ) override;
    void    update() override;
    void    render ( sf::RenderWindow& window ) override;

private:
    enum class FocusField { Username, Password };
    enum class StatusKind { None, Pending, Error, Success };

    void do_login();
    void do_register();

    AccountService& accounts_;

    sf::Font font_;

    std::string username_buf_;
    std::string password_buf_;
    FocusField  focus_       = FocusField::Username;

    std::string  status_msg_;
    StatusKind   status_kind_ = StatusKind::None;
    sf::Clock    status_clock_;   // time since last status set
    sf::Clock    caret_clock_;    // drives blinking caret
    sf::Clock    anim_clock_;     // background animation
};

}  // namespace angry
