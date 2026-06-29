#pragma once
#include <pu/Plutonium>
#include <SpotifyAuth.hpp>
#include <LocalServer.hpp>
#include <memory>
#include <functional>
#include <ctime>
#include <string>

enum class Tab      { Player, Favorites };
enum class RightTab  { Artist, Queue };

class MainLayout : public pu::ui::Layout {
private:
    // Sidebar
    pu::ui::elm::Rectangle::Ref sidebarBg;
    pu::ui::elm::TextBlock::Ref sidebarTitle;
    pu::ui::elm::Rectangle::Ref tabIndicator;
    pu::ui::elm::Rectangle::Ref tab1Bg;
    pu::ui::elm::TextBlock::Ref tab1Text;
    pu::ui::elm::Rectangle::Ref tab2Bg;
    pu::ui::elm::TextBlock::Ref tab2Text;
    pu::ui::elm::TextBlock::Ref statusText;
    pu::ui::elm::TextBlock::Ref deviceText;

    // Player tab
    pu::ui::elm::Rectangle::Ref albumArtBg;
    pu::ui::elm::Image::Ref albumArtImage;
    pu::ui::elm::TextBlock::Ref trackText;
    pu::ui::elm::TextBlock::Ref artistText;
    pu::ui::elm::Rectangle::Ref prevBtnBg;
    pu::ui::elm::TextBlock::Ref prevBtnText;
    pu::ui::elm::Rectangle::Ref playBtnBg;
    pu::ui::elm::TextBlock::Ref playBtnText;
    pu::ui::elm::Rectangle::Ref nextBtnBg;
    pu::ui::elm::TextBlock::Ref nextBtnText;

    // Favorites tab
    pu::ui::elm::TextBlock::Ref favText;

    // Right panel
    pu::ui::elm::Rectangle::Ref rightVertSep;
    pu::ui::elm::Rectangle::Ref rightTab1Bg;
    pu::ui::elm::TextBlock::Ref rightTab1Text;
    pu::ui::elm::Rectangle::Ref rightTab2Bg;
    pu::ui::elm::TextBlock::Ref rightTab2Text;
    pu::ui::elm::Rectangle::Ref rightTabIndicator;
    pu::ui::elm::Rectangle::Ref rightHorizSep;
    pu::ui::elm::TextBlock::Ref artistPlaceholder;
    pu::ui::elm::TextBlock::Ref queuePlaceholder;

    // State
    Tab currentTab;
    RightTab currentRightTab;
    std::function<void()> refreshCallback;
    time_t lastRefresh = 0;

    void OnRenderCallback();
    void SetPlayerTabVisible(bool visible);
    void SetFavoritesTabVisible(bool visible);
    void SetRightPanelVisible(bool visible);

public:
    MainLayout();
    PU_SMART_CTOR(MainLayout)

    void SetStatus(const std::string& text);
    void SetTrack(const std::string& trackName, const std::string& artistName, bool isPlaying);
    void SetDevice(const std::string& deviceName);
    void SetAlbumArt(pu::sdl2::TextureHandle::Ref handle);
    void UpdatePlayButton(bool isPlaying);
    void SwitchToTab(Tab tab);
    Tab GetCurrentTab() const { return this->currentTab; }
    void SwitchRightTab(RightTab tab);
    RightTab GetCurrentRightTab() const { return this->currentRightTab; }
    void SetRefreshCallback(std::function<void()> fn);
};

class MainApplication : public pu::ui::Application {
private:
    MainLayout::Ref mainLayout;
    pu::ui::Layout::Ref loginLayout;
    std::unique_ptr<LocalServer> localServer;
    spotify::Tokens currentTokens;
    bool mainLayoutActive = false;
    bool isPlaying = false;
    std::string currentAlbumUrl;

    void FetchAndShowPlayerState();
    void OnPlayPause();
    void OnPrev();
    void OnNext();

public:
    using Application::Application;
    PU_SMART_CTOR(MainApplication)

    void OnLoad() override;
    void OnLoginSuccess(const spotify::Tokens& tokens);
};
