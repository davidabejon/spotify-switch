#pragma once
#include <pu/Plutonium>
#include <SpotifyAuth.hpp>

// Shown after successful login
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
    pu::ui::Layout::Ref loginLayout; // holds LoginLayout by base pointer

public:
    using Application::Application;
    PU_SMART_CTOR(MainApplication)

    void OnLoad() override;
    void OnLoginSuccess(const spotify::Tokens& tokens);
};
