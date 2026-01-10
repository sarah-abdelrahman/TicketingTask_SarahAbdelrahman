#include "TVM_app.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <future>
#include <random>
#include <cstdlib>
using nlohmann::json;


// -------------------- small helpers --------------------

static std::string envOr(const char* key, const char* defVal) {
    if (const char* v = std::getenv(key)) return v;
    return defVal;
}

static int envIntOr(const char* key, int defVal) {
    try {
        return std::stoi(envOr(key, ""));
    } catch (...) {
        return defVal;
    }
}

// ==================== JsonFileLogger ====================

JsonFileLogger::JsonFileLogger(std::string filePath)
    : m_filePath(std::move(filePath)) {}

json JsonFileLogger::loadOrCreateArray() {
    std::ifstream in(m_filePath);
    if (!in.good()) {
        return json::array();
    }

    try {
        json data;
        in >> data;
        if (!data.is_array()) {
            // ASSUMPTION: if file content is not array, reset to empty array.
            return json::array();
        }
        return data;
    } catch (...) {
        // ASSUMPTION: if file is corrupted, reset to empty array.
        return json::array();
    }
}

void JsonFileLogger::saveArray(const json& arr) {
    std::ofstream out(m_filePath, std::ios::trunc);
    out << arr.dump(2); // pretty JSON
}

void JsonFileLogger::appendTicket(const TicketInfo& ticket) {
    json arr = loadOrCreateArray();

    // Log ONLY required fields:
    // - Ticket ID
    // - Creation Date
    // - Validity in Days
    // - Line Number
    json entry = {
        {"ticketId", ticket.ticketId},
        {"creationDate", ticket.creationDate},
        {"validityDays", ticket.validityDays},
        {"lineNumber", ticket.lineNumber}
    };

    arr.push_back(entry);
    saveArray(arr);
}

// ==================== MqttClientAdapter ====================

MqttClientAdapter::MqttClientAdapter(std::string host, int port, std::string clientId)
    : m_host(std::move(host)), m_port(port), m_clientId(std::move(clientId)) {

    m_serverUri = "tcp://" + m_host + ":" + std::to_string(m_port);
    m_client = std::make_unique<mqtt::async_client>(m_serverUri, m_clientId);

    m_connOpts = mqtt::connect_options{};
    m_connOpts.set_clean_session(true);
    m_connOpts.set_keep_alive_interval(20);

    m_connOpts.set_automatic_reconnect(true);
}

MqttClientAdapter::~MqttClientAdapter() {
    try {
        if (m_client && m_client->is_connected()) {
            m_client->disconnect()->wait();
            std::cout << "[MQTT] Disconnected successfully\n";
        }
    } catch (const mqtt::exception& e) {
        // Destructor must not throw
        std::cerr << "[MQTT] Disconnect error: " << e.what() << "\n";
    }
}

bool MqttClientAdapter::connect() {
    if (!m_client) {
        std::cerr << "[MQTT] Client not initialized\n";
        return false;
    }

    try {
        std::cout << "[MQTT] Connecting to " << m_serverUri << " ...\n";

        m_client->connect(m_connOpts)->wait();

        m_connected = m_client->is_connected();
        if (m_connected) {
            std::cout << "[MQTT] Connected successfully\n";
        } else {
            std::cerr << "[MQTT] Connect failed (no exception, but not connected)\n";
        }
        return m_connected;

    } catch (const mqtt::exception& e) {
        std::cerr << "[MQTT] Connect exception: " << e.what() << "\n";
        m_connected = false;
        return false;
    }
}

bool MqttClientAdapter::publish(const std::string& topic,
                                const std::string& payload) {
    if (!m_client || !m_connected || !m_client->is_connected()) {
        std::cerr << "[MQTT] Publish failed: not connected\n";
        return false;
    }

    try {
        const int qos = 1;
        const bool retained = false;

        auto msg = mqtt::make_message(topic, payload);
        msg->set_qos(qos);
        msg->set_retained(retained);

        m_client->publish(msg)->wait();

        std::cout << "[MQTT] Published successfully\n"
                  << "       Topic: " << topic << "\n"
                  << "       Payload: " << payload << "\n";

        return true;

    } catch (const mqtt::exception& e) {
        std::cerr << "[MQTT] Publish exception: " << e.what() << "\n";
        return false;
    }
}



// ==================== BackOfficeRestClient (libcurl) ====================

BackOfficeRestClient::BackOfficeRestClient(std::string baseUrl, int timeoutSeconds)
    : m_baseUrl(std::move(baseUrl)), m_timeoutSeconds(timeoutSeconds) {
}

size_t BackOfficeRestClient::writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* out = static_cast<std::string*>(userdata);
    out->append(ptr, size * nmemb);
    return size * nmemb;
}

SaleResult BackOfficeRestClient::createTicket(const SaleInput& input, const std::string& requestId) {
    SaleResult result;
            return result;
}

// ==================== TicketSaleService ====================

TicketSaleService::TicketSaleService(BackOfficeRestClient& rest,
                                     MqttClientAdapter& mqtt,
                                     JsonFileLogger& logger,
                                     std::string validationTopic,
                                     int timeoutSeconds)
    : m_rest(rest),
      m_mqtt(mqtt),
      m_logger(logger),
      m_validationTopic(std::move(validationTopic)),
      m_timeoutSeconds(timeoutSeconds) {}


SaleResult TicketSaleService::processSale(const SaleInput& input) {
    const std::string requestId;
    SaleResult result;
    result.success=true;

    // Log ticket info to JSON file
    m_logger.appendTicket(result.ticket);

    // Publish base64 for Gate validation
    json msg = {
        {"requestId", requestId},
        {"ticketBase64", result.ticket.ticketBase64}
    };
 
    const bool published = m_mqtt.publish(m_validationTopic, msg.dump());
    if (!published) {
        // ASSUMPTION:
        // Ticket is created but not delivered to Gate. Decide system behavior.
        // Option A (current): treat as failure for TVM user experience.
        result.success = false;
        result.errorMessage = "Ticket created, but MQTT publish failed.";

        // Option B: keep success true and only warn:
        // result.success = true;
        // result.errorMessage = "Warning: MQTT publish failed.";

    }    else
    {
            std::cout << "\npuplish success ";

    }
    
    return result;
}

// ==================== SaleRequestHandler ====================

SaleRequestHandler::SaleRequestHandler(TicketSaleService& service)
    : m_service(service) {}

SaleInput SaleRequestHandler::readInput() {
    SaleInput in{};
    std::cout << "\nEnter validity days: ";
    std::cin >> in.validityDays;
    std::cout << "Enter line number: ";
    std::cin >> in.lineNumber;
    return in;
}

bool SaleRequestHandler::isValid(const SaleInput& input) {
    // Minimal validation
    if (input.validityDays <= 0) return false;
    if (input.lineNumber.empty()) return false;
    return true;
}

void SaleRequestHandler::run() {
    while (true) {
        SaleInput input = readInput();

        if (!isValid(input)) {
            std::cout << "Not valid input. Please try again.\n";
            continue;
        }

        SaleResult result = m_service.processSale(input);
        if (!result.success) {
            std::cout << "ERROR: " << result.errorMessage << "\n";
            continue;
        }

        std::cout << "Ticket created successfully: " << result.ticket.ticketId << "\n";
    }
}

// ==================== main() ====================

int main() {
    
    //   MQTT_HOST=127.0.0.1 
    //   MQTT_PORT must match mosquitto.conf listener port
    const std::string mqttHost = envOr("MQTT_HOST", "127.0.0.1");
    const int mqttPort = envIntOr("MQTT_PORT", 1883);

    // If Back-Office runs in same container, 127.0.0.1
    const std::string backofficeBase = envOr("BACKOFFICE_BASE_URL", "http://127.0.0.1:8080");

    const std::string validationTopic = envOr("Sale_TOPIC", "Sale/request");
    const std::string logFile = envOr("TICKETS_LOG_FILE", "tickets.json");

    std::cout << "[CONFIG] MQTT=" << mqttHost << ":" << mqttPort << "\n";
    std::cout << "[CONFIG] BackOffice=" << backofficeBase << "\n";
    std::cout << "[CONFIG] ValidationTopic=" << validationTopic << "\n";
    std::cout << "[CONFIG] LogFile=" << logFile << "\n";

    // Setup dependencies
    JsonFileLogger logger(logFile);
    BackOfficeRestClient rest(backofficeBase, 5);

    MqttClientAdapter mqtt(mqttHost, mqttPort, "tvm-client");
    if (!mqtt.connect()) {
        std::cerr << "ERROR: Failed to connect to MQTT broker at "
                  << mqttHost << ":" << mqttPort << "\n";
        return 1;
    }

    TicketSaleService service(rest, mqtt, logger, validationTopic, 5);
    SaleRequestHandler handler(service);
    handler.run();

    return 0;
}
