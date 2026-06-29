#pragma once
#include <pu/Plutonium>
#include <SpotifyAuth.hpp>
#include <functional>

class LoginLayout : public pu::ui::Layout {
public:
    using OnLoginSuccessCallback = std::function<void(const spotify::Tokens&)>;

private:
    std::string codeVerifier;
    OnLoginSuccessCallback onLoginSuccess;

    pu::ui::elm::TextBlock::Ref titleText;
    pu::ui::elm::TextBlock::Ref step1Text;
    pu::ui::elm::TextBlock::Ref urlText;
    pu::ui::elm::TextBlock::Ref step2Text;
    pu::ui::elm::TextBlock::Ref statusText;
    pu::ui::elm::Button::Ref enterCodeBtn;

public:
    LoginLayout(const std::string& authUrl,
                const std::string& verifier,
                OnLoginSuccessCallback cb);
    PU_SMART_CTOR(LoginLayout)

    void OnEnterCode();
};
