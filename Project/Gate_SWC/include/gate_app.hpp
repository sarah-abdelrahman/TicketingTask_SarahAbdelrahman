#pragma once

#include <string>
#include <nlohmann/json.hpp>

struct ValidationResult {
    bool ok = false;            // HTTP call + JSON parse succeeded
    bool valid = false;         // server says ticket valid/invalid
    long httpCode = 0;          // HTTP status
    std::string errorMessage;   // non-empty if ok == false
};

class BackOfficeValidatorClient {
public:
    BackOfficeValidatorClient(std::string baseUrl, int timeoutSeconds);

    ValidationResult validateTicket(const std::string& ticketBase64) const;

private:
    std::string m_baseUrl;
    std::string m_validateUrl;
    int m_timeoutSeconds;

    static size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
    static bool postJson(const std::string& url,
                         const std::string& body,
                         int timeoutSeconds,
                         long& httpCodeOut,
                         std::string& responseOut,
                         std::string& errorOut);
};

class GateConsoleApp {
public:
    explicit GateConsoleApp(BackOfficeValidatorClient client);

    void run();

private:
    BackOfficeValidatorClient m_client;

    static std::string readTicketBase64();
};
