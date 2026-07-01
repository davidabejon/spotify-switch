#include <MainApplication.hpp>
#include <LayoutConstants.hpp>
#include <SpotifyAuth.hpp>
#include <DebugLog.hpp>
#include <cstdio>

static std::string capitalizeFirst(const std::string& s) {
    if (s.empty()) return s;
    std::string out = s;
    out[0] = static_cast<char>(toupper(static_cast<unsigned char>(out[0])));
    return out;
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

// --- MainLayout user content setters ---

void MainLayout::SetUserProfile(const spotify::UserProfile& profile) {
    if (!profile.valid) return;
    this->userNameText->SetText(profile.displayName);
    // Scale flag to match name text height (3:2 aspect ratio covers most flags)
    const s32 nameH = this->userNameText->GetHeight();
    this->userFlagImg->SetWidth(nameH * 3 / 2);
    this->userFlagImg->SetHeight(nameH);
    this->userFlagImg->SetX(UINFO_X + this->userNameText->GetWidth() + 12);
    this->userFlagImg->SetY(UNAME_Y);
    this->userEmailText->SetText(profile.email.empty() ? "" : profile.email);
    this->userPlanText->SetText(profile.product.empty() ? "" : "Plan: " + capitalizeFirst(profile.product));
    this->userFollowersText->SetText(formatFollowers(profile.followers));
}

void MainLayout::SetUserFlag(pu::sdl2::TextureHandle::Ref handle) {
    this->userFlagImg->SetImage(handle);
    if (this->currentTab == Tab::User)
        this->userFlagImg->SetVisible(true);
}

void MainLayout::SetUserAvatar(pu::sdl2::TextureHandle::Ref handle) {
    this->userAvatarImg->SetImage(handle);
    this->userAvatarImg->SetWidth(UAVATAR_SIZE);
    this->userAvatarImg->SetHeight(UAVATAR_SIZE);
}

// --- MainApplication user methods ---

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

    if (!profile.country.empty()) {
        std::string cc = profile.country;
        for (char& c : cc) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
        const auto flagData = spotify::downloadAlbumArt("https://flagcdn.com/w80/" + cc + ".png");
        if (!flagData.empty()) {
            auto* rawTex = pu::ui::render::LoadImageFromBuffer(
                static_cast<const void*>(flagData.data()), flagData.size());
            if (rawTex)
                this->mainLayout->SetUserFlag(pu::sdl2::TextureHandle::New(rawTex));
        }
    }
}
