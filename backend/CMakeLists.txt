cmake_minimum_required(VERSION 3.15)
project(market_simulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(RDKAFKA REQUIRED rdkafka)

# Try to find nlohmann_json
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    find_path(NLOHMANN_JSON_INCLUDE_DIRS "nlohmann/json.hpp")
endif()

# Add executables
add_executable(kafka_test kafka_test.cpp)
add_executable(order_book order_book.cpp)

# Link and include for kafka_test
target_link_libraries(kafka_test PRIVATE ${RDKAFKA_LIBRARIES})
target_include_directories(kafka_test PRIVATE ${RDKAFKA_INCLUDE_DIRS})
target_compile_options(kafka_test PRIVATE ${RDKAFKA_CFLAGS_OTHER})

# Link and include for order_book
target_link_libraries(order_book PRIVATE ${RDKAFKA_LIBRARIES})
target_include_directories(order_book PRIVATE ${RDKAFKA_INCLUDE_DIRS})
target_compile_options(order_book PRIVATE ${RDKAFKA_CFLAGS_OTHER})

# Add nlohmann_json if found
if(nlohmann_json_FOUND)
    target_link_libraries(kafka_test PRIVATE nlohmann_json::nlohmann_json)
    target_link_libraries(order_book PRIVATE nlohmann_json::nlohmann_json)
elseif(NLOHMANN_JSON_INCLUDE_DIRS)
    target_include_directories(kafka_test PRIVATE ${NLOHMANN_JSON_INCLUDE_DIRS})
    target_include_directories(order_book PRIVATE ${NLOHMANN_JSON_INCLUDE_DIRS})
endif()

# Uncomment these lines later when you want to build Python bindings
# find_package(pybind11 REQUIRED)
# pybind11_add_module(orderbook_cpp ../backend_py/order_book_pybind.cpp order_book.cpp)
# target_link_libraries(orderbook_cpp PRIVATE ${RDKAFKA_LIBRARIES} nlohmann_json::nlohmann_json)
# target_include_directories(orderbook_cpp PRIVATE ${RDKAFKA_INCLUDE_DIRS})
# target_compile_options(orderbook_cpp PRIVATE ${RDKAFKA_CFLAGS_OTHER})