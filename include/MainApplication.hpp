#pragma once
#include <pu/Plutonium>
#include <SpotifyAuth.hpp>
#include <LocalServer.hpp>
#include <memory>
#include <functional>
#include <ctime>

class MainLayout : public pu::ui::Layout {
private:
    pu::ui::elm::TextBlock::Ref statusText;
    pu::ui::elm::TextBlock::Ref trackText;
    pu::ui::elm::TextBlock::Ref deviceText;

    std::function<void()> refreshCallback;
    time_t lastRefresh = 0;

    void OnRenderCallback();

public:
    MainLayout();
    PU_SMART_CTOR(MainLayout)

    void SetStatus(const std::string& text);
    void SetTrack(const std::string& trackName, const std::string& artistName, bool isPlaying);
    void SetDevice(const std::string& deviceName);
    // Called once after the initial fetch so the periodic timer starts from that moment.
    void SetRefreshCallback(std::function<void()> fn);
};

class MainApplication : public pu::ui::Application {
private:
    MainLayout::Ref mainLayout;
    pu::ui::Layout::Ref loginLayout;
    std::unique_ptr<LocalServer> localServer;
    spotify::Tokens currentTokens;

    void FetchAndShowPlayerState();

public:
    using Application::Application;
    PU_SMART_CTOR(MainApplication)

    void OnLoad() override;
    void OnLoginSuccess(const spotify::Tokens& tokens);
};
