cmake_minimum_required(VERSION 3.15)
project(kafka_test)

set(CMAKE_CXX_STANDARD 17)

find_package(RdKafka CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(pybind11 CONFIG REQUIRED)

add_executable(kafka_test kafka_test.cpp)
add_executable(order_book order_book.cpp)

target_link_libraries(kafka_test PRIVATE RdKafka::rdkafka nlohmann_json::nlohmann_json)
target_link_libraries(order_book PRIVATE RdKafka::rdkafka nlohmann_json::nlohmann_json)

pybind11_add_module(orderbook_cpp orderbook_pybind.cpp order_book.cpp)
target_link_libraries(orderbook_cpp PRIVATE RdKafka::rdkafka nlohmann_json::nlohmann_json)