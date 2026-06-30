#include <MainApplication.hpp>
#include <LoginLayout.hpp>
#include <TokenStorage.hpp>
#include <SpotifyAuth.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <DebugLog.hpp>
#include <cstdio>
#include <ctime>

// --- Layout constants ---

static constexpr s32 SCREEN_W   = 1920;
static constexpr s32 SCREEN_H   = 1080;

static constexpr s32 SIDEBAR_W  = 250;
static constexpr s32 CONTENT_X  = SIDEBAR_W;
static constexpr s32 CONTENT_W  = SCREEN_W - SIDEBAR_W;
static constexpr s32 CONTENT_CX = CONTENT_X + CONTENT_W / 2; // 1085

// RIGHT_X defined early so PLAYER_CX can reference it
static constexpr s32 RIGHT_X    = 1100;

// Player block centered between sidebar and right panel left edge.
// Vertical centering: block height = ART(520) + gap(28) + track(58) + artist+gap(90) + btn(77) = 773px
// → ART_Y = (1080 - 773) / 2 = 154
static constexpr s32 ART_SIZE   = 520;
static constexpr s32 PLAYER_CX  = (SIDEBAR_W + RIGHT_X) / 2; // 675
static constexpr s32 ART_X      = PLAYER_CX - ART_SIZE / 2;  // 415
static constexpr s32 ART_Y      = 154;

static constexpr s32 TRACK_Y    = ART_Y + ART_SIZE + 28;     // 702
static constexpr s32 ARTIST_Y   = TRACK_Y + 58;              // 760

static constexpr s32 CTRL_Y     = ARTIST_Y + 90;             // 850
static constexpr s32 CTRL_SMALL = 70;
static constexpr s32 CTRL_LARGE = 84;
static constexpr s32 CTRL_GAP   = 110;

static constexpr s32 PREV_CX    = PLAYER_CX - CTRL_GAP;      // 565
static constexpr s32 PLAY_CX    = PLAYER_CX;                 // 675
static constexpr s32 NEXT_CX    = PLAYER_CX + CTRL_GAP;      // 785

static constexpr s32 TAB_H      = 64;
static constexpr s32 TAB1_Y     = 280;
static constexpr s32 TAB2_Y     = TAB1_Y + TAB_H + 4;        // 348

// Right panel — same margin on the right as the player content has from the sidebar (165 px)
static constexpr s32 RIGHT_MARGIN = ART_X - SIDEBAR_W;                  // 165
static constexpr s32 RIGHT_W      = SCREEN_W - RIGHT_X - RIGHT_MARGIN;  // 655
static constexpr s32 RIGHT_TAB_H  = 52;
static constexpr s32 RIGHT_TAB_W  = RIGHT_W / 2;                        // 327

// Right-panel horizontal geometry
static constexpr s32 RART_IMG_SIZE  = 200;
static constexpr s32 RART_IMG_X     = RIGHT_X + 28;                                // 1128
static constexpr s32 RART_INFO_X    = RART_IMG_X + RART_IMG_SIZE + 20;             // 1348
static constexpr s32 RART_INFO_W    = SCREEN_W - RIGHT_MARGIN - RART_INFO_X - 10;  // ~387
static constexpr s32 RALBUM_IMG_SIZE = 100;
static constexpr s32 RALBUM_TEXT_X   = RART_IMG_X + RALBUM_IMG_SIZE + 14;          // 1242
static constexpr s32 RALBUM_FULL_W   = SCREEN_W - RIGHT_MARGIN - RART_IMG_X - 10;  // ~617
static constexpr s32 RALBUM_TEXT_W   = RALBUM_FULL_W - RALBUM_IMG_SIZE - 14;       // ~503

// Flex-column space-around vertical layout
// Content area Y=208, H=726. Blocks: artist 200px + album 120px = 320px.
// space-around (2 items): outer=g, inner=2g → 4g = 406 → g=101
static constexpr s32 RCONT_Y         = ART_Y + RIGHT_TAB_H + 2;                    // 208
static constexpr s32 RCONT_H         = CTRL_Y + CTRL_LARGE - RCONT_Y;              // 726
static constexpr s32 RFLEX_G         = (RCONT_H - RART_IMG_SIZE - 120) / 4;        // 101
static constexpr s32 RART_BLOCK_Y    = RCONT_Y + RFLEX_G;                          // 309
static constexpr s32 RALBUM_BLOCK_Y  = RART_BLOCK_Y + RART_IMG_SIZE + 2 * RFLEX_G; // 711

// Artist info text Y (generous spacing alongside the 200px image)
static constexpr s32 RART_NAME_Y     = RART_BLOCK_Y;                               // 309
static constexpr s32 RART_GENRES_Y   = RART_NAME_Y   + 52;                         // 361
static constexpr s32 RART_FOLLOW_Y   = RART_GENRES_Y + 42;                         // 403
static constexpr s32 RART_POP_Y      = RART_FOLLOW_Y + 36;                         // 439

// Album info text Y
static constexpr s32 RALBUM_SEP_Y    = RART_BLOCK_Y + RART_IMG_SIZE + RFLEX_G;    // 610
static constexpr s32 RALBUM_IMG_Y    = RALBUM_BLOCK_Y + 24;                        // 735  (image starts below label)
static constexpr s32 RALBUM_HDR_Y    = RALBUM_BLOCK_Y - 8;                         // 703  (label a few px above image gap)
static constexpr s32 RALBUM_NAME_Y   = RALBUM_IMG_Y;                               // 735  (title aligned with image top)
static constexpr s32 RALBUM_INFO1_Y  = RALBUM_NAME_Y + 36;                         // 771
static constexpr s32 RALBUM_INFO2_Y  = RALBUM_INFO1_Y + 30;                        // 801

static constexpr time_t REFRESH_INTERVAL_SECS = 5;

static constexpr s32 SPINNER_SIZE = 128;
static constexpr s32 SPINNER_X    = PLAYER_CX - SPINNER_SIZE / 2;
static constexpr s32 SPINNER_Y    = ART_Y + ART_SIZE / 2 - SPINNER_SIZE / 2;

// --- User tab layout ---

static constexpr s32 UAVATAR_SIZE   = 200;
static constexpr s32 UBLOCK_Y       = (SCREEN_H - 250) / 2;        // 415
static constexpr s32 UAVATAR_X      = CONTENT_X + 120;             // 370
static constexpr s32 UAVATAR_Y      = UBLOCK_Y;
static constexpr s32 UINFO_X        = UAVATAR_X + UAVATAR_SIZE + 60; // 630
static constexpr s32 UNAME_Y        = UBLOCK_Y;
static constexpr s32 UEMAIL_Y       = UNAME_Y + 68;
static constexpr s32 UCOUNTRY_Y     = UEMAIL_Y + 46;
static constexpr s32 UPLAN_Y        = UCOUNTRY_Y + 46;
static constexpr s32 UFOLLOWERS_Y   = UPLAN_Y + 46;

// Queue tab card layout
static constexpr s32 RQUEUE_ITEM_H   = 110;
static constexpr s32 RQUEUE_GAP      = 14;
static constexpr s32 RQUEUE_TOTAL_H  = 5 * RQUEUE_ITEM_H + 4 * RQUEUE_GAP;  // 606
static constexpr s32 RQUEUE_START_Y  = RCONT_Y + (RCONT_H - RQUEUE_TOTAL_H) / 2;  // 268
static constexpr s32 RQUEUE_CARD_X   = RIGHT_X + 10;   // 1110
static constexpr s32 RQUEUE_CARD_W   = RIGHT_W - 20;   // 635
static constexpr s32 RQUEUE_IMG_SIZE = 80;
static constexpr s32 RQUEUE_IMG_X    = RQUEUE_CARD_X + 12;                          // 1122
static constexpr s32 RQUEUE_TEXT_X   = RQUEUE_IMG_X + RQUEUE_IMG_SIZE + 14;         // 1216
static constexpr s32 RQUEUE_TEXT_W   = RQUEUE_CARD_X + RQUEUE_CARD_W - RQUEUE_TEXT_X - 10; // ~479

// --- Colors ---

static const pu::ui::Color CLR_BG      {  18,  18,  18, 255 };
static const pu::ui::Color CLR_SIDEBAR {  28,  28,  28, 255 };
static const pu::ui::Color CLR_TAB_SEL {  48,  48,  48, 255 };
static const pu::ui::Color CLR_GREEN   {  29, 185,  84, 255 };
static const pu::ui::Color CLR_WHITE   { 255, 255, 255, 255 };
static const pu::ui::Color CLR_GRAY    { 150, 150, 150, 255 };
static const pu::ui::Color CLR_HINT    {  70,  70,  70, 255 };
static const pu::ui::Color CLR_ART_BG  {  40,  40,  40, 255 };
static const pu::ui::Color CLR_BTN     {  55,  55,  55, 255 };
static const pu::ui::Color CLR_SEP        {  50,  50,  50, 255 };
static const pu::ui::Color CLR_SPINNER_BG {   0,   0,   0, 160 };

// --- Local IP helper ---

static std::string getLocalIp() {
    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return "";

    struct sockaddr_in dest = {};
    dest.sin_family      = AF_INET;
    dest.sin_port        = htons(53);
    dest.sin_addr.s_addr = inet_addr("8.8.8.8");

    if (connect(fd, reinterpret_cast<struct sockaddr*>(&dest), sizeof(dest)) < 0) {
        close(fd);
        return "";
    }

    struct sockaddr_in local = {};
    socklen_t len = sizeof(local);
    if (getsockname(fd, reinterpret_cast<struct sockaddr*>(&local), &len) < 0) {
        close(fd);
        return "";
    }
    close(fd);

    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &local.sin_addr, buf, sizeof(buf));
    return std::string(buf);
}

static std::string encodeIp(const std::string& ip) {
    std::string out = ip;
    for (char& c : out) if (c == '.') c = '_';
    return out;
}

// =============================================================================
// MainLayout
// =============================================================================

MainLayout::MainLayout() : Layout::Layout(), currentTab(Tab::Player), currentRightTab(RightTab::Artist) {
    this->SetBackgroundColor(CLR_BG);

    // ---- Sidebar ----

    this->sidebarBg = pu::ui::elm::Rectangle::New(0, 0, SIDEBAR_W, SCREEN_H, CLR_SIDEBAR);
    this->Add(this->sidebarBg);

    // "Switch" at Medium font ≈ 90px wide — used only to center the group
    static constexpr s32 LOGO_SIZE      = 48;
    static constexpr s32 LOGO_TEXT_GAP  = 10;
    static constexpr s32 TEXT_W_EST     = 90;
    static constexpr s32 GROUP_W        = LOGO_SIZE + LOGO_TEXT_GAP + TEXT_W_EST;
    static constexpr s32 LOGO_X         = (SIDEBAR_W - GROUP_W) / 2;
    static constexpr s32 LOGO_Y         = 20;
    static constexpr s32 TITLE_X        = LOGO_X + LOGO_SIZE + LOGO_TEXT_GAP;
    static constexpr s32 TITLE_Y        = LOGO_Y + (LOGO_SIZE - 28) / 2; // vertically center text inside logo height

    auto* logoTex = pu::ui::render::LoadImageFromFile("romfs:/spotify.png");
    this->sidebarLogoImg = pu::ui::elm::Image::New(LOGO_X, LOGO_Y, logoTex ? pu::sdl2::TextureHandle::New(logoTex) : nullptr);
    this->sidebarLogoImg->SetWidth(LOGO_SIZE);
    this->sidebarLogoImg->SetHeight(LOGO_SIZE);
    this->Add(this->sidebarLogoImg);

    this->sidebarTitle = pu::ui::elm::TextBlock::New(TITLE_X, TITLE_Y, "Switch");
    this->sidebarTitle->SetColor(CLR_GREEN);
    this->sidebarTitle->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
    this->Add(this->sidebarTitle);

    // Tab 1 — Reproductor (selected by default)
    this->tab1Bg = pu::ui::elm::Rectangle::New(0, TAB1_Y, SIDEBAR_W, TAB_H, CLR_TAB_SEL);
    this->Add(this->tab1Bg);
    this->tab1Text = pu::ui::elm::TextBlock::New(28, TAB1_Y + 18, "Reproductor");
    this->tab1Text->SetColor(CLR_WHITE);
    this->tab1Text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
    this->Add(this->tab1Text);

    // Tab 2 — Usuario
    this->tab2Bg = pu::ui::elm::Rectangle::New(0, TAB2_Y, SIDEBAR_W, TAB_H, CLR_SIDEBAR);
    this->Add(this->tab2Bg);
    this->tab2Text = pu::ui::elm::TextBlock::New(28, TAB2_Y + 18, "Usuario");
    this->tab2Text->SetColor(CLR_GRAY);
    this->tab2Text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
    this->Add(this->tab2Text);

    // Green selection bar on the left edge
    this->tabIndicator = pu::ui::elm::Rectangle::New(0, TAB1_Y, 4, TAB_H, CLR_GREEN);
    this->Add(this->tabIndicator);

    // Status line (auth state) near bottom of sidebar
    this->statusText = pu::ui::elm::TextBlock::New(12, SCREEN_H - 96, "");
    this->statusText->SetColor(CLR_GRAY);
    this->statusText->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small));
    this->Add(this->statusText);

    // Device name below status
    this->deviceText = pu::ui::elm::TextBlock::New(12, SCREEN_H - 60, "");
    this->deviceText->SetColor(CLR_GRAY);
    this->deviceText->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small));
    this->Add(this->deviceText);

    // Bottom hint
    auto hint = pu::ui::elm::TextBlock::New(0, SCREEN_H - 32, "Pulsa + para salir");
    hint->SetColor(CLR_HINT);
    hint->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small));
    hint->SetX(SCREEN_W / 2 - 100);
    this->Add(hint);

    // ---- Player tab ----

    // Album art placeholder background
    this->albumArtBg = pu::ui::elm::Rectangle::New(ART_X, ART_Y, ART_SIZE, ART_SIZE, CLR_ART_BG, 12);
    this->Add(this->albumArtBg);

    // Album art image (starts empty)
    this->albumArtImage = pu::ui::elm::Image::New(ART_X, ART_Y, nullptr);
    this->albumArtImage->SetWidth(ART_SIZE);
    this->albumArtImage->SetHeight(ART_SIZE);
    this->Add(this->albumArtImage);

    // Track name
    this->trackText = pu::ui::elm::TextBlock::New(ART_X, TRACK_Y, "");
    this->trackText->SetColor(CLR_WHITE);
    this->trackText->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Large));
    this->trackText->SetClampWidth(ART_SIZE);
    this->trackText->SetClampSpeed(pu::ui::elm::TextBlock::DefaultClampSpeedSteps / 3);
    this->trackText->SetClampDelay(pu::ui::elm::TextBlock::DefaultClampStaticDelaySteps);
    this->Add(this->trackText);

    // Artist name
    this->artistText = pu::ui::elm::TextBlock::New(ART_X, ARTIST_Y, "");
    this->artistText->SetColor(CLR_GRAY);
    this->artistText->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
    this->artistText->SetClampWidth(ART_SIZE);
    this->artistText->SetClampSpeed(pu::ui::elm::TextBlock::DefaultClampSpeedSteps / 3);
    this->artistText->SetClampDelay(pu::ui::elm::TextBlock::DefaultClampStaticDelaySteps);
    this->Add(this->artistText);

    // Prev button  (circle, centered at PREV_CX)
    this->prevBtnBg = pu::ui::elm::Rectangle::New(
        PREV_CX - CTRL_SMALL / 2, CTRL_Y, CTRL_SMALL, CTRL_SMALL, CLR_BTN, CTRL_SMALL / 2);
    this->Add(this->prevBtnBg);
    this->prevBtnText = pu::ui::elm::TextBlock::New(PREV_CX - 14, CTRL_Y + 21, "|<");
    this->prevBtnText->SetColor(CLR_WHITE);
    this->prevBtnText->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
    this->Add(this->prevBtnText);

    // Play/Pause button (larger circle, centered at PLAY_CX)
    this->playBtnBg = pu::ui::elm::Rectangle::New(
        PLAY_CX - CTRL_LARGE / 2, CTRL_Y - (CTRL_LARGE - CTRL_SMALL) / 2,
        CTRL_LARGE, CTRL_LARGE, CLR_GREEN, CTRL_LARGE / 2);
    this->Add(this->playBtnBg);
    this->playBtnText = pu::ui::elm::TextBlock::New(PLAY_CX - 9, CTRL_Y + 21, "||");
    this->playBtnText->SetColor(CLR_WHITE);
    this->playBtnText->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
    this->Add(this->playBtnText);

    // Next button (circle, centered at NEXT_CX)
    this->nextBtnBg = pu::ui::elm::Rectangle::New(
        NEXT_CX - CTRL_SMALL / 2, CTRL_Y, CTRL_SMALL, CTRL_SMALL, CLR_BTN, CTRL_SMALL / 2);
    this->Add(this->nextBtnBg);
    this->nextBtnText = pu::ui::elm::TextBlock::New(NEXT_CX - 14, CTRL_Y + 21, ">|");
    this->nextBtnText->SetColor(CLR_WHITE);
    this->nextBtnText->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
    this->Add(this->nextBtnText);

    // ---- User tab (hidden by default) ----

    this->userAvatarBg = pu::ui::elm::Rectangle::New(UAVATAR_X, UAVATAR_Y, UAVATAR_SIZE, UAVATAR_SIZE, CLR_ART_BG, 10);
    this->userAvatarBg->SetVisible(false);
    this->Add(this->userAvatarBg);

    this->userAvatarImg = pu::ui::elm::Image::New(UAVATAR_X, UAVATAR_Y, nullptr);
    this->userAvatarImg->SetWidth(UAVATAR_SIZE);
    this->userAvatarImg->SetHeight(UAVATAR_SIZE);
    this->userAvatarImg->SetVisible(false);
    this->Add(this->userAvatarImg);

    this->userNameText = pu::ui::elm::TextBlock::New(UINFO_X, UNAME_Y, "");
    this->userNameText->SetColor(CLR_WHITE);
    this->userNameText->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Large));
    this->userNameText->SetVisible(false);
    this->Add(this->userNameText);

    this->userEmailText = pu::ui::elm::TextBlock::New(UINFO_X, UEMAIL_Y, "");
    this->userEmailText->SetColor(CLR_GRAY);
    this->userEmailText->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
    this->userEmailText->SetVisible(false);
    this->Add(this->userEmailText);

    this->userCountryText = pu::ui::elm::TextBlock::New(UINFO_X, UCOUNTRY_Y, "");
    this->userCountryText->SetColor(CLR_GRAY);
    this->userCountryText->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small));
    this->userCountryText->SetVisible(false);
    this->Add(this->userCountryText);

    this->userPlanText = pu::ui::elm::TextBlock::New(UINFO_X, UPLAN_Y, "");
    this->userPlanText->SetColor(CLR_GRAY);
    this->userPlanText->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small));
    this->userPlanText->SetVisible(false);
    this->Add(this->userPlanText);

    this->userFollowersText = pu::ui::elm::TextBlock::New(UINFO_X, UFOLLOWERS_Y, "");
    this->userFollowersText->SetColor(CLR_GRAY);
    this->userFollowersText->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small));
    this->userFollowersText->SetVisible(false);
    this->Add(this->userFollowersText);

    // ---- Right panel ----

    // Borders around the right panel (left, right, bottom)
    this->rightVertSep = pu::ui::elm::Rectangle::New(RIGHT_X - 1, ART_Y, 1, CTRL_Y + CTRL_LARGE - ART_Y + 1, CLR_SEP);
    this->Add(this->rightVertSep);

    this->rightRightBorder = pu::ui::elm::Rectangle::New(RIGHT_X + RIGHT_W, ART_Y, 1, CTRL_Y + CTRL_LARGE - ART_Y + 1, CLR_SEP);
    this->Add(this->rightRightBorder);

    this->rightBottomBorder = pu::ui::elm::Rectangle::New(RIGHT_X - 1, CTRL_Y + CTRL_LARGE, RIGHT_W + 2, 1, CLR_SEP);
    this->Add(this->rightBottomBorder);

    // Tab 1 background (Artista — active by default)
    this->rightTab1Bg = pu::ui::elm::Rectangle::New(RIGHT_X, ART_Y, RIGHT_TAB_W, RIGHT_TAB_H, CLR_TAB_SEL);
    this->Add(this->rightTab1Bg);

    // Tab 2 background (Cola)
    this->rightTab2Bg = pu::ui::elm::Rectangle::New(RIGHT_X + RIGHT_TAB_W, ART_Y, RIGHT_TAB_W, RIGHT_TAB_H, CLR_BG);
    this->Add(this->rightTab2Bg);

    // Tab texts (centered within each 410px half)
    this->rightTab1Text = pu::ui::elm::TextBlock::New(RIGHT_X + RIGHT_TAB_W / 2 - 50, ART_Y + 13, "Artista");
    this->rightTab1Text->SetColor(CLR_WHITE);
    this->rightTab1Text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
    this->Add(this->rightTab1Text);

    this->rightTab2Text = pu::ui::elm::TextBlock::New(RIGHT_X + RIGHT_TAB_W + RIGHT_TAB_W / 2 - 28, ART_Y + 13, "Cola");
    this->rightTab2Text->SetColor(CLR_GRAY);
    this->rightTab2Text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
    this->Add(this->rightTab2Text);

    // Green underline indicator on the active tab
    this->rightTabIndicator = pu::ui::elm::Rectangle::New(RIGHT_X, ART_Y + RIGHT_TAB_H - 3, RIGHT_TAB_W, 3, CLR_GREEN);
    this->Add(this->rightTabIndicator);

    // Horizontal separator below tab bar
    this->rightHorizSep = pu::ui::elm::Rectangle::New(RIGHT_X, ART_Y + RIGHT_TAB_H, RIGHT_W, 1, CLR_SEP);
    this->Add(this->rightHorizSep);

    // Artist tab content
    this->rightArtistImgBg = pu::ui::elm::Rectangle::New(RART_IMG_X, RART_BLOCK_Y, RART_IMG_SIZE, RART_IMG_SIZE, CLR_ART_BG, 10);
    this->Add(this->rightArtistImgBg);

    this->rightArtistImg = pu::ui::elm::Image::New(RART_IMG_X, RART_BLOCK_Y, nullptr);
    this->rightArtistImg->SetWidth(RART_IMG_SIZE);
    this->rightArtistImg->SetHeight(RART_IMG_SIZE);
    this->Add(this->rightArtistImg);

    this->rightArtistName = pu::ui::elm::TextBlock::New(RART_INFO_X, RART_NAME_Y, "");
    this->rightArtistName->SetColor(CLR_WHITE);
    this->rightArtistName->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Large));
    this->rightArtistName->SetClampWidth(RART_INFO_W);
    this->rightArtistName->SetClampSpeed(pu::ui::elm::TextBlock::DefaultClampSpeedSteps / 3);
    this->rightArtistName->SetClampDelay(pu::ui::elm::TextBlock::DefaultClampStaticDelaySteps);
    this->Add(this->rightArtistName);

    this->rightArtistGenres = pu::ui::elm::TextBlock::New(RART_INFO_X, RART_GENRES_Y, "");
    this->rightArtistGenres->SetColor(CLR_GRAY);
    this->rightArtistGenres->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
    this->rightArtistGenres->SetClampWidth(RART_INFO_W);
    this->rightArtistGenres->SetClampSpeed(pu::ui::elm::TextBlock::DefaultClampSpeedSteps / 3);
    this->rightArtistGenres->SetClampDelay(pu::ui::elm::TextBlock::DefaultClampStaticDelaySteps);
    this->Add(this->rightArtistGenres);

    this->rightArtistFollowers = pu::ui::elm::TextBlock::New(RART_INFO_X, RART_FOLLOW_Y, "");
    this->rightArtistFollowers->SetColor(CLR_GRAY);
    this->rightArtistFollowers->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small));
    this->Add(this->rightArtistFollowers);

    this->rightArtistPopularity = pu::ui::elm::TextBlock::New(RART_INFO_X, RART_POP_Y, "");
    this->rightArtistPopularity->SetColor(CLR_GRAY);
    this->rightArtistPopularity->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small));
    this->Add(this->rightArtistPopularity);

    // Album section — separator, thumbnail, header + info lines
    this->rightAlbumSep = pu::ui::elm::Rectangle::New(RART_IMG_X, RALBUM_SEP_Y, RALBUM_FULL_W, 1, CLR_SEP);
    this->Add(this->rightAlbumSep);

    this->rightAlbumImgBg = pu::ui::elm::Rectangle::New(RART_IMG_X, RALBUM_IMG_Y, RALBUM_IMG_SIZE, RALBUM_IMG_SIZE, CLR_ART_BG, 6);
    this->Add(this->rightAlbumImgBg);

    this->rightAlbumImg = pu::ui::elm::Image::New(RART_IMG_X, RALBUM_IMG_Y, nullptr);
    this->rightAlbumImg->SetWidth(RALBUM_IMG_SIZE);
    this->rightAlbumImg->SetHeight(RALBUM_IMG_SIZE);
    this->Add(this->rightAlbumImg);

    this->rightAlbumHeader = pu::ui::elm::TextBlock::New(RART_IMG_X, RALBUM_HDR_Y, "ALBUM");
    this->rightAlbumHeader->SetColor(CLR_GRAY);
    this->rightAlbumHeader->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small));
    this->Add(this->rightAlbumHeader);

    this->rightAlbumName = pu::ui::elm::TextBlock::New(RALBUM_TEXT_X, RALBUM_NAME_Y, "");
    this->rightAlbumName->SetColor(CLR_WHITE);
    this->rightAlbumName->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
    this->rightAlbumName->SetClampWidth(RALBUM_TEXT_W);
    this->rightAlbumName->SetClampSpeed(pu::ui::elm::TextBlock::DefaultClampSpeedSteps / 3);
    this->rightAlbumName->SetClampDelay(pu::ui::elm::TextBlock::DefaultClampStaticDelaySteps);
    this->Add(this->rightAlbumName);

    this->rightAlbumTypeYear = pu::ui::elm::TextBlock::New(RALBUM_TEXT_X, RALBUM_INFO1_Y, "");
    this->rightAlbumTypeYear->SetColor(CLR_GRAY);
    this->rightAlbumTypeYear->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small));
    this->Add(this->rightAlbumTypeYear);

    this->rightAlbumTracks = pu::ui::elm::TextBlock::New(RALBUM_TEXT_X, RALBUM_INFO2_Y, "");
    this->rightAlbumTracks->SetColor(CLR_GRAY);
    this->rightAlbumTracks->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small));
    this->Add(this->rightAlbumTracks);

    // Queue tab content (5 cards: [0]=currently playing green bg, [1..4]=next in queue)
    for (int i = 0; i < 5; ++i) {
        const s32 cy = RQUEUE_START_Y + i * (RQUEUE_ITEM_H + RQUEUE_GAP);
        const pu::ui::Color cardBgColor = (i == 0) ? CLR_GREEN : CLR_ART_BG;

        this->queueCardBg[i] = pu::ui::elm::Rectangle::New(
            RQUEUE_CARD_X, cy, RQUEUE_CARD_W, RQUEUE_ITEM_H - 2, cardBgColor, 8);
        this->queueCardBg[i]->SetVisible(false);
        this->Add(this->queueCardBg[i]);

        this->queueCardImgBg[i] = pu::ui::elm::Rectangle::New(
            RQUEUE_IMG_X, cy + 15, RQUEUE_IMG_SIZE, RQUEUE_IMG_SIZE, CLR_SIDEBAR, 6);
        this->queueCardImgBg[i]->SetVisible(false);
        this->Add(this->queueCardImgBg[i]);

        this->queueCardImg[i] = pu::ui::elm::Image::New(RQUEUE_IMG_X, cy + 15, nullptr);
        this->queueCardImg[i]->SetWidth(RQUEUE_IMG_SIZE);
        this->queueCardImg[i]->SetHeight(RQUEUE_IMG_SIZE);
        this->queueCardImg[i]->SetVisible(false);
        this->Add(this->queueCardImg[i]);

        this->queueCardTitle[i] = pu::ui::elm::TextBlock::New(RQUEUE_TEXT_X, cy + 22, "");
        this->queueCardTitle[i]->SetColor(CLR_WHITE);
        this->queueCardTitle[i]->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
        this->queueCardTitle[i]->SetClampWidth(RQUEUE_TEXT_W);
        this->queueCardTitle[i]->SetClampSpeed(pu::ui::elm::TextBlock::DefaultClampSpeedSteps / 3);
        this->queueCardTitle[i]->SetClampDelay(pu::ui::elm::TextBlock::DefaultClampStaticDelaySteps);
        this->queueCardTitle[i]->SetVisible(false);
        this->Add(this->queueCardTitle[i]);

        this->queueCardArtist[i] = pu::ui::elm::TextBlock::New(RQUEUE_TEXT_X, cy + 58, "");
        this->queueCardArtist[i]->SetColor(CLR_WHITE);
        this->queueCardArtist[i]->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small));
        this->queueCardArtist[i]->SetClampWidth(RQUEUE_TEXT_W);
        this->queueCardArtist[i]->SetVisible(false);
        this->Add(this->queueCardArtist[i]);
    }

    // Loading spinner — centered over album art, shown during skip pending polling
    this->spinnerBackdrop = pu::ui::elm::Rectangle::New(
        ART_X, ART_Y, ART_SIZE, ART_SIZE, CLR_SPINNER_BG, 12);
    this->spinnerBackdrop->SetVisible(false);
    this->Add(this->spinnerBackdrop);

    {
        auto* spinTex = pu::ui::render::LoadImageFromFile("romfs:/loading.png");
        this->spinnerImg = pu::ui::elm::Image::New(
            SPINNER_X, SPINNER_Y,
            spinTex ? pu::sdl2::TextureHandle::New(spinTex) : nullptr);
        this->spinnerImg->SetWidth(SPINNER_SIZE);
        this->spinnerImg->SetHeight(SPINNER_SIZE);
        this->spinnerImg->SetVisible(false);
        this->Add(this->spinnerImg);
    }

    // No-playback overlay — shown only when there is no active playback
    this->noPlaybackText = pu::ui::elm::TextBlock::New(
        PLAYER_CX - 290, SCREEN_H / 2 - 20, "No hay reproduccion activa");
    this->noPlaybackText->SetColor(CLR_GRAY);
    this->noPlaybackText->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Large));
    this->noPlaybackText->SetVisible(false);
    this->Add(this->noPlaybackText);

    // Periodic refresh via render callback
    this->AddRenderCallback([this]() { this->OnRenderCallback(); });

    // Spinner rotation — runs every frame, no-op when hidden
    this->AddRenderCallback([this]() {
        if (!this->spinnerVisible) return;
        this->spinnerAngle += 4.0f;
        if (this->spinnerAngle >= 360.0f) this->spinnerAngle -= 360.0f;
        this->spinnerImg->SetRotationAngle(this->spinnerAngle);
    });
}

// --- Tab switching ---

void MainLayout::SetPlayerTabVisible(bool visible) {
    const bool showContent  = visible && this->playbackActive;
    const bool showNoPlay   = visible && !this->playbackActive;
    this->albumArtBg->SetVisible(showContent);
    this->albumArtImage->SetVisible(showContent);
    this->trackText->SetVisible(showContent);
    this->artistText->SetVisible(showContent);
    this->prevBtnBg->SetVisible(showContent);
    this->prevBtnText->SetVisible(showContent);
    this->playBtnBg->SetVisible(showContent);
    this->playBtnText->SetVisible(showContent);
    this->nextBtnBg->SetVisible(showContent);
    this->nextBtnText->SetVisible(showContent);
    this->spinnerBackdrop->SetVisible(showContent && this->spinnerVisible);
    this->spinnerImg->SetVisible(showContent && this->spinnerVisible);
    this->noPlaybackText->SetVisible(showNoPlay);
}

void MainLayout::SetPlaybackActive(bool active) {
    this->playbackActive = active;
    if (this->currentTab == Tab::Player)
        this->SetPlayerTabVisible(true);
    // Right panel only makes sense when there is active playback
    if (this->currentTab == Tab::Player)
        this->SetRightPanelVisible(active);
}

void MainLayout::SetUserTabVisible(bool visible) {
    this->userAvatarBg->SetVisible(visible);
    this->userAvatarImg->SetVisible(visible);
    this->userNameText->SetVisible(visible);
    this->userEmailText->SetVisible(visible);
    this->userCountryText->SetVisible(visible);
    this->userPlanText->SetVisible(visible);
    this->userFollowersText->SetVisible(visible);
}

void MainLayout::SetRightPanelVisible(bool visible) {
    this->rightVertSep->SetVisible(visible);
    this->rightRightBorder->SetVisible(visible);
    this->rightBottomBorder->SetVisible(visible);
    this->rightTab1Bg->SetVisible(visible);
    this->rightTab2Bg->SetVisible(visible);
    this->rightTab1Text->SetVisible(visible);
    this->rightTab2Text->SetVisible(visible);
    this->rightTabIndicator->SetVisible(visible);
    this->rightHorizSep->SetVisible(visible);
    const bool showArtist = visible && this->currentRightTab == RightTab::Artist;
    this->rightArtistImgBg->SetVisible(showArtist);
    this->rightArtistImg->SetVisible(showArtist);
    this->rightArtistName->SetVisible(showArtist);
    this->rightArtistGenres->SetVisible(showArtist);
    this->rightArtistFollowers->SetVisible(showArtist);
    this->rightArtistPopularity->SetVisible(showArtist);
    this->rightAlbumSep->SetVisible(showArtist);
    this->rightAlbumImgBg->SetVisible(showArtist);
    this->rightAlbumImg->SetVisible(showArtist);
    this->rightAlbumHeader->SetVisible(showArtist);
    this->rightAlbumName->SetVisible(showArtist);
    this->rightAlbumTypeYear->SetVisible(showArtist);
    this->rightAlbumTracks->SetVisible(showArtist);
    const bool showQueue = visible && this->currentRightTab == RightTab::Queue;
    for (int i = 0; i < 5; ++i) {
        this->queueCardBg[i]->SetVisible(showQueue);
        this->queueCardImgBg[i]->SetVisible(showQueue);
        this->queueCardImg[i]->SetVisible(showQueue);
        this->queueCardTitle[i]->SetVisible(showQueue);
        this->queueCardArtist[i]->SetVisible(showQueue);
    }
}

void MainLayout::SwitchToTab(Tab tab) {
    this->currentTab = tab;
    const bool isPlayer = (tab == Tab::Player);

    this->tab1Bg->SetColor(isPlayer ? CLR_TAB_SEL : CLR_SIDEBAR);
    this->tab2Bg->SetColor(isPlayer ? CLR_SIDEBAR : CLR_TAB_SEL);
    this->tab1Text->SetColor(isPlayer ? CLR_WHITE : CLR_GRAY);
    this->tab2Text->SetColor(isPlayer ? CLR_GRAY : CLR_WHITE);
    this->tabIndicator->SetY(isPlayer ? TAB1_Y : TAB2_Y);

    this->SetPlayerTabVisible(isPlayer);
    this->SetUserTabVisible(!isPlayer);
    this->SetRightPanelVisible(isPlayer && this->playbackActive);
}

void MainLayout::SwitchRightTab(RightTab tab) {
    this->currentRightTab = tab;
    const bool isArtist = (tab == RightTab::Artist);

    this->rightTab1Bg->SetColor(isArtist ? CLR_TAB_SEL : CLR_BG);
    this->rightTab2Bg->SetColor(isArtist ? CLR_BG : CLR_TAB_SEL);
    this->rightTab1Text->SetColor(isArtist ? CLR_WHITE : CLR_GRAY);
    this->rightTab2Text->SetColor(isArtist ? CLR_GRAY : CLR_WHITE);
    this->rightTabIndicator->SetX(isArtist ? RIGHT_X : RIGHT_X + RIGHT_TAB_W);
    this->rightArtistImgBg->SetVisible(isArtist);
    this->rightArtistImg->SetVisible(isArtist);
    this->rightArtistName->SetVisible(isArtist);
    this->rightArtistGenres->SetVisible(isArtist);
    this->rightArtistFollowers->SetVisible(isArtist);
    this->rightArtistPopularity->SetVisible(isArtist);
    this->rightAlbumSep->SetVisible(isArtist);
    this->rightAlbumImgBg->SetVisible(isArtist);
    this->rightAlbumImg->SetVisible(isArtist);
    this->rightAlbumHeader->SetVisible(isArtist);
    this->rightAlbumName->SetVisible(isArtist);
    this->rightAlbumTypeYear->SetVisible(isArtist);
    this->rightAlbumTracks->SetVisible(isArtist);
    const bool showQueue = !isArtist;
    for (int i = 0; i < 5; ++i) {
        this->queueCardBg[i]->SetVisible(showQueue);
        this->queueCardImgBg[i]->SetVisible(showQueue);
        this->queueCardImg[i]->SetVisible(showQueue);
        this->queueCardTitle[i]->SetVisible(showQueue);
        this->queueCardArtist[i]->SetVisible(showQueue);
    }
}

// --- Periodic refresh ---

void MainLayout::OnRenderCallback() {
    if (!this->refreshCallback) return;
    const time_t now = time(nullptr);
    if (now - this->lastRefresh < REFRESH_INTERVAL_SECS) return;
    this->lastRefresh = now;
    this->refreshCallback();
}

void MainLayout::SetRefreshCallback(std::function<void()> fn) {
    this->refreshCallback = std::move(fn);
    this->lastRefresh = time(nullptr);
}

void MainLayout::SetLoadingSpinner(bool visible) {
    this->spinnerVisible = visible;
    if (!visible) this->spinnerAngle = 0.0f;
    const bool canShow = (this->currentTab == Tab::Player) && this->playbackActive;
    this->spinnerBackdrop->SetVisible(visible && canShow);
    this->spinnerImg->SetVisible(visible && canShow);
}

// --- Content setters ---

void MainLayout::SetStatus(const std::string& text) {
    this->statusText->SetText(text);
}

void MainLayout::SetDevice(const std::string& deviceName) {
    this->deviceText->SetText(deviceName.empty() ? "" : deviceName);
}

void MainLayout::UpdatePlayButton(bool isPlaying) {
    if (isPlaying) {
        this->playBtnText->SetText("||");
        this->playBtnText->SetX(PLAY_CX - 9);
    } else {
        this->playBtnText->SetText(">");
        this->playBtnText->SetX(PLAY_CX - 7);
    }
}

void MainLayout::SetTrack(const std::string& trackName, const std::string& artistName, bool isPlaying) {
    this->trackText->SetText(trackName.empty() ? "Sin reproduccion activa" : trackName);
    this->artistText->SetText(artistName);
    this->UpdatePlayButton(isPlaying);
}

void MainLayout::SetAlbumArt(pu::sdl2::TextureHandle::Ref handle) {
    this->albumArtImage->SetImage(handle);
    // SetImage resets render dimensions to the texture's natural size — re-apply explicitly.
    this->albumArtImage->SetWidth(ART_SIZE);
    this->albumArtImage->SetHeight(ART_SIZE);
}

static std::string formatFollowers(long n) {
    char buf[32];
    if (n >= 1000000)
        snprintf(buf, sizeof(buf), "%.1fM seguidores", (float)n / 1000000.0f);
    else if (n >= 1000)
        snprintf(buf, sizeof(buf), "%.0fK seguidores", (float)n / 1000.0f);
    else
        snprintf(buf, sizeof(buf), "%ld seguidores", n);
    return buf;
}

void MainLayout::SetArtistInfo(const spotify::ArtistInfo& info) {
    if (!info.valid) return;
    this->rightArtistName->SetText(info.name);
    this->rightArtistGenres->SetText(info.genres);
    this->rightArtistFollowers->SetText(formatFollowers(info.followers));
    char popBuf[32];
    snprintf(popBuf, sizeof(popBuf), "Popularidad: %d / 100", info.popularity);
    this->rightArtistPopularity->SetText(popBuf);
}

void MainLayout::SetArtistImage(pu::sdl2::TextureHandle::Ref handle) {
    this->rightArtistImg->SetImage(handle);
    this->rightArtistImg->SetWidth(RART_IMG_SIZE);
    this->rightArtistImg->SetHeight(RART_IMG_SIZE);
}

static std::string formatAlbumType(const std::string& t) {
    if (t == "album")       return "Album";
    if (t == "single")      return "Single";
    if (t == "compilation") return "Recopilatorio";
    return t;
}

void MainLayout::SetAlbumThumbnail(pu::sdl2::TextureHandle::Ref handle) {
    this->rightAlbumImg->SetImage(handle);
    this->rightAlbumImg->SetWidth(RALBUM_IMG_SIZE);
    this->rightAlbumImg->SetHeight(RALBUM_IMG_SIZE);
}

void MainLayout::SetAlbumInfo(const spotify::AlbumInfo& info) {
    if (!info.valid) return;
    this->rightAlbumName->SetText(info.name);
    const auto year = info.releaseDate.size() >= 4 ? info.releaseDate.substr(0, 4) : info.releaseDate;
    this->rightAlbumTypeYear->SetText(formatAlbumType(info.albumType) + " · " + year);
    char buf[128];
    if (info.label.empty())
        snprintf(buf, sizeof(buf), "%d canciones", info.totalTracks);
    else
        snprintf(buf, sizeof(buf), "%d canciones · %s", info.totalTracks, info.label.c_str());
    this->rightAlbumTracks->SetText(buf);
}

static std::string capitalizeFirst(const std::string& s) {
    if (s.empty()) return s;
    std::string out = s;
    out[0] = static_cast<char>(toupper(static_cast<unsigned char>(out[0])));
    return out;
}

void MainLayout::SetUserProfile(const spotify::UserProfile& profile) {
    if (!profile.valid) return;
    this->userNameText->SetText(profile.displayName);
    this->userEmailText->SetText(profile.email.empty() ? "" : profile.email);
    this->userCountryText->SetText(profile.country.empty() ? "" : "Pais: " + profile.country);
    this->userPlanText->SetText(profile.product.empty() ? "" : "Plan: " + capitalizeFirst(profile.product));
    this->userFollowersText->SetText(formatFollowers(profile.followers));
}

void MainLayout::SetUserAvatar(pu::sdl2::TextureHandle::Ref handle) {
    this->userAvatarImg->SetImage(handle);
    this->userAvatarImg->SetWidth(UAVATAR_SIZE);
    this->userAvatarImg->SetHeight(UAVATAR_SIZE);
}

void MainLayout::SetQueueInfo(const spotify::QueueInfo& info) {
    for (int i = 0; i < 5; ++i) {
        if (i < info.trackCount) {
            this->queueCardTitle[i]->SetText(info.tracks[i].name);
            this->queueCardArtist[i]->SetText(info.tracks[i].artistName);
        } else {
            this->queueCardTitle[i]->SetText("");
            this->queueCardArtist[i]->SetText("");
        }
    }
}

void MainLayout::SetQueueImage(int index, pu::sdl2::TextureHandle::Ref handle) {
    if (index < 0 || index >= 5) return;
    this->queueCardImg[index]->SetImage(handle);
    this->queueCardImg[index]->SetWidth(RQUEUE_IMG_SIZE);
    this->queueCardImg[index]->SetHeight(RQUEUE_IMG_SIZE);
}

// =============================================================================
// MainApplication
// =============================================================================

void MainApplication::OnLoad() {
    this->mainLayout = MainLayout::New();

    this->SetOnInput([&](const u64 keys_down, const u64 keys_up, const u64 keys_held,
                         const pu::ui::TouchPoint touch_pos) {
        (void)keys_up; (void)keys_held; (void)touch_pos;

        if (keys_down & HidNpadButton_Plus) {
            this->Close();
            return;
        }

        if (!this->mainLayoutActive) return;

        // L / R → sidebar tab switching
        if (keys_down & HidNpadButton_L)
            this->mainLayout->SwitchToTab(Tab::Player);
        if (keys_down & HidNpadButton_R)
            this->mainLayout->SwitchToTab(Tab::User);
        // ZL / ZR → right panel tab switching (only in Player tab with active playback)
        if (this->mainLayout->GetCurrentTab() == Tab::Player && this->mainLayout->GetPlaybackActive()) {
            if (keys_down & HidNpadButton_ZL)
                this->mainLayout->SwitchRightTab(RightTab::Artist);
            if (keys_down & HidNpadButton_ZR)
                this->mainLayout->SwitchRightTab(RightTab::Queue);
        }

        // Player controls (only in Player tab, blocked while a skip is in flight)
        if (this->mainLayout->GetCurrentTab() == Tab::Player && !this->actionsBlocked) {
            if (keys_down & HidNpadButton_A)
                this->OnPlayPause();
            if (keys_down & HidNpadButton_Left)
                this->OnPrev();
            if (keys_down & HidNpadButton_Right)
                this->OnNext();
        }
    });

    const auto saved = TokenStorage::loadTokens();
    if (saved.valid) {
        this->currentTokens = saved;
        this->mainLayoutActive = true;
        this->mainLayout->SetStatus("Sesion iniciada.");
        this->LoadLayout(this->mainLayout);
        this->FetchUserProfile();
        this->FetchAndShowPlayerState();
        this->mainLayout->SetRefreshCallback([this]() { this->FetchAndShowPlayerState(); });
        return;
    }

    const std::string ip = getLocalIp();
    if (ip.empty()) {
        this->mainLayout->SetStatus(
            "Conecta la Switch a una red WiFi para iniciar sesion.");
        this->LoadLayout(this->mainLayout);
        return;
    }

    const auto verifier     = spotify::generateCodeVerifier();
    const auto challenge    = spotify::generateCodeChallenge(verifier);
    const auto randomState  = spotify::generateState();
    const auto fullState    = randomState + "." + encodeIp(ip);
    const auto authUrl      = spotify::buildAuthUrl(challenge, fullState);

    static constexpr int PORT = 8080;
    this->localServer = std::make_unique<LocalServer>(PORT);
    this->localServer->start();

    this->loginLayout = LoginLayout::New(authUrl, verifier,
        this->localServer.get(),
        [this](const spotify::Tokens& tokens) {
            this->OnLoginSuccess(tokens);
        });

    this->LoadLayout(this->loginLayout);
}

void MainApplication::FetchAndShowPlayerState() {
    if (time(nullptr) + 60 >= this->currentTokens.expiresAt) {
        debugLog("APP: refreshing access token");
        this->currentTokens = spotify::refreshAccessToken(this->currentTokens.refreshToken);
        if (this->currentTokens.valid) TokenStorage::saveTokens(this->currentTokens);
    }
    if (!this->currentTokens.valid) {
        this->mainLayout->SetStatus("Error al refrescar el token.");
        this->actionsBlocked = false;
        this->mainLayout->SetLoadingSpinner(false);
        return;
    }

    const auto player = spotify::getPlayerState(this->currentTokens.accessToken);

    if (player.tokenExpired) {
        debugLog("APP: 401 from /me/player — forcing token refresh");
        this->currentTokens = spotify::refreshAccessToken(this->currentTokens.refreshToken);
        if (this->currentTokens.valid) {
            TokenStorage::saveTokens(this->currentTokens);
            // Retry once with the new token
            const auto retried = spotify::getPlayerState(this->currentTokens.accessToken);
            this->mainLayout->SetPlaybackActive(retried.valid);
            if (!retried.valid) { this->isPlaying = false; this->actionsBlocked = false; this->mainLayout->SetLoadingSpinner(false); return; }
            this->isPlaying = retried.isPlaying;
            this->currentTrackName = retried.trackName;
            this->mainLayout->SetTrack(retried.trackName, retried.artistName, retried.isPlaying);
            this->mainLayout->SetDevice(retried.deviceName);
        } else {
            debugLog("APP: refresh failed — forcing re-login");
            this->currentTokens = spotify::Tokens();
            TokenStorage::saveTokens(this->currentTokens);
            this->mainLayout->SetStatus("Sesion expirada. Reinicia la app para volver a iniciar sesion.");
        }
        if (!this->actionsBlocked || this->currentTrackName != this->blockedFromTrackName) {
            this->actionsBlocked = false;
            this->mainLayout->SetLoadingSpinner(false);
        }
        return;
    }

    this->mainLayout->SetPlaybackActive(player.valid);

    if (!player.valid) {
        this->isPlaying = false;
        this->actionsBlocked = false;
        this->mainLayout->SetLoadingSpinner(false);
        return;
    }

    this->isPlaying = player.isPlaying;
    const bool trackChanged = (player.trackName != this->blockedFromTrackName);
    this->currentTrackName = player.trackName;
    this->mainLayout->SetTrack(player.trackName, player.artistName, player.isPlaying);
    this->mainLayout->SetDevice(player.deviceName);

    // Download album art only when the art URL changes
    if (!player.albumImageUrl.empty() && player.albumImageUrl != this->currentAlbumUrl) {
        this->currentAlbumUrl = player.albumImageUrl;
        const auto artData = spotify::downloadAlbumArt(player.albumImageUrl);
        if (!artData.empty()) {
            auto* rawTex = pu::ui::render::LoadImageFromBuffer(
                static_cast<const void*>(artData.data()), artData.size());
            if (rawTex) {
                auto handle = pu::sdl2::TextureHandle::New(rawTex);
                this->mainLayout->SetAlbumArt(handle);
                this->mainLayout->SetAlbumThumbnail(handle);
            }
        }
    }

    // Fetch album info only when the album ID changes (separate guard)
    if (!player.albumId.empty() && player.albumId != this->currentAlbumId) {
        this->currentAlbumId = player.albumId;
        const auto albumInfo = spotify::getAlbumInfo(player.albumId, this->currentTokens.accessToken);
        if (albumInfo.valid)
            this->mainLayout->SetAlbumInfo(albumInfo);
    }

    // Fetch artist info only when the artist changes
    if (!player.artistId.empty() && player.artistId != this->currentArtistId) {
        this->currentArtistId = player.artistId;
        const auto info = spotify::getArtistInfo(player.artistId, this->currentTokens.accessToken);
        if (info.valid) {
            this->mainLayout->SetArtistInfo(info);
            if (!info.imageUrl.empty()) {
                const auto imgData = spotify::downloadAlbumArt(info.imageUrl);
                if (!imgData.empty()) {
                    auto* rawTex = pu::ui::render::LoadImageFromBuffer(
                        static_cast<const void*>(imgData.data()), imgData.size());
                    if (rawTex)
                        this->mainLayout->SetArtistImage(pu::sdl2::TextureHandle::New(rawTex));
                }
            }
        }
    }

    // Fetch queue every cycle (changes with each track skip)
    const auto queueInfo = spotify::getQueue(this->currentTokens.accessToken);
    if (queueInfo.valid) {
        this->mainLayout->SetQueueInfo(queueInfo);
        for (int i = 0; i < queueInfo.trackCount; ++i) {
            const auto& url = queueInfo.tracks[i].imageUrl;
            if (!url.empty() && url != this->currentQueueUrls[i]) {
                this->currentQueueUrls[i] = url;
                const auto imgData = spotify::downloadAlbumArt(url);
                if (!imgData.empty()) {
                    auto* rawTex = pu::ui::render::LoadImageFromBuffer(
                        static_cast<const void*>(imgData.data()), imgData.size());
                    if (rawTex)
                        this->mainLayout->SetQueueImage(i, pu::sdl2::TextureHandle::New(rawTex));
                }
            }
        }
    }
    if (!this->actionsBlocked || trackChanged) {
        this->actionsBlocked = false;
        this->mainLayout->SetLoadingSpinner(false);
    }
}

void MainApplication::OnPlayPause() {
    if (this->isPlaying) {
        spotify::pause(this->currentTokens.accessToken);
    } else {
        spotify::play(this->currentTokens.accessToken);
    }
    // Optimistic UI update — periodic refresh will confirm the real state
    this->isPlaying = !this->isPlaying;
    this->mainLayout->UpdatePlayButton(this->isPlaying);
}

void MainApplication::OnPrev() {
    this->blockedFromTrackName = this->currentTrackName;
    this->actionsBlocked = true;
    this->mainLayout->SetLoadingSpinner(true);
    spotify::skipPrevious(this->currentTokens.accessToken);
    // Brief wait then refresh so the new track info appears quickly
    this->mainLayout->SetRefreshCallback(nullptr); // prevent double-refresh race
    this->FetchAndShowPlayerState();
    this->mainLayout->SetRefreshCallback([this]() { this->FetchAndShowPlayerState(); });
}

void MainApplication::OnNext() {
    this->blockedFromTrackName = this->currentTrackName;
    this->actionsBlocked = true;
    this->mainLayout->SetLoadingSpinner(true);
    spotify::skipNext(this->currentTokens.accessToken);
    this->mainLayout->SetRefreshCallback(nullptr);
    this->FetchAndShowPlayerState();
    this->mainLayout->SetRefreshCallback([this]() { this->FetchAndShowPlayerState(); });
}

void MainApplication::FetchUserProfile() {
    if (this->userProfileFetched) return;
    debugLog("APP: fetching user profile");
    const auto profile = spotify::getUserProfile(this->currentTokens.accessToken);
    if (!profile.valid) return;

    this->userProfileFetched = true;
    this->mainLayout->SetUserProfile(profile);

    if (!profile.imageUrl.empty()) {
        const auto imgData = spotify::downloadAlbumArt(profile.imageUrl);
        if (!imgData.empty()) {
            auto* rawTex = pu::ui::render::LoadImageFromBuffer(
                static_cast<const void*>(imgData.data()), imgData.size());
            if (rawTex)
                this->mainLayout->SetUserAvatar(pu::sdl2::TextureHandle::New(rawTex));
        }
    }
}

void MainApplication::OnLoginSuccess(const spotify::Tokens& tokens) {
    debugLog("APP: OnLoginSuccess");
    if (this->localServer) {
        this->localServer->stop();
        this->localServer.reset();
    }
    this->currentTokens = tokens;
    TokenStorage::saveTokens(tokens);
    this->mainLayoutActive = true;
    this->userProfileFetched = false;
    this->mainLayout->SetStatus("Sesion iniciada.");
    this->LoadLayout(this->mainLayout);
    this->FetchUserProfile();
    this->FetchAndShowPlayerState();
    this->mainLayout->SetRefreshCallback([this]() { this->FetchAndShowPlayerState(); });
    debugLog("APP: ready");
}
