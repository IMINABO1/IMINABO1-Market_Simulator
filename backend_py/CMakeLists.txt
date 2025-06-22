cmake_minimum_required(VERSION 3.15)
project(orderbook_cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DNOMINMAX)

find_package(pybind11 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Use pkg-config to find rdkafka instead of CONFIG
find_package(PkgConfig REQUIRED)
pkg_check_modules(RDKAFKA REQUIRED rdkafka)

# your module
pybind11_add_module(orderbook_cpp
  order_book_pybind.cpp
  ../backend/order_book.cpp
)

target_link_libraries(orderbook_cpp PRIVATE
  nlohmann_json::nlohmann_json
  ${RDKAFKA_LIBRARIES}
)

target_include_directories(orderbook_cpp PRIVATE
  ${RDKAFKA_INCLUDE_DIRS}
)

target_compile_options(orderbook_cpp PRIVATE
  ${RDKAFKA_CFLAGS_OTHER}
)

if(WIN32)
  target_link_libraries(orderbook_cpp PRIVATE ws2_32 crypt32)
endif()