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

struct PlayerState {
    std::string trackName;
    std::string artistName;
    std::string artistId;
    std::string albumId;
    std::string deviceName;
    std::string albumImageUrl;
    bool isPlaying;
    bool valid;

    PlayerState() : isPlaying(false), valid(false) {}
};

struct ArtistInfo {
    std::string name;
    std::string imageUrl;
    std::string genres;
    long followers;
    int popularity;
    bool valid;

    ArtistInfo() : followers(0), popularity(0), valid(false) {}
};

std::string generateCodeVerifier();
std::string generateCodeChallenge(const std::string& verifier);
std::string generateState();
std::string buildAuthUrl(const std::string& challenge, const std::string& state);
Tokens exchangeCode(const std::string& code, const std::string& verifier);
Tokens refreshAccessToken(const std::string& existingRefreshToken);
PlayerState getPlayerState(const std::string& accessToken);
ArtistInfo  getArtistInfo(const std::string& artistId, const std::string& accessToken);

struct AlbumInfo {
    std::string name;
    std::string albumType;    // "album", "single", "compilation"
    std::string releaseDate;  // e.g. "2023-04-14"
    std::string label;
    int totalTracks;
    int popularity;
    bool valid;

    AlbumInfo() : totalTracks(0), popularity(0), valid(false) {}
};

AlbumInfo   getAlbumInfo(const std::string& albumId, const std::string& accessToken);

// Album art — no auth needed (Spotify CDN)
std::string downloadAlbumArt(const std::string& url);

// Playback control
void play(const std::string& accessToken);
void pause(const std::string& accessToken);
void skipNext(const std::string& accessToken);
void skipPrevious(const std::string& accessToken);

} // namespace spotify
