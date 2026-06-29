#pragma once
#include <pu/Plutonium>
#include <SpotifyAuth.hpp>
#include <LocalServer.hpp>
#include <functional>

class LoginLayout : public pu::ui::Layout {
public:
    using OnLoginSuccessCallback = std::function<void(const spotify::Tokens&)>;

private:
    std::string codeVerifier;
    OnLoginSuccessCallback onLoginSuccess;
    LocalServer* server;
    bool exchangePending;
    std::string pendingCode;

    pu::ui::elm::TextBlock::Ref titleText;
    pu::ui::elm::TextBlock::Ref step1Text;
    pu::ui::elm::TextBlock::Ref urlText;
    pu::ui::elm::TextBlock::Ref step2Text;
    pu::ui::elm::TextBlock::Ref statusText;
    pu::ui::elm::Image::Ref qrImage;
    pu::ui::elm::TextBlock::Ref scanHintText;

    static pu::sdl2::TextureHandle::Ref buildQrTexture(const std::string& url, s32* outSize);
    void OnRenderCallback();

public:
    LoginLayout(const std::string& authUrl,
                const std::string& verifier,
                LocalServer* srv,
                OnLoginSuccessCallback cb);
    PU_SMART_CTOR(LoginLayout)
};
