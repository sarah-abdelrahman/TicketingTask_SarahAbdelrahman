#include <iostream>
#include <string>
#include <cstdlib>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using nlohmann::json;

static std::string envOr(const char* key, const char* defVal) {
    if (const char* v = std::getenv(key)) return v;
    return defVal;
}

static size_t write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* out = static_cast<std::string*>(userdata);
    out->append(ptr, size * nmemb);
    return size * nmemb;
}

static bool postJson(const std::string& url, const std::string& body, int timeoutSec,
                     long& httpCodeOut, std::string& respOut) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body.size());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSec);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &respOut);

    CURLcode rc = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCodeOut);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return rc == CURLE_OK;
}

int main() {
    const std::string backofficeBase = envOr("BACKOFFICE_BASE_URL", "http://127.0.0.1:8080");
    const std::string validateUrl = backofficeBase + "/api/v1/validate";

    std::cout << "[GATE] BackOffice Validate URL: " << validateUrl << "\n";

    curl_global_init(CURL_GLOBAL_DEFAULT);

    while (true) {
        std::string ticketBase64;
        std::cout << "\nEnter ticketBase64 (or 'exit'): ";
        std::cin >> ticketBase64;

        if (ticketBase64 == "exit") break;

        json req = { {"ticketBase64", ticketBase64} };

        long httpCode = 0;
        std::string resp;

        const bool ok = postJson(validateUrl, req.dump(), 5, httpCode, resp);
        if (!ok) {
            std::cout << "[GATE] ERROR: REST request failed\n";
            continue;
        }
        if (httpCode < 200 || httpCode >= 300) {
            std::cout << "[GATE] ERROR: HTTP " << httpCode << " resp=" << resp << "\n";
            continue;
        }

        try {
            json j = json::parse(resp);
            const bool valid = j.value("valid", false);

            if (valid) std::cout << "[GATE] Ticket VALID\n";
            else       std::cout << "[GATE] Ticket INVALID\n";
        }
        catch (...) {
            std::cout << "[GATE] ERROR: invalid JSON response: " << resp << "\n";
        }
    }

    curl_global_cleanup();
    return 0;
}
