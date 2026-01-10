#include "Backoffice_app.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <random>
#include <cstring>
#include <iomanip>  

// POSIX sockets
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

using nlohmann::json;

// ============================
// TimeUtils helper
// ============================
std::string TimeUtils::nowUtcIso8601() {
    std::time_t t = std::time(nullptr);
    std::tm utc{};
#if defined(_WIN32)
    gmtime_s(&utc, &t);
#else
    gmtime_r(&t, &utc);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &utc);
    return std::string(buf);
}

static std::time_t parseUtcIso8601(const std::string& s) {
    // Input format: "YYYY-MM-DDTHH:MM:SSZ"
    std::tm tm{};
    std::istringstream iss(s);
    iss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    if (iss.fail()) return (std::time_t)-1;

    // ASSUMPTION: Linux/glibc -> timegm exists
    return timegm(&tm);
}

static bool isExpiredUtc(const std::string& creationUtc, int validityDays) {
    if (validityDays <= 0) return true;

    std::time_t created = parseUtcIso8601(creationUtc);
    if (created == (std::time_t)-1) return true;

    const std::time_t now = std::time(nullptr);
    const std::time_t expires = created + (std::time_t)validityDays * 24 * 60 * 60;
    return now > expires;
}

// ============================
// Base64
// ============================
std::string Base64::encode(const std::string& in) {
    static const char* tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, valb = -6;

    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(tbl[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(tbl[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

// ============================
// TicketStock
// ============================
TicketStock::TicketStock(std::string filePath)
    : m_filePath(std::move(filePath)) {}

json TicketStock::loadOrCreateArray() {
    std::ifstream in(m_filePath);
    if (!in.good()) return json::array();

    try {
        json data;
        in >> data;
        if (!data.is_array()) return json::array();
        return data;
    } catch (...) {
        return json::array();
    }
}

void TicketStock::saveArray(const json& arr) {
    std::ofstream out(m_filePath, std::ios::trunc);
    out << arr.dump(2);
}

void TicketStock::append(const Ticket& t) {
    json arr = loadOrCreateArray();

    json entry = {
        {"creationDateUtc", t.creationDateUtc},
        {"validityDays", t.validityDays},
        {"lineNumber", t.lineNumber},
        {"ticketBase64", t.ticketBase64}
    };

    arr.push_back(entry);
    saveArray(arr);
}

// NEW: lookup function for validation
bool TicketStock::containsTicketBase64(const std::string& ticketBase64, Ticket& out) const {
    // Reuse same file read logic (const-correctness not strict here; minimal change)
    // If you want strict const, make loadOrCreateArray() const as well.
    std::ifstream in(m_filePath);
    if (!in.good()) return false;

    json arr;
    try {
        in >> arr;
        if (!arr.is_array()) return false;
    } catch (...) {
        return false;
    }

    for (const auto& e : arr) {
        if (!e.is_object()) continue;
        if (e.value("ticketBase64", "") == ticketBase64) {
            out.ticketBase64 = ticketBase64;
            out.creationDateUtc = e.value("creationDateUtc", "");
            out.validityDays = e.value("validityDays", 0);
            out.lineNumber = e.value("lineNumber", "");
            return true;
        }
    }
    return false;
}

// ============================
// BackOfficeServer
// ============================
BackOfficeServer::BackOfficeServer(BackOfficeConfig cfg, TicketStock& stock)
    : m_cfg(std::move(cfg)), m_stock(stock) {}

int BackOfficeServer::createListenSocket() {
    const int serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        perror("socket");
        return -1;
    }

    int opt = 1;
    ::setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_cfg.port);
    addr.sin_addr.s_addr = inet_addr(m_cfg.bindIp.c_str());

    if (::bind(serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("bind");
        ::close(serverFd);
        return -1;
    }

    if (::listen(serverFd, m_cfg.listenBacklog) < 0) {
        perror("listen");
        ::close(serverFd);
        return -1;
    }

    return serverFd;
}

bool BackOfficeServer::isTicketCreateRequest(const std::string& req) const {
    // Minimal routing: check request line contains method + path
    const std::string needle = "POST " + m_cfg.ticketsEndpoint;
    return req.find(needle) != std::string::npos;
}

// NEW: validation route matcher
bool BackOfficeServer::isValidateRequest(const std::string& req) const {
    const std::string needle = "POST " + m_cfg.validateEndpoint;
    return req.find(needle) != std::string::npos;
}

std::string BackOfficeServer::extractBody(const std::string& req) const {
    // HTTP headers end with \r\n\r\n
    const std::string sep = "\r\n\r\n";
    size_t pos = req.find(sep);
    if (pos == std::string::npos) return "";
    return req.substr(pos + sep.size());
}

const char* BackOfficeServer::statusText(int statusCode) {
    switch (statusCode) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        default:  return "OK";
    }
}

std::string BackOfficeServer::httpJsonResponse(int statusCode, const std::string& jsonBody) {
    std::ostringstream resp;
    resp << "HTTP/1.1 " << statusCode << " " << statusText(statusCode) << "\r\n"
         << "Content-Type: application/json\r\n"
         << "Content-Length: " << jsonBody.size() << "\r\n"
         << "\r\n"
         << jsonBody;
    return resp.str();
}

std::string BackOfficeServer::httpEmptyResponse(int statusCode) {
    std::ostringstream resp;
    resp << "HTTP/1.1 " << statusCode << " " << statusText(statusCode) << "\r\n"
         << "Content-Length: 0\r\n\r\n";
    return resp.str();
}

std::string BackOfficeServer::handleCreateTicket(const std::string& bodyJson) {
    // - Response contains ONLY: success + ticketBase64
    // - But we also store full ticket into stock file.

    json reply;

    try {
        json req = json::parse(bodyJson);

        const int validityDays = req.value("validityDays", 0);
        const std::string lineNumber = req.value("lineNumber", "");

        if (validityDays <= 0 || lineNumber.empty()) {
            reply["success"] = false;
            reply["ticketBase64"] = "";
            return reply.dump();
        }

        Ticket t;
        t.creationDateUtc = TimeUtils::nowUtcIso8601();
        t.validityDays = validityDays;
        t.lineNumber = lineNumber;

        std::ostringstream payload;
        payload << ";creationDateUtc=" << t.creationDateUtc
                << ";validityDays=" << t.validityDays
                << ";lineNumber=" << t.lineNumber;

        t.ticketBase64 = Base64::encode(payload.str());

        m_stock.append(t);

        reply["success"] = true;
        reply["ticketBase64"] = t.ticketBase64;
        return reply.dump();
    }
    catch (...) {
        reply["success"] = false;
        reply["ticketBase64"] = "";
        return reply.dump();
    }
}

// NEW: validation handler used by Gate
std::string BackOfficeServer::handleValidateTicket(const std::string& bodyJson) {
    json reply;
    reply["valid"] = false;

    try {
        json req = json::parse(bodyJson);
        const std::string ticketBase64 = req.value("ticketBase64", "");
        if (ticketBase64.empty()) {
            return reply.dump();
        }

        Ticket stored;
        const bool found = m_stock.containsTicketBase64(ticketBase64, stored);
        if (!found) {
            return reply.dump();
        }

        if (isExpiredUtc(stored.creationDateUtc, stored.validityDays)) {
            return reply.dump();
        }

        reply["valid"] = true;
        return reply.dump();
    }
    catch (...) {
        return reply.dump();
    }
}

void BackOfficeServer::handleClient(int clientFd) {
    std::string req;
    req.resize(m_cfg.readBufferSize);

    const ssize_t n = ::read(clientFd, req.data(), req.size() - 1);
    if (n <= 0) return;

    req.resize(static_cast<size_t>(n));

    const std::string body = extractBody(req);

    if (isTicketCreateRequest(req)) {
        const std::string responseBody = handleCreateTicket(body);
        const std::string resp = httpJsonResponse(200, responseBody);
        ::write(clientFd, resp.c_str(), resp.size());
        return;
    }

    // NEW: route validate endpoint
    if (isValidateRequest(req)) {
        const std::string responseBody = handleValidateTicket(body);
        const std::string resp = httpJsonResponse(200, responseBody);
        ::write(clientFd, resp.c_str(), resp.size());
        return;
    }

    const std::string resp = httpEmptyResponse(404);
    ::write(clientFd, resp.c_str(), resp.size());
}

int BackOfficeServer::run() {
    const int serverFd = createListenSocket();
    if (serverFd < 0) return 1;

    std::cout << "[BackOffice] Listening on http://" << m_cfg.bindIp << ":" << m_cfg.port << "\n";
    std::cout << "[BackOffice] Endpoint: POST " << m_cfg.ticketsEndpoint << "\n";
    std::cout << "[BackOffice] Validate: POST " << m_cfg.validateEndpoint << "\n";
    std::cout << "[BackOffice] Stock file: " << m_cfg.ticketsPath << "\n";

    while (true) {
        const int clientFd = ::accept(serverFd, nullptr, nullptr);
        if (clientFd < 0) continue;

        handleClient(clientFd);
        ::close(clientFd);
    }

    ::close(serverFd);
    return 0;
}

// ============================
// main
// ============================
int main() {
    BackOfficeConfig cfg;

    TicketStock stock(cfg.ticketsPath);
    BackOfficeServer server(cfg, stock);
    return server.run();
}
