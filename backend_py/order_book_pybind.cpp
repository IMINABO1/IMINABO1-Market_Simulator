#define NOMINMAX
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/chrono.h>
#include "../backend/order_book.h"

namespace py = pybind11;

PYBIND11_MODULE(orderbook_cpp, m) {
    py::class_<Trade>(m, "Trade")
        .def_readonly("buy_order_id", &Trade::buy_order_id)
        .def_readonly("sell_order_id", &Trade::sell_order_id)
        .def_readonly("price", &Trade::price)
        .def_readonly("quantity", &Trade::quantity)
        .def_readonly("timestamp", &Trade::timestamp)
        .def("repr", &Trade::repr);

    py::enum_<OrderType>(m, "OrderType")
        .value("LIMIT", OrderType::LIMIT)
        .value("MARKET", OrderType::MARKET);

    py::class_<Order>(m, "Order")
        .def(py::init<int, double, unsigned int, bool, std::time_t, OrderType>())
        .def("get_order_id", &Order::get_order_id)
        .def("get_price", &Order::get_price)
        .def("get_qty", &Order::get_qty)
        .def("get_side", &Order::get_side)
        .def("get_timestamp", &Order::get_timestamp)
        .def("get_order_type", &Order::get_order_type)
        .def("is_expired", &Order::is_expired)
        .def("repr", &Order::repr);

    py::class_<OrderBook>(m, "OrderBook")
        .def(py::init<>())
        .def("add_order", &OrderBook::add_order)
        .def("add_order_legacy", &OrderBook::add_order_legacy)
        .def("remove_order", &OrderBook::remove_order)
        .def("clean_expired_orders", &OrderBook::clean_expired_orders)
        .def("repr", py::overload_cast<>(&OrderBook::repr))
        .def("repr", py::overload_cast<bool>(&OrderBook::repr))
        .def("get_best_bid", &OrderBook::get_best_bid)
        .def("get_best_ask", &OrderBook::get_best_ask)
        .def("update_order", &OrderBook::update_order)
        .def("get_trade_log", &OrderBook::get_trade_log, py::return_value_policy::reference_internal)
        .def("repr_trade_log", &OrderBook::repr_trade_log);
}