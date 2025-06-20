cmake_minimum_required(VERSION 3.15)
project(kafka_test)

set(CMAKE_CXX_STANDARD 17)

find_package(RdKafka CONFIG REQUIRED)

add_executable(kafka_test kafka_test.cpp)
target_link_libraries(kafka_test PRIVATE RdKafka::rdkafka)

add_executable(order_book order_book.cpp)
target_link_libraries(order_book PRIVATE RdKafka::rdkafka)