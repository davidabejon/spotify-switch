#include <LoginLayout.hpp>
#include <SpotifyAuth.hpp>
#include <qrcodegen.h>
#include <switch.h>
#include <SDL2/SDL.h>
#include <algorithm>

// Screen: 1920x1080
static constexpr s32 W          = 1920;
static constexpr s32 H          = 1080;
static constexpr s32 MARGIN     = 100;
static constexpr s32 QR_X       = 1280;   // QR panel starts here
static constexpr s32 LEFT_W     = QR_X - MARGIN * 2;

static const pu::ui::Color CLR_BG     {  18,  18,  18, 255 };
static const pu::ui::Color CLR_WHITE  { 255, 255, 255, 255 };
static const pu::ui::Color CLR_GRAY   { 150, 150, 150, 255 };
static const pu::ui::Color CLR_GREEN  {  29, 185,  84, 255 };
static const pu::ui::Color CLR_RED    { 220,  50,  50, 255 };

pu::sdl2::TextureHandle::Ref LoginLayout::buildQrTexture(const std::string& url, s32* outSize) {
    if (outSize) *outSize = 0;

    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
    uint8_t tempBuf[qrcodegen_BUFFER_LEN_MAX];

    const bool ok = qrcodegen_encodeText(
        url.c_str(), tempBuf, qrcode,
        qrcodegen_Ecc_LOW,
        qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX,
        qrcodegen_Mask_AUTO, true);

    if (!ok) return nullptr;

    const int modules = qrcodegen_getSize(qrcode);
    const int quiet   = 4;
    const int total   = modules + quiet * 2;
    const int scale   = std::max(1, 580 / total);
    const int side    = total * scale;

    SDL_Surface* surface = SDL_CreateRGBSurface(0, side, side, 32,
        0x00FF0000u, 0x0000FF00u, 0x000000FFu, 0xFF000000u);
    if (!surface) return nullptr;

    SDL_FillRect(surface, nullptr,
        SDL_MapRGBA(surface->format, 255, 255, 255, 255));

    const int offset = quiet * scale;
    for (int row = 0; row < modules; row++) {
        for (int col = 0; col < modules; col++) {
            if (qrcodegen_getModule(qrcode, col, row)) {
                SDL_Rect rect = { offset + col * scale, offset + row * scale, scale, scale };
                SDL_FillRect(surface, &rect,
                    SDL_MapRGBA(surface->format, 0, 0, 0, 255));
            }
        }
    }

    auto* tex = pu::ui::render::ConvertToTexture(surface);
    if (!tex) return nullptr;
    if (outSize) *outSize = static_cast<s32>(side);
    return pu::sdl2::TextureHandle::New(tex);
}

LoginLayout::LoginLayout(const std::string& authUrl,
                         const std::string& verifier,
                         LocalServer* srv,
                         OnLoginSuccessCallback cb)
    : Layout::Layout(), codeVerifier(verifier), onLoginSuccess(cb),
      server(srv), exchangePending(false)
{
    this->SetBackgroundColor(CLR_BG);

    // Title
    this->titleText = pu::ui::elm::TextBlock::New(0, 50, "Spotify Switch");
    this->titleText->SetColor(CLR_GREEN);
    this->titleText->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Large));
    this->titleText->SetX((W / 2) - 180);
    this->Add(this->titleText);

    // Step 1
    this->step1Text = pu::ui::elm::TextBlock::New(MARGIN, 190,
        "Paso 1 - Escanea el codigo QR con la camara del movil:");
    this->step1Text->SetColor(CLR_WHITE);
    this->step1Text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
    this->Add(this->step1Text);

    // Auth URL (scrolling, smaller — mostly for debugging)
    this->urlText = pu::ui::elm::TextBlock::New(MARGIN, 250, authUrl);
    this->urlText->SetColor(CLR_GREEN);
    this->urlText->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small));
    this->urlText->SetClampWidth(LEFT_W);
    this->urlText->SetClampSpeed(pu::ui::elm::TextBlock::DefaultClampSpeedSteps);
    this->urlText->SetClampDelay(pu::ui::elm::TextBlock::DefaultClampStaticDelaySteps);
    this->Add(this->urlText);

    // Step 2
    this->step2Text = pu::ui::elm::TextBlock::New(MARGIN, 330,
        "Paso 2 - Inicia sesion con tu cuenta de Spotify.");
    this->step2Text->SetColor(CLR_WHITE);
    this->step2Text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
    this->Add(this->step2Text);

    // Status
    this->statusText = pu::ui::elm::TextBlock::New(MARGIN, 410,
        "Esperando autorizacion...");
    this->statusText->SetColor(CLR_GRAY);
    this->statusText->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
    this->Add(this->statusText);

    // Footer
    auto hint = pu::ui::elm::TextBlock::New((W / 2) - 120, 1030, "Pulsa + para salir");
    hint->SetColor(CLR_GRAY);
    hint->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small));
    this->Add(hint);

    // QR code (right panel)
    s32 qrSide = 0;
    auto texHandle = buildQrTexture(authUrl, &qrSide);
    if (texHandle && qrSide > 0) {
        const s32 panelW = W - QR_X;
        const s32 qrX    = QR_X + (panelW - qrSide) / 2;
        const s32 qrY    = (H - qrSide) / 2;

        this->qrImage = pu::ui::elm::Image::New(qrX, qrY, texHandle);
        this->qrImage->SetWidth(qrSide);
        this->qrImage->SetHeight(qrSide);
        this->Add(this->qrImage);

        this->scanHintText = pu::ui::elm::TextBlock::New(0, qrY + qrSide + 16,
            "Escanea con la camara del movil");
        this->scanHintText->SetColor(CLR_GRAY);
        this->scanHintText->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small));
        this->scanHintText->SetX(qrX + (qrSide / 2) - 160);
        this->Add(this->scanHintText);
    }

    this->SetOnInput([](const u64, const u64, const u64, const pu::ui::TouchPoint) {});

    // Poll the local server every frame
    this->AddRenderCallback([this]() { this->OnRenderCallback(); });
}

void LoginLayout::OnRenderCallback() {
    if (this->exchangePending) {
        // Second frame after code was received — now do the blocking token exchange
        this->exchangePending = false;
        const auto tokens = spotify::exchangeCode(this->pendingCode, this->codeVerifier);
        if (!tokens.valid) {
            this->statusText->SetText("Error: fallo al obtener tokens. Intentalo de nuevo.");
            this->statusText->SetColor(CLR_RED);
            return;
        }
        this->onLoginSuccess(tokens);
        return;
    }

    if (!this->server) return;
    const auto result = this->server->poll();
    if (!result.ready) return;

    if (!result.error.empty()) {
        this->statusText->SetText("Error de autorizacion: " + result.error);
        this->statusText->SetColor(CLR_RED);
        return;
    }

    if (!result.code.empty()) {
        this->statusText->SetText("Codigo recibido. Obteniendo tokens...");
        this->statusText->SetColor(CLR_GREEN);
        this->pendingCode    = result.code;
        this->exchangePending = true;
        // Return now so the status text renders before the blocking HTTP call
    }
}
