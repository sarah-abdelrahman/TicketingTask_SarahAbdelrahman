#pragma once

#include <string>
#include <cstdint>
#include <nlohmann/json.hpp>

// ============================
// Configuration (no magic numbers)
// ============================
struct BackOfficeConfig {
    std::string bindIp = "127.0.0.1";         // same container access
    uint16_t port = 8080;                     // REST port
    int listenBacklog = 5;                    // pending connections
    size_t readBufferSize = 4096;             // request buffer
    std::string ticketsPath = "tickets_stock.json"; // stock file
    std::string ticketsEndpoint = "/api/v1/tickets";
};

// ============================
// Ticket model
// ============================
struct Ticket {
    std::string creationDateUtc; // ISO 8601 UTC
    int validityDays = 0;
    std::string lineNumber;
    std::string ticketBase64;    // encoded ticket payload
};

// ============================
// Utility
// ============================
class TimeUtils {
public:
    static std::string nowUtcIso8601();
};

class Base64 {
public:
    static std::string encode(const std::string& in);
};

// ============================
// Stock file (JSON array)
// ============================
class TicketStock {
public:
    explicit TicketStock(std::string filePath);

    // Append ticket entry to stock JSON file
    void append(const Ticket& t);

private:
    std::string m_filePath;

    nlohmann::json loadOrCreateArray();
    void saveArray(const nlohmann::json& arr);
};

// ============================
// HTTP Server (minimal POSIX)
// ============================
class BackOfficeServer {
public:
    BackOfficeServer(BackOfficeConfig cfg, TicketStock& stock);

    // blocking loop
    int run();

private:
    BackOfficeConfig m_cfg;
    TicketStock& m_stock;

    int createListenSocket();
    void handleClient(int clientFd);

    // Routing & handlers
    bool isTicketCreateRequest(const std::string& req) const;
    std::string extractBody(const std::string& req) const;

    // Business logic
    std::string handleCreateTicket(const std::string& bodyJson);

    // Response builders
    static std::string httpJsonResponse(int statusCode, const std::string& jsonBody);
    static std::string httpEmptyResponse(int statusCode);

    static const char* statusText(int statusCode);
};
