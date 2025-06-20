// #define NOMINMAX
/*
    the code should work fine but if it doesnt work and the issue with
    std::min on line 249 and 289 then uncomment that definition on top
    it should stop Windows 11 OS min and max from intefering with the code
    if it still doesnt work then remove the brackets 
    right now its (std::min) so you remove the brackets and you will have std::min
    do this in conjucntion with uncommenting that definition
    its your last hope
    if it doesn't work put the fries 🍟 in the bag 💰
    in other words give up
*/
#include <iostream>
#include <string>
#include <ctime>
#include <map>
#include <algorithm> 
#include <functional>
#include <utility>
#include <stdexcept>
#include <unordered_map>
#include <memory>
#include <optional>
#include <vector>
#include <queue>
#include <chrono>
#include <thread>

#include <librdkafka/rdkafka.h>

// Forward declarations
class Order;
class Trade;

enum class OrderType {
    LIMIT,
    MARKET
};

class Trade {
public:
    int buy_order_id;
    int sell_order_id;
    double price;
    unsigned int quantity;
    std::chrono::time_point<std::chrono::steady_clock> timestamp;

    Trade(int buy_id, int sell_id, double p, unsigned int qty)
        : buy_order_id(buy_id), sell_order_id(sell_id), price(p), quantity(qty),
          timestamp(std::chrono::steady_clock::now()) {}

    void repr() const {
        std::cout << "Trade: Buy Order " << buy_order_id
                  << " | Sell Order " << sell_order_id
                  << " | Price: " << price
                  << " | Quantity: " << quantity << std::endl;
    }
};

class Order {
    int order_id;
    double price;
    unsigned int quantity;
    bool side; // True for Buy, False for Sell
    std::time_t timestamp;
    OrderType order_type;
    std::optional<std::chrono::time_point<std::chrono::steady_clock>> expiry_time;
    std::chrono::time_point<std::chrono::steady_clock> creation_time;

public:
    Order(int id, double p, unsigned int qty, bool s, std::time_t t,
          OrderType type = OrderType::LIMIT,
          std::optional<std::chrono::seconds> ttl = std::nullopt)
        : order_id(id), price(p), quantity(qty), side(s), timestamp(t),
          order_type(type), creation_time(std::chrono::steady_clock::now()) {
        if (ttl.has_value()) {
            expiry_time = creation_time + ttl.value();
        }
    }

    void repr() const {
        std::cout   << "Order Id : " << order_id << ",\n"
                    << "Price : " << price << ",\n"
                    << "Quantity : " << quantity << ",\n"
                    << "Side : " << (side ? "Buy" : "Sell") << ",\n"
                    << "Type : " << (order_type == OrderType::MARKET ? "Market" : "Limit") << ",\n"
                    << "Timestamp : " << timestamp << "\n";
    }

    bool operator==(const Order& other) const {
        return (order_id == other.order_id) &&
               (price == other.price) &&
               (quantity == other.quantity) &&
               (side == other.side) &&
               (timestamp == other.timestamp);
    }

    int get_order_id() const { return order_id; }
    double get_price() const { return price; }
    unsigned int get_qty() const { return quantity; }
    bool get_side() const { return side; }
    time_t get_timestamp() const { return timestamp; }
    OrderType get_order_type() const { return order_type; }
    std::chrono::time_point<std::chrono::steady_clock> get_creation_time() const { return creation_time; }

    bool is_expired() const {
        if (!expiry_time.has_value()) return false;
        return std::chrono::steady_clock::now() > expiry_time.value();
    }

    void set_price(double new_price) { price = new_price; }
    void set_qty(unsigned int new_qty) { quantity = new_qty; }
    void set_side(bool new_side) { side = new_side; }
    void toggle_side() { side = !side; }

    ~Order() {}
};

struct BuyOrderComparator {
    bool operator()(const std::pair<Order, std::chrono::time_point<std::chrono::steady_clock>>& a,
                    const std::pair<Order, std::chrono::time_point<std::chrono::steady_clock>>& b) const {
        if (a.first.get_order_type() != b.first.get_order_type()) {
            return a.first.get_order_type() == OrderType::LIMIT && b.first.get_order_type() == OrderType::MARKET;
        }
        if (a.first.get_price() != b.first.get_price()) {
            return a.first.get_price() < b.first.get_price();
        }
        return a.second > b.second;
    }
};

struct SellOrderComparator {
    bool operator()(const std::pair<Order, std::chrono::time_point<std::chrono::steady_clock>>& a,
                    const std::pair<Order, std::chrono::time_point<std::chrono::steady_clock>>& b) const {
        if (a.first.get_order_type() != b.first.get_order_type()) {
            return a.first.get_order_type() == OrderType::LIMIT && b.first.get_order_type() == OrderType::MARKET;
        }
        if (a.first.get_price() != b.first.get_price()) {
            return a.first.get_price() > b.first.get_price();
        }
        return a.second > b.second;
    }
};

// Delivery report callback for librdkafka
void dr_msg_cb(rd_kafka_t *rk, const rd_kafka_message_t *rkmessage, void *opaque) {
    if (rkmessage->err) {
        std::cerr << "%% Message delivery failed: " << rd_kafka_err2str(rkmessage->err) << std::endl;
    } else {
        std::cout << "%% Message delivered (" << rkmessage->len << " bytes) to topic "
                  << rd_kafka_topic_name(rkmessage->rkt) << " [" << rkmessage->partition << "] "
                  << "at offset " << rkmessage->offset << std::endl;
    }
}

class OrderBook {
private:
    std::priority_queue<std::pair<Order, std::chrono::time_point<std::chrono::steady_clock>>,
                        std::vector<std::pair<Order, std::chrono::time_point<std::chrono::steady_clock>>>,
                        BuyOrderComparator> buy_orders;

    std::priority_queue<std::pair<Order, std::chrono::time_point<std::chrono::steady_clock>>,
                        std::vector<std::pair<Order, std::chrono::time_point<std::chrono::steady_clock>>>,
                        SellOrderComparator> sell_orders;

    std::unordered_map<int, std::shared_ptr<Order>> order_lookup;
    std::vector<Trade> trade_log;

    rd_kafka_t *producer;
    rd_kafka_conf_t *conf;

    // Declarations of member functions
    std::vector<Trade> match_order(const Order& incoming_order);
    void add_order_to_book(const Order& order);

public:
    // Declarations of constructor and other public member functions
    OrderBook();
    ~OrderBook();

    std::vector<Trade> add_order(const Order& order);
    void add_order_legacy(const Order& order);
    void remove_order(const int order_id);
    void clean_expired_orders();

    void repr();
    void repr(bool side);

    Order get_best_bid() const;
    Order get_best_ask() const;

    void update_order(int order_id, std::optional<double> new_price,
                      std::optional<unsigned int> new_qty, std::optional<bool> new_side);

    const std::vector<Trade>& get_trade_log() const;
    void repr_trade_log() const;

private:
    void flip_order(int order_id);
};

// OrderBook method definitions (outside the class for clarity)

OrderBook::OrderBook() : producer(nullptr), conf(nullptr) {
    char errstr[512];

    conf = rd_kafka_conf_new();
    if (rd_kafka_conf_set(conf, "bootstrap.servers", "localhost:9092",
                          errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
        std::cerr << "Kafka config error: " << errstr << std::endl;
        exit(1);
    }
    rd_kafka_conf_set_dr_msg_cb(conf, dr_msg_cb);

    producer = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
    if (!producer) {
        std::cerr << "Failed to create Kafka producer: " << errstr << std::endl;
        exit(1);
    }
    conf = nullptr;
}

OrderBook::~OrderBook() {
    if (producer) {
        std::cout << "Flushing Kafka producer..." << std::endl;
        rd_kafka_flush(producer, 10 * 1000);
        rd_kafka_destroy(producer);
        std::cout << "Kafka producer destroyed." << std::endl;
    }
    if (conf) {
        rd_kafka_conf_destroy(conf);
    }
}

std::vector<Trade> OrderBook::match_order(const Order& incoming_order) {
    std::vector<Trade> trades;
    Order working_order = incoming_order;

    if (working_order.get_side()) { // Buy order
        while (working_order.get_qty() > 0 && !sell_orders.empty()) {
            auto best_sell_pair = sell_orders.top();
            if (order_lookup.find(best_sell_pair.first.get_order_id()) == order_lookup.end()) {
                sell_orders.pop();
                continue;
            }

            Order best_sell = best_sell_pair.first;
            if (best_sell.is_expired()) {
                sell_orders.pop();
                order_lookup.erase(best_sell.get_order_id());
                continue;
            }

            if (working_order.get_order_type() == OrderType::LIMIT &&
                working_order.get_price() < best_sell.get_price()) {
                break;
            }

            unsigned int trade_qty = (std::min)(working_order.get_qty(), best_sell.get_qty());
            double trade_price = best_sell.get_price();

            Trade trade(working_order.get_order_id(), best_sell.get_order_id(),
                        trade_price, trade_qty);
            trades.push_back(trade);

            working_order.set_qty(working_order.get_qty() - trade_qty);
            sell_orders.pop();

            if (best_sell.get_qty() > trade_qty) {
                best_sell.set_qty(best_sell.get_qty() - trade_qty);
                sell_orders.push(std::make_pair(best_sell, best_sell_pair.second));
                if (order_lookup.find(best_sell.get_order_id()) != order_lookup.end()) {
                    order_lookup[best_sell.get_order_id()]->set_qty(best_sell.get_qty());
                }
            } else {
                order_lookup.erase(best_sell.get_order_id());
            }
        }
    } else { // Sell order
        while (working_order.get_qty() > 0 && !buy_orders.empty()) {
            auto best_buy_pair = buy_orders.top();
            if (order_lookup.find(best_buy_pair.first.get_order_id()) == order_lookup.end()) {
                buy_orders.pop();
                continue;
            }

            Order best_buy = best_buy_pair.first;
            if (best_buy.is_expired()) {
                buy_orders.pop();
                order_lookup.erase(best_buy.get_order_id());
                continue;
            }

            if (working_order.get_order_type() == OrderType::LIMIT &&
                working_order.get_price() > best_buy.get_price()) {
                break;
            }

            unsigned int trade_qty = (std::min)(working_order.get_qty(), best_buy.get_qty());
            double trade_price = best_buy.get_price();

            Trade trade(best_buy.get_order_id(), working_order.get_order_id(),
                        trade_price, trade_qty);
            trades.push_back(trade);

            working_order.set_qty(working_order.get_qty() - trade_qty);
            buy_orders.pop();

            if (best_buy.get_qty() > trade_qty) {
                best_buy.set_qty(best_buy.get_qty() - trade_qty);
                buy_orders.push(std::make_pair(best_buy, best_buy_pair.second));
                if (order_lookup.find(best_buy.get_order_id()) != order_lookup.end()) {
                    order_lookup[best_buy.get_order_id()]->set_qty(best_buy.get_qty());
                }
            } else {
                order_lookup.erase(best_buy.get_order_id());
            }
        }
    }

    if (working_order.get_qty() > 0 && working_order.get_order_type() == OrderType::LIMIT) {
        add_order_to_book(working_order);
    }
    return trades;
}

void OrderBook::add_order_to_book(const Order& order) {
    std::shared_ptr<Order> order_ptr = std::make_shared<Order>(order);
    order_lookup[order.get_order_id()] = order_ptr;

    auto timestamp = std::chrono::steady_clock::now();
    if (order.get_side()) {
        buy_orders.push(std::make_pair(*order_ptr, timestamp));
    } else {
        sell_orders.push(std::make_pair(*order_ptr, timestamp));
    }
}

std::vector<Trade> OrderBook::add_order(const Order& order) {
    if (order.is_expired()) {
        std::cout << "Order " << order.get_order_id() << " is expired and will not be processed." << std::endl;
        return {};
    }

    std::vector<Trade> trades = match_order(order);

    for (const auto& trade : trades) {
        trade_log.push_back(trade);
        trade.repr();

        std::string trade_msg = "Trade: Buy " + std::to_string(trade.buy_order_id) +
                                " Sell " + std::to_string(trade.sell_order_id) +
                                " Price: " + std::to_string(trade.price) +
                                " Qty: " + std::to_string(trade.quantity);

        if (rd_kafka_producev(
                producer,
                RD_KAFKA_V_TOPIC("order-updates"),
                RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
                RD_KAFKA_V_VALUE(const_cast<char*>(trade_msg.c_str()), trade_msg.size()),
                RD_KAFKA_V_END) == -1) {
            std::cerr << "%% Failed to produce to topic " << "order-updates" << ": "
                      << rd_kafka_err2str(rd_kafka_last_error()) << std::endl;
        } else {
            std::cout << "%% Enqueued message (" << trade_msg.size() << " bytes) for topic "
                      << "order-updates" << std::endl;
        }
        rd_kafka_poll(producer, 0);
    }
    rd_kafka_flush(producer, 100);
    return trades;
}

void OrderBook::add_order_legacy(const Order& order) {
    add_order_to_book(order);
}

void OrderBook::remove_order(const int order_id) {
    order_lookup.erase(order_id);
}

void OrderBook::clean_expired_orders() {
    std::cout << "Expired orders are cleaned automatically during matching." << std::endl;
}

void OrderBook::repr() {
    std::vector<Order> current_buys, current_sells;

    auto temp_buy_queue = buy_orders;
    while (!temp_buy_queue.empty()) {
        auto order_pair = temp_buy_queue.top();
        temp_buy_queue.pop();
        if (!order_pair.first.is_expired() &&
            order_lookup.find(order_pair.first.get_order_id()) != order_lookup.end()) {
            current_buys.push_back(order_pair.first);
        }
    }

    auto temp_sell_queue = sell_orders;
    while (!temp_sell_queue.empty()) {
        auto order_pair = temp_sell_queue.top();
        temp_sell_queue.pop();
        if (!order_pair.first.is_expired() &&
            order_lookup.find(order_pair.first.get_order_id()) != order_lookup.end()) {
            current_sells.push_back(order_pair.first);
        }
    }

    std::cout << "\nBuy Orders:\n";
    for (const auto& order : current_buys) {
        order.repr();
        std::cout << "\n";
    }

    std::cout << "\nSell Orders:\n";
    for (const auto& order : current_sells) {
        order.repr();
        std::cout << "\n";
    }
}

void OrderBook::repr(bool side) {
    if (side) {
        std::cout << "\nBuy Orders:\n";
        auto temp_buy_queue = buy_orders;
        while (!temp_buy_queue.empty()) {
            auto order_pair = temp_buy_queue.top();
            temp_buy_queue.pop();
            if (!order_pair.first.is_expired() &&
                order_lookup.find(order_pair.first.get_order_id()) != order_lookup.end()) {
                order_pair.first.repr();
                std::cout << "\n";
            }
        }
    } else {
        std::cout << "\nSell Orders:\n";
        auto temp_sell_queue = sell_orders;
        while (!temp_sell_queue.empty()) {
            auto order_pair = temp_sell_queue.top();
            temp_sell_queue.pop();
            if (!order_pair.first.is_expired() &&
                order_lookup.find(order_pair.first.get_order_id()) != order_lookup.end()) {
                order_pair.first.repr();
                std::cout << "\n";
            }
        }
    }
}

Order OrderBook::get_best_bid() const {
    if (!buy_orders.empty()) {
        auto temp_queue = buy_orders;
        while (!temp_queue.empty()) {
            auto order_pair = temp_queue.top();
            temp_queue.pop();
            if (!order_pair.first.is_expired() &&
                order_lookup.find(order_pair.first.get_order_id()) != order_lookup.end()) {
                return order_pair.first;
            }
        }
    }
    throw std::runtime_error("No buy order available");
}

Order OrderBook::get_best_ask() const {
    if (!sell_orders.empty()) {
        auto temp_queue = sell_orders;
        while (!temp_queue.empty()) {
            auto order_pair = temp_queue.top();
            temp_queue.pop();
            if (!order_pair.first.is_expired() &&
                order_lookup.find(order_pair.first.get_order_id()) != order_lookup.end()) {
                return order_pair.first;
            }
        }
    }
    throw std::runtime_error("No sell order available");
}

void OrderBook::update_order(int order_id, std::optional<double> new_price,
                  std::optional<unsigned int> new_qty, std::optional<bool> new_side) {
    auto it = order_lookup.find(order_id);
    if (it == order_lookup.end()) {
        std::cout << "Order not found\n";
        return;
    }

    Order updated_order = *it->second;
    remove_order(order_id);

    if (new_price.has_value()) {
        updated_order.set_price(new_price.value());
    }
    if (new_qty.has_value()) {
        updated_order.set_qty(new_qty.value());
    }
    if (new_side.has_value()) {
        updated_order.set_side(new_side.value());
    }
    add_order(updated_order);
}

const std::vector<Trade>& OrderBook::get_trade_log() const {
    return trade_log;
}

void OrderBook::repr_trade_log() const {
    std::cout << "\n=== Trade Log ===\n";
    for (const auto& trade : trade_log) {
        trade.repr();
    }
    std::cout << "================\n";
}

void OrderBook::flip_order(int order_id) {
    auto it = order_lookup.find(order_id);
    if (it != order_lookup.end() && it->second) {
        Order temp = *(it->second);
        remove_order(order_id);
        temp.toggle_side();
        add_order(temp);
    }
}

// Example usage
int main() {
    OrderBook book;

    Order limit_buy(1, 100.0, 10, true, std::time(nullptr), OrderType::LIMIT);
    Order limit_sell(2, 101.0, 5, false, std::time(nullptr), OrderType::LIMIT);
    Order market_buy(3, 0.0, 7, true, std::time(nullptr), OrderType::MARKET);
    Order expiring_order(4, 99.0, 3, true, std::time(nullptr), OrderType::LIMIT,
                         std::chrono::seconds(5));

    std::cout << "Adding limit orders...\n";
    book.add_order(limit_buy);
    book.add_order(limit_sell);

    std::cout << "\nOrder book state:\n";
    book.repr();

    std::cout << "\nAdding market buy order (should match with sell order)...\n";
    book.add_order(market_buy);

    std::cout << "\nOrder book state after market order:\n";
    book.repr();

    std::cout << "\nTrade log:\n";
    book.repr_trade_log();

    std::cout << "\nTesting is_expired (wait 6 seconds for expiry):\n";
    std::this_thread::sleep_for(std::chrono::seconds(6));
    std::cout << "Order 4 expired? " << (expiring_order.is_expired() ? "Yes" : "No") << std::endl;

    std::cout << "\nTesting add_order_legacy:\n";
    Order legacy_order(5, 98.0, 2, false, std::time(nullptr), OrderType::LIMIT);
    book.add_order_legacy(legacy_order);
    book.repr();

    std::cout << "\nTesting remove_order (removing order 5):\n";
    book.remove_order(5);
    book.repr();

    std::cout << "\nTesting update_order (change price and side of order 1):\n";
    book.update_order(1, 105.0, std::nullopt, false);
    book.repr();

    std::cout << "\nTesting clean_expired_orders:\n";
    book.clean_expired_orders();

    try {
        auto best_bid = book.get_best_bid();
        std::cout << "\nBest Bid:\n";
        best_bid.repr();
    } catch (const std::exception& e) {
        std::cout << "\nNo best bid: " << e.what() << std::endl;
    }
    try {
        auto best_ask = book.get_best_ask();
        std::cout << "\nBest Ask:\n";
        best_ask.repr();
    } catch (const std::exception& e) {
        std::cout << "\nNo best ask: " << e.what() << std::endl;
    }

    std::cout << "\nTesting repr(true) for buy orders:\n";
    book.repr(true);
    std::cout << "\nTesting repr(false) for sell orders:\n";
    book.repr(false);

    std::cout << "\nTesting repr_trade_log:\n";
    book.repr_trade_log();

    return 0;
}
