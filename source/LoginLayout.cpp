#include <LoginLayout.hpp>
#include <SpotifyAuth.hpp>
#include <switch.h>

// Screen is 1280x720
static constexpr s32 SCREEN_W = 1280;
static constexpr s32 MARGIN   = 80;

static const pu::ui::Color CLR_WHITE  { 255, 255, 255, 255 };
static const pu::ui::Color CLR_GRAY   { 160, 160, 160, 255 };
static const pu::ui::Color CLR_GREEN  {  29, 185,  84, 255 };
static const pu::ui::Color CLR_DARK   {  18,  18,  18, 255 };
static const pu::ui::Color CLR_PANEL  {  30,  30,  30, 255 };
static const pu::ui::Color CLR_RED    { 220,  50,  50, 255 };

LoginLayout::LoginLayout(const std::string& authUrl,
                         const std::string& verifier,
                         OnLoginSuccessCallback cb)
    : Layout::Layout(), codeVerifier(verifier), onLoginSuccess(cb)
{
    this->SetBackgroundColor(CLR_DARK);

    // Title
    this->titleText = pu::ui::elm::TextBlock::New(0, 28, "Spotify Switch");
    this->titleText->SetColor(CLR_GREEN);
    this->titleText->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Large));
    // Center horizontally (approximate — font metrics unavailable at construct time)
    this->titleText->SetX((SCREEN_W / 2) - 140);
    this->Add(this->titleText);

    // Step 1
    this->step1Text = pu::ui::elm::TextBlock::New(MARGIN, 120,
        "Paso 1 - Abre esta URL en tu navegador:");
    this->step1Text->SetColor(CLR_WHITE);
    this->step1Text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
    this->Add(this->step1Text);

    // Auth URL (scrolls horizontally if too wide)
    this->urlText = pu::ui::elm::TextBlock::New(MARGIN, 162, authUrl);
    this->urlText->SetColor(CLR_GREEN);
    this->urlText->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small));
    this->urlText->SetClampWidth(SCREEN_W - MARGIN * 2);
    this->urlText->SetClampSpeed(pu::ui::elm::TextBlock::DefaultClampSpeedSteps);
    this->urlText->SetClampDelay(pu::ui::elm::TextBlock::DefaultClampStaticDelaySteps);
    this->Add(this->urlText);

    // Step 2
    this->step2Text = pu::ui::elm::TextBlock::New(MARGIN, 218,
        "Paso 2 - Inicia sesion en Spotify y copia el codigo que aparece en pantalla.");
    this->step2Text->SetColor(CLR_WHITE);
    this->step2Text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
    this->Add(this->step2Text);

    // Status (shown during/after code exchange)
    this->statusText = pu::ui::elm::TextBlock::New(MARGIN, 270, "");
    this->statusText->SetColor(CLR_GRAY);
    this->statusText->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
    this->Add(this->statusText);

    // Enter code button
    static constexpr s32 BTN_W = 460;
    static constexpr s32 BTN_H = 64;
    this->enterCodeBtn = pu::ui::elm::Button::New(
        (SCREEN_W - BTN_W) / 2, 370,
        BTN_W, BTN_H,
        "Introducir codigo (Paso 3)",
        CLR_WHITE, CLR_GREEN);
    this->enterCodeBtn->SetOnClick([this]() { this->OnEnterCode(); });
    this->Add(this->enterCodeBtn);

    // Footer hint
    auto hint = pu::ui::elm::TextBlock::New(0, 680, "Pulsa + para salir");
    hint->SetColor(CLR_GRAY);
    hint->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small));
    hint->SetX((SCREEN_W / 2) - 80);
    this->Add(hint);
}

void LoginLayout::OnEnterCode() {
    SwkbdConfig kbd;
    swkbdCreate(&kbd, 0);
    swkbdConfigSetType(&kbd, SwkbdType_Normal);
    swkbdConfigSetStringLenMax(&kbd, 512);
    swkbdConfigSetHeaderText(&kbd, "Codigo de autenticacion de Spotify");
    swkbdConfigSetGuideText(&kbd, "Pega o escribe el codigo de la pagina web");

    char code[512] = {};
    const auto rc = swkbdShow(&kbd, code, sizeof(code));
    swkbdClose(&kbd);

    if (R_FAILED(rc) || code[0] == '\0') {
        this->statusText->SetText("Operacion cancelada.");
        this->statusText->SetColor(CLR_GRAY);
        return;
    }

    this->statusText->SetText("Verificando codigo con Spotify...");
    this->statusText->SetColor(CLR_GRAY);
    this->enterCodeBtn->SetContent("Verificando...");

    const auto tokens = spotify::exchangeCode(std::string(code), this->codeVerifier);

    if (!tokens.valid) {
        this->statusText->SetText("Error: codigo invalido o expirado. Intentalo de nuevo.");
        this->statusText->SetColor(CLR_RED);
        this->enterCodeBtn->SetContent("Introducir codigo (Paso 3)");
        return;
    }

    this->onLoginSuccess(tokens);
}
