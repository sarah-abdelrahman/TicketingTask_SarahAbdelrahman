#include <mqtt/async_client.h>
#include <iostream>
#include <string>
#include <random>
#include <chrono>
#include <thread>
#include <future>

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
    explicit Callback(std::promise<std::string>& p) : prom_(p) {}

    void message_arrived(mqtt::const_message_ptr msg) override {
        prom_.set_value(msg->to_string());
    }
    void connection_lost(const std::string&) override {}
    void delivery_complete(mqtt::delivery_token_ptr) override {}

private:
    std::promise<std::string>& prom_;
};

int main() {
    const std::string host = getenv_or("MQTT_HOST", "mosquitto");
    const std::string port = getenv_or("MQTT_PORT", "1883");
    const std::string server_uri = "tcp://" + host + ":" + port;

    const std::string client_id = "tvm_" + std::to_string(std::time(nullptr));
    mqtt::async_client client(server_uri, client_id);

    mqtt::connect_options conn;
    conn.set_clean_session(true);
    conn.set_keep_alive_interval(20);

    std::promise<std::string> prom;
    auto fut = prom.get_future();
    Callback cb(prom);
    client.set_callback(cb);

    try {
        std::cout << "[TVM] Connecting to " << server_uri << " ...\n";
        client.connect(conn)->wait();

        client.subscribe("ticketing/tvm/response", 1)->wait();

        int r = random_int(1000, 9999);
        std::string req = "TVM request | rand=" + std::to_string(r);

        auto msg = mqtt::make_message("ticketing/tvm/request", req);
        msg->set_qos(1);

        std::cout << "[TVM] Publishing: " << req << "\n";
        client.publish(msg)->wait();

        std::cout << "[TVM] Waiting for response...\n";
        if (fut.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
            std::cerr << "[TVM] Timeout waiting for response.\n";
            return 2;
        }

        std::cout << "[TVM] Response: " << fut.get() << "\n";

        client.disconnect()->wait();
        return 0;
    } catch (const mqtt::exception& e) {
        std::cerr << "[TVM] MQTT error: " << e.what() << "\n";
        return 1;
    }
}
