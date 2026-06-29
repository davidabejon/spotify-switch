#pragma once
#include <pu/Plutonium>
#include <SpotifyAuth.hpp>
#include <LocalServer.hpp>
#include <memory>

class MainLayout : public pu::ui::Layout {
private:
    pu::ui::elm::TextBlock::Ref statusText;

public:
    MainLayout();
    PU_SMART_CTOR(MainLayout)

    void SetStatus(const std::string& text);
};

class MainApplication : public pu::ui::Application {
private:
    MainLayout::Ref mainLayout;
    pu::ui::Layout::Ref loginLayout;
    std::unique_ptr<LocalServer> localServer;

public:
    using Application::Application;
    PU_SMART_CTOR(MainApplication)

    void OnLoad() override;
    void OnLoginSuccess(const spotify::Tokens& tokens);
};
