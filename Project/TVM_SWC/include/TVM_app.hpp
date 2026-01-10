#pragma once

#include <string>

#include <nlohmann/json.hpp>

// Forward declare mosquitto struct to keep header light
struct mosquitto;

/*
 * ============================
 * Models / DTOs
 * ============================
 */

struct SaleInput {
    int validityDays = 0;
    std::string lineNumber;
};

struct TicketInfo {
    std::string ticketId;       
    std::string creationDate;   
    int validityDays = 0;
    std::string lineNumber;
    std::string ticketBase64;       

};

struct SaleResult {
    bool success = false;
    std::string errorMessage;
    TicketInfo ticket;
};

/*
 * ============================
 * JSON File Logger
 * Writes a REAL JSON file containing a JSON array:
 * [
 *   {ticketId, creationDate, validityDays, lineNumber},
 *   ...
 * ]
 * ============================
 */
class JsonFileLogger {
public:
    explicit JsonFileLogger(std::string filePath);

    // Append a ticket entry into the JSON array file.
    void appendTicket(const TicketInfo& ticket);

private:
    std::string m_filePath;

    nlohmann::json loadOrCreateArray();
    void saveArray(const nlohmann::json& arr);
};

/*
 * ============================
 * MQTT Adapter (libmosquitto)
 * ============================
 */
#pragma once
#include <string>
#include <memory>

// Paho MQTT C++
#include <mqtt/async_client.h>

class MqttClientAdapter {
public:
    MqttClientAdapter(std::string host, int port, std::string clientId);
    ~MqttClientAdapter();

    bool connect();
    bool publish(const std::string& topic, const std::string& payload);

private:
    std::string m_host;
    int m_port;
    std::string m_clientId;

    std::string m_serverUri;  
    mqtt::connect_options m_connOpts;
    std::unique_ptr<mqtt::async_client> m_client;

    bool m_connected = false;
};

/*
 * ============================
 * REST Adapter (libcurl)
 * - TVM sends POST /api/v1/tickets to Back-Office
 * - Back-Office returns JSON with ticket fields including base64.
 * ============================
 */
class BackOfficeRestClient {
public:
    explicit BackOfficeRestClient(std::string baseUrl, int timeoutSeconds = 5);

    SaleResult createTicket(const SaleInput& input, const std::string& requestId);

private:
    std::string m_baseUrl;
    int m_timeoutSeconds;

    static size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
};

/*
 * ============================
 * TicketSaleService
 * - constructs REST request
 * - waits max 5 seconds
 * - on success: log ticket info + publish base64 to MQTT validation topic
 * ============================
 */
class TicketSaleService {
public:
    TicketSaleService(BackOfficeRestClient& rest,
                      MqttClientAdapter& mqtt,
                      JsonFileLogger& logger,
                      std::string validationTopic = "Sale/request",
                      int timeoutSeconds = 5);

    SaleResult processSale(const SaleInput& input);

private:
    BackOfficeRestClient& m_rest;
    MqttClientAdapter& m_mqtt;
    JsonFileLogger& m_logger;

    std::string m_validationTopic;
    int m_timeoutSeconds;

    std::string newRequestId() const;
};

/*
 * ============================
 * SaleRequestHandler
 * - gets user input
 * - validates
 * - invokes service
 * - handles retry loop
 * ============================
 */
class SaleRequestHandler {
public:
    explicit SaleRequestHandler(TicketSaleService& service);
    void run();
 
private:
    TicketSaleService& m_service;

    SaleInput readInput();
    bool isValid(const SaleInput& input);
};

#define TVM_MAX_VALIDITY_DAYS (int)30
#define TVM_MAX_LINE_NUMBER (int)4
