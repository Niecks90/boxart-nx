/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#include "steamgriddb.hpp"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

using json = nlohmann::json;

namespace BoxArt {

SteamGridDBClient::SteamGridDBClient() {
    LoadApiKey();
}

SteamGridDBClient::~SteamGridDBClient() {
}

bool SteamGridDBClient::LoadApiKey() {
    FILE* f = std::fopen(CONFIG_PATH, "r");
    if (!f) return false;

    char line[256];
    while (std::fgets(line, sizeof(line), f)) {
        std::string s = line;
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos || s[start] == '#' || s[start] == ';' || s[start] == '[') {
            continue;
        }

        size_t eq = s.find('=');
        if (eq != std::string::npos) {
            std::string k = s.substr(start, eq - start);
            std::string v = s.substr(eq + 1);

            // Trim key
            size_t k_end = k.find_last_not_of(" \t\r\n");
            if (k_end != std::string::npos) k = k.substr(0, k_end + 1);

            // Trim value
            size_t v_start = v.find_first_not_of(" \t\r\n");
            size_t v_end = v.find_last_not_of(" \t\r\n");
            if (v_start != std::string::npos && v_end != std::string::npos) {
                v = v.substr(v_start, v_end - v_start + 1);
            } else {
                v.clear();
            }

            if (k == "api_key" || k == "apiKey" || k == "key") {
                m_api_key = v;
            }
        }
    }

    std::fclose(f);
    return !m_api_key.empty();
}

void SteamGridDBClient::SetApiKey(const std::string& key) {
    m_api_key = key;

    mkdir("sdmc:/config", 0777);
    mkdir("sdmc:/config/boxart-nx", 0777);

    FILE* f = std::fopen(CONFIG_PATH, "w");
    if (f) {
        std::fprintf(f, "[steamgriddb]\napi_key=%s\n", m_api_key.c_str());
        std::fclose(f);
    }
}

static size_t WriteStringCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    std::string* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), total_size);
    return total_size;
}

static size_t WriteBinaryCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    auto* vec = static_cast<std::vector<uint8_t>*>(userp);
    const uint8_t* bytes = static_cast<const uint8_t*>(contents);
    vec->insert(vec->end(), bytes, bytes + total_size);
    return total_size;
}

bool SteamGridDBClient::PerformGetRequest(const std::string& url, std::string& out_response, std::string& out_error) {
    out_response.clear();
    CURL* curl = curl_easy_init();
    if (!curl) {
        out_error = "curl_easy_init failed";
        return false;
    }

    struct curl_slist* headers = nullptr;
    if (!m_api_key.empty()) {
        std::string auth_header = "Authorization: Bearer " + m_api_key;
        headers = curl_slist_append(headers, auth_header.c_str());
    }
    headers = curl_slist_append(headers, "Accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteStringCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out_response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "boxart-nx/1.0.0");

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        out_error = std::string("Network error: ") + curl_easy_strerror(res);
        return false;
    }

    if (http_code != 200) {
        out_error = "HTTP error " + std::to_string(http_code);
        return false;
    }

    return true;
}

static std::string UrlEncode(const std::string& str) {
    std::string encoded;
    for (char c : str) {
        if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else if (c == ' ') {
            encoded += "%20";
        } else {
            char buf[4];
            std::snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)c);
            encoded += buf;
        }
    }
    return encoded;
}

bool SteamGridDBClient::SearchGames(const std::string& query, std::vector<SteamGameMatch>& out_matches, std::string& out_error) {
    out_matches.clear();
    if (m_api_key.empty()) {
        out_error = "SteamGridDB API key not set";
        return false;
    }

    std::string enc = UrlEncode(query);
    std::string url = "https://www.steamgriddb.com/api/v2/search/autocomplete/" + enc;

    std::string response;
    if (!PerformGetRequest(url, response, out_error)) {
        return false;
    }

    json j;
    try {
        j = json::parse(response);
    } catch (const std::exception& e) {
        out_error = std::string("JSON parse error: ") + e.what();
        return false;
    }

    if (!j.value("success", false)) {
        out_error = "API returned failure";
        return false;
    }

    if (j.contains("data") && j["data"].is_array()) {
        for (const auto& item : j["data"]) {
            SteamGameMatch match;
            if (item.contains("id")) match.id = item["id"].get<int>();
            if (item.contains("name")) match.name = item["name"].get<std::string>();
            if (item.contains("release_date") && !item["release_date"].is_null()) {
                match.release_date = item["release_date"].get<int>();
            }
            out_matches.push_back(std::move(match));
        }
    }

    return true;
}

bool SteamGridDBClient::FetchGridsForGame(int game_id, std::vector<SteamGridResult>& out_results, std::string& out_error) {
    out_results.clear();
    if (m_api_key.empty()) {
        out_error = "SteamGridDB API key not set";
        return false;
    }

    // Request square grids (dimensions 600x600, 1024x1024)
    std::string url = "https://www.steamgriddb.com/api/v2/grids/game/" + std::to_string(game_id) +
                      "?dimensions=600x600,1024x1024&mimes=image/png,image/jpeg,image/webp&types=static";

    std::string response;
    if (!PerformGetRequest(url, response, out_error)) {
        return false;
    }

    json j;
    try {
        j = json::parse(response);
    } catch (const std::exception& e) {
        out_error = std::string("JSON parse error: ") + e.what();
        return false;
    }

    if (!j.value("success", false)) {
        out_error = "API returned failure";
        return false;
    }

    if (j.contains("data") && j["data"].is_array()) {
        for (const auto& item : j["data"]) {
            SteamGridResult res;
            if (item.contains("id")) res.id = item["id"].get<int>();
            if (item.contains("width")) res.width = item["width"].get<int>();
            if (item.contains("height")) res.height = item["height"].get<int>();
            if (item.contains("score")) res.score = item["score"].get<int>();
            if (item.contains("style") && !item["style"].is_null()) res.style = item["style"].get<std::string>();
            if (item.contains("url")) res.url = item["url"].get<std::string>();
            if (item.contains("thumb")) res.thumb_url = item["thumb"].get<std::string>();
            if (item.contains("nsfw")) res.nsfw = item["nsfw"].get<bool>();
            out_results.push_back(std::move(res));
        }
    }

    return true;
}

bool SteamGridDBClient::DownloadImage(const std::string& url, std::vector<uint8_t>& out_data, std::string& out_error) {
    out_data.clear();
    CURL* curl = curl_easy_init();
    if (!curl) {
        out_error = "curl_easy_init failed";
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteBinaryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out_data);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "boxart-nx/1.0.0");

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        out_error = std::string("Download failed: ") + curl_easy_strerror(res);
        return false;
    }

    if (http_code != 200 || out_data.empty()) {
        out_error = "Failed to download image (HTTP " + std::to_string(http_code) + ")";
        return false;
    }

    return true;
}

} // namespace BoxArt
