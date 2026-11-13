/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <SDL.h>

namespace BoxArt {

struct SteamGridResult {
    int id = 0;
    int width = 0;
    int height = 0;
    int score = 0;
    std::string style;
    std::string url;
    std::string thumb_url;
    bool nsfw = false;

    // Downloaded thumbnail texture for UI preview
    SDL_Texture* thumb_texture = nullptr;
    std::vector<uint8_t> thumb_data;

    void FreeTexture();
};

struct SteamGameMatch {
    int id = 0;
    std::string name;
    int release_date = 0;
};

class SteamGridDBClient {
public:
    static constexpr const char* CONFIG_PATH = "sdmc:/config/boxart-nx/config.ini";

    SteamGridDBClient();
    ~SteamGridDBClient();

    // Loads API key from sdmc:/config/boxart-nx/config.ini
    bool LoadApiKey();

    // Sets and saves API key
    void SetApiKey(const std::string& key);

    const std::string& GetApiKey() const { return m_api_key; }
    bool HasApiKey() const { return !m_api_key.empty(); }

    // Searches for games matching query name
    bool SearchGames(const std::string& query, std::vector<SteamGameMatch>& out_matches, std::string& out_error);

    // Fetches grid covers for a game ID (dimensions 600x600, 1024x1024)
    bool FetchGridsForGame(int game_id, std::vector<SteamGridResult>& out_results, std::string& out_error);

    // Downloads image from URL into memory buffer
    bool DownloadImage(const std::string& url, std::vector<uint8_t>& out_data, std::string& out_error);

    // Downloads and creates an SDL texture for preview
    SDL_Texture* LoadPreviewTexture(SDL_Renderer* renderer, const std::string& url);

private:
    std::string m_api_key;
    bool PerformGetRequest(const std::string& url, std::string& out_response, std::string& out_error);
};

} // namespace BoxArt
