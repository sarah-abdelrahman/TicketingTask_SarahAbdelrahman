cmake_minimum_required(VERSION 3.15)
project(Ticketing_system)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# ----------------------------
# Find Paho MQTT libraries
# ----------------------------
find_library(PAHO_MQTTPP3 NAMES paho-mqttpp3)
find_library(PAHO_MQTT3A  NAMES paho-mqtt3a)
# ----------------------------
# Find libcurl properly
# ----------------------------
find_package(CURL REQUIRED)

add_executable(tvm_app
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/TVM_app.cpp
)

target_include_directories(tvm_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(tvm_app PRIVATE
    ${PAHO_MQTTPP3}
    ${PAHO_MQTT3A}
    CURL::libcurl
)

add_custom_target(tvm DEPENDS tvm_app)
