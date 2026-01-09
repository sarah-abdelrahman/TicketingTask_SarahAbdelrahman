cmake_minimum_required(VERSION 3.15)
project(Ticketing_system)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Find Paho MQTT libraries
find_library(PAHO_MQTTPP3 NAMES paho-mqttpp3)
find_library(PAHO_MQTT3AS NAMES paho-mqtt3as)

if(NOT PAHO_MQTTPP3 OR NOT PAHO_MQTT3AS)
    message(FATAL_ERROR "Paho MQTT libraries not found.")
endif()

add_executable(backoffice_app
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/Backoffice_app.cpp
)

target_include_directories(backoffice_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(backoffice_app
    ${PAHO_MQTTPP3}
    ${PAHO_MQTT3AS}
)

add_custom_target(backoffice DEPENDS backoffice_app)
