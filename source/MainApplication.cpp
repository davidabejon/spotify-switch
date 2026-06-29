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

// Returns the Switch's local IP, or empty string if not connected to a network.
// Connects a UDP socket to an external address (no packet sent) so the kernel
// fills in the local address via getsockname.
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

// Encodes an IPv4 address for embedding in the OAuth state parameter.
// Dots are replaced with underscores so the result is base64url-safe.
// Example: "192.168.1.100" -> "192_168_1_100"
static std::string encodeIp(const std::string& ip) {
    std::string out = ip;
    for (char& c : out) if (c == '.') c = '_';
    return out;
}

// --- MainLayout ---

MainLayout::MainLayout() : Layout::Layout() {
    this->SetBackgroundColor(pu::ui::Color(18, 18, 18, 255));

    auto title = pu::ui::elm::TextBlock::New(0, 120, "Spotify Switch");
    title->SetColor(pu::ui::Color(29, 185, 84, 255));
    title->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Large));
    title->SetX(960 - 180);
    this->Add(title);

    this->statusText = pu::ui::elm::TextBlock::New(0, 300, "Iniciando...");
    this->statusText->SetColor(pu::ui::Color(200, 200, 200, 255));
    this->statusText->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
    this->statusText->SetX(960 - 250);
    this->Add(this->statusText);

    auto hint = pu::ui::elm::TextBlock::New(960 - 120, 1030, "Pulsa + para salir");
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

    // OAuth requires WiFi — detect local IP first
    const std::string ip = getLocalIp();
    if (ip.empty()) {
        this->mainLayout->SetStatus(
            "Conecta la Switch a una red WiFi para iniciar sesion con Spotify.");
        this->LoadLayout(this->mainLayout);
        return;
    }

    // Embed the Switch IP in the OAuth state so the GitHub Pages relay can
    // redirect the mobile browser back to the local server automatically.
    // Format: "<random_base64url>.<ip_with_underscores>"
    const auto verifier     = spotify::generateCodeVerifier();
    const auto challenge    = spotify::generateCodeChallenge(verifier);
    const auto randomState  = spotify::generateState();
    const auto fullState    = randomState + "." + encodeIp(ip);
    const auto authUrl      = spotify::buildAuthUrl(challenge, fullState);

    // Start local HTTP server that will receive the code from the relay redirect
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

void MainApplication::OnLoginSuccess(const spotify::Tokens& tokens) {
    debugLog("APP: OnLoginSuccess");
    if (this->localServer) {
        this->localServer->stop();
        this->localServer.reset();
    }
    debugLog("APP: server stopped");
    TokenStorage::saveTokens(tokens);
    debugLog("APP: tokens saved");
    this->mainLayout->SetStatus("Sesion iniciada con Spotify. Bienvenido!");
    debugLog("APP: SetStatus done");
    this->LoadLayout(this->mainLayout);
    debugLog("APP: LoadLayout done");
}
