#include <MainApplication.hpp>
#include <LoginLayout.hpp>
#include <TokenStorage.hpp>
#include <SpotifyAuth.hpp>

// --- MainLayout ---

MainLayout::MainLayout() : Layout::Layout() {
    this->SetBackgroundColor(pu::ui::Color(18, 18, 18, 255));

    auto title = pu::ui::elm::TextBlock::New(0, 80, "Spotify Switch");
    title->SetColor(pu::ui::Color(29, 185, 84, 255));
    title->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Large));
    title->SetX(640 - 140);
    this->Add(title);

    this->statusText = pu::ui::elm::TextBlock::New(0, 200, "Iniciando...");
    this->statusText->SetColor(pu::ui::Color(200, 200, 200, 255));
    this->statusText->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
    this->statusText->SetX(640 - 200);
    this->Add(this->statusText);

    auto hint = pu::ui::elm::TextBlock::New(640 - 80, 680, "Pulsa + para salir");
    hint->SetColor(pu::ui::Color(100, 100, 100, 255));
    hint->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small));
    this->Add(hint);
}

void MainLayout::SetStatus(const std::string& text) {
    this->statusText->SetText(text);
}

// --- MainApplication ---

void MainApplication::OnLoad() {
    this->mainLayout = MainLayout::New();

    this->SetOnInput([&](const u64 keys_down, const u64 keys_up, const u64 keys_held,
                         const pu::ui::TouchPoint touch_pos) {
        (void)keys_up; (void)keys_held; (void)touch_pos;
        if (keys_down & HidNpadButton_Plus) {
            this->Close();
        }
    });

    const auto saved = TokenStorage::loadTokens();
    if (saved.valid) {
        this->mainLayout->SetStatus("Sesion iniciada. Tokens cargados correctamente.");
        this->LoadLayout(this->mainLayout);
        return;
    }

    // No saved session — start OAuth flow
    const auto verifier  = spotify::generateCodeVerifier();
    const auto challenge = spotify::generateCodeChallenge(verifier);
    const auto state     = spotify::generateState();
    const auto authUrl   = spotify::buildAuthUrl(challenge, state);

    this->loginLayout = LoginLayout::New(authUrl, verifier,
        [this](const spotify::Tokens& tokens) {
            this->OnLoginSuccess(tokens);
        });

    this->LoadLayout(this->loginLayout);
}

void MainApplication::OnLoginSuccess(const spotify::Tokens& tokens) {
    TokenStorage::saveTokens(tokens);
    this->mainLayout->SetStatus("Sesion iniciada con Spotify. Bienvenido!");
    this->LoadLayout(this->mainLayout);
}
