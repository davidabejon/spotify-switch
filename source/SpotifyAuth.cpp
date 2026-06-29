#include <SpotifyAuth.hpp>
#include <curl/curl.h>
#include <mbedtls/sha256.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>

namespace spotify {

static const char CLIENT_ID[] = "2979d578384e470a9a8440f7a495f611";
static const char SCOPES[] =
    "user-read-playback-state "
    "user-modify-playback-state "
    "user-read-currently-playing";

// --- Base64url (no padding, URL-safe alphabet) ---

static const char B64URL_TABLE[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

static std::string base64url(const uint8_t* data, size_t len) {
    std::string out;
    out.reserve(((len + 2) / 3) * 4);
    for (size_t i = 0; i < len; i += 3) {
        uint32_t triple = (static_cast<uint32_t>(data[i]) << 16);
        if (i + 1 < len) triple |= (static_cast<uint32_t>(data[i + 1]) << 8);
        if (i + 2 < len) triple |=  static_cast<uint32_t>(data[i + 2]);

        out += B64URL_TABLE[(triple >> 18) & 0x3F];
        out += B64URL_TABLE[(triple >> 12) & 0x3F];
        if (i + 1 < len) out += B64URL_TABLE[(triple >> 6) & 0x3F];
        if (i + 2 < len) out += B64URL_TABLE[ triple       & 0x3F];
    }
    return out;
}

// --- Percent-encoding for URL query parameters ---

static std::string urlEncode(const std::string& s) {
    std::string out;
    out.reserve(s.size() * 3);
    for (unsigned char c : s) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out += static_cast<char>(c);
        } else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", c);
            out += buf;
        }
    }
    return out;
}

// --- Minimal JSON field extraction ---

static std::string jsonGetString(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\":\"";
    const auto pos = json.find(needle);
    if (pos == std::string::npos) return "";
    const auto start = pos + needle.size();
    const auto end = json.find('"', start);
    if (end == std::string::npos) return "";
    return json.substr(start, end - start);
}

static long jsonGetLong(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\":";
    const auto pos = json.find(needle);
    if (pos == std::string::npos) return 0;
    return strtol(json.c_str() + pos + needle.size(), nullptr, 10);
}

// --- curl write callback ---

static size_t curlWrite(void* ptr, size_t size, size_t nmemb, std::string* out) {
    out->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

// --- HTTP POST to Spotify API ---

static void ensureCurlInit() {
    static bool initialized = false;
    if (!initialized) {
        curl_global_init(CURL_GLOBAL_ALL);
        initialized = true;
    }
}

static std::string httpPost(const std::string& url, const std::string& body) {
    ensureCurlInit();
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    // SSL peer verification is disabled because Switch homebrew does not have
    // access to a system CA store. Communication is still encrypted (TLS).
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return response;
}

// --- Random bytes from /dev/urandom ---

static void randomBytes(uint8_t* buf, size_t len) {
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        ssize_t n = read(fd, buf, len);
        (void)n;
        close(fd);
    }
}

// --- Public API ---

std::string generateCodeVerifier() {
    uint8_t bytes[32] = {};
    randomBytes(bytes, sizeof(bytes));
    return base64url(bytes, sizeof(bytes));
}

std::string generateCodeChallenge(const std::string& verifier) {
    uint8_t hash[32] = {};
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0 /* SHA-256, not SHA-224 */);
    mbedtls_sha256_update(&ctx,
        reinterpret_cast<const unsigned char*>(verifier.c_str()),
        verifier.size());
    mbedtls_sha256_finish(&ctx, hash);
    mbedtls_sha256_free(&ctx);
    return base64url(hash, sizeof(hash));
}

std::string generateState() {
    uint8_t bytes[12] = {};
    randomBytes(bytes, sizeof(bytes));
    return base64url(bytes, sizeof(bytes));
}

std::string buildAuthUrl(const std::string& challenge, const std::string& state) {
    return std::string("https://accounts.spotify.com/authorize")
        + "?response_type=code"
        + "&client_id="             + urlEncode(CLIENT_ID)
        + "&scope="                 + urlEncode(SCOPES)
        + "&redirect_uri="          + urlEncode(REDIRECT_URI)
        + "&state="                 + urlEncode(state)
        + "&code_challenge_method=S256"
        + "&code_challenge="        + urlEncode(challenge);
}

Tokens exchangeCode(const std::string& code, const std::string& verifier) {
    const std::string body =
        std::string("grant_type=authorization_code")
        + "&code="          + urlEncode(code)
        + "&redirect_uri="  + urlEncode(REDIRECT_URI)
        + "&client_id="     + urlEncode(CLIENT_ID)
        + "&code_verifier=" + urlEncode(verifier);

    const auto response = httpPost("https://accounts.spotify.com/api/token", body);
    if (response.empty()) return Tokens();

    const auto at = jsonGetString(response, "access_token");
    if (at.empty()) return Tokens();

    const auto rt = jsonGetString(response, "refresh_token");
    const auto expiresIn = jsonGetLong(response, "expires_in");
    return Tokens(at, rt, static_cast<long>(time(nullptr)) + expiresIn);
}

Tokens refreshAccessToken(const std::string& existingRefreshToken) {
    const std::string body =
        std::string("grant_type=refresh_token")
        + "&refresh_token=" + urlEncode(existingRefreshToken)
        + "&client_id="     + urlEncode(CLIENT_ID);

    const auto response = httpPost("https://accounts.spotify.com/api/token", body);
    if (response.empty()) return Tokens();

    const auto at = jsonGetString(response, "access_token");
    if (at.empty()) return Tokens();

    auto rt = jsonGetString(response, "refresh_token");
    if (rt.empty()) rt = existingRefreshToken; // Spotify may reuse the same token

    const auto expiresIn = jsonGetLong(response, "expires_in");
    return Tokens(at, rt, static_cast<long>(time(nullptr)) + expiresIn);
}

} // namespace spotify
