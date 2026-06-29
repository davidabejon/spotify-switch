#pragma once
#include <string>

namespace spotify {

struct Tokens {
    std::string accessToken;
    std::string refreshToken;
    long expiresAt;
    bool valid;

    Tokens() : expiresAt(0), valid(false) {}
    Tokens(const std::string& at, const std::string& rt, long ea)
        : accessToken(at), refreshToken(rt), expiresAt(ea), valid(true) {}
};

std::string generateCodeVerifier();
std::string generateCodeChallenge(const std::string& verifier);
std::string generateState();
std::string buildAuthUrl(const std::string& challenge, const std::string& state);
Tokens exchangeCode(const std::string& code, const std::string& verifier);
Tokens refreshAccessToken(const std::string& existingRefreshToken);

} // namespace spotify
