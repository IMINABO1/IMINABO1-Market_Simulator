cmake_minimum_required(VERSION 3.15)
project(orderbook_cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DNOMINMAX)

find_package(pybind11 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(RdKafka   CONFIG REQUIRED)   # ← this pulls in the C library

# your module
pybind11_add_module(orderbook_cpp
  order_book_pybind.cpp
  ../backend/order_book.cpp
)

target_link_libraries(orderbook_cpp PRIVATE
  RdKafka::rdkafka           # ← only the C library
  nlohmann_json::nlohmann_json
  Threads::Threads
)

if(WIN32)
  target_link_libraries(orderbook_cpp PRIVATE ws2_32 crypt32)
endif()
