#include <mqtt/async_client.h>
#include <iostream>
#include <string>
#include <random>
#include <chrono>
#include <thread>

static std::string getenv_or(const char* key, const std::string& defval) {
    const char* v = std::getenv(key);
    return (v && *v) ? std::string(v) : defval;
}

static int random_int(int lo, int hi) {
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(rng);
}

class Callback : public virtual mqtt::callback {
public:
    Callback(mqtt::async_client& client) : client_(client) {}

    void connected(const std::string& cause) override {
        std::cout << "[BACKOFFICE] Connected. cause=" << cause << "\n";
    }

    void connection_lost(const std::string& cause) override {
        std::cout << "[BACKOFFICE] Connection lost. cause=" << cause << "\n";
    }

    void message_arrived(mqtt::const_message_ptr msg) override {
        const auto& topic = msg->get_topic();
        const auto payload = msg->to_string();

        std::cout << "[BACKOFFICE] Received topic=" << topic
                  << " payload='" << payload << "'\n";

        std::string response_topic;

        if (topic == "ticketing/tvm/request") {
            response_topic = "ticketing/tvm/response";
        } else if (topic == "ticketing/gate/request") {
            response_topic = "ticketing/gate/response";
        } else {
            std::cout << "[BACKOFFICE] Unknown request topic. Ignored.\n";
            return;
        }

        int r = random_int(1, 100);
        std::string response = "ACK from backoffice | rand=" + std::to_string(r);

        auto out = mqtt::make_message(response_topic, response);
        out->set_qos(1);

        try {
            client_.publish(out)->wait();
            std::cout << "[BACKOFFICE] Replied topic=" << response_topic
                      << " payload='" << response << "'\n";
        } catch (const mqtt::exception& e) {
            std::cerr << "[BACKOFFICE] Publish failed: " << e.what() << "\n";
        }
    }

    void delivery_complete(mqtt::delivery_token_ptr) override {}

private:
    mqtt::async_client& client_;
};

int main() {
    const std::string host = getenv_or("MQTT_HOST", "mosquitto");
    const std::string port = getenv_or("MQTT_PORT", "1883");
    const std::string server_uri = "tcp://" + host + ":" + port;

    const std::string client_id = "backoffice_" + std::to_string(std::time(nullptr));

    mqtt::async_client client(server_uri, client_id);

    mqtt::connect_options conn;
    conn.set_clean_session(true);
    conn.set_keep_alive_interval(20);

    Callback cb(client);
    client.set_callback(cb);

    std::cout << "[BACKOFFICE] Connecting to " << server_uri << " ...\n";
    try {
        client.connect(conn)->wait();
        client.subscribe("ticketing/tvm/request", 1)->wait();
        client.subscribe("ticketing/gate/request", 1)->wait();
        std::cout << "[BACKOFFICE] Subscribed. Running...\n";
    } catch (const mqtt::exception& e) {
        std::cerr << "[BACKOFFICE] Connect/subscribe failed: " << e.what() << "\n";
        return 1;
    }

    // Run forever
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}


